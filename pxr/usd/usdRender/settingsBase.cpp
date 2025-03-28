//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdRender/settingsBase.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRenderSettingsBase,
        TfType::Bases< UsdTyped > >();
    
}

/* virtual */
UsdRenderSettingsBase::~UsdRenderSettingsBase()
{
}

/* static */
UsdRenderSettingsBase
UsdRenderSettingsBase::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRenderSettingsBase();
    }
    return UsdRenderSettingsBase(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdRenderSettingsBase::_GetSchemaKind() const
{
    return UsdRenderSettingsBase::schemaKind;
}

/* static */
const TfType &
UsdRenderSettingsBase::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRenderSettingsBase>();
    return tfType;
}

/* static */
bool 
UsdRenderSettingsBase::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRenderSettingsBase::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRenderSettingsBase::GetResolutionAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->resolution);
}

UsdAttribute
UsdRenderSettingsBase::CreateResolutionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->resolution,
                       SdfValueTypeNames->Int2,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRenderSettingsBase::GetPixelAspectRatioAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->pixelAspectRatio);
}

UsdAttribute
UsdRenderSettingsBase::CreatePixelAspectRatioAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->pixelAspectRatio,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRenderSettingsBase::GetAspectRatioConformPolicyAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->aspectRatioConformPolicy);
}

UsdAttribute
UsdRenderSettingsBase::CreateAspectRatioConformPolicyAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->aspectRatioConformPolicy,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRenderSettingsBase::GetDataWindowNDCAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->dataWindowNDC);
}

UsdAttribute
UsdRenderSettingsBase::CreateDataWindowNDCAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->dataWindowNDC,
                       SdfValueTypeNames->Float4,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRenderSettingsBase::GetInstantaneousShutterAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->instantaneousShutter);
}

UsdAttribute
UsdRenderSettingsBase::CreateInstantaneousShutterAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->instantaneousShutter,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRenderSettingsBase::GetDisableMotionBlurAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->disableMotionBlur);
}

UsdAttribute
UsdRenderSettingsBase::CreateDisableMotionBlurAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->disableMotionBlur,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRenderSettingsBase::GetDisableDepthOfFieldAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->disableDepthOfField);
}

UsdAttribute
UsdRenderSettingsBase::CreateDisableDepthOfFieldAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->disableDepthOfField,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdRenderSettingsBase::GetCameraRel() const
{
    return GetPrim().GetRelationship(UsdRenderTokens->camera);
}

UsdRelationship
UsdRenderSettingsBase::CreateCameraRel() const
{
    return GetPrim().CreateRelationship(UsdRenderTokens->camera,
                       /* custom = */ false);
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
UsdRenderSettingsBase::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRenderTokens->resolution,
        UsdRenderTokens->pixelAspectRatio,
        UsdRenderTokens->aspectRatioConformPolicy,
        UsdRenderTokens->dataWindowNDC,
        UsdRenderTokens->instantaneousShutter,
        UsdRenderTokens->disableMotionBlur,
        UsdRenderTokens->disableDepthOfField,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdTyped::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
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
