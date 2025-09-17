//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_INSTANCEABLE_PRIM_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_INSTANCEABLE_PRIM_ADAPTER_H

#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// An abstract adapter class for prims that are instanceable. Adapters for
/// instanceable prims should derive from this class instead of
/// UsdImaginggPrimAdapter.
class UsdImagingInstanceablePrimAdapter : public UsdImagingPrimAdapter
{
public:
    using BaseAdapter = UsdImagingPrimAdapter;

protected:
    friend class UsdImagingInstanceAdapter;
    friend class UsdImagingPointInstancerAdapter;
    // ---------------------------------------------------------------------- //
    /// \name Utility
    // ---------------------------------------------------------------------- //
    
    // Given the USD path for a prim of this adapter's type, returns
    // the prim's Hydra cache path.
    USDIMAGING_API
    SdfPath
    ResolveCachePath(
        const SdfPath& usdPath,
        const UsdImagingInstancerContext*
            instancerContext = nullptr) const override;
    
    // Given the cachePath and instancerContext, resolve the proxy prim path
    USDIMAGING_API
    SdfPath
    ResolveProxyPrimPath(
        const SdfPath& cachePath,
        const UsdImagingInstancerContext*
            instancerContext = nullptr) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_INSTANCEABLE_PRIM_ADAPTER_H
