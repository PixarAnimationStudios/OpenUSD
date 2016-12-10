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
#include "usdMaya/translatorCamera.h"

#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"
#include "usdMaya/translatorUtil.h"
#include "usdMaya/util.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <maya/MDagModifier.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnCamera.h>
#include <maya/MPlug.h>
#include <maya/MObject.h>

#include <string>
#include <vector>

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((MayaCameraTypeName, "camera"))
    ((MayaCameraShapeNameSuffix, "Shape"))

    ((MayaCameraAttrNameHorizontalAperture, "horizontalFilmAperture"))
    ((MayaCameraAttrNameVerticalAperture, "verticalFilmAperture"))
    ((MayaCameraAttrNameHorizontalApertureOffset, "horizontalFilmOffset"))
    ((MayaCameraAttrNameVerticalApertureOffset, "verticalFilmOffset"))
    ((MayaCameraAttrNameFocalLength, "focalLength"))
    ((MayaCameraAttrNameFocusDistance, "focusDistance"))
    ((MayaCameraAttrNameFStop, "fStop"))
    ((MayaCameraAttrNameNearClippingPlane, "nearClipPlane"))
    ((MayaCameraAttrNameFarClippingPlane, "farClipPlane"))
);


static
bool
_CheckUsdTypeAndResizeArrays(
        const UsdAttribute& usdAttr,
        const TfType& expectedType,
        const PxrUsdMayaPrimReaderArgs& args,
        std::vector<double>* timeSamples,
        MTimeArray* timeArray,
        MDoubleArray* valueArray)
{
    // Validate that the attribute holds values of the expected type.
    const TfType type = usdAttr.GetTypeName().GetType();
    if (type != expectedType) {
        TF_CODING_ERROR("Unsupported type name for USD attribute '%s': %s",
            usdAttr.GetName().GetText(), type.GetTypeName().c_str());
        return false;
    }

    if (not PxrUsdMayaTranslatorUtil::GetTimeSamples(usdAttr, args,
            timeSamples)) {
        return false;
    }

    size_t numTimeSamples = timeSamples->size();
    if (numTimeSamples < 1) {
        return false;
    }

    timeArray->setLength(numTimeSamples);
    valueArray->setLength(numTimeSamples);

    return true;
}

static
bool
_GetTimeAndValueArrayForUsdAttribute(
        const UsdAttribute& usdAttr,
        const PxrUsdMayaPrimReaderArgs& args,
        MTimeArray* timeArray,
        MDoubleArray* valueArray,
        bool millimetersToInches=false)
{
    static const TfType& floatType = TfType::Find<float>();
    std::vector<double> timeSamples;

    if (not _CheckUsdTypeAndResizeArrays(usdAttr,
                                         floatType,
                                         args,
                                         &timeSamples,
                                         timeArray,
                                         valueArray)) {
        return false;
    }

    size_t numTimeSamples = timeSamples.size();

    for (size_t i = 0; i < numTimeSamples; ++i) {
        const double timeSample = timeSamples[i];
        float attrValue;
        if (not usdAttr.Get(&attrValue, timeSample)) {
            return false;
        }
        if (millimetersToInches) {
            attrValue = PxrUsdMayaUtil::ConvertMMToInches(attrValue);
        }
        timeArray->set(MTime(timeSample), i);
        valueArray->set(attrValue, i);
    }

    return true;
}

// This is primarily intended for use in translating the clippingRange
// USD attribute which is stored in USD as a single GfVec2f value but
// in Maya as separate nearClipPlane and farClipPlane attributes.
static
bool
_GetTimeAndValueArraysForUsdAttribute(
        const UsdAttribute& usdAttr,
        const PxrUsdMayaPrimReaderArgs& args,
        MTimeArray* timeArray,
        MDoubleArray* valueArray1,
        MDoubleArray* valueArray2)
{
    static const TfType& vec2fType = TfType::Find<GfVec2f>();
    std::vector<double> timeSamples;

    if (not _CheckUsdTypeAndResizeArrays(usdAttr,
                                         vec2fType,
                                         args,
                                         &timeSamples,
                                         timeArray,
                                         valueArray1)) {
        return false;
    }

    size_t numTimeSamples = timeSamples.size();
    valueArray2->setLength(numTimeSamples);

    for (size_t i = 0; i < numTimeSamples; ++i) {
        const double timeSample = timeSamples[i];
        GfVec2f attrValue;
        if (not usdAttr.Get(&attrValue, timeSample)) {
            return false;
        }
        timeArray->set(MTime(timeSample), i);
        valueArray1->set(attrValue[0], i);
        valueArray2->set(attrValue[1], i);
    }

    return true;
}

static
bool
_CreateAnimCurveForPlug(
        MPlug& plug,
        MTimeArray& timeArray,
        MDoubleArray& valueArray,
        PxrUsdMayaPrimReaderContext* context)
{
    MFnAnimCurve animFn;
    MStatus status;
    MObject animObj = animFn.create(plug, NULL, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    status = animFn.addKeys(&timeArray, &valueArray);
    CHECK_MSTATUS_AND_RETURN(status, false);

    if (context) {
        // used for undo/redo
        context->RegisterNewMayaNode(animFn.name().asChar(), animObj);
    }

    return true;
}

static
bool
_TranslateAnimatedUsdAttributeToPlug(
        const UsdAttribute& usdAttr,
        MPlug& plug,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context,
        bool millimetersToInches=false)
{
    if (not args.GetReadAnimData()) {
        return false;
    }

    MTimeArray timeArray;
    MDoubleArray valueArray;
    if (not _GetTimeAndValueArrayForUsdAttribute(usdAttr,
                                                 args,
                                                 &timeArray,
                                                 &valueArray,
                                                 millimetersToInches)) {
        return false;
    }

    if (not _CreateAnimCurveForPlug(plug, timeArray, valueArray, context)) {
        return false;
    }

    return true;
}

static
bool
_TranslateAnimatedUsdAttributeToPlugs(
        const UsdAttribute& usdAttr,
        MPlug& plug1,
        MPlug& plug2,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context)
{
    if (not args.GetReadAnimData()) {
        return false;
    }

    MTimeArray timeArray;
    MDoubleArray valueArray1;
    MDoubleArray valueArray2;
    if (not _GetTimeAndValueArraysForUsdAttribute(usdAttr,
                                                  args,
                                                  &timeArray,
                                                  &valueArray1,
                                                  &valueArray2)) {
        return false;
    }

    if (not _CreateAnimCurveForPlug(plug1, timeArray, valueArray1, context)) {
        return false;
    }

    if (not _CreateAnimCurveForPlug(plug2, timeArray, valueArray2, context)) {
        return false;
    }

    return true;
}

static
bool
_TranslateUsdAttributeToPlug(
        const UsdAttribute& usdAttr,
        const MFnCamera& cameraFn,
        TfToken plugName,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context,
        bool millimetersToInches=false)
{
    MStatus status;

    MPlug plug = cameraFn.findPlug(plugName.GetText(), true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // First check for and translate animation if there is any.
    if (not _TranslateAnimatedUsdAttributeToPlug(usdAttr,
                                                 plug,
                                                 args,
                                                 context,
                                                 millimetersToInches)) {
        // If that fails, then try just setting a static value.
        UsdTimeCode timeCode = UsdTimeCode::EarliestTime();
        float attrValue;
        usdAttr.Get(&attrValue, timeCode);
        if (millimetersToInches) {
            attrValue = PxrUsdMayaUtil::ConvertMMToInches(attrValue);
        }
        status = plug.setFloat(attrValue);
        CHECK_MSTATUS_AND_RETURN(status, false);
    }

    return true;
}

/* static */
bool
PxrUsdMayaTranslatorCamera::Read(
        const UsdGeomCamera& usdCamera,
        MObject parentNode,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context)
{
    if (not usdCamera) {
        return false;
    }

    const UsdPrim& prim = usdCamera.GetPrim();
    const SdfPath primPath = prim.GetPath();

    MStatus status;

    // Create the transform node for the camera.
    MObject transformObj;
    if (not PxrUsdMayaTranslatorUtil::CreateTransformNode(prim,
                                                          parentNode,
                                                          args,
                                                          context,
                                                          &status,
                                                          &transformObj)) {
        return false;
    }

    // Create the camera shape node.
    MDagModifier dagMod;
    MObject cameraObj = dagMod.createNode(_tokens->MayaCameraTypeName.GetText(),
                                          transformObj,
                                          &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = dagMod.doIt();
    CHECK_MSTATUS_AND_RETURN(status, false);
    TF_VERIFY(not cameraObj.isNull());
    MFnCamera cameraFn(cameraObj, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    const std::string cameraShapeName = prim.GetName().GetString() +
        _tokens->MayaCameraShapeNameSuffix.GetString();
    cameraFn.setName(cameraShapeName.c_str(), &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    if (context) {
        const SdfPath shapePrimPath = primPath.AppendChild(TfToken(cameraShapeName));
        context->RegisterNewMayaNode(shapePrimPath.GetString(), cameraObj);
    }

    // Now translate all of the USD camera attributes over to plugs on the
    // Maya cameraFn.
    UsdTimeCode timeCode = UsdTimeCode::EarliestTime();
    UsdAttribute usdAttr;
    TfToken plugName;

    // Set the type of projection. This is NOT keyable in Maya.
    TfToken projection;
    usdCamera.GetProjectionAttr().Get(&projection, timeCode);
    const bool isOrthographic = (projection == UsdGeomTokens->orthographic);
    status = cameraFn.setIsOrtho(isOrthographic);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Setup the aperture.
    usdAttr = usdCamera.GetHorizontalApertureAttr();
    plugName = _tokens->MayaCameraAttrNameHorizontalAperture;
    if (not _TranslateUsdAttributeToPlug(usdAttr, cameraFn, plugName,
                                         args, context,
                                         /* millimetersToInches */ true)) {
        return false;
    }

    usdAttr = usdCamera.GetVerticalApertureAttr();
    plugName = _tokens->MayaCameraAttrNameVerticalAperture;
    if (not _TranslateUsdAttributeToPlug(usdAttr, cameraFn, plugName,
                                         args, context,
                                         /* millimetersToInches */ true)) {
        return false;
    }

    // XXX: 
    // Lens Squeeze Ratio is DEPRECATED on USD schema.
    // Writing it out here for backwards compatibility (see bug 123124).
    cameraFn.setLensSqueezeRatio(1.0);

    usdAttr = usdCamera.GetHorizontalApertureOffsetAttr();
    plugName = _tokens->MayaCameraAttrNameHorizontalApertureOffset;
    if (not _TranslateUsdAttributeToPlug(usdAttr, cameraFn, plugName,
                                         args, context,
                                         /* millimetersToInches */ true)) {
        return false;
    }

    usdAttr = usdCamera.GetVerticalApertureOffsetAttr();
    plugName = _tokens->MayaCameraAttrNameVerticalApertureOffset;
    if (not _TranslateUsdAttributeToPlug(usdAttr, cameraFn, plugName,
                                         args, context,
                                         /* millimetersToInches */ true)) {
        return false;
    }

    // Set the lens parameters.
    usdAttr = usdCamera.GetFocalLengthAttr();
    plugName = _tokens->MayaCameraAttrNameFocalLength;
    if (not _TranslateUsdAttributeToPlug(usdAttr, cameraFn, plugName,
                                         args, context)) {
        return false;
    }

    usdAttr = usdCamera.GetFocusDistanceAttr();
    plugName = _tokens->MayaCameraAttrNameFocusDistance;
    if (not _TranslateUsdAttributeToPlug(usdAttr, cameraFn, plugName,
                                         args, context)) {
        return false;
    }

    usdAttr = usdCamera.GetFStopAttr();
    plugName = _tokens->MayaCameraAttrNameFStop;
    if (not _TranslateUsdAttributeToPlug(usdAttr, cameraFn, plugName,
                                         args, context)) {
        return false;
    }

    // Set the clipping planes. This one is a little different from the others
    // because it is stored in USD as a single GfVec2f value but in Maya as
    // separate nearClipPlane and farClipPlane attributes.
    usdAttr = usdCamera.GetClippingRangeAttr();
    MPlug nearClipPlug = cameraFn.findPlug(
        _tokens->MayaCameraAttrNameNearClippingPlane.GetText(), true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    MPlug farClipPlug = cameraFn.findPlug(
        _tokens->MayaCameraAttrNameFarClippingPlane.GetText(), true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    if (not _TranslateAnimatedUsdAttributeToPlugs(usdAttr,
                                                  nearClipPlug,
                                                  farClipPlug,
                                                  args,
                                                  context)) {
        GfVec2f clippingRange;
        usdCamera.GetClippingRangeAttr().Get(&clippingRange, timeCode);
        status = cameraFn.setNearClippingPlane(clippingRange[0]);
        CHECK_MSTATUS_AND_RETURN(status, false);
        status = cameraFn.setFarClippingPlane(clippingRange[1]);
        CHECK_MSTATUS_AND_RETURN(status, false);
    }

    return true;
}
