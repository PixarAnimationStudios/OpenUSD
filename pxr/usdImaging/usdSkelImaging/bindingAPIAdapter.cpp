//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/bindingAPIAdapter.h"

#include "pxr/usdImaging/usdSkelImaging/dataSourceBindingAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdSkelImagingBindingAPIAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingAPISchemaAdapterFactory<Adapter> >();
}

UsdSkelImagingBindingAPIAdapter::
UsdSkelImagingBindingAPIAdapter() = default;

UsdSkelImagingBindingAPIAdapter::
~UsdSkelImagingBindingAPIAdapter() = default;

HdContainerDataSourceHandle
UsdSkelImagingBindingAPIAdapter::GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfToken const& appliedInstanceName,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return nullptr;
    }

    return UsdSkelImagingDataSourceBindingAPI::New(
        prim.GetPath(), prim, stageGlobals);
}

HdDataSourceLocatorSet
UsdSkelImagingBindingAPIAdapter::InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfToken const& appliedInstanceName,
        TfTokenVector const& properties,
        UsdImagingPropertyInvalidationType invalidationType)
{
    if (!subprim.IsEmpty() || !appliedInstanceName.IsEmpty()) {
        return {};
    }

    return UsdSkelImagingDataSourceBindingAPI::Invalidate(
        prim, subprim,properties, invalidationType);
}

PXR_NAMESPACE_CLOSE_SCOPE
