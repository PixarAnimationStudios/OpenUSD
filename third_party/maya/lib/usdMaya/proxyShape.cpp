//
// Copyright 2016 Pixar
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
#include "usdMaya/proxyShape.h"

#include "usdMaya/hdImagingShape.h"
#include "usdMaya/query.h"
#include "usdMaya/stageCache.h"
#include "usdMaya/stageData.h"
#include "usdMaya/util.h"

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/ray.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/stageCacheContext.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdUtils/stageCache.h"

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MDGContext.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MPoint.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MSelectionMask.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTime.h>
#include <maya/MViewport2Renderer.h>

#include <map>
#include <string>
#include <utility>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(UsdMayaProxyShapeTokens,
                        PXRUSDMAYA_PROXY_SHAPE_TOKENS);


// Hydra performs its own high-performance frustum culling, so
// we don't want to rely on Maya to do it on the CPU. AS such, the best
// performance comes from telling Maya to pretend that every object has no
// bounds.
TF_DEFINE_ENV_SETTING(PIXMAYA_ENABLE_BOUNDING_BOX_MODE, false,
                      "Enable bounding box rendering (slows refresh rate)");

UsdMayaProxyShape::ClosestPointDelegate
UsdMayaProxyShape::_sharedClosestPointDelegate = nullptr;

UsdMayaProxyShape::ObjectSoftSelectEnabledDelgate
UsdMayaProxyShape::_sharedObjectSoftSelectEnabledDelgate = nullptr;


// ========================================================

const MTypeId UsdMayaProxyShape::typeId(0x0010A259);
const MString UsdMayaProxyShape::typeName(
    UsdMayaProxyShapeTokens->MayaTypeName.GetText());

const MString UsdMayaProxyShape::displayFilterName(
    TfStringPrintf("%sDisplayFilter",
                   UsdMayaProxyShapeTokens->MayaTypeName.GetText()).c_str());
const MString UsdMayaProxyShape::displayFilterLabel("USD Proxies");

// Attributes
MObject UsdMayaProxyShape::filePathAttr;
MObject UsdMayaProxyShape::primPathAttr;
MObject UsdMayaProxyShape::excludePrimPathsAttr;
MObject UsdMayaProxyShape::timeAttr;
MObject UsdMayaProxyShape::variantKeyAttr;
MObject UsdMayaProxyShape::complexityAttr;
MObject UsdMayaProxyShape::inStageDataAttr;
MObject UsdMayaProxyShape::inStageDataCachedAttr;
MObject UsdMayaProxyShape::fastPlaybackAttr;
MObject UsdMayaProxyShape::outStageDataAttr;
MObject UsdMayaProxyShape::drawRenderPurposeAttr;
MObject UsdMayaProxyShape::drawProxyPurposeAttr;
MObject UsdMayaProxyShape::drawGuidePurposeAttr;
MObject UsdMayaProxyShape::softSelectableAttr;


/* static */
void*
UsdMayaProxyShape::creator()
{
    return new UsdMayaProxyShape();
}

/* static */
MStatus
UsdMayaProxyShape::initialize()
{
    MStatus retValue = MS::kSuccess;

    //
    // create attr factories
    //
    MFnCompoundAttribute compoundAttrFn;
    MFnEnumAttribute     enumAttrFn;
    MFnNumericAttribute  numericAttrFn;
    MFnTypedAttribute    typedAttrFn;
    MFnUnitAttribute     unitAttrFn;

    filePathAttr = typedAttrFn.create(
        "filePath",
        "fp",
        MFnData::kString,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setReadable(false);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(filePathAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    primPathAttr = typedAttrFn.create(
        "primPath",
        "pp",
        MFnData::kString,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(primPathAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    excludePrimPathsAttr = typedAttrFn.create(
        "excludePrimPaths",
        "epp",
        MFnData::kString,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(excludePrimPathsAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    timeAttr = unitAttrFn.create(
        "time",
        "tm",
        MFnUnitAttribute::kTime,
        0.0,
        &retValue);
    unitAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(timeAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    variantKeyAttr = typedAttrFn.create(
        "variantKey",
        "variantKey",
        MFnData::kString,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setReadable(false);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(variantKeyAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    complexityAttr = numericAttrFn.create(
        "complexity",
        "cplx",
        MFnNumericData::kInt,
        0,
        &retValue);
    numericAttrFn.setMin(0);
    numericAttrFn.setSoftMax(4);
    numericAttrFn.setMax(8);
    numericAttrFn.setChannelBox(true);
    numericAttrFn.setStorable(false);
    numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(complexityAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    inStageDataAttr = typedAttrFn.create(
        "inStageData",
        "id",
        UsdMayaStageData::mayaTypeId,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setReadable(false);
    typedAttrFn.setStorable(false);
    typedAttrFn.setDisconnectBehavior(MFnNumericAttribute::kReset); // on disconnect, reset to Null
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(inStageDataAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // inStageData or filepath-> inStageDataCached -> outStageData
    inStageDataCachedAttr = typedAttrFn.create(
        "inStageDataCached",
        "idc",
        UsdMayaStageData::mayaTypeId,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setStorable(false);
    typedAttrFn.setWritable(false);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(inStageDataCachedAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    fastPlaybackAttr = numericAttrFn.create(
        "fastPlayback",
        "fs",
        MFnNumericData::kBoolean,
        0,
        &retValue);
    numericAttrFn.setInternal(true);
    numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(fastPlaybackAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    outStageDataAttr = typedAttrFn.create(
        "outStageData",
        "od",
        UsdMayaStageData::mayaTypeId,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setStorable(false);
    typedAttrFn.setWritable(false);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(outStageDataAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    drawRenderPurposeAttr = numericAttrFn.create(
        "drawRenderPurpose",
        "drp",
        MFnNumericData::kBoolean,
        0.0,
        &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setKeyable(true);
    numericAttrFn.setReadable(false);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(drawRenderPurposeAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    drawProxyPurposeAttr = numericAttrFn.create(
        "drawProxyPurpose",
        "dpp",
        MFnNumericData::kBoolean,
        1.0,
        &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setKeyable(true);
    numericAttrFn.setReadable(false);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(drawProxyPurposeAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    drawGuidePurposeAttr = numericAttrFn.create(
        "drawGuidePurpose",
        "dgp",
        MFnNumericData::kBoolean,
        0.0,
        &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setKeyable(true);
    numericAttrFn.setReadable(false);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(drawGuidePurposeAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    softSelectableAttr = numericAttrFn.create(
        "softSelectable",
        "softSelectable",
        MFnNumericData::kBoolean,
        0.0,
        &retValue);
    numericAttrFn.setStorable(false);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(softSelectableAttr);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    //
    // add attribute dependencies
    //
    retValue = attributeAffects(filePathAttr, inStageDataCachedAttr);
    retValue = attributeAffects(filePathAttr, outStageDataAttr);

    retValue = attributeAffects(primPathAttr, inStageDataCachedAttr);
    retValue = attributeAffects(primPathAttr, outStageDataAttr);

    retValue = attributeAffects(variantKeyAttr, inStageDataCachedAttr);
    retValue = attributeAffects(variantKeyAttr, outStageDataAttr);

    retValue = attributeAffects(inStageDataAttr, inStageDataCachedAttr);
    retValue = attributeAffects(inStageDataAttr, outStageDataAttr);

    retValue = attributeAffects(inStageDataCachedAttr, outStageDataAttr);

    return retValue;
}

/* static */
UsdMayaProxyShape*
UsdMayaProxyShape::GetShapeAtDagPath(const MDagPath& dagPath)
{
    MObject mObj = dagPath.node();
    if (mObj.apiType() != MFn::kPluginShape) {
        TF_CODING_ERROR(
                "Could not get UsdMayaProxyShape for non-plugin shape node "
                "at DAG path: %s (apiTypeStr = %s)",
                dagPath.fullPathName().asChar(),
                mObj.apiTypeStr());
        return nullptr;
    }

    const MFnDependencyNode depNodeFn(mObj);
    UsdMayaProxyShape* pShape =
        static_cast<UsdMayaProxyShape*>(depNodeFn.userNode());
    if (!pShape) {
        TF_CODING_ERROR(
                "Could not get UsdMayaProxyShape for node at DAG path: %s",
                dagPath.fullPathName().asChar());
        return nullptr;
    }

    return pShape;
}

/* static */
void
UsdMayaProxyShape::SetClosestPointDelegate(ClosestPointDelegate delegate)
{
    _sharedClosestPointDelegate = delegate;
}

/* static */
void
UsdMayaProxyShape::SetObjectSoftSelectEnabledDelegate(
        ObjectSoftSelectEnabledDelgate delegate)
{
    _sharedObjectSoftSelectEnabledDelgate = delegate;
}

/* static */
bool
UsdMayaProxyShape::GetObjectSoftSelectEnabled()
{
    // If the delegate isn't set, we just assume soft select isn't currently
    // enabled - this will mean that the object is selectable in VP2, by default
    if (!_sharedObjectSoftSelectEnabledDelgate) {
        return false;
    }
    return _sharedObjectSoftSelectEnabledDelgate();
}

/* virtual */
void
UsdMayaProxyShape::postConstructor()
{
    setRenderable(true);

    // This shape uses Hydra for imaging, so make sure that the
    // pxrHdImagingShape is setup.
    PxrMayaHdImagingShape::GetOrCreateInstance();
}

/* virtual */
MStatus
UsdMayaProxyShape::compute(const MPlug& plug, MDataBlock& dataBlock)
{
    if (plug == excludePrimPathsAttr ||
            plug == timeAttr ||
            plug == complexityAttr ||
            plug == drawRenderPurposeAttr ||
            plug == drawProxyPurposeAttr ||
            plug == drawGuidePurposeAttr) {
        // If the attribute that needs to be computed is one of these, then it
        // does not affect the ouput stage data, but it *does* affect imaging
        // the shape. In that case, we notify Maya that the shape needs to be
        // redrawn and let it take care of computing the attribute. This covers
        // the case where an attribute on the proxy shape may have an incoming
        // connection from another node (e.g. "time1.outTime" being connected
        // to the proxy shape's "time" attribute). In that case,
        // setDependentsDirty() might not get called and only compute() might.
        MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());
        return MS::kUnknownParameter;
    }
    else if (plug == inStageDataCachedAttr) {
        return computeInStageDataCached(dataBlock);
    }
    else if (plug == outStageDataAttr) {
        return computeOutStageData(dataBlock);
    }

    return MS::kUnknownParameter;
}

MStatus
UsdMayaProxyShape::computeInStageDataCached(MDataBlock& dataBlock)
{
    MStatus retValue = MS::kSuccess;

    MDataHandle inDataHandle = dataBlock.inputValue(inStageDataAttr, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // If inData has an incoming connection, then use it. Otherwise generate stage from the filepath
    if (!inDataHandle.data().isNull() ) {
        //
        // Propagate inData -> inDataCached
        //
        MDataHandle inDataCachedHandle = dataBlock.outputValue(inStageDataCachedAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        inDataCachedHandle.copy(inDataHandle);

        inDataCachedHandle.setClean();
        return MS::kSuccess;
    }
    else {
        //
        // Calculate from USD filepath and primPath and variantKey
        //

        // Get input attr values
        const MString file = dataBlock.inputValue(filePathAttr, &retValue).asString();
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        //
        // let the usd stage cache deal with caching the usd stage data
        //
        std::string fileString = TfStringTrimRight(file.asChar());

        // == Load the Stage
        UsdStageRefPtr usdStage;
        SdfPath        primPath;

        // get the variantKey
        MDataHandle variantKeyHandle =
            dataBlock.inputValue(variantKeyAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);
        const MString variantKey = variantKeyHandle.asString();

        SdfLayerRefPtr sessionLayer;
        std::vector<std::pair<std::string, std::string> > variantSelections;
        std::string variantKeyString = variantKey.asChar();
        if (!variantKeyString.empty()) {
            variantSelections.push_back(
                std::make_pair("modelingVariant",variantKeyString));

            // Get the primPath
            const MString primPathMString =
                dataBlock.inputValue(primPathAttr, &retValue).asString();
            CHECK_MSTATUS_AND_RETURN_IT(retValue);

            std::vector<std::string> primPathEltStrs =
                TfStringTokenize(primPathMString.asChar(),"/");
            if (!primPathEltStrs.empty()) {
                sessionLayer =
                    UsdUtilsStageCache::GetSessionLayerForVariantSelections(
                        TfToken(primPathEltStrs[0]), variantSelections);
            }
        }

        if (SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(fileString)) {
            UsdStageCacheContext ctx(UsdMayaStageCache::Get());
            if (sessionLayer) {
                usdStage = UsdStage::Open(rootLayer,
                        sessionLayer,
                        ArGetResolver().GetCurrentContext());
            } else {
                usdStage = UsdStage::Open(rootLayer,
                        ArGetResolver().GetCurrentContext());
            }

            usdStage->SetEditTarget(usdStage->GetSessionLayer());
        }

        if (usdStage) {
            primPath = usdStage->GetPseudoRoot().GetPath();
        }

        // Create the output outData ========
        MFnPluginData pluginDataFn;
        MObject stageDataObj =
            pluginDataFn.create(UsdMayaStageData::mayaTypeId, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        UsdMayaStageData* stageData =
            reinterpret_cast<UsdMayaStageData*>(pluginDataFn.data(&retValue));
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        // Set the outUsdStageData
        stageData->stage = usdStage;
        stageData->primPath = primPath;

        //
        // set the data on the output plug
        //
        MDataHandle inDataCachedHandle =
            dataBlock.outputValue(inStageDataCachedAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        inDataCachedHandle.set(stageData);
        inDataCachedHandle.setClean();
        return MS::kSuccess;
    }
}

MStatus
UsdMayaProxyShape::computeOutStageData(MDataBlock& dataBlock)
{
    MStatus retValue = MS::kSuccess;

    TfReset(_boundingBoxCache);

    // Reset the stage listener until we determine that everything is valid.
    _stageNoticeListener.SetStage(UsdStageWeakPtr());
    _stageNoticeListener.SetStageContentsChangedCallback(nullptr);

    MDataHandle inDataCachedHandle =
        dataBlock.inputValue(inStageDataCachedAttr, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    UsdStageRefPtr usdStage;

    UsdMayaStageData* inData =
        dynamic_cast<UsdMayaStageData*>(inDataCachedHandle.asPluginData());
    if(inData)
    {
        usdStage = inData->stage;
    }

    // If failed to get a valid stage, then
    // Propagate inDataCached -> outData
    // and return
    if (!usdStage) {
        MDataHandle outDataHandle = dataBlock.outputValue(outStageDataAttr, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);
        outDataHandle.copy(inDataCachedHandle);
        return MS::kSuccess;
    }

    // Get the primPath
    const MString primPath = dataBlock.inputValue(primPathAttr, &retValue).asString();
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // Get the prim
    // If no primPath string specified, then use the pseudo-root.
    UsdPrim usdPrim;
    std::string primPathStr = primPath.asChar();
    if ( !primPathStr.empty() ) {
        SdfPath primPath(primPathStr);

        // Validate assumption: primPath is descendent of passed-in stage primPath
        //   Make sure that the primPath is a child of the passed in stage's primpath
        if ( primPath.HasPrefix(inData->primPath) ) {
            usdPrim = usdStage->GetPrimAtPath( primPath );
        }
        else {
            TF_WARN("%s: Shape primPath <%s> is not a descendant of input "
                    "stage primPath <%s>",
                    MPxSurfaceShape::name().asChar(),
                    primPath.GetText(),
                    inData->primPath.GetText());
        }
    } else {
        usdPrim = usdStage->GetPseudoRoot();
    }

    // Create the output outData
    MFnPluginData pluginDataFn;
    MObject stageDataObj =
        pluginDataFn.create(UsdMayaStageData::mayaTypeId, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    UsdMayaStageData* stageData =
        reinterpret_cast<UsdMayaStageData*>(pluginDataFn.data(&retValue));
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // Set the outUsdStageData
    stageData->stage = usdStage;
    stageData->primPath = usdPrim ? usdPrim.GetPath() :
                                    usdStage->GetPseudoRoot().GetPath();

    //
    // set the data on the output plug
    //
    MDataHandle outDataHandle =
        dataBlock.outputValue(outStageDataAttr, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    outDataHandle.set(stageData);
    outDataHandle.setClean();

    // Start listening for notices for the USD stage.
    _stageNoticeListener.SetStage(usdStage);
    _stageNoticeListener.SetStageContentsChangedCallback(
        std::bind(&UsdMayaProxyShape::_OnStageContentsChanged,
                  this,
                  std::placeholders::_1));

    return MS::kSuccess;
}

/* virtual */
bool
UsdMayaProxyShape::isBounded() const
{
    return !_useFastPlayback &&
        TfGetEnvSetting(PIXMAYA_ENABLE_BOUNDING_BOX_MODE)&& isStageValid();
}

/* virtual */
MBoundingBox
UsdMayaProxyShape::boundingBox() const
{
    if (_useFastPlayback) {
        return UsdMayaUtil::GetInfiniteBoundingBox();
    }

    MStatus status;

    // Make sure outStage is up to date
    UsdMayaProxyShape* nonConstThis = const_cast<UsdMayaProxyShape*>(this);
    MDataBlock dataBlock = nonConstThis->forceCache();
    dataBlock.inputValue(outStageDataAttr, &status);
    CHECK_MSTATUS_AND_RETURN(status, MBoundingBox());

    // XXX:
    // If we could cheaply determine whether a stage only has static geometry,
    // we could make this value a constant one for that case, avoiding the
    // memory overhead of a cache entry per frame
    MDataHandle timeHandle = dataBlock.inputValue(timeAttr, &status);
    UsdTimeCode currTime = UsdTimeCode(timeHandle.asTime().value());

    std::map<UsdTimeCode, MBoundingBox>::const_iterator cacheLookup =
        _boundingBoxCache.find(currTime);

    if (cacheLookup != _boundingBoxCache.end()) {
        return cacheLookup->second;
    }

    UsdPrim prim = usdPrim();
    if (!prim) {
        return MBoundingBox();
    }

    const UsdGeomImageable imageablePrim(prim);

    bool drawRenderPurpose = false;
    bool drawProxyPurpose = true;
    bool drawGuidePurpose = false;
    _GetDrawPurposeToggles(
        dataBlock,
        &drawRenderPurpose,
        &drawProxyPurpose,
        &drawGuidePurpose);

    const TfToken purpose1 = UsdGeomTokens->default_;
    const TfToken purpose2 =
        drawRenderPurpose ? UsdGeomTokens->render : TfToken();
    const TfToken purpose3 =
        drawProxyPurpose ? UsdGeomTokens->proxy : TfToken();
    const TfToken purpose4 =
        drawGuidePurpose ? UsdGeomTokens->guide : TfToken();

    const GfBBox3d allBox = imageablePrim.ComputeUntransformedBound(
        currTime,
        purpose1,
        purpose2,
        purpose3,
        purpose4);

    MBoundingBox& retval = nonConstThis->_boundingBoxCache[currTime];

    const GfRange3d boxRange = allBox.ComputeAlignedBox();
    if (!boxRange.IsEmpty()) {
        const GfVec3d boxMin = boxRange.GetMin();
        const GfVec3d boxMax = boxRange.GetMax();
        retval = MBoundingBox(
            MPoint(boxMin[0], boxMin[1], boxMin[2]),
            MPoint(boxMax[0], boxMax[1], boxMax[2]));
    }

    return retval;
}

bool
UsdMayaProxyShape::isStageValid() const
{
    MStatus localStatus;
    UsdMayaProxyShape* nonConstThis = const_cast<UsdMayaProxyShape*>(this);
    MDataBlock dataBlock = nonConstThis->forceCache();

    MDataHandle outDataHandle = dataBlock.inputValue(outStageDataAttr, &localStatus);
    CHECK_MSTATUS_AND_RETURN(localStatus, false);

    UsdMayaStageData* outData =
        dynamic_cast<UsdMayaStageData*>(outDataHandle.asPluginData());
    if(!outData || !outData->stage) {
        return false;
    }

    return true;
}

/* virtual */
MStatus
UsdMayaProxyShape::setDependentsDirty(const MPlug& plug, MPlugArray& plugArray)
{
    // If/when the MPxDrawOverride for the proxy shape specifies
    // isAlwaysDirty=false to improve performance, we must be sure to notify
    // the Maya renderer that the geometry is dirty and needs to be redrawn
    // when any plug on the proxy shape is dirtied.
    MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());
    return MPxSurfaceShape::setDependentsDirty(plug, plugArray);
}

/* virtual */
bool
UsdMayaProxyShape::setInternalValueInContext(
        const MPlug& plug,
        const MDataHandle& dataHandle,
        MDGContext& ctx)
{
    if (plug == fastPlaybackAttr) {
        _useFastPlayback = dataHandle.asBool();
        return true;
    }

    return MPxSurfaceShape::setInternalValueInContext(plug, dataHandle, ctx);
}

/* virtual */
bool
UsdMayaProxyShape::getInternalValueInContext(
        const MPlug& plug,
        MDataHandle& dataHandle,
        MDGContext& ctx)
{
    if (plug == fastPlaybackAttr) {
        dataHandle.set(_useFastPlayback);
        return true;
    }

    return MPxSurfaceShape::getInternalValueInContext(plug, dataHandle, ctx);
}

/* virtual */
UsdPrim
UsdMayaProxyShape::usdPrim() const
{
    return _GetUsdPrim( const_cast<UsdMayaProxyShape*>(this)->forceCache() );
}

UsdPrim
UsdMayaProxyShape::_GetUsdPrim(MDataBlock dataBlock) const
{
    MStatus localStatus;
    UsdPrim usdPrim;

    MDataHandle outDataHandle =
        dataBlock.inputValue(outStageDataAttr, &localStatus);
    CHECK_MSTATUS_AND_RETURN(localStatus, usdPrim);

    UsdMayaStageData* outData = dynamic_cast<UsdMayaStageData*>(outDataHandle.asPluginData());
    if(!outData) {
        return usdPrim; // empty UsdPrim
    }

    if(!outData->stage) {
        return usdPrim; // empty UsdPrim
    }

    usdPrim = (outData->primPath.IsEmpty()) ?
                outData->stage->GetPseudoRoot() :
                outData->stage->GetPrimAtPath(outData->primPath);

    return usdPrim;
}

int
UsdMayaProxyShape::getComplexity() const
{
    return _GetComplexity( const_cast<UsdMayaProxyShape*>(this)->forceCache() );
}

int
UsdMayaProxyShape::_GetComplexity(MDataBlock dataBlock) const
{
    int complexity = 0;
    MStatus status;

    complexity = dataBlock.inputValue(complexityAttr, &status).asInt();

    return complexity;
}

UsdTimeCode
UsdMayaProxyShape::getTime() const
{
    return _GetTime( const_cast<UsdMayaProxyShape*>(this)->forceCache() );
}

UsdTimeCode
UsdMayaProxyShape::_GetTime(MDataBlock dataBlock) const
{
    MStatus status;

    return UsdTimeCode(dataBlock.inputValue(timeAttr, &status).asTime().value());
}

SdfPathVector
UsdMayaProxyShape::getExcludePrimPaths() const
{
    return _GetExcludePrimPaths( const_cast<UsdMayaProxyShape*>(this)->forceCache() );
}

SdfPathVector
UsdMayaProxyShape::_GetExcludePrimPaths(MDataBlock dataBlock) const
{
    SdfPathVector ret;

    const MString excludePrimPathsStr =
        dataBlock.inputValue(excludePrimPathsAttr).asString();
    std::vector<std::string> excludePrimPaths =
        TfStringTokenize(excludePrimPathsStr.asChar(), ",");
    ret.resize(excludePrimPaths.size());
    for (size_t i = 0; i < excludePrimPaths.size(); ++i) {
        ret[i] = SdfPath(TfStringTrim(excludePrimPaths[i]));
    }

    return ret;
}

bool
UsdMayaProxyShape::_GetDrawPurposeToggles(
        MDataBlock dataBlock,
        bool* drawRenderPurpose,
        bool* drawProxyPurpose,
        bool* drawGuidePurpose) const
{
    MStatus status;

    MDataHandle drawRenderPurposeHandle =
        dataBlock.inputValue(drawRenderPurposeAttr, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MDataHandle drawProxyPurposeHandle =
        dataBlock.inputValue(drawProxyPurposeAttr, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MDataHandle drawGuidePurposeHandle =
        dataBlock.inputValue(drawGuidePurposeAttr, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    if (drawRenderPurpose) {
        *drawRenderPurpose = drawRenderPurposeHandle.asBool();
    }
    if (drawProxyPurpose) {
        *drawProxyPurpose = drawProxyPurposeHandle.asBool();
    }
    if (drawGuidePurpose) {
        *drawGuidePurpose = drawGuidePurposeHandle.asBool();
    }

    return true;
}

bool
UsdMayaProxyShape::GetAllRenderAttributes(
        UsdPrim* usdPrimOut,
        SdfPathVector* excludePrimPathsOut,
        int* complexityOut,
        UsdTimeCode* timeOut,
        bool* drawRenderPurpose,
        bool* drawProxyPurpose,
        bool* drawGuidePurpose)
{
    MDataBlock dataBlock = forceCache();

    *usdPrimOut = _GetUsdPrim(dataBlock);
    if (!usdPrimOut->IsValid()) {
        return false;
    }

    *excludePrimPathsOut = _GetExcludePrimPaths(dataBlock);
    *complexityOut = _GetComplexity(dataBlock);
    *timeOut = _GetTime(dataBlock);

    _GetDrawPurposeToggles(
        dataBlock,
        drawRenderPurpose,
        drawProxyPurpose,
        drawGuidePurpose);

    return true;
}

UsdMayaProxyShape::UsdMayaProxyShape() :
    MPxSurfaceShape(),
    _useFastPlayback(false)
{
    TfRegistryManager::GetInstance().SubscribeTo<UsdMayaProxyShape>();
}

/* virtual */
UsdMayaProxyShape::~UsdMayaProxyShape()
{
    //
    // empty
    //
}

MSelectionMask
UsdMayaProxyShape::getShapeSelectionMask() const
{
    // The intent of this function is to control whether this object is
    // selectable at all in VP2

    // However, due to a bug / quirk, it could be used to specifically control
    // whether the object was SOFT-selectable if you were using
    // MAYA_VP2_USE_VP1_SELECTON; in this mode, this setting is NOT querierd
    // when doing "normal" selection, but IS queried when doing soft
    // selection.

    // Unfortunately, it is queried for both "normal" selection AND soft
    // selection if you are using "true" VP2 selection.  So in order to
    // control soft selection, in both modes, we keep track of whether
    // we currently have object soft-select enabled, and then return an empty
    // selection mask if it is, but this object is set to be non-soft-selectable

    static const MSelectionMask emptyMask;
    static const MSelectionMask normalMask(MSelectionMask::kSelectMeshes);

    if (GetObjectSoftSelectEnabled() && !_CanBeSoftSelected()) {
        // Disable selection, to disable soft-selection
        return emptyMask;
    }
    return normalMask;
}

bool
UsdMayaProxyShape::_CanBeSoftSelected() const
{
    UsdMayaProxyShape* nonConstThis = const_cast<UsdMayaProxyShape*>(this);
    MDataBlock dataBlock = nonConstThis->forceCache();
    MStatus status;
    MDataHandle softSelHandle =
        dataBlock.inputValue(softSelectableAttr, &status);
    if (!status) {
        return false;
    }

    return softSelHandle.asBool();
}

void
UsdMayaProxyShape::_OnStageContentsChanged(
        const UsdNotice::StageContentsChanged& notice)
{
    // If the USD stage this proxy represents changes without Maya's knowledge,
    // we need to inform Maya that the shape is dirty and needs to be redrawn.
    MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());
}

bool
UsdMayaProxyShape::closestPoint(
    const MPoint& raySource,
    const MVector& rayDirection,
    MPoint& theClosestPoint,
    MVector& theClosestNormal,
    bool  /*findClosestOnMiss*/,
    double  /*tolerance*/)
{
    if (_sharedClosestPointDelegate) {
        GfRay ray(
            GfVec3d(raySource.x, raySource.y, raySource.z),
            GfVec3d(rayDirection.x, rayDirection.y, rayDirection.z));
        GfVec3d hitPoint;
        GfVec3d hitNorm;
        if (_sharedClosestPointDelegate(*this, ray, &hitPoint, &hitNorm)) {
            theClosestPoint = MPoint(hitPoint[0], hitPoint[1], hitPoint[2]);
            theClosestNormal = MVector(hitNorm[0], hitNorm[1], hitNorm[2]);
            return true;
        }
    }

    return false;
}

bool UsdMayaProxyShape::canMakeLive() const {
    return (bool) _sharedClosestPointDelegate;
}


PXR_NAMESPACE_CLOSE_SCOPE
