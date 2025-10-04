//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/orientationAttributeAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldOrientationAttributeAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldOrientationAttributeAPI::~UsdLightFieldOrientationAttributeAPI()
{
}

/* static */
UsdLightFieldOrientationAttributeAPI
UsdLightFieldOrientationAttributeAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldOrientationAttributeAPI();
    }
    return UsdLightFieldOrientationAttributeAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldOrientationAttributeAPI::_GetSchemaKind() const
{
    return UsdLightFieldOrientationAttributeAPI::schemaKind;
}

/* static */
bool
UsdLightFieldOrientationAttributeAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldOrientationAttributeAPI>(whyNot);
}

/* static */
UsdLightFieldOrientationAttributeAPI
UsdLightFieldOrientationAttributeAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldOrientationAttributeAPI>()) {
        return UsdLightFieldOrientationAttributeAPI(prim);
    }
    return UsdLightFieldOrientationAttributeAPI();
}

/* static */
const TfType &
UsdLightFieldOrientationAttributeAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldOrientationAttributeAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldOrientationAttributeAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldOrientationAttributeAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLightFieldOrientationAttributeAPI::GetOrientationsAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->orientations);
}

UsdAttribute
UsdLightFieldOrientationAttributeAPI::CreateOrientationsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->orientations,
                       SdfValueTypeNames->QuatfArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldOrientationAttributeAPI::GetOrientationshAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->orientationsh);
}

UsdAttribute
UsdLightFieldOrientationAttributeAPI::CreateOrientationshAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->orientationsh,
                       SdfValueTypeNames->QuathArray,
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
UsdLightFieldOrientationAttributeAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLightFieldTokens->orientations,
        UsdLightFieldTokens->orientationsh,
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
