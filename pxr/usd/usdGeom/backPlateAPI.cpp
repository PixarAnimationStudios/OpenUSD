//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/backPlateAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomBackPlateAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdGeomBackPlateAPI::~UsdGeomBackPlateAPI()
{
}

/* static */
UsdGeomBackPlateAPI
UsdGeomBackPlateAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomBackPlateAPI();
    }
    TfToken name;
    if (!IsBackPlateAPIPath(path, &name)) {
        TF_CODING_ERROR("Invalid backPlate path <%s>.", path.GetText());
        return UsdGeomBackPlateAPI();
    }
    return UsdGeomBackPlateAPI(stage->GetPrimAtPath(path.GetPrimPath()), name);
}

UsdGeomBackPlateAPI
UsdGeomBackPlateAPI::Get(const UsdPrim &prim, const TfToken &name)
{
    return UsdGeomBackPlateAPI(prim, name);
}

/* static */
std::vector<UsdGeomBackPlateAPI>
UsdGeomBackPlateAPI::GetAll(const UsdPrim &prim)
{
    std::vector<UsdGeomBackPlateAPI> schemas;
    
    for (const auto &schemaName :
         UsdAPISchemaBase::_GetMultipleApplyInstanceNames(prim, _GetStaticTfType())) {
        schemas.emplace_back(prim, schemaName);
    }

    return schemas;
}


/* static */
bool 
UsdGeomBackPlateAPI::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdGeomTokens->backPlate_MultipleApplyTemplate_ScaleTweak),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdGeomTokens->backPlate_MultipleApplyTemplate_RotateXYZTweak),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdGeomTokens->backPlate_MultipleApplyTemplate_TranslateTweak),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdGeomTokens->backPlate_MultipleApplyTemplate_Image),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdGeomTokens->backPlate_MultipleApplyTemplate_AlphaImage),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthImage),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthMinOffset),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthNormalizingFactor),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthCameraSpaceOffset),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaSlope),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaOffset),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaPower),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdGeomTokens->backPlate_MultipleApplyTemplate_PlateVisibility),
    };

    return find(attrsAndRels.begin(), attrsAndRels.end(), baseName)
            != attrsAndRels.end();
}

/* static */
bool
UsdGeomBackPlateAPI::IsBackPlateAPIPath(
    const SdfPath &path, TfToken *name)
{
    if (!path.IsPropertyPath()) {
        return false;
    }

    std::string propertyName = path.GetName();
    TfTokenVector tokens = SdfPath::TokenizeIdentifierAsTokens(propertyName);

    // The baseName of the  path can't be one of the 
    // schema properties. We should validate this in the creation (or apply)
    // API.
    TfToken baseName = *tokens.rbegin();
    if (IsSchemaPropertyBaseName(baseName)) {
        return false;
    }

    if (tokens.size() >= 2
        && tokens[0] == UsdGeomTokens->backPlate) {
        *name = TfToken(propertyName.substr(
           UsdGeomTokens->backPlate.GetString().size() + 1));
        return true;
    }

    return false;
}

/* virtual */
UsdSchemaKind UsdGeomBackPlateAPI::_GetSchemaKind() const
{
    return UsdGeomBackPlateAPI::schemaKind;
}

/* static */
bool
UsdGeomBackPlateAPI::CanApply(
    const UsdPrim &prim, const TfToken &name, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdGeomBackPlateAPI>(name, whyNot);
}

/* static */
UsdGeomBackPlateAPI
UsdGeomBackPlateAPI::Apply(const UsdPrim &prim, const TfToken &name)
{
    if (prim.ApplyAPI<UsdGeomBackPlateAPI>(name)) {
        return UsdGeomBackPlateAPI(prim, name);
    }
    return UsdGeomBackPlateAPI();
}

/* static */
const TfType &
UsdGeomBackPlateAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomBackPlateAPI>();
    return tfType;
}

/* static */
bool 
UsdGeomBackPlateAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomBackPlateAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/// Returns the property name prefixed with the correct namespace prefix, which
/// is composed of the the API's propertyNamespacePrefix metadata and the
/// instance name of the API.
static inline
TfToken
_GetNamespacedPropertyName(const TfToken instanceName, const TfToken propName)
{
    return UsdSchemaRegistry::MakeMultipleApplyNameInstance(propName, instanceName);
}

UsdAttribute
UsdGeomBackPlateAPI::GetScaleTweakAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdGeomTokens->backPlate_MultipleApplyTemplate_ScaleTweak));
}

UsdAttribute
UsdGeomBackPlateAPI::CreateScaleTweakAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdGeomTokens->backPlate_MultipleApplyTemplate_ScaleTweak),
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomBackPlateAPI::GetRotateXYZTweakAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdGeomTokens->backPlate_MultipleApplyTemplate_RotateXYZTweak));
}

UsdAttribute
UsdGeomBackPlateAPI::CreateRotateXYZTweakAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdGeomTokens->backPlate_MultipleApplyTemplate_RotateXYZTweak),
                       SdfValueTypeNames->Float3,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomBackPlateAPI::GetTranslateTweakAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdGeomTokens->backPlate_MultipleApplyTemplate_TranslateTweak));
}

UsdAttribute
UsdGeomBackPlateAPI::CreateTranslateTweakAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdGeomTokens->backPlate_MultipleApplyTemplate_TranslateTweak),
                       SdfValueTypeNames->Float3,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomBackPlateAPI::GetImageAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdGeomTokens->backPlate_MultipleApplyTemplate_Image));
}

UsdAttribute
UsdGeomBackPlateAPI::CreateImageAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdGeomTokens->backPlate_MultipleApplyTemplate_Image),
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomBackPlateAPI::GetAlphaImageAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdGeomTokens->backPlate_MultipleApplyTemplate_AlphaImage));
}

UsdAttribute
UsdGeomBackPlateAPI::CreateAlphaImageAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdGeomTokens->backPlate_MultipleApplyTemplate_AlphaImage),
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomBackPlateAPI::GetDepthImageAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthImage));
}

UsdAttribute
UsdGeomBackPlateAPI::CreateDepthImageAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthImage),
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomBackPlateAPI::GetDepthMinOffsetAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthMinOffset));
}

UsdAttribute
UsdGeomBackPlateAPI::CreateDepthMinOffsetAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthMinOffset),
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomBackPlateAPI::GetDepthNormalizingFactorAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthNormalizingFactor));
}

UsdAttribute
UsdGeomBackPlateAPI::CreateDepthNormalizingFactorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthNormalizingFactor),
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomBackPlateAPI::GetDepthCameraSpaceOffsetAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthCameraSpaceOffset));
}

UsdAttribute
UsdGeomBackPlateAPI::CreateDepthCameraSpaceOffsetAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthCameraSpaceOffset),
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomBackPlateAPI::GetLumaSlopeAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaSlope));
}

UsdAttribute
UsdGeomBackPlateAPI::CreateLumaSlopeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaSlope),
                       SdfValueTypeNames->Float3,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomBackPlateAPI::GetLumaOffsetAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaOffset));
}

UsdAttribute
UsdGeomBackPlateAPI::CreateLumaOffsetAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaOffset),
                       SdfValueTypeNames->Float3,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomBackPlateAPI::GetLumaPowerAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaPower));
}

UsdAttribute
UsdGeomBackPlateAPI::CreateLumaPowerAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaPower),
                       SdfValueTypeNames->Float3,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomBackPlateAPI::GetPlateVisibilityAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdGeomTokens->backPlate_MultipleApplyTemplate_PlateVisibility));
}

UsdAttribute
UsdGeomBackPlateAPI::CreatePlateVisibilityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdGeomTokens->backPlate_MultipleApplyTemplate_PlateVisibility),
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdGeomBackPlateAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->backPlate_MultipleApplyTemplate_ScaleTweak,
        UsdGeomTokens->backPlate_MultipleApplyTemplate_RotateXYZTweak,
        UsdGeomTokens->backPlate_MultipleApplyTemplate_TranslateTweak,
        UsdGeomTokens->backPlate_MultipleApplyTemplate_Image,
        UsdGeomTokens->backPlate_MultipleApplyTemplate_AlphaImage,
        UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthImage,
        UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthMinOffset,
        UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthNormalizingFactor,
        UsdGeomTokens->backPlate_MultipleApplyTemplate_DepthCameraSpaceOffset,
        UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaSlope,
        UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaOffset,
        UsdGeomTokens->backPlate_MultipleApplyTemplate_LumaPower,
        UsdGeomTokens->backPlate_MultipleApplyTemplate_PlateVisibility,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

/*static*/
TfTokenVector
UsdGeomBackPlateAPI::GetSchemaAttributeNames(
    bool includeInherited, const TfToken &instanceName)
{
    const TfTokenVector &attrNames = GetSchemaAttributeNames(includeInherited);
    if (instanceName.IsEmpty()) {
        return attrNames;
    }
    TfTokenVector result;
    result.reserve(attrNames.size());
    for (const TfToken &attrName : attrNames) {
        result.push_back(
            UsdSchemaRegistry::MakeMultipleApplyNameInstance(attrName, instanceName));
    }
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

PXR_NAMESPACE_OPEN_SCOPE
#include <cmath>
#include "pxr/base/gf/camera.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec2f.h"


float 
UsdGeomBackPlateAPI::ComputeEffectiveDimension(
    bool computeWidth, UsdTimeCode time) const {

    GfVec2f scale;
    float focalLength;
    float aperture;
    GetScaleTweakAttr().Get(&scale, time);
    float scaleFactor;

    GfCamera camera = UsdGeomCamera(GetPrim()).GetCamera(time);
    
    if (computeWidth) {
        aperture = camera.GetHorizontalAperture();
        scaleFactor = scale[0];
    } else {
        aperture = camera.GetVerticalAperture();
        scaleFactor = scale[1];
    }
    
    if (camera.GetProjection() == GfCamera::Orthographic) {
        return aperture * scaleFactor;
    }
    focalLength = camera.GetFocalLength() * GfCamera::FOCAL_LENGTH_UNIT;

    // Calculate the ratio of the aperture to the focal length and then multiply
    // by the focus distance and scaling factor to get the actual dimension.
    return camera.GetFocusDistance() * aperture * GfCamera::APERTURE_UNIT * 
           scaleFactor / focalLength;
}

void 
UsdGeomBackPlateAPI::SetAspectRatio(
    float width, float height, UsdTimeCode time) const {
    GfVec2f scale;
    GetScaleTweakAttr().Get(&scale, time);
    float computedWidth = ComputeEffectiveDimension(true, time);
    float computedHeight = ComputeEffectiveDimension(false, time);

    float adjustWidthFactor = width/computedWidth;
    float adjustHeightFactor = height/computedHeight;

    scale[0] *= adjustWidthFactor;
    scale[1] *= adjustHeightFactor;

    GetScaleTweakAttr().Set(scale);
}

void 
UsdGeomBackPlateAPI::SetCameraSpacePosition(GfVec3f pos, UsdTimeCode time) const {
    GfCamera camera = UsdGeomCamera(GetPrim()).GetCamera(time);
    GfVec3f translation  = pos - GfVec3f(0,0, camera.GetFocusDistance());
    GetTranslateTweakAttr().Set(translation, time);
}

GfVec3f 
UsdGeomBackPlateAPI::GetCameraSpacePosition(UsdTimeCode time) const {
    GfVec3f position;
    GfCamera camera = UsdGeomCamera(GetPrim()).GetCamera(time);
    GetTranslateTweakAttr().Get(&position, time);
    return position + GfVec3f(0,0, camera.GetFocusDistance());
}

void 
UsdGeomBackPlateAPI::SetWorldSpacePosition(GfVec3f pos, UsdTimeCode time) const {
    UsdGeomCamera cam = UsdGeomCamera(GetPrim());
    GfVec3f xform = static_cast<GfVec3f>(
        cam.ComputeLocalToWorldTransform(time).ExtractTranslation()); 
    UsdGeomBackPlateAPI::SetCameraSpacePosition(pos - xform, time);
}

GfVec3f 
UsdGeomBackPlateAPI::GetWorldSpacePosition(UsdTimeCode time) const {
    UsdGeomCamera cam = UsdGeomCamera(GetPrim());
    GfVec3f xform = static_cast<GfVec3f>(
        cam.ComputeLocalToWorldTransform(time).ExtractTranslation()); 
    return GetCameraSpacePosition(time) + xform;
}

PXR_NAMESPACE_CLOSE_SCOPE
