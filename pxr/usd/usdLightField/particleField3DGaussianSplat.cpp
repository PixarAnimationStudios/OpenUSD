//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/particleField3DGaussianSplat.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldParticleField3DGaussianSplat,
        TfType::Bases< UsdLightFieldParticleField > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ParticleField3DGaussianSplat")
    // to find TfType<UsdLightFieldParticleField3DGaussianSplat>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLightFieldParticleField3DGaussianSplat>("ParticleField3DGaussianSplat");
}

/* virtual */
UsdLightFieldParticleField3DGaussianSplat::~UsdLightFieldParticleField3DGaussianSplat()
{
}

/* static */
UsdLightFieldParticleField3DGaussianSplat
UsdLightFieldParticleField3DGaussianSplat::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldParticleField3DGaussianSplat();
    }
    return UsdLightFieldParticleField3DGaussianSplat(stage->GetPrimAtPath(path));
}

/* static */
UsdLightFieldParticleField3DGaussianSplat
UsdLightFieldParticleField3DGaussianSplat::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("ParticleField3DGaussianSplat");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldParticleField3DGaussianSplat();
    }
    return UsdLightFieldParticleField3DGaussianSplat(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdLightFieldParticleField3DGaussianSplat::_GetSchemaKind() const
{
    return UsdLightFieldParticleField3DGaussianSplat::schemaKind;
}

/* static */
const TfType &
UsdLightFieldParticleField3DGaussianSplat::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldParticleField3DGaussianSplat>();
    return tfType;
}

/* static */
bool 
UsdLightFieldParticleField3DGaussianSplat::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldParticleField3DGaussianSplat::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::GetProjectionModeHintAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->projectionModeHint);
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::CreateProjectionModeHintAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->projectionModeHint,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::GetSortingModeHintAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->sortingModeHint);
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::CreateSortingModeHintAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->sortingModeHint,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
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
UsdLightFieldParticleField3DGaussianSplat::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLightFieldTokens->projectionModeHint,
        UsdLightFieldTokens->sortingModeHint,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdLightFieldParticleField::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

UsdLightFieldPositionAttributeAPI
UsdLightFieldParticleField3DGaussianSplat::LightFieldPositionAttributeAPI() const
{
    return UsdLightFieldPositionAttributeAPI(GetPrim());
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::GetPositionsAttr() const
{
    return LightFieldPositionAttributeAPI().GetPositionsAttr();
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::CreatePositionsAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightFieldPositionAttributeAPI().CreatePositionsAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::GetPositionshAttr() const
{
    return LightFieldPositionAttributeAPI().GetPositionshAttr();
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::CreatePositionshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightFieldPositionAttributeAPI().CreatePositionshAttr(
        defaultValue, writeSparsely);
}

UsdLightFieldOrientationAttributeAPI
UsdLightFieldParticleField3DGaussianSplat::LightFieldOrientationAttributeAPI() const
{
    return UsdLightFieldOrientationAttributeAPI(GetPrim());
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::GetOrientationsAttr() const
{
    return LightFieldOrientationAttributeAPI().GetOrientationsAttr();
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::CreateOrientationsAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightFieldOrientationAttributeAPI().CreateOrientationsAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::GetOrientationshAttr() const
{
    return LightFieldOrientationAttributeAPI().GetOrientationshAttr();
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::CreateOrientationshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightFieldOrientationAttributeAPI().CreateOrientationshAttr(
        defaultValue, writeSparsely);
}

UsdLightFieldScaleAttributeAPI
UsdLightFieldParticleField3DGaussianSplat::LightFieldScaleAttributeAPI() const
{
    return UsdLightFieldScaleAttributeAPI(GetPrim());
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::GetScalesAttr() const
{
    return LightFieldScaleAttributeAPI().GetScalesAttr();
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::CreateScalesAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightFieldScaleAttributeAPI().CreateScalesAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::GetScaleshAttr() const
{
    return LightFieldScaleAttributeAPI().GetScaleshAttr();
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::CreateScaleshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightFieldScaleAttributeAPI().CreateScaleshAttr(
        defaultValue, writeSparsely);
}

UsdLightFieldOpacityAttributeAPI
UsdLightFieldParticleField3DGaussianSplat::LightFieldOpacityAttributeAPI() const
{
    return UsdLightFieldOpacityAttributeAPI(GetPrim());
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::GetOpacitiesAttr() const
{
    return LightFieldOpacityAttributeAPI().GetOpacitiesAttr();
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::CreateOpacitiesAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightFieldOpacityAttributeAPI().CreateOpacitiesAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::GetOpacitieshAttr() const
{
    return LightFieldOpacityAttributeAPI().GetOpacitieshAttr();
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::CreateOpacitieshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightFieldOpacityAttributeAPI().CreateOpacitieshAttr(
        defaultValue, writeSparsely);
}

UsdLightFieldKernelGaussianEllipsoidAPI
UsdLightFieldParticleField3DGaussianSplat::LightFieldKernelGaussianEllipsoidAPI() const
{
    return UsdLightFieldKernelGaussianEllipsoidAPI(GetPrim());
}

UsdLightFieldSphericalHarmonicsAttributeAPI
UsdLightFieldParticleField3DGaussianSplat::LightFieldSphericalHarmonicsAttributeAPI() const
{
    return UsdLightFieldSphericalHarmonicsAttributeAPI(GetPrim());
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::GetRadianceSphericalHarmonicsDegreeAttr() const
{
    return LightFieldSphericalHarmonicsAttributeAPI().GetRadianceSphericalHarmonicsDegreeAttr();
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::CreateRadianceSphericalHarmonicsDegreeAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightFieldSphericalHarmonicsAttributeAPI().CreateRadianceSphericalHarmonicsDegreeAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::GetRadianceSphericalHarmonicsCoefficientsAttr() const
{
    return LightFieldSphericalHarmonicsAttributeAPI().GetRadianceSphericalHarmonicsCoefficientsAttr();
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::CreateRadianceSphericalHarmonicsCoefficientsAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightFieldSphericalHarmonicsAttributeAPI().CreateRadianceSphericalHarmonicsCoefficientsAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::GetRadianceSphericalHarmonicsCoefficientshAttr() const
{
    return LightFieldSphericalHarmonicsAttributeAPI().GetRadianceSphericalHarmonicsCoefficientshAttr();
}

UsdAttribute
UsdLightFieldParticleField3DGaussianSplat::CreateRadianceSphericalHarmonicsCoefficientshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightFieldSphericalHarmonicsAttributeAPI().CreateRadianceSphericalHarmonicsCoefficientshAttr(
        defaultValue, writeSparsely);
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
