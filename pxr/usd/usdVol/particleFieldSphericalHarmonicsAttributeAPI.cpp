//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/particleFieldSphericalHarmonicsAttributeAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolParticleFieldSphericalHarmonicsAttributeAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::~UsdVolParticleFieldSphericalHarmonicsAttributeAPI()
{
}

/* static */
UsdVolParticleFieldSphericalHarmonicsAttributeAPI
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolParticleFieldSphericalHarmonicsAttributeAPI();
    }
    return UsdVolParticleFieldSphericalHarmonicsAttributeAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdVolParticleFieldSphericalHarmonicsAttributeAPI::_GetSchemaKind() const
{
    return UsdVolParticleFieldSphericalHarmonicsAttributeAPI::schemaKind;
}

/* static */
bool
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdVolParticleFieldSphericalHarmonicsAttributeAPI>(whyNot);
}

/* static */
UsdVolParticleFieldSphericalHarmonicsAttributeAPI
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdVolParticleFieldSphericalHarmonicsAttributeAPI>()) {
        return UsdVolParticleFieldSphericalHarmonicsAttributeAPI(prim);
    }
    return UsdVolParticleFieldSphericalHarmonicsAttributeAPI();
}

/* static */
const TfType &
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolParticleFieldSphericalHarmonicsAttributeAPI>();
    return tfType;
}

/* static */
bool 
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::GetRadianceSphericalHarmonicsDegreeAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->radianceSphericalHarmonicsDegree);
}

UsdAttribute
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::CreateRadianceSphericalHarmonicsDegreeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->radianceSphericalHarmonicsDegree,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::GetRadianceSphericalHarmonicsCoefficientsAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->radianceSphericalHarmonicsCoefficients);
}

UsdAttribute
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::CreateRadianceSphericalHarmonicsCoefficientsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->radianceSphericalHarmonicsCoefficients,
                       SdfValueTypeNames->Float3Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::GetRadianceSphericalHarmonicsCoefficientshAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->radianceSphericalHarmonicsCoefficientsh);
}

UsdAttribute
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::CreateRadianceSphericalHarmonicsCoefficientshAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->radianceSphericalHarmonicsCoefficientsh,
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
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdVolTokens->radianceSphericalHarmonicsDegree,
        UsdVolTokens->radianceSphericalHarmonicsCoefficients,
        UsdVolTokens->radianceSphericalHarmonicsCoefficientsh,
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

UsdVolParticleFieldRadianceBaseAPI
UsdVolParticleFieldSphericalHarmonicsAttributeAPI::ParticleFieldRadianceBaseAPI() const
{
    return UsdVolParticleFieldRadianceBaseAPI(GetPrim());
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
