//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
