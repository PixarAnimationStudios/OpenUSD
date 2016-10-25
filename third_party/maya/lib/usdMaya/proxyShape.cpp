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
#include "usdMaya/query.h"
#include "usdMaya/stageCache.h"
#include "usdMaya/stageData.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/hash.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usd/stageCacheContext.h"
#include "pxr/usd/usdUtils/stageCache.h"
#include "pxr/base/gf/bbox3d.h"

#include <maya/MDagPath.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MGlobal.h>
#include <maya/MFnPluginData.h>

#include <mutex>

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

// ========================================================

/* static */
void* 
UsdMayaProxyShape::creator(
        const PluginStaticData& psData)
{
    return new UsdMayaProxyShape(psData);
}

/* static */
MStatus 
UsdMayaProxyShape::initialize(
        PluginStaticData* psData)
{
    MStatus retValue = MS::kSuccess;

    //
    // create attr factories
    //
    MFnNumericAttribute  numericAttrFn;
    MFnTypedAttribute    typedAttrFn;
    MFnEnumAttribute     enumAttrFn;
    MFnCompoundAttribute compoundAttrFn;

    psData->filePath = typedAttrFn.create(
        "filePath",
        "fp",
        MFnData::kString,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setCached(true);
    typedAttrFn.setReadable(false);
    typedAttrFn.setStorable(true);
    typedAttrFn.setWritable(true);
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
    typedAttrFn.setCached(true);
    typedAttrFn.setReadable(true);
    typedAttrFn.setStorable(true);
    typedAttrFn.setWritable(true);
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
    typedAttrFn.setCached(true);
    typedAttrFn.setReadable(true);
    typedAttrFn.setStorable(true);
    typedAttrFn.setWritable(true);
    typedAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->excludePrimPaths);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->time = numericAttrFn.create(
        "time",
        "tm",
        MFnNumericData::kDouble,
        0,    // Default to "default"
        &retValue);
    numericAttrFn.setCached(true);
    numericAttrFn.setConnectable(true);
    numericAttrFn.setReadable(true);
    numericAttrFn.setStorable(true);
    numericAttrFn.setWritable(true);
    numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->time);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->variantKey = typedAttrFn.create(
        "variantKey",
        "variantKey",
        MFnData::kString,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setCached(true);
    typedAttrFn.setReadable(false);
    typedAttrFn.setStorable(true);
    typedAttrFn.setWritable(true);
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
    numericAttrFn.setWritable(true);
    numericAttrFn.setKeyable(true);
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
    typedAttrFn.setCached(true);
    typedAttrFn.setReadable(false);
    typedAttrFn.setStorable(false);
    typedAttrFn.setWritable(true);
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
    typedAttrFn.setCached(true);
    typedAttrFn.setReadable(true);
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
    numericAttrFn.setKeyable(false);
    numericAttrFn.setReadable(true);
    numericAttrFn.setStorable(true);
    numericAttrFn.setWritable(true);
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
    numericAttrFn.setKeyable(false);
    numericAttrFn.setReadable(true);
    numericAttrFn.setStorable(true);
    numericAttrFn.setWritable(true);
    numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->tint);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->tintColor = numericAttrFn.createColor(
        "tintColor",
        "tcol",
        &retValue);
    numericAttrFn.setCached(true);
    numericAttrFn.setKeyable(false);
    numericAttrFn.setReadable(true);
    numericAttrFn.setStorable(true);
    numericAttrFn.setWritable(true);
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
    typedAttrFn.setCached(true);
    typedAttrFn.setReadable(true);
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
    numericAttrFn.setCached(true);
    numericAttrFn.setKeyable(true);
    numericAttrFn.setReadable(false);
    numericAttrFn.setStorable(true);
    numericAttrFn.setWritable(true);
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
    numericAttrFn.setCached(true);
    numericAttrFn.setKeyable(true);
    numericAttrFn.setReadable(false);
    numericAttrFn.setStorable(true);
    numericAttrFn.setWritable(true);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(psData->displayRenderGuides);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    psData->softSelectable = numericAttrFn.create(
        "softSelectable",
        "softSelectable",
        MFnNumericData::kBoolean,
        0.0,
        &retValue);
    numericAttrFn.setKeyable(false);
    numericAttrFn.setStorable(false);
    numericAttrFn.setAffectsAppearance(true);
    retValue = addAttribute(psData->softSelectable);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    //
    // add attribute dependencies
    //
    retValue = attributeAffects(psData->inStageData, psData->inStageDataCached);
    retValue = attributeAffects(psData->inStageData, psData->outStageData);

    retValue = attributeAffects(psData->filePath, psData->inStageDataCached);
    retValue = attributeAffects(psData->filePath, psData->outStageData);

    retValue = attributeAffects(psData->variantKey, psData->inStageDataCached);
    retValue = attributeAffects(psData->variantKey, psData->outStageData);

    retValue = attributeAffects(psData->inStageDataCached, psData->outStageData);

    retValue = attributeAffects(psData->primPath, psData->outStageData);
    
    return retValue;
}

void UsdMayaProxyShape::postConstructor()
{
    // 
    // don't allow shading groups to be assigned
    //
    setRenderable(true);
}



MStatus UsdMayaProxyShape::compute(
    const MPlug& plug,
    MDataBlock& dataBlock)
{
    MStatus retValue = MS::kUnknownParameter;

    //
    // make sure the state of the model is normal
    //

    if(plug == _psData.inStageDataCached) 
    {
        retValue = computeInStageDataCached(dataBlock);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);
    }
    else if(plug == _psData.outStageData) 
    {
        retValue = computeOutStageData(dataBlock);
        CHECK_MSTATUS_AND_RETURN_IT(retValue);
    }
    else {
        return MS::kUnknownParameter;
    }

    return MS::kSuccess;
}


MStatus UsdMayaProxyShape::computeInStageDataCached(MDataBlock& dataBlock)
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

        if (not TfStringStartsWith(fileString, "./")) {
            fileString = PxrUsdMayaQuery::ResolvePath(fileString);
        }

        // Fall back on checking if path is just a standard absolute path
        if ( fileString.empty() ) {
            fileString = file.asChar();
        }

        // == Load the Stage
        UsdStageRefPtr usdStage;
        SdfPath        primPath;

        // Check path validity
        // Don't try to create a stage for a non-existent file. Some processes
        // such as mbuild may author a file path here does not yet exist until a
        // later operation (e.g., the mayaConvert target will produce the .mb
        // for the USD standin before the usd target runs the usdModelForeman to
        // assemble all the necessary usd files).
        bool isValidPath = (TfStringStartsWith(fileString, "//") ||
                            TfIsFile(fileString, true /*resolveSymlinks*/));

        if (isValidPath) {

            // get the variantKey
            MDataHandle variantKeyHandle = 
                dataBlock.inputValue(_psData.variantKey, &retValue);
            CHECK_MSTATUS_AND_RETURN_IT(retValue);
            const MString variantKey = variantKeyHandle.asString();
            
            SdfLayerRefPtr sessionLayer;
            std::vector<std::pair<std::string, std::string> > variantSelections;
            std::string variantKeyString = variantKey.asChar();
            if (not variantKeyString.empty()) {
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
            SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(fileString);
            {
                UsdStageCacheContext ctx(UsdMayaStageCache::Get());
                if (sessionLayer) {
                    usdStage = UsdStage::Open(rootLayer, 
                            sessionLayer,
                            ArGetResolver().GetCurrentContext());
                } else {
                    usdStage = UsdStage::Open(rootLayer,
                            ArGetResolver().GetCurrentContext());
                }
            }
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



MStatus UsdMayaProxyShape::computeOutStageData(MDataBlock& dataBlock)
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
                MPxNode::name() + ": Stage primPath '" + MString(inData->primPath.GetText()) +
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


bool UsdMayaProxyShape::isBounded() const
{
    return !_useFastPlayback && isStageValid()
                && TfGetEnvSetting(PIXMAYA_ENABLE_BOUNDING_BOX_MODE);
}


MBoundingBox UsdMayaProxyShape::boundingBox() const
{
    if (_useFastPlayback)
        return MBoundingBox();    

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
    MDataHandle timeHandle = dataBlock.inputValue( _psData.time, &status);
    UsdTimeCode currTime = UsdTimeCode(timeHandle.asDouble());

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
        if (showGuides and showRenderGuides) {
            allBox = imageablePrim.ComputeUntransformedBound(
                currTime,
                UsdGeomTokens->default_, 
                UsdGeomTokens->proxy,
                UsdGeomTokens->guide, 
                UsdGeomTokens->render);
        } else if (showGuides and not showRenderGuides) {
            allBox = imageablePrim.ComputeUntransformedBound(
                currTime,
                UsdGeomTokens->default_, 
                UsdGeomTokens->proxy,
                UsdGeomTokens->guide);
        } else if (not showGuides and showRenderGuides) {
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

bool UsdMayaProxyShape::isStageValid() const 
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

bool UsdMayaProxyShape::setInternalValueInContext(
    const MPlug& plug,
    const MDataHandle& dataHandle,
    MDGContext& ctx)
{
    if (plug == _psData.fastPlayback)
    {
        _useFastPlayback = dataHandle.asBool();
    	return true;
    }
    
    return MPxSurfaceShape::setInternalValueInContext(plug, dataHandle, ctx);
}

bool UsdMayaProxyShape::getInternalValueInContext(
    const MPlug& plug,
    MDataHandle& dataHandle,
    MDGContext& ctx)
{
    if (plug == _psData.fastPlayback)
    {
        dataHandle.set(_useFastPlayback);
        return true;
    }
    
    return MPxSurfaceShape::setInternalValueInContext(plug, dataHandle, ctx);
}


UsdPrim UsdMayaProxyShape::usdPrim() const 
{
    return _GetUsdPrim( const_cast<UsdMayaProxyShape*>(this)->forceCache() );
}

UsdPrim UsdMayaProxyShape::_GetUsdPrim( MDataBlock dataBlock ) const
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

int UsdMayaProxyShape::getComplexity() const
{
    return _GetComplexity( const_cast<UsdMayaProxyShape*>(this)->forceCache() );
}

int UsdMayaProxyShape::_GetComplexity( MDataBlock dataBlock ) const
{
    int complexity = 0;
    MStatus status;

    complexity = dataBlock.inputValue( _psData.complexity, &status).asInt();

    return complexity;
}

UsdTimeCode UsdMayaProxyShape::getTime() const
{
    return _GetTime( const_cast<UsdMayaProxyShape*>(this)->forceCache() );
}

UsdTimeCode UsdMayaProxyShape::_GetTime( MDataBlock dataBlock ) const
{
    MStatus status;

    return UsdTimeCode(dataBlock.inputValue( _psData.time, &status).asDouble());
}

SdfPathVector
UsdMayaProxyShape::getExcludePrimPaths() const 
{
    return _GetExcludePrimPaths( const_cast<UsdMayaProxyShape*>(this)->forceCache() );
}

SdfPathVector
UsdMayaProxyShape::_GetExcludePrimPaths( MDataBlock dataBlock )  const
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
UsdMayaProxyShape::_GetDisplayGuides( MDataBlock dataBlock ) const
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
UsdMayaProxyShape::_GetDisplayRenderGuides( MDataBlock dataBlock ) const
{
    MStatus status;
    bool retValue = true;

    MDataHandle displayRenderGuidesHandle = dataBlock.inputValue( _psData.displayRenderGuides, &status);
    CHECK_MSTATUS_AND_RETURN(status, true );
    
    retValue = displayRenderGuidesHandle.asBool();

    return retValue;
}

bool
UsdMayaProxyShape::getTint(GfVec4f *outTintColor) const
{
    return _GetTint( const_cast<UsdMayaProxyShape*>(this)->forceCache(), outTintColor );
}

bool
UsdMayaProxyShape::_GetTint( MDataBlock dataBlock, GfVec4f *outTintColor ) const
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
    GfVec4f* tintColor )
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



UsdMayaProxyShape::UsdMayaProxyShape(
        const PluginStaticData& psData)
    : MPxSurfaceShape()
    , _psData(psData)
    , _useFastPlayback( false )
{
    //
    // empty
    //
}

UsdMayaProxyShape::~UsdMayaProxyShape()
{
    //
    // empty
    //
}

MSelectionMask  
UsdMayaProxyShape::getShapeSelectionMask() const
{
    if (_CanBeSoftSelected()) {
        // to support soft selection (mode=Object), we need to add kSelectMeshes
        // to our selection mask.  
        MSelectionMask::SelectionType selType = MSelectionMask::kSelectMeshes;
        return MSelectionMask(selType);
    }
    return MPxSurfaceShape::getShapeSelectionMask();
}

bool
UsdMayaProxyShape::_CanBeSoftSelected() const
{
    UsdMayaProxyShape* nonConstThis = const_cast<UsdMayaProxyShape*>(this);
    MDataBlock dataBlock = nonConstThis->forceCache();
    MStatus status;
    MDataHandle softSelHandle = dataBlock.inputValue(_psData.softSelectable, &status);
    if (not status) {
        return false;
    }
    return softSelHandle.asBool();

}
