//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "dataSourceParticleField.h"

#include "pxr/usdImaging/usdImaging/dataSourcePrimvars.h"

#include "pxr/usd/usdVol/particleField3DGaussianSplat.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourceParticleFieldPrim::UsdImagingDataSourceParticleFieldPrim(
    const SdfPath& sceneIndexPath, UsdPrim usdPrim,
    const UsdImagingDataSourceStageGlobals& stageGlobals)
    : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals) {}

static const UsdImagingDataSourceCustomPrimvars::Mappings
_GetCustomPrimvarMappings(const UsdPrim& usdPrim) {
    UsdImagingDataSourceCustomPrimvars::Mappings mappings = {
        {
            UsdVolTokens->radianceSphericalHarmonicsDegree,
            UsdVolTokens->radianceSphericalHarmonicsDegree,
            HdPrimvarSchemaTokens->constant
        }
    };

    // XXX: Todo: instead of creating the GS prim here, check for application
    // of FooAttributeAPI, to make this code reusable for other concrete
    // particle field instantiations.
    UsdVolParticleField3DGaussianSplat gs(usdPrim);
    TfToken usdName;

    gs.UsesFloatPositions(&usdName);
    mappings.push_back(
        {
            UsdVolTokens->positions,
            usdName,
            HdPrimvarSchemaTokens->vertex
        });
    gs.UsesFloatOrientations(&usdName);
    mappings.push_back(
        {
            UsdVolTokens->orientations,
            usdName,
            HdPrimvarSchemaTokens->vertex
        });
    gs.UsesFloatScales(&usdName);
    mappings.push_back(
        {
            UsdVolTokens->scales,
            usdName,
            HdPrimvarSchemaTokens->vertex
        });
    gs.UsesFloatOpacities(&usdName);
    mappings.push_back(
        {
            UsdVolTokens->opacities,
            usdName,
            HdPrimvarSchemaTokens->vertex
        });
    gs.UsesFloatRadianceCoefficients(&usdName);
    mappings.push_back(
        {
            UsdVolTokens->radianceSphericalHarmonicsCoefficients,
            usdName,
            HdPrimvarSchemaTokens->vertex
        });

    return mappings;
}

HdDataSourceBaseHandle
UsdImagingDataSourceParticleFieldPrim::Get(const TfToken& name) {
    HdDataSourceBaseHandle const result = UsdImagingDataSourceGprim::Get(name);

    if (name == HdPrimvarsSchema::GetSchemaToken()) {
        return HdOverlayContainerDataSource::New(
            HdContainerDataSource::Cast(result),
            UsdImagingDataSourceCustomPrimvars::New(
                _GetSceneIndexPath(), _GetUsdPrim(),
                _GetCustomPrimvarMappings(_GetUsdPrim()), _GetStageGlobals()));
    }

    // XXX: Todo: create a hydra "particleField" schema with the following
    // data members, and populate it here:
    // token particleField.kernelType =
    //     ["gaussianEllipsoid", "gaussianSurflet", "constantSurflet"]
    // ... populated from the typename of the applied KernelBaseAPI.
    // token particleField.projectionModeHint = ["perspective", "tangential"]
    // token particleField.sortingModeHint = ["zDepth", "cameraDistance"]
    // ... populated from UsdVolParticleField3DGaussianSplat
    //     projectionModeHint and sortingModeHint
    // In particular, "kernelType" needs to be transported in order to make this
    // code reusable for other concrete particle field instantiations.

    return result;
}

/*static*/
HdDataSourceLocatorSet
UsdImagingDataSourceParticleFieldPrim::Invalidate(
    UsdPrim const& prim, const TfToken& subprim,
    const TfTokenVector& properties,
    const UsdImagingPropertyInvalidationType invalidationType) {
    HdDataSourceLocatorSet result =
        UsdImagingDataSourceGprim::Invalidate(
            prim, subprim, properties, invalidationType);

    if (subprim.IsEmpty()) {
        // Unfortunately can't use CustomPrimvars::Invalidate here, since we
        // need to check for both float/half names.
        for (const TfToken &propertyName : properties) {
            if (propertyName == UsdVolTokens->positions ||
                propertyName == UsdVolTokens->positionsh) {
                result.insert(HdPrimvarsSchema::GetDefaultLocator().Append(
                    UsdVolTokens->positions));
            }
            if (propertyName == UsdVolTokens->orientations ||
                propertyName == UsdVolTokens->orientationsh) {
                result.insert(HdPrimvarsSchema::GetDefaultLocator().Append(
                    UsdVolTokens->orientations));
            }
            if (propertyName == UsdVolTokens->scales ||
                propertyName == UsdVolTokens->scalesh) {
                result.insert(HdPrimvarsSchema::GetDefaultLocator().Append(
                    UsdVolTokens->scales));
            }
            if (propertyName == UsdVolTokens->opacities ||
                propertyName == UsdVolTokens->opacitiesh) {
                result.insert(HdPrimvarsSchema::GetDefaultLocator().Append(
                    UsdVolTokens->opacities));
            }
            if (propertyName == UsdVolTokens->radianceSphericalHarmonicsCoefficients ||
                propertyName == UsdVolTokens->radianceSphericalHarmonicsCoefficientsh) {
                result.insert(HdPrimvarsSchema::GetDefaultLocator().Append(
                    UsdVolTokens->radianceSphericalHarmonicsCoefficients));
            }
            if (propertyName == UsdVolTokens->radianceSphericalHarmonicsDegree) {
                result.insert(HdPrimvarsSchema::GetDefaultLocator().Append(
                    UsdVolTokens->radianceSphericalHarmonicsDegree));
            }
        }
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
