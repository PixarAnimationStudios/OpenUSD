//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdRiPxrImaging/pxrAovLightAdapter.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"

#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdRiPxrImagingAovLightAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter>>();
    t.SetFactory<UsdImagingPrimAdapterFactory<Adapter>>();
}

UsdRiPxrImagingAovLightAdapter::~UsdRiPxrImagingAovLightAdapter() = default;

TfTokenVector
UsdRiPxrImagingAovLightAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdRiPxrImagingAovLightAdapter::GetImagingSubprimType(
    UsdPrim const& prim,
    TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->light;
    }

    return TfToken();
}

bool
UsdRiPxrImagingAovLightAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return UsdImagingLightAdapter::IsEnabledSceneLights() &&
           index->IsSprimTypeSupported(HdPrimTypeTokens->light);
}

SdfPath
UsdRiPxrImagingAovLightAdapter::Populate(
    UsdPrim const& prim,
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const* instancerContext)
{
    return _AddSprim(HdPrimTypeTokens->light, prim, index, instancerContext);
}

void
UsdRiPxrImagingAovLightAdapter::_RemovePrim(
    SdfPath const& cachePath,
    UsdImagingIndexProxy* index)
{
    _RemoveSprim(HdPrimTypeTokens->light, cachePath, index);
}

PXR_NAMESPACE_CLOSE_SCOPE
