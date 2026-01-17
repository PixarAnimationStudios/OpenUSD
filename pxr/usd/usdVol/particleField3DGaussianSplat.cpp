//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/particleField3DGaussianSplat.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolParticleField3DGaussianSplat,
        TfType::Bases< UsdVolParticleField > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ParticleField3DGaussianSplat")
    // to find TfType<UsdVolParticleField3DGaussianSplat>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdVolParticleField3DGaussianSplat>("ParticleField3DGaussianSplat");
}

/* virtual */
UsdVolParticleField3DGaussianSplat::~UsdVolParticleField3DGaussianSplat()
{
}

/* static */
UsdVolParticleField3DGaussianSplat
UsdVolParticleField3DGaussianSplat::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolParticleField3DGaussianSplat();
    }
    return UsdVolParticleField3DGaussianSplat(stage->GetPrimAtPath(path));
}

/* static */
UsdVolParticleField3DGaussianSplat
UsdVolParticleField3DGaussianSplat::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("ParticleField3DGaussianSplat");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolParticleField3DGaussianSplat();
    }
    return UsdVolParticleField3DGaussianSplat(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdVolParticleField3DGaussianSplat::_GetSchemaKind() const
{
    return UsdVolParticleField3DGaussianSplat::schemaKind;
}

/* static */
const TfType &
UsdVolParticleField3DGaussianSplat::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolParticleField3DGaussianSplat>();
    return tfType;
}

/* static */
bool 
UsdVolParticleField3DGaussianSplat::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolParticleField3DGaussianSplat::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::GetProjectionModeHintAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->projectionModeHint);
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::CreateProjectionModeHintAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->projectionModeHint,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::GetSortingModeHintAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->sortingModeHint);
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::CreateSortingModeHintAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->sortingModeHint,
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
UsdVolParticleField3DGaussianSplat::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdVolTokens->projectionModeHint,
        UsdVolTokens->sortingModeHint,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdVolParticleField::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

UsdVolParticleFieldPositionAttributeAPI
UsdVolParticleField3DGaussianSplat::ParticleFieldPositionAttributeAPI() const
{
    return UsdVolParticleFieldPositionAttributeAPI(GetPrim());
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::GetPositionsAttr() const
{
    return ParticleFieldPositionAttributeAPI().GetPositionsAttr();
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::CreatePositionsAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return ParticleFieldPositionAttributeAPI().CreatePositionsAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::GetPositionshAttr() const
{
    return ParticleFieldPositionAttributeAPI().GetPositionshAttr();
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::CreatePositionshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return ParticleFieldPositionAttributeAPI().CreatePositionshAttr(
        defaultValue, writeSparsely);
}

UsdVolParticleFieldOrientationAttributeAPI
UsdVolParticleField3DGaussianSplat::ParticleFieldOrientationAttributeAPI() const
{
    return UsdVolParticleFieldOrientationAttributeAPI(GetPrim());
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::GetOrientationsAttr() const
{
    return ParticleFieldOrientationAttributeAPI().GetOrientationsAttr();
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::CreateOrientationsAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return ParticleFieldOrientationAttributeAPI().CreateOrientationsAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::GetOrientationshAttr() const
{
    return ParticleFieldOrientationAttributeAPI().GetOrientationshAttr();
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::CreateOrientationshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return ParticleFieldOrientationAttributeAPI().CreateOrientationshAttr(
        defaultValue, writeSparsely);
}

UsdVolParticleFieldScaleAttributeAPI
UsdVolParticleField3DGaussianSplat::ParticleFieldScaleAttributeAPI() const
{
    return UsdVolParticleFieldScaleAttributeAPI(GetPrim());
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::GetScalesAttr() const
{
    return ParticleFieldScaleAttributeAPI().GetScalesAttr();
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::CreateScalesAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return ParticleFieldScaleAttributeAPI().CreateScalesAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::GetScaleshAttr() const
{
    return ParticleFieldScaleAttributeAPI().GetScaleshAttr();
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::CreateScaleshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return ParticleFieldScaleAttributeAPI().CreateScaleshAttr(
        defaultValue, writeSparsely);
}

UsdVolParticleFieldOpacityAttributeAPI
UsdVolParticleField3DGaussianSplat::ParticleFieldOpacityAttributeAPI() const
{
    return UsdVolParticleFieldOpacityAttributeAPI(GetPrim());
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::GetOpacitiesAttr() const
{
    return ParticleFieldOpacityAttributeAPI().GetOpacitiesAttr();
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::CreateOpacitiesAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return ParticleFieldOpacityAttributeAPI().CreateOpacitiesAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::GetOpacitieshAttr() const
{
    return ParticleFieldOpacityAttributeAPI().GetOpacitieshAttr();
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::CreateOpacitieshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return ParticleFieldOpacityAttributeAPI().CreateOpacitieshAttr(
        defaultValue, writeSparsely);
}

UsdVolParticleFieldKernelGaussianEllipsoidAPI
UsdVolParticleField3DGaussianSplat::ParticleFieldKernelGaussianEllipsoidAPI() const
{
    return UsdVolParticleFieldKernelGaussianEllipsoidAPI(GetPrim());
}

UsdVolParticleFieldSphericalHarmonicsAttributeAPI
UsdVolParticleField3DGaussianSplat::ParticleFieldSphericalHarmonicsAttributeAPI() const
{
    return UsdVolParticleFieldSphericalHarmonicsAttributeAPI(GetPrim());
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::GetRadianceSphericalHarmonicsDegreeAttr() const
{
    return ParticleFieldSphericalHarmonicsAttributeAPI().GetRadianceSphericalHarmonicsDegreeAttr();
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::CreateRadianceSphericalHarmonicsDegreeAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return ParticleFieldSphericalHarmonicsAttributeAPI().CreateRadianceSphericalHarmonicsDegreeAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::GetRadianceSphericalHarmonicsCoefficientsAttr() const
{
    return ParticleFieldSphericalHarmonicsAttributeAPI().GetRadianceSphericalHarmonicsCoefficientsAttr();
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::CreateRadianceSphericalHarmonicsCoefficientsAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return ParticleFieldSphericalHarmonicsAttributeAPI().CreateRadianceSphericalHarmonicsCoefficientsAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::GetRadianceSphericalHarmonicsCoefficientshAttr() const
{
    return ParticleFieldSphericalHarmonicsAttributeAPI().GetRadianceSphericalHarmonicsCoefficientshAttr();
}

UsdAttribute
UsdVolParticleField3DGaussianSplat::CreateRadianceSphericalHarmonicsCoefficientshAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return ParticleFieldSphericalHarmonicsAttributeAPI().CreateRadianceSphericalHarmonicsCoefficientshAttr(
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

PXR_NAMESPACE_OPEN_SCOPE

bool
UsdVolParticleField3DGaussianSplat::UsesFloatPositions(
    UsdAttribute *positionsAttr) const
{
    return _UsesFloatAttr(
        UsdVolTokens->positions, UsdVolTokens->positionsh, positionsAttr);
}

bool
UsdVolParticleField3DGaussianSplat::UsesFloatPositions(
    TfToken *positionsToken) const
{
    return _UsesFloatAttr(
        UsdVolTokens->positions, UsdVolTokens->positionsh, positionsToken);
}

bool
UsdVolParticleField3DGaussianSplat::UsesFloatOrientations(
    UsdAttribute *orientationsAttr) const
{
    return _UsesFloatAttr(
        UsdVolTokens->orientations, UsdVolTokens->orientationsh,
        orientationsAttr);
}

bool
UsdVolParticleField3DGaussianSplat::UsesFloatOrientations(
    TfToken *orientationsToken) const
{
    return _UsesFloatAttr(
        UsdVolTokens->orientations, UsdVolTokens->orientationsh,
        orientationsToken);
}

bool
UsdVolParticleField3DGaussianSplat::UsesFloatScales(
    UsdAttribute *scalesAttr) const
{
    return _UsesFloatAttr(
        UsdVolTokens->scales, UsdVolTokens->scalesh, scalesAttr);
}

bool
UsdVolParticleField3DGaussianSplat::UsesFloatScales(TfToken *scalesToken) const
{
    return _UsesFloatAttr(
        UsdVolTokens->scales, UsdVolTokens->scalesh, scalesToken);
}

bool
UsdVolParticleField3DGaussianSplat::UsesFloatOpacities(
    UsdAttribute *opacitiesAttr) const
{
    return _UsesFloatAttr(
        UsdVolTokens->opacities, UsdVolTokens->opacitiesh, opacitiesAttr);
}

bool
UsdVolParticleField3DGaussianSplat::UsesFloatOpacities(
    TfToken *opacitiesToken) const
{
    return _UsesFloatAttr(
        UsdVolTokens->opacities, UsdVolTokens->opacitiesh, opacitiesToken);
}

bool
UsdVolParticleField3DGaussianSplat::UsesFloatRadianceCoefficients(
    UsdAttribute *radianceCoefficientsAttr) const
{
    return _UsesFloatAttr(
        UsdVolTokens->radianceSphericalHarmonicsCoefficients,
        UsdVolTokens->radianceSphericalHarmonicsCoefficientsh,
        radianceCoefficientsAttr);
}

bool
UsdVolParticleField3DGaussianSplat::UsesFloatRadianceCoefficients(
    TfToken *radianceCoefficientsToken) const
{
    return _UsesFloatAttr(
        UsdVolTokens->radianceSphericalHarmonicsCoefficients,
        UsdVolTokens->radianceSphericalHarmonicsCoefficientsh,
        radianceCoefficientsToken);
}

bool
UsdVolParticleField3DGaussianSplat::_UsesFloatAttr(
    TfToken const& floatName, TfToken const& halfName,
    UsdAttribute *outAttr) const
{
    *outAttr = GetPrim().GetAttribute(floatName);
    VtValue floatTimeSamples;
    outAttr->Get(&floatTimeSamples, UsdTimeCode::EarliestTime());
    if (floatTimeSamples.GetArraySize() > 0) {
        return true;
    }
    *outAttr = GetPrim().GetAttribute(halfName);
    return false;
}

bool
UsdVolParticleField3DGaussianSplat::_UsesFloatAttr(
    TfToken const& floatName, TfToken const& halfName, TfToken *outToken) const
{
    VtValue floatTimeSamples;
    GetPrim().GetAttribute(floatName).Get(
        &floatTimeSamples, UsdTimeCode::EarliestTime());
    if (floatTimeSamples.GetArraySize() > 0) {
        if (outToken) {
            *outToken = floatName;
        }
        return true;
    }
    if (outToken) {
        *outToken = halfName;
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
