//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/gaussiansAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldGaussiansAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldGaussiansAPI::~UsdLightFieldGaussiansAPI()
{
}

/* static */
UsdLightFieldGaussiansAPI
UsdLightFieldGaussiansAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldGaussiansAPI();
    }
    return UsdLightFieldGaussiansAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldGaussiansAPI::_GetSchemaKind() const
{
    return UsdLightFieldGaussiansAPI::schemaKind;
}

/* static */
bool
UsdLightFieldGaussiansAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldGaussiansAPI>(whyNot);
}

/* static */
UsdLightFieldGaussiansAPI
UsdLightFieldGaussiansAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldGaussiansAPI>()) {
        return UsdLightFieldGaussiansAPI(prim);
    }
    return UsdLightFieldGaussiansAPI();
}

/* static */
const TfType &
UsdLightFieldGaussiansAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldGaussiansAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldGaussiansAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldGaussiansAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLightFieldGaussiansAPI::GetGaussianShapeAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->gaussianShape);
}

UsdAttribute
UsdLightFieldGaussiansAPI::CreateGaussianShapeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->gaussianShape,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldGaussiansAPI::GetSortingModeHintAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->sortingModeHint);
}

UsdAttribute
UsdLightFieldGaussiansAPI::CreateSortingModeHintAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->sortingModeHint,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldGaussiansAPI::GetProjectionModeHintAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->projectionModeHint);
}

UsdAttribute
UsdLightFieldGaussiansAPI::CreateProjectionModeHintAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->projectionModeHint,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldGaussiansAPI::GetOrientationsAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->orientations);
}

UsdAttribute
UsdLightFieldGaussiansAPI::CreateOrientationsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->orientations,
                       SdfValueTypeNames->QuathArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldGaussiansAPI::GetOrientationsfAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->orientationsf);
}

UsdAttribute
UsdLightFieldGaussiansAPI::CreateOrientationsfAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->orientationsf,
                       SdfValueTypeNames->QuatfArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldGaussiansAPI::GetScalesAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->scales);
}

UsdAttribute
UsdLightFieldGaussiansAPI::CreateScalesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->scales,
                       SdfValueTypeNames->Half3Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldGaussiansAPI::GetScalesfAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->scalesf);
}

UsdAttribute
UsdLightFieldGaussiansAPI::CreateScalesfAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->scalesf,
                       SdfValueTypeNames->Float3Array,
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
UsdLightFieldGaussiansAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLightFieldTokens->gaussianShape,
        UsdLightFieldTokens->sortingModeHint,
        UsdLightFieldTokens->projectionModeHint,
        UsdLightFieldTokens->orientations,
        UsdLightFieldTokens->orientationsf,
        UsdLightFieldTokens->scales,
        UsdLightFieldTokens->scalesf,
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

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
