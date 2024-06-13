//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_LEGACY_MESHLIGHT_ADAPTER_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_LEGACY_MESHLIGHT_ADAPTER_H

#include "pxr/usdImaging/usdImaging/meshAdapter.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

// Until we transition to a full scene index plugin this adapter allows us to
// use a PxrMesh instead of a Mesh to gain access to the light data on a mesh
// inside of HdPrman.

class HdPrman_LegacyMeshLightAdapter : public UsdImagingMeshAdapter
{
public:
    using BaseAdapter = UsdImagingMeshAdapter;

    void TrackVariability(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        HdDirtyBits* timeVaryingBits,
        UsdImagingInstancerContext const* instancerContext
        = nullptr) const override;

    HdDirtyBits ProcessPropertyChange(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        TfToken const& propertyName) override;

    VtValue GetMaterialResource(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        UsdTimeCode time) const override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_LEGACY_MESHLIGHT_ADAPTER_H
