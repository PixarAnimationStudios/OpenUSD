//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/geomModelAPIAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceMapped.h"
#include "pxr/usdImaging/usdImaging/geomModelSchema.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/imaging/hd/extentSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/usd/modelAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

std::vector<UsdImagingDataSourceMapped::AttributeMapping>
_GetAttributeMappings()
{
    std::vector<UsdImagingDataSourceMapped::AttributeMapping> result;

    for (const TfToken &usdName :
             UsdGeomModelAPI::GetSchemaAttributeNames(
                 /* includeInherited = */ false)) {

        const std::pair<std::string, bool> nameAndMatch =
            SdfPath::StripPrefixNamespace(
                usdName.GetString(), "model");
        if (nameAndMatch.second) {
            result.push_back(
                { usdName,
                  HdDataSourceLocator(TfToken(nameAndMatch.first)) });
        }
    }

    return result;
}

const UsdImagingDataSourceMapped::AttributeMappings &
_GetMappings() {
    static const UsdImagingDataSourceMapped::AttributeMappings result(
        _GetAttributeMappings(), UsdImagingGeomModelSchema::GetDefaultLocator());
    return result;
}

}

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingGeomModelAPIAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingAPISchemaAdapterFactory<Adapter> >();
}

HdContainerDataSourceHandle
UsdImagingGeomModelAPIAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    // Reflect UsdGeomModelAPI as UsdImagingGeomModelSchema.
    HdContainerDataSourceHandle geomModelDs =
        UsdImagingDataSourceMapped::New(
            prim, prim.GetPath(), _GetMappings(), stageGlobals);

    // For model components, overlay applyDrawMode=true.
    if (UsdModelAPI(prim).IsKind(KindTokens->component)) {
        static HdContainerDataSourceHandle const applyDrawModeDs =
            UsdImagingGeomModelSchema::Builder()
                .SetApplyDrawMode(
                    HdRetainedTypedSampledDataSource<bool>::New(true))
                .Build();
        geomModelDs = HdOverlayContainerDataSource::
            OverlayedContainerDataSources(applyDrawModeDs, geomModelDs);
    }
    
    return HdRetainedContainerDataSource::New(
        UsdImagingGeomModelSchema::GetSchemaToken(), geomModelDs);
}


HdDataSourceLocatorSet
UsdImagingGeomModelAPIAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    return UsdImagingDataSourceMapped::Invalidate(
        properties, _GetMappings());
}

PXR_NAMESPACE_CLOSE_SCOPE
