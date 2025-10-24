//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/sphericalHarmonicsAttributeAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldSphericalHarmonicsAttributeAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldSphericalHarmonicsAttributeAPI::~UsdLightFieldSphericalHarmonicsAttributeAPI()
{
}

/* static */
UsdLightFieldSphericalHarmonicsAttributeAPI
UsdLightFieldSphericalHarmonicsAttributeAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldSphericalHarmonicsAttributeAPI();
    }
    return UsdLightFieldSphericalHarmonicsAttributeAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldSphericalHarmonicsAttributeAPI::_GetSchemaKind() const
{
    return UsdLightFieldSphericalHarmonicsAttributeAPI::schemaKind;
}

/* static */
bool
UsdLightFieldSphericalHarmonicsAttributeAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldSphericalHarmonicsAttributeAPI>(whyNot);
}

/* static */
UsdLightFieldSphericalHarmonicsAttributeAPI
UsdLightFieldSphericalHarmonicsAttributeAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldSphericalHarmonicsAttributeAPI>()) {
        return UsdLightFieldSphericalHarmonicsAttributeAPI(prim);
    }
    return UsdLightFieldSphericalHarmonicsAttributeAPI();
}

/* static */
const TfType &
UsdLightFieldSphericalHarmonicsAttributeAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldSphericalHarmonicsAttributeAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldSphericalHarmonicsAttributeAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldSphericalHarmonicsAttributeAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLightFieldSphericalHarmonicsAttributeAPI::GetRadianceSphericalHarmonicsDegreeAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->radianceSphericalHarmonicsDegree);
}

UsdAttribute
UsdLightFieldSphericalHarmonicsAttributeAPI::CreateRadianceSphericalHarmonicsDegreeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->radianceSphericalHarmonicsDegree,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldSphericalHarmonicsAttributeAPI::GetRadianceSphericalHarmonicsCoefficientsAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->radianceSphericalHarmonicsCoefficients);
}

UsdAttribute
UsdLightFieldSphericalHarmonicsAttributeAPI::CreateRadianceSphericalHarmonicsCoefficientsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->radianceSphericalHarmonicsCoefficients,
                       SdfValueTypeNames->Float3Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldSphericalHarmonicsAttributeAPI::GetRadianceSphericalHarmonicsCoefficientshAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->radianceSphericalHarmonicsCoefficientsh);
}

UsdAttribute
UsdLightFieldSphericalHarmonicsAttributeAPI::CreateRadianceSphericalHarmonicsCoefficientshAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->radianceSphericalHarmonicsCoefficientsh,
                       SdfValueTypeNames->Half3Array,
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
UsdLightFieldSphericalHarmonicsAttributeAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLightFieldTokens->radianceSphericalHarmonicsDegree,
        UsdLightFieldTokens->radianceSphericalHarmonicsCoefficients,
        UsdLightFieldTokens->radianceSphericalHarmonicsCoefficientsh,
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

UsdLightFieldRadianceBaseAPI
UsdLightFieldSphericalHarmonicsAttributeAPI::LightFieldRadianceBaseAPI() const
{
    return UsdLightFieldRadianceBaseAPI(GetPrim());
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
