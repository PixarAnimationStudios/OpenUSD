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
#include "pxr/pxr.h"
#include "usdMaya/proxyShape.h"
#include "usdMaya/query.h"
#include "usdMaya/stageCache.h"
#include "usdMaya/stageData.h"

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/ray.h"
#include "pxr/base/gf/vec4f.h"
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
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/bboxCache.h"
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
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPoint.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
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


TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaProxyShapeTokens,
                        PXRUSDMAYA_PROXY_SHAPE_TOKENS);


// Hydra performs its own high-performance frustum culling, so
// we don't want to rely on Maya to do it on the CPU. AS such, the best
// performance comes from telling Maya to pretend that every object has no
// bounds.
//
// This has the side effect of disabling bounding box display mode, but that
// is usually an acceptable loss.
//
TF_DEFINE_ENV_SETTING(PIXMAYA_ENABLE_BOUNDING_BOX_MODE, false,
                      "Enable bounding box rendering (slows refresh rate)");

UsdMayaProxyShape::ClosestPointDelegate
UsdMayaProxyShape::_sharedClosestPointDelegate = nullptr;

UsdMayaProxyShape::ObjectSoftSelectEnabledDelgate
UsdMayaProxyShape::_sharedObjectSoftSelectEnabledDelgate = nullptr;

bool
UsdMayaIsBoundingBoxModeEnabled()
{
    return TfGetEnvSetting(PIXMAYA_ENABLE_BOUNDING_BOX_MODE);
}

// ========================================================

/* static */
void*
UsdMayaProxyShape::creator(const PluginStaticData& psData)
{
    return new UsdMayaProxyShape(psData);
}

/* static */
MStatus
UsdMayaProxyShape::initialize(PluginStaticData* psData)
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

    psData->filePath = typedAttrFn.create(
        "filePath",
        "fp",
        MFnData::kString,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setReadable(false);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->filePath);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->primPath = typedAttrFn.create(
        "primPath",
        "pp",
        MFnData::kString,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->primPath);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->excludePrimPaths = typedAttrFn.create(
        "excludePrimPaths",
        "epp",
        MFnData::kString,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->excludePrimPaths);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->time = unitAttrFn.create(
        "time",
        "tm",
        MFnUnitAttribute::kTime,
        0.0,
        &retValue);
    unitAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->time);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->variantKey = typedAttrFn.create(
        "variantKey",
        "variantKey",
        MFnData::kString,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setReadable(false);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->variantKey);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->complexity = numericAttrFn.create(
        "complexity",
        "cplx",
        MFnNumericData::kInt,
        0,
        &retValue);
    numericAttrFn.setMin(0);
    numericAttrFn.setSoftMax(4);
    numericAttrFn.setMax(8);
    numericAttrFn.setStorable(false); // not written to the file
    numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->complexity);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->inStageData = typedAttrFn.create(
        "inStageData",
        "id",
        psData->stageDataTypeId,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setReadable(false);
    typedAttrFn.setStorable(false);
    typedAttrFn.setDisconnectBehavior(MFnNumericAttribute::kReset); // on disconnect, reset to Null
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->inStageData);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // inStageData or filepath-> inStageDataCached -> outStageData
    psData->inStageDataCached = typedAttrFn.create(
        "inStageDataCached",
        "idc",
        psData->stageDataTypeId,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setStorable(false);
    typedAttrFn.setWritable(false);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->inStageDataCached);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->fastPlayback = numericAttrFn.create(
        "fastPlayback",
        "fs",
        MFnNumericData::kBoolean,
        0,
        &retValue);
    numericAttrFn.setInternal(true);
    numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->fastPlayback);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->tint = numericAttrFn.create(
        "tint",
        "tn",
        MFnNumericData::kBoolean,
        0,
        &retValue);
    numericAttrFn.setInternal(true);
    numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->tint);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->tintColor = numericAttrFn.createColor(
        "tintColor",
        "tcol",
        &retValue);
    numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->tintColor);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->outStageData = typedAttrFn.create(
        "outStageData",
        "od",
        psData->stageDataTypeId,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setStorable(false);
    typedAttrFn.setWritable(false);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->outStageData);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->displayGuides = numericAttrFn.create(
        "displayGuides",
        "displayGuides",
        MFnNumericData::kBoolean,
        0.0,
        &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setKeyable(true);
    numericAttrFn.setReadable(false);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(psData->displayGuides);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->displayRenderGuides = numericAttrFn.create(
        "displayRenderGuides",
        "displayRenderGuides",
        MFnNumericData::kBoolean,
        0.0,
        &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    numericAttrFn.setKeyable(true);
    numericAttrFn.setReadable(false);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(psData->displayRenderGuides);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->softSelectable = numericAttrFn.create(
        "softSelectable",
        "softSelectable",
        MFnNumericData::kBoolean,
        0.0,
        &retValue);
    numericAttrFn.setStorable(false);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(psData->softSelectable);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    //
    // add attribute dependencies
    //
    retValue = attributeAffects(psData->filePath, psData->inStageDataCached);
    retValue = attributeAffects(psData->filePath, psData->outStageData);

    retValue = attributeAffects(psData->primPath, psData->inStageDataCached);
    retValue = attributeAffects(psData->primPath, psData->outStageData);

    retValue = attributeAffects(psData->variantKey, psData->inStageDataCached);
    retValue = attributeAffects(psData->variantKey, psData->outStageData);

    retValue = attributeAffects(psData->inStageData, psData->inStageDataCached);
    retValue = attributeAffects(psData->inStageData, psData->outStageData);

    retValue = attributeAffects(psData->inStageDataCached, psData->outStageData);

    return retValue;
}

/* static */
UsdMayaProxyShape*
UsdMayaProxyShape::GetShapeAtDagPath(const MDagPath& dagPath)
{
    MObject mObj = dagPath.node();
    if (mObj.apiType() != MFn::kPluginShape) {
        MGlobal::displayError(
            TfStringPrintf(
                "Could not get UsdMayaProxyShape for non-plugin shape node "
                "at dag path: %s (apiTypeStr = %s)",
                dagPath.fullPathName().asChar(),
                mObj.apiTypeStr()).c_str());
        return nullptr;
    }

    const MFnDependencyNode depNodeFn(mObj);
    UsdMayaProxyShape* pShape =
        static_cast<UsdMayaProxyShape*>(depNodeFn.userNode());
    if (!pShape) {
        MGlobal::displayError(
            TfStringPrintf(
                "Could not get UsdMayaProxyShape for node at dag path: %s",
                dagPath.fullPathName().asChar()).c_str());
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
    //
    // don't allow shading groups to be assigned
    //
    setRenderable(true);
}

/* virtual */
MStatus
UsdMayaProxyShape::compute(const MPlug& plug, MDataBlock& dataBlock)
{
    if (plug == _psData.excludePrimPaths ||
            plug == _psData.time ||
            plug == _psData.complexity ||
            plug == _psData.tint ||
            plug == _psData.tintColor ||
            plug == _psData.displayGuides ||
            plug == _psData.displayRenderGuides) {
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
    else if (plug == _psData.inStageDataCached) {
        return computeInStageDataCached(dataBlock);
    }
    else if (plug == _psData.outStageData) {
        return computeOutStageData(dataBlock);
    }

    return MS::kUnknownParameter;
}

MStatus
UsdMayaProxyShape::computeInStageDataCached(MDataBlock& dataBlock)
{
    MStatus retValue = MS::kSuccess;

    MDataHandle inDataHandle = dataBlock.inputValue(_psData.inStageData, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // If inData has an incoming connection, then use it. Otherwise generate stage from the filepath
    if (!inDataHandle.data().isNull() ) {
        //
        // Propagate inData -> inDataCached
        //
        MDataHandle inDataCachedHandle = dataBlock.outputValue(_psData.inStageDataCached, &retValue);
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
        const MString file = dataBlock.inputValue(_psData.filePath, &retValue).asString();
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
            dataBlock.inputValue(_psData.variantKey, &retValue);
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
                dataBlock.inputValue(_psData.primPath, &retValue).asString();
            CHECK_MSTATUS_AND_RETURN_IT(retValue);

            std::vector<std::string> primPathEltStrs =
                TfStringTokenize(primPathMString.asChar(),"/");
            if (primPathEltStrs.size() > 0) {
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
        MFnPluginData pluginDataFactory;
        MObject aUsdStageDataObject = pluginDataFactory.create(
                _psData.stageDataTypeId,
            &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        UsdMayaStageData* usdStageData =
            reinterpret_cast<UsdMayaStageData*>(
                pluginDataFactory.data(&retValue));
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        // Set the outUsdStageData
        usdStageData->stage    = usdStage;
        usdStageData->primPath = primPath;

        //
        // set the data on the output plug
        //
        MDataHandle inDataCachedHandle = dataBlock.outputValue(_psData.inStageDataCached, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);

        inDataCachedHandle.set(usdStageData);
        inDataCachedHandle.setClean();
        return MS::kSuccess;
    }
    return MS::kFailure;
}

MStatus
UsdMayaProxyShape::computeOutStageData(MDataBlock& dataBlock)
{
    MStatus retValue = MS::kSuccess;

    TfReset(_boundingBoxCache);

    MDataHandle inDataCachedHandle = dataBlock.inputValue(_psData.inStageDataCached, &retValue);
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
        MDataHandle outDataHandle = dataBlock.outputValue(_psData.outStageData, &retValue);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);
        outDataHandle.copy(inDataCachedHandle);
        return MS::kSuccess;
    }

    // Get the primPath
    const MString primPath = dataBlock.inputValue(_psData.primPath, &retValue).asString();
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
            MGlobal::displayWarning(
                MPxSurfaceShape::name() + ": Stage primPath '" +
                MString(inData->primPath.GetText()) +
                "'' not a parent of primPath '");
        }
    } else {
        usdPrim = usdStage->GetPseudoRoot();
    }

    // Create the output outData
    MFnPluginData pluginDataFactory;
    MObject aUsdStageDataObject = pluginDataFactory.create(
            _psData.stageDataTypeId,
        &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    UsdMayaStageData* usdStageData =
        reinterpret_cast<UsdMayaStageData*>(
            pluginDataFactory.data(&retValue));
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    // Set the outUsdStageData
    usdStageData->stage    = usdStage;
    usdStageData->primPath = usdPrim ? usdPrim.GetPath() : usdStage->GetPseudoRoot().GetPath();

    //
    // set the data on the output plug
    //
    MDataHandle outDataHandle = dataBlock.outputValue(_psData.outStageData, &retValue);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    outDataHandle.set(usdStageData);
    outDataHandle.setClean();

    return MS::kSuccess;
}

/* virtual */
bool
UsdMayaProxyShape::isBounded() const
{
    return !_useFastPlayback && isStageValid()
                && TfGetEnvSetting(PIXMAYA_ENABLE_BOUNDING_BOX_MODE);
}

/* virtual */
MBoundingBox
UsdMayaProxyShape::boundingBox() const
{
    if (_useFastPlayback) {
        return MBoundingBox();
    }

    MStatus status;

    // Make sure outStage is up to date
    UsdMayaProxyShape* nonConstThis = const_cast<UsdMayaProxyShape*>(this);
    MDataBlock dataBlock = nonConstThis->forceCache();
    dataBlock.inputValue( _psData.outStageData, &status);
    CHECK_MSTATUS_AND_RETURN(status, MBoundingBox() );

    // XXX:
    // If we could cheaply determine whether a stage only has static geometry,
    // we could make this value a constant one for that case, avoiding the
    // memory overhead of a cache entry per frame
    MDataHandle timeHandle = dataBlock.inputValue(_psData.time, &status);
    UsdTimeCode currTime = UsdTimeCode(timeHandle.asTime().value());

    std::map<UsdTimeCode, MBoundingBox>::const_iterator cacheLookup =
        _boundingBoxCache.find(currTime);

    if (cacheLookup != _boundingBoxCache.end()) {
        return cacheLookup->second;
    }

    GfBBox3d allBox;
    UsdPrim prim = usdPrim();
    if (prim) {
        UsdGeomImageable imageablePrim( prim );
        bool showGuides = displayGuides();
        bool showRenderGuides = displayRenderGuides();
        if (showGuides && showRenderGuides) {
            allBox = imageablePrim.ComputeUntransformedBound(
                currTime,
                UsdGeomTokens->default_,
                UsdGeomTokens->proxy,
                UsdGeomTokens->guide,
                UsdGeomTokens->render);
        } else if (showGuides && !showRenderGuides) {
            allBox = imageablePrim.ComputeUntransformedBound(
                currTime,
                UsdGeomTokens->default_,
                UsdGeomTokens->proxy,
                UsdGeomTokens->guide);
        } else if (!showGuides && showRenderGuides) {
            allBox = imageablePrim.ComputeUntransformedBound(
                currTime,
                UsdGeomTokens->default_,
                UsdGeomTokens->proxy,
                UsdGeomTokens->render);
        } else {
            allBox = imageablePrim.ComputeUntransformedBound(
                currTime,
                UsdGeomTokens->default_,
                UsdGeomTokens->proxy);
        }
    } else {
        return MBoundingBox();
    }

    MBoundingBox &retval = nonConstThis->_boundingBoxCache[currTime];

    GfRange3d boxRange = allBox.ComputeAlignedBox();
    // Convert to GfRange3d to MBoundingBox
    if ( !boxRange.IsEmpty() ) {
        retval = MBoundingBox( MPoint( boxRange.GetMin()[0],
                                       boxRange.GetMin()[1],
                                       boxRange.GetMin()[2]),
                               MPoint( boxRange.GetMax()[0],
                                       boxRange.GetMax()[1],
                                       boxRange.GetMax()[2]) );
    }

    return retval;
}

bool
UsdMayaProxyShape::isStageValid() const
{
    MStatus localStatus;
    UsdMayaProxyShape* nonConstThis = const_cast<UsdMayaProxyShape*>(this);
    MDataBlock dataBlock = nonConstThis->forceCache();

    MDataHandle outDataHandle = dataBlock.inputValue( _psData.outStageData, &localStatus);
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
    if (plug == _psData.fastPlayback) {
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
    if (plug == _psData.fastPlayback) {
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

    MDataHandle outDataHandle = dataBlock.inputValue( _psData.outStageData, &localStatus);
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

    complexity = dataBlock.inputValue( _psData.complexity, &status).asInt();

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

    return UsdTimeCode(dataBlock.inputValue(_psData.time, &status).asTime().value());
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
        dataBlock.inputValue(_psData.excludePrimPaths).asString();
    std::vector<std::string> excludePrimPaths =
        TfStringTokenize(excludePrimPathsStr.asChar(), ",");
    ret.resize(excludePrimPaths.size());
    for (size_t i = 0; i < excludePrimPaths.size(); ++i) {
        ret[i] = SdfPath(TfStringTrim(excludePrimPaths[i]));
    }

    return ret;
}

bool
UsdMayaProxyShape::displayGuides() const
{
    return _GetDisplayGuides( const_cast<UsdMayaProxyShape*>(this)->forceCache() );
}

bool
UsdMayaProxyShape::_GetDisplayGuides(MDataBlock dataBlock) const
{
    MStatus status;
    bool retValue = true;

    MDataHandle displayGuidesHandle = dataBlock.inputValue( _psData.displayGuides, &status);
    CHECK_MSTATUS_AND_RETURN(status, true );

    retValue = displayGuidesHandle.asBool();

    return retValue;
}

bool
UsdMayaProxyShape::displayRenderGuides() const
{
    return _GetDisplayRenderGuides( const_cast<UsdMayaProxyShape*>(this)->forceCache() );
}

bool
UsdMayaProxyShape::_GetDisplayRenderGuides(MDataBlock dataBlock) const
{
    MStatus status;
    bool retValue = true;

    MDataHandle displayRenderGuidesHandle = dataBlock.inputValue( _psData.displayRenderGuides, &status);
    CHECK_MSTATUS_AND_RETURN(status, true );

    retValue = displayRenderGuidesHandle.asBool();

    return retValue;
}

bool
UsdMayaProxyShape::getTint(GfVec4f* outTintColor) const
{
    return _GetTint( const_cast<UsdMayaProxyShape*>(this)->forceCache(), outTintColor );
}

bool
UsdMayaProxyShape::_GetTint(MDataBlock dataBlock, GfVec4f* outTintColor) const
{
    // We're hardcoding this for now -- could add more support later if need be
    static const float tintAlpha = 0.35f;

    MStatus status;
    bool retValue = true;

    MDataHandle tintHandle = dataBlock.inputValue( _psData.tint, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    retValue = tintHandle.asBool();

    MDataHandle tintColorHandle = dataBlock.inputValue( _psData.tintColor, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    float3 &tintColor = tintColorHandle.asFloat3();

    *outTintColor = { tintColor[0], tintColor[1], tintColor[2], tintAlpha };

    return retValue;
}

bool
UsdMayaProxyShape::GetAllRenderAttributes(
        UsdPrim* usdPrimOut,
        SdfPathVector* excludePrimPathsOut,
        int* complexityOut,
        UsdTimeCode* timeOut,
        bool* guidesOut,
        bool* renderGuidesOut,
        bool* tint,
        GfVec4f* tintColor)
{
    MDataBlock dataBlock = forceCache();

    *usdPrimOut = _GetUsdPrim( dataBlock );
    if (!usdPrimOut->IsValid())
        return false;

    *excludePrimPathsOut = _GetExcludePrimPaths( dataBlock );
    *complexityOut = _GetComplexity( dataBlock );
    *timeOut = _GetTime( dataBlock );
    *guidesOut = _GetDisplayGuides( dataBlock );
    *renderGuidesOut = _GetDisplayRenderGuides( dataBlock );
    *tint = _GetTint( dataBlock, tintColor );

    return true;
}

UsdMayaProxyShape::UsdMayaProxyShape(const PluginStaticData& psData) :
        MPxSurfaceShape(),
        _psData(psData),
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
    MDataHandle softSelHandle = dataBlock.inputValue(_psData.softSelectable, &status);
    if (!status) {
        return false;
    }
    return softSelHandle.asBool();

}

bool
UsdMayaProxyShape::closestPoint(
    const MPoint& raySource,
    const MVector& rayDirection,
    MPoint& theClosestPoint,
    MVector& theClosestNormal,
    bool findClosestOnMiss,
    double tolerance)
{
    if (_sharedClosestPointDelegate) {
        GfRay ray(
            GfVec3d(raySource.x, raySource.y, raySource.z),
            GfVec3d(rayDirection.x, rayDirection.y, rayDirection.z));
        GfVec3d hitPoint;
        if (_sharedClosestPointDelegate(*this, ray, &hitPoint)) {
            theClosestPoint = MPoint(hitPoint[0], hitPoint[1], hitPoint[2]);
            // XXX: Need support in Hydra for sidecar information like surface
            // normals in order to implement this. Right now, we're just
            // returning a sane default of the up-axis.
            theClosestNormal = MGlobal::upAxis();
            return true;
        }
    }

    return false;
}

bool UsdMayaProxyShape::canMakeLive() const {
    return (bool) _sharedClosestPointDelegate;
}


PXR_NAMESPACE_CLOSE_SCOPE
