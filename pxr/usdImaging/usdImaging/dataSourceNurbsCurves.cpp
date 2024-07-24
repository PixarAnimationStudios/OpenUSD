//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/dataSourceNurbsCurves.h"

#include "pxr/usdImaging/usdImaging/dataSourceMapped.h"

#include "pxr/usd/usdGeom/nurbsCurves.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/nurbsCurvesSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

std::vector<UsdImagingDataSourceMapped::AttributeMapping>
_GetAttributeMappings()
{
    std::vector<UsdImagingDataSourceMapped::AttributeMapping> result;

    for (const TfToken &usdName :
             UsdGeomNurbsCurves::GetSchemaAttributeNames(
                 /* includeInherited = */ false)) {
        if (usdName == UsdGeomTokens->pointWeights) {
            // Suppress pointWeights from UsdGeomNurbsCurves (which is a custom
            // primvar we process in the prim source below).
            continue;
        }
        result.push_back({ usdName, HdDataSourceLocator(usdName) });
    }

    for (const TfToken &usdName :
             UsdGeomCurves::GetSchemaAttributeNames(
                 /* includeInherited = */ false)) {
        if (usdName == UsdGeomTokens->widths) {
            // Suppress widths from UsdGeomCurves (which is a custom
            // primvar that UsdImagingDataSourceGprim gives us).
            continue;
        }
        result.push_back({ usdName, HdDataSourceLocator(usdName) });
    }

    return result;
}

const UsdImagingDataSourceMapped::AttributeMappings &
_GetMappings() {
    static const UsdImagingDataSourceMapped::AttributeMappings result(
        _GetAttributeMappings(), HdNurbsCurvesSchema::GetDefaultLocator());
    return result;
}

const UsdImagingDataSourceCustomPrimvars::Mappings &
_GetCustomPrimvarMappings(const UsdPrim &usdPrim)
{
    static const TfToken &hdPrimvarName = UsdGeomTokens->pointWeights;

    static const UsdImagingDataSourceCustomPrimvars::Mappings mappings = {
        { hdPrimvarName, UsdGeomTokens->pointWeights },
    };

    return mappings;
}

}

// ----------------------------------------------------------------------------

UsdImagingDataSourceNurbsCurvesPrim::UsdImagingDataSourceNurbsCurvesPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdImagingDataSourceNurbsCurvesPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourceGprim::GetNames();
    result.push_back(HdNurbsCurvesSchema::GetSchemaToken());
    return result;
}

HdDataSourceBaseHandle 
UsdImagingDataSourceNurbsCurvesPrim::Get(const TfToken & name)
{
    if (name == HdNurbsCurvesSchema::GetSchemaToken()) {
        return
            UsdImagingDataSourceMapped::New(
                _GetUsdPrim(),
                _GetSceneIndexPath(),
                _GetMappings(),
                _GetStageGlobals());
    }

    if (name == HdPrimvarsSchema::GetSchemaToken()) {
        return
            HdOverlayContainerDataSource::New(
                HdContainerDataSource::Cast(
                    UsdImagingDataSourceGprim::Get(name)),
                UsdImagingDataSourceCustomPrimvars::New(
                    _GetSceneIndexPath(),
                    _GetUsdPrim(),
                    _GetCustomPrimvarMappings(_GetUsdPrim()),
                    _GetStageGlobals()));
    }

    return UsdImagingDataSourceGprim::Get(name);
}

HdDataSourceLocatorSet
UsdImagingDataSourceNurbsCurvesPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    TRACE_FUNCTION();

    HdDataSourceLocatorSet locators =
        UsdImagingDataSourceMapped::Invalidate(
            properties, _GetMappings());
    
    locators.insert(
        UsdImagingDataSourceGprim::Invalidate(
            prim, subprim, properties, invalidationType));

    locators.insert(
        UsdImagingDataSourceCustomPrimvars::Invalidate(
            properties, _GetCustomPrimvarMappings(prim)));

    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE
