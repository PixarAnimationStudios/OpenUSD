//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/sphericalHarmonicsAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldSphericalHarmonicsAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldSphericalHarmonicsAPI::~UsdLightFieldSphericalHarmonicsAPI()
{
}

/* static */
UsdLightFieldSphericalHarmonicsAPI
UsdLightFieldSphericalHarmonicsAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldSphericalHarmonicsAPI();
    }
    return UsdLightFieldSphericalHarmonicsAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldSphericalHarmonicsAPI::_GetSchemaKind() const
{
    return UsdLightFieldSphericalHarmonicsAPI::schemaKind;
}

/* static */
bool
UsdLightFieldSphericalHarmonicsAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldSphericalHarmonicsAPI>(whyNot);
}

/* static */
UsdLightFieldSphericalHarmonicsAPI
UsdLightFieldSphericalHarmonicsAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldSphericalHarmonicsAPI>()) {
        return UsdLightFieldSphericalHarmonicsAPI(prim);
    }
    return UsdLightFieldSphericalHarmonicsAPI();
}

/* static */
const TfType &
UsdLightFieldSphericalHarmonicsAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldSphericalHarmonicsAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldSphericalHarmonicsAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldSphericalHarmonicsAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLightFieldSphericalHarmonicsAPI::GetSphericalHarmonicsColorSpaceAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->sphericalHarmonicsColorSpace);
}

UsdAttribute
UsdLightFieldSphericalHarmonicsAPI::CreateSphericalHarmonicsColorSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->sphericalHarmonicsColorSpace,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldSphericalHarmonicsAPI::GetPrimvarsSphericalHarmonicsAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->primvarsSphericalHarmonics);
}

UsdAttribute
UsdLightFieldSphericalHarmonicsAPI::CreatePrimvarsSphericalHarmonicsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->primvarsSphericalHarmonics,
                       SdfValueTypeNames->Half3Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldSphericalHarmonicsAPI::GetPrimvarsSphericalHarmonicsfAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->primvarsSphericalHarmonicsf);
}

UsdAttribute
UsdLightFieldSphericalHarmonicsAPI::CreatePrimvarsSphericalHarmonicsfAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->primvarsSphericalHarmonicsf,
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
UsdLightFieldSphericalHarmonicsAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLightFieldTokens->sphericalHarmonicsColorSpace,
        UsdLightFieldTokens->primvarsSphericalHarmonics,
        UsdLightFieldTokens->primvarsSphericalHarmonicsf,
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
