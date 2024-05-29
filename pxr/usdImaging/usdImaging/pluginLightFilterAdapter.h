//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_PLUGIN_LIGHT_FILTER_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_PLUGIN_LIGHT_FILTER_ADAPTER_H

/// \file usdImaging/pluginLightFilterAdapter.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/lightFilterAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE


class UsdPrim;

/// \class UsdImagingPluginLightFilterAdapter
///
/// Adapter class for lights of type PluginLightFilter
///
class UsdImagingPluginLightFilterAdapter : public UsdImagingLightFilterAdapter {
public:
    typedef UsdImagingLightFilterAdapter BaseAdapter;

    UsdImagingPluginLightFilterAdapter()
        : UsdImagingLightFilterAdapter()
    {}

    USDIMAGING_API
    virtual ~UsdImagingPluginLightFilterAdapter();

    USDIMAGING_API
    virtual SdfPath Populate(UsdPrim const& prim,
                     UsdImagingIndexProxy* index,
                     UsdImagingInstancerContext const* instancerContext = NULL);

    USDIMAGING_API
    virtual bool IsSupported(UsdImagingIndexProxy const* index) const;

protected:
    virtual void _RemovePrim(SdfPath const& cachePath,
                             UsdImagingIndexProxy* index) final;

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_PLUGIN_LIGHT_FILTER_ADAPTER_H
