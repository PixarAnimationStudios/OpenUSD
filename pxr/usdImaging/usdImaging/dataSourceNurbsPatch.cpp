//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/dataSourceNurbsPatch.h"

#include "pxr/usdImaging/usdImaging/dataSourceMapped.h"

#include "pxr/usd/usdGeom/nurbsPatch.h"

#include "pxr/imaging/hd/nurbsPatchSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

HdDataSourceLocator
_ToLocator(const TfToken &name)
{
    const TfTokenVector tokens = SdfPath::TokenizeIdentifierAsTokens(name);
    return HdDataSourceLocator(tokens.size(), tokens.data());
}
    
std::vector<UsdImagingDataSourceMapped::PropertyMapping>
_GetPropertyMappings()
{
    std::vector<UsdImagingDataSourceMapped::PropertyMapping> result;

    // Pick up from UsdGeomGprim
    result.push_back(
        UsdImagingDataSourceMapped::AttributeMapping{
            UsdGeomTokens->doubleSided,
            HdDataSourceLocator(HdNurbsPatchSchemaTokens->doubleSided)});
    result.push_back(
        UsdImagingDataSourceMapped::AttributeMapping{
            UsdGeomTokens->orientation,
            HdDataSourceLocator(HdNurbsPatchSchemaTokens->orientation)});
    
    for (const TfToken &usdName :
             UsdGeomNurbsPatch::GetSchemaAttributeNames(
                 /* includeInherited = */ false)) {
        if (usdName == UsdGeomTokens->pointWeights) {
            // Suppress pointWeights from UsdGeomNurbsCurves (which is a custom
            // primvar we process in the prim source below).
            continue;
        }

        result.push_back(
            UsdImagingDataSourceMapped::AttributeMapping{
                usdName, _ToLocator(usdName)});
    }

    return result;
}

const UsdImagingDataSourceMapped::PropertyMappings &
_GetMappings() {
    static const UsdImagingDataSourceMapped::PropertyMappings result(
        _GetPropertyMappings(), HdNurbsPatchSchema::GetDefaultLocator());
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

UsdImagingDataSourceNurbsPatchPrim::UsdImagingDataSourceNurbsPatchPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdImagingDataSourceNurbsPatchPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourceGprim::GetNames();
    result.push_back(HdNurbsPatchSchema::GetSchemaToken());
    return result;
}

HdDataSourceBaseHandle 
UsdImagingDataSourceNurbsPatchPrim::Get(const TfToken & name)
{
    if (name == HdNurbsPatchSchema::GetSchemaToken()) {
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
UsdImagingDataSourceNurbsPatchPrim::Invalidate(
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
