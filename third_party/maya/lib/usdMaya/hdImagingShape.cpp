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
#include "usdMaya/hdImagingShape.h"

#include "usdMaya/blockSceneModificationContext.h"
#include "usdMaya/translatorUtil.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MFn.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MNamespace.h>
#include <maya/MObject.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(PxrMayaHdImagingShapeTokens,
                        PXRUSDMAYA_HD_IMAGING_SHAPE_TOKENS);

const MTypeId PxrMayaHdImagingShape::typeId(0x00126402);
const MString PxrMayaHdImagingShape::typeName(
    PxrMayaHdImagingShapeTokens->MayaTypeName.GetText());

static const std::string _HdImagingTransformName("HdImaging");
static const std::string _HdImagingShapeName =
    TfStringPrintf("%sShape", _HdImagingTransformName.c_str());
static const std::string _HdImagingShapePath =
    TfStringPrintf(
        "|%s|%s",
        _HdImagingTransformName.c_str(),
        _HdImagingShapeName.c_str());


/* static */
void*
PxrMayaHdImagingShape::creator()
{
    return new PxrMayaHdImagingShape();
}

/* static */
MStatus
PxrMayaHdImagingShape::initialize()
{
    return MS::kSuccess;
}

/* static */
PxrMayaHdImagingShape*
PxrMayaHdImagingShape::GetShapeAtDagPath(const MDagPath& dagPath)
{
    const MObject mObj = dagPath.node();
    if (mObj.apiType() != MFn::kPluginShape) {
        TF_CODING_ERROR(
            "Could not get PxrMayaHdImagingShape for non-plugin shape node at "
            "DAG path: %s (apiTypeStr = %s)",
            dagPath.fullPathName().asChar(),
            mObj.apiTypeStr());
        return nullptr;
    }

    const MFnDependencyNode depNodeFn(mObj);
    PxrMayaHdImagingShape* imagingShape =
        static_cast<PxrMayaHdImagingShape*>(depNodeFn.userNode());
    if (!imagingShape) {
        TF_CODING_ERROR(
            "Could not get PxrMayaHdImagingShape for node at DAG path: %s",
            dagPath.fullPathName().asChar());
        return nullptr;
    }

    return imagingShape;
}

/* static */
MObject
PxrMayaHdImagingShape::GetOrCreateInstance()
{
    MStatus status;

    // Ensure that we search for (or create) the nodes in the root namespace,
    // in case this function is getting invoked by a node in a non-root
    // namespace (e.g. a USD proxy shape that represents the "Collapsed"
    // representation of an assembly).
    const MString currNamespace = MNamespace::currentNamespace(&status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());
    const MString rootNamespace = MNamespace::rootNamespace(&status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    MNamespace::setCurrentNamespace(rootNamespace);

    MObject hdImagingShapeObj;
    if (UsdMayaUtil::GetMObjectByName(_HdImagingShapePath, hdImagingShapeObj)) {
        MNamespace::setCurrentNamespace(currNamespace);

        return hdImagingShapeObj;
    }

    // We never intend for the imaging shape to get saved out to the Maya scene
    // file, so make sure that we preserve the scene modification status from
    // before we create the shape.
    const UsdMayaBlockSceneModificationContext blockModContext;

    // Create a transform node for the shape.
    MObject hdImagingTransformObj;
    if (!UsdMayaTranslatorUtil::CreateNode(
            _HdImagingTransformName.c_str(),
            "transform",
            MObject::kNullObj,
            &status,
            &hdImagingTransformObj)) {
        TF_RUNTIME_ERROR("Failed to create transform node %s for %s",
                         _HdImagingTransformName.c_str(),
                         _HdImagingShapeName.c_str());

        MNamespace::setCurrentNamespace(currNamespace);

        return MObject();
    }

    // Set the do not write flag on the shape's transform and lock it. If there
    // is an error, let Maya report it but keep going.
    MFnDependencyNode depNodeFn(hdImagingTransformObj, &status);
    CHECK_MSTATUS(status);

    status = depNodeFn.setDoNotWrite(true);
    CHECK_MSTATUS(status);

    status = depNodeFn.setLocked(true);
    CHECK_MSTATUS(status);

    // Create the HdImagingShape.
    if (!UsdMayaTranslatorUtil::CreateNode(
            _HdImagingShapeName.c_str(),
            PxrMayaHdImagingShapeTokens->MayaTypeName.GetText(),
            hdImagingTransformObj,
            &status,
            &hdImagingShapeObj)) {
        TF_RUNTIME_ERROR("Failed to create %s", _HdImagingShapeName.c_str());

        MNamespace::setCurrentNamespace(currNamespace);

        return MObject();
    }

    // We have to lock the pxrHdImagingShape here as opposed to in the shape's
    // postConstructor(), otherwise the rename CreateNode() above tries to do
    // will fail.
    status = depNodeFn.setObject(hdImagingShapeObj);
    CHECK_MSTATUS(status);

    status = depNodeFn.setLocked(true);
    CHECK_MSTATUS(status);

    MNamespace::setCurrentNamespace(currNamespace);

    return hdImagingShapeObj;
}

/* virtual */
bool
PxrMayaHdImagingShape::isBounded() const
{
    return false;
}

/* virtual */
MBoundingBox
PxrMayaHdImagingShape::boundingBox() const
{
    return MBoundingBox();
}

/* virtual */
void
PxrMayaHdImagingShape::postConstructor()
{
    MStatus status = setDoNotWrite(true);
    CHECK_MSTATUS(status);
}

PxrMayaHdImagingShape::PxrMayaHdImagingShape() : MPxSurfaceShape()
{
}

/* virtual */
PxrMayaHdImagingShape::~PxrMayaHdImagingShape()
{
}


PXR_NAMESPACE_CLOSE_SCOPE
