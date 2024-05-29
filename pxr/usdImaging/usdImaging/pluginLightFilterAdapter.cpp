//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/pluginLightFilterAdapter.h"
#include "pxr/usdImaging/usdImaging/lightAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingPluginLightFilterAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingPluginLightFilterAdapter::~UsdImagingPluginLightFilterAdapter() 
{
}

bool
UsdImagingPluginLightFilterAdapter::IsSupported(
        UsdImagingIndexProxy const* index) const
{
    return UsdImagingLightAdapter::IsEnabledSceneLights() &&
          index->IsSprimTypeSupported(HdPrimTypeTokens->lightFilter);
}

SdfPath
UsdImagingPluginLightFilterAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    index->InsertSprim(HdPrimTypeTokens->lightFilter, prim.GetPath(), prim);
    HD_PERF_COUNTER_INCR(HdPrimTypeTokens->lightFilter);

    return prim.GetPath();
}

void
UsdImagingPluginLightFilterAdapter::_RemovePrim(SdfPath const& cachePath,
                                         UsdImagingIndexProxy* index)
{
    index->RemoveSprim(HdPrimTypeTokens->lightFilter, cachePath);
}

PXR_NAMESPACE_CLOSE_SCOPE
