//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/pluginLightAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingPluginLightAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingPluginLightAdapter::~UsdImagingPluginLightAdapter() 
{
}

TfTokenVector
UsdImagingPluginLightAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingPluginLightAdapter::GetImagingSubprimType(
    UsdPrim const& prim,
    TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->pluginLight;
    }

    return TfToken();
}

bool
UsdImagingPluginLightAdapter::IsSupported(
        UsdImagingIndexProxy const* index) const
{
    return UsdImagingLightAdapter::IsEnabledSceneLights() &&
          index->IsSprimTypeSupported(HdPrimTypeTokens->pluginLight);
}

SdfPath
UsdImagingPluginLightAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddSprim(HdPrimTypeTokens->pluginLight, prim, index, instancerContext);
}

void
UsdImagingPluginLightAdapter::_RemovePrim(SdfPath const& cachePath,
                                         UsdImagingIndexProxy* index)
{
    _RemoveSprim(HdPrimTypeTokens->pluginLight, cachePath, index);
}

PXR_NAMESPACE_CLOSE_SCOPE
