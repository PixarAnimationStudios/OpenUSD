//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/particleField_3DGaussianSplat.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldParticleField_3DGaussianSplat,
        TfType::Bases< UsdLightFieldParticleField > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ParticleField_3DGaussianSplat")
    // to find TfType<UsdLightFieldParticleField_3DGaussianSplat>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLightFieldParticleField_3DGaussianSplat>("ParticleField_3DGaussianSplat");
}

/* virtual */
UsdLightFieldParticleField_3DGaussianSplat::~UsdLightFieldParticleField_3DGaussianSplat()
{
}

/* static */
UsdLightFieldParticleField_3DGaussianSplat
UsdLightFieldParticleField_3DGaussianSplat::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldParticleField_3DGaussianSplat();
    }
    return UsdLightFieldParticleField_3DGaussianSplat(stage->GetPrimAtPath(path));
}

/* static */
UsdLightFieldParticleField_3DGaussianSplat
UsdLightFieldParticleField_3DGaussianSplat::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("ParticleField_3DGaussianSplat");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldParticleField_3DGaussianSplat();
    }
    return UsdLightFieldParticleField_3DGaussianSplat(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdLightFieldParticleField_3DGaussianSplat::_GetSchemaKind() const
{
    return UsdLightFieldParticleField_3DGaussianSplat::schemaKind;
}

/* static */
const TfType &
UsdLightFieldParticleField_3DGaussianSplat::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldParticleField_3DGaussianSplat>();
    return tfType;
}

/* static */
bool 
UsdLightFieldParticleField_3DGaussianSplat::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldParticleField_3DGaussianSplat::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::GetProjectionModeHintAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->projectionModeHint);
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::CreateProjectionModeHintAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->projectionModeHint,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::GetSortingModeHintAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->sortingModeHint);
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::CreateSortingModeHintAttr(VtValue const &defaultValue, bool writeSparsely) const
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
UsdLightFieldParticleField_3DGaussianSplat::GetSchemaAttributeNames(bool includeInherited)
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
UsdLightFieldParticleField_3DGaussianSplat::PositionAttributeAPI() const
{
    return UsdLightFieldPositionAttributeAPI(GetPrim());
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::GetPositionsAttr() const
{
    return PositionAttributeAPI().GetPositionsAttr();
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::CreatePositionsAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return PositionAttributeAPI().CreatePositionsAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::GetPositionshAttr() const
{
    return PositionAttributeAPI().GetPositionshAttr();
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::CreatePositionshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return PositionAttributeAPI().CreatePositionshAttr(
        defaultValue, writeSparsely);
}

UsdLightFieldOrientationAttributeAPI
UsdLightFieldParticleField_3DGaussianSplat::OrientationAttributeAPI() const
{
    return UsdLightFieldOrientationAttributeAPI(GetPrim());
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::GetOrientationsAttr() const
{
    return OrientationAttributeAPI().GetOrientationsAttr();
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::CreateOrientationsAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return OrientationAttributeAPI().CreateOrientationsAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::GetOrientationshAttr() const
{
    return OrientationAttributeAPI().GetOrientationshAttr();
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::CreateOrientationshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return OrientationAttributeAPI().CreateOrientationshAttr(
        defaultValue, writeSparsely);
}

UsdLightFieldScaleAttributeAPI
UsdLightFieldParticleField_3DGaussianSplat::ScaleAttributeAPI() const
{
    return UsdLightFieldScaleAttributeAPI(GetPrim());
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::GetScalesAttr() const
{
    return ScaleAttributeAPI().GetScalesAttr();
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::CreateScalesAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return ScaleAttributeAPI().CreateScalesAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::GetScaleshAttr() const
{
    return ScaleAttributeAPI().GetScaleshAttr();
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::CreateScaleshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return ScaleAttributeAPI().CreateScaleshAttr(
        defaultValue, writeSparsely);
}

UsdLightFieldOpacityAttributeAPI
UsdLightFieldParticleField_3DGaussianSplat::OpacityAttributeAPI() const
{
    return UsdLightFieldOpacityAttributeAPI(GetPrim());
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::GetOpacitiesAttr() const
{
    return OpacityAttributeAPI().GetOpacitiesAttr();
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::CreateOpacitiesAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return OpacityAttributeAPI().CreateOpacitiesAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::GetOpacitieshAttr() const
{
    return OpacityAttributeAPI().GetOpacitieshAttr();
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::CreateOpacitieshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return OpacityAttributeAPI().CreateOpacitieshAttr(
        defaultValue, writeSparsely);
}

UsdLightFieldGaussianShapeAPI
UsdLightFieldParticleField_3DGaussianSplat::GaussianShapeAPI() const
{
    return UsdLightFieldGaussianShapeAPI(GetPrim());
}

UsdLightFieldGaussianFalloffFunctionAPI
UsdLightFieldParticleField_3DGaussianSplat::GaussianFalloffFunctionAPI() const
{
    return UsdLightFieldGaussianFalloffFunctionAPI(GetPrim());
}

UsdLightFieldSphericalHarmonicsAttributeAPI
UsdLightFieldParticleField_3DGaussianSplat::SphericalHarmonicsAttributeAPI() const
{
    return UsdLightFieldSphericalHarmonicsAttributeAPI(GetPrim());
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::GetRadianceSphericalHarmonicsDegreeAttr() const
{
    return SphericalHarmonicsAttributeAPI().GetRadianceSphericalHarmonicsDegreeAttr();
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::CreateRadianceSphericalHarmonicsDegreeAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return SphericalHarmonicsAttributeAPI().CreateRadianceSphericalHarmonicsDegreeAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::GetPrimvarsRadianceSphericalHarmonicsCoefficientsAttr() const
{
    return SphericalHarmonicsAttributeAPI().GetPrimvarsRadianceSphericalHarmonicsCoefficientsAttr();
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::CreatePrimvarsRadianceSphericalHarmonicsCoefficientsAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return SphericalHarmonicsAttributeAPI().CreatePrimvarsRadianceSphericalHarmonicsCoefficientsAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::GetPrimvarsRadianceSphericalHarmonicsCoefficientshAttr() const
{
    return SphericalHarmonicsAttributeAPI().GetPrimvarsRadianceSphericalHarmonicsCoefficientshAttr();
}

UsdAttribute
UsdLightFieldParticleField_3DGaussianSplat::CreatePrimvarsRadianceSphericalHarmonicsCoefficientshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return SphericalHarmonicsAttributeAPI().CreatePrimvarsRadianceSphericalHarmonicsCoefficientshAttr(
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
