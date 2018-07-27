//
// Copyright 2018 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/pxr.h"
#include "usdMaya/pointBasedDeformerNode.h"

#include "usdMaya/stageData.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/pointBased.h"

#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnData.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MItGeometry.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTime.h>
#include <maya/MTypeId.h>

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(UsdMayaPointBasedDeformerNodeTokens,
                        PXRUSDMAYA_POINT_BASED_DEFORMER_NODE_TOKENS);


const MTypeId UsdMayaPointBasedDeformerNode::typeId(0x00126401);
const MString UsdMayaPointBasedDeformerNode::typeName(
    UsdMayaPointBasedDeformerNodeTokens->MayaTypeName.GetText());

// Attributes
MObject UsdMayaPointBasedDeformerNode::inUsdStageAttr;
MObject UsdMayaPointBasedDeformerNode::primPathAttr;
MObject UsdMayaPointBasedDeformerNode::timeAttr;


/* static */
void*
UsdMayaPointBasedDeformerNode::creator()
{
    return new UsdMayaPointBasedDeformerNode();
}

/* static */
MStatus
UsdMayaPointBasedDeformerNode::initialize()
{
    MStatus status;

    MFnTypedAttribute typedAttrFn;
    MFnUnitAttribute unitAttrFn;

    inUsdStageAttr = typedAttrFn.create("inUsdStage",
                                        "is",
                                        UsdMayaStageData::mayaTypeId,
                                        MObject::kNullObj,
                                        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = typedAttrFn.setReadable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = typedAttrFn.setStorable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = typedAttrFn.setHidden(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = typedAttrFn.setDisconnectBehavior(MFnAttribute::kReset);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(inUsdStageAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnStringData stringDataFn;
    const MObject defaultStringDataObj = stringDataFn.create("");

    primPathAttr = typedAttrFn.create("primPath",
                                      "pp",
                                      MFnData::kString,
                                      defaultStringDataObj,
                                      &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(primPathAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    timeAttr = unitAttrFn.create("time",
                                 "tm",
                                 MFnUnitAttribute::kTime,
                                 0.0,
                                 &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(timeAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = attributeAffects(inUsdStageAttr, outputGeom);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(primPathAttr, outputGeom);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(timeAttr, outputGeom);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}

/* virtual */
MStatus
UsdMayaPointBasedDeformerNode::deform(
        MDataBlock& block,
        MItGeometry& iter,
        const MMatrix& /* mat */,
        unsigned int multiIndex)
{
    MStatus status;

    // Get the USD stage.
    const MDataHandle inUsdStageHandle =
        block.inputValue(inUsdStageAttr, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    UsdMayaStageData* stageData =
        dynamic_cast<UsdMayaStageData*>(inUsdStageHandle.asPluginData());
    if (!stageData || !stageData->stage) {
        return MS::kFailure;
    }

    const UsdStageRefPtr& usdStage = stageData->stage;

    // Get the prim path.
    const MDataHandle primPathHandle = block.inputValue(primPathAttr, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    const std::string primPathString =
        TfStringTrim(primPathHandle.asString().asChar());

    if (primPathString.empty()) {
        return MS::kFailure;
    }

    const SdfPath primPath(primPathString);

    const UsdPrim& usdPrim = usdStage->GetPrimAtPath(primPath);
    const UsdGeomPointBased usdPointBased(usdPrim);
    if (!usdPointBased) {
        return MS::kFailure;
    }

    const MDataHandle timeHandle = block.inputValue(timeAttr, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    const UsdTimeCode usdTime(timeHandle.asTime().value());

    const MDataHandle envelopeHandle = block.inputValue(envelope, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    const float envelope = envelopeHandle.asFloat();

    VtVec3fArray usdPoints;
    if (!usdPointBased.GetPointsAttr().Get(&usdPoints, usdTime) ||
            usdPoints.empty()) {
        return MS::kFailure;
    }

    for ( ; !iter.isDone(); iter.next()) {
        const int index = iter.index();
        if (index < 0 || static_cast<size_t>(index) >= usdPoints.size()) {
            continue;
        }

        const MPoint mayaPoint = iter.position();
        const float weight = weightValue(block, multiIndex, index);

        const GfVec3f& usdPoint = usdPoints[static_cast<size_t>(index)];

        const GfVec3f deformedPoint =
            GfLerp<GfVec3f>(weight * envelope,
                            GfVec3f(mayaPoint[0], mayaPoint[1], mayaPoint[2]),
                            usdPoint);

        iter.setPosition(
            MPoint(deformedPoint[0], deformedPoint[1], deformedPoint[2]));
    }

    return status;
}

UsdMayaPointBasedDeformerNode::UsdMayaPointBasedDeformerNode() :
    MPxDeformerNode()
{
}

/* virtual */
UsdMayaPointBasedDeformerNode::~UsdMayaPointBasedDeformerNode()
{
}


PXR_NAMESPACE_CLOSE_SCOPE
