//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_LEGACY_VOLUMELIGHT_ADAPTER_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_LEGACY_VOLUMELIGHT_ADAPTER_H

#include "pxr/base/arch/defines.h"
#if !defined(ARCH_OS_WINDOWS)

#include "pxr/usdImaging/usdImaging/volumeAdapter.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

// Until we transition to a full scene index plugin this adapter allows us to
// use a PxrVolume instead of a Volume to gain access to the light data on a volume
// inside of HdPrman.

class HdPrman_LegacyVolumeLightAdapter : public UsdImagingVolumeAdapter
{
public:
    using BaseAdapter = UsdImagingVolumeAdapter;

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

    HdVolumeFieldDescriptorVector GetVolumeFieldDescriptors(
        UsdPrim const& usdPrim,
        SdfPath const& id,
        UsdTimeCode time) const override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // !defined(ARCH_OS_WINDOWS)

#endif  // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_LEGACY_VOLUMELIGHT_ADAPTER_H
