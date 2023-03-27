//
// Copyright 2022 Pixar
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
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_REPRESENTED_BY_ANCESTOR_PRIM_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_REPRESENTED_BY_ANCESTOR_PRIM_ADAPTER_H

/// \file usdImaging/representedByAncestorPrimAdapter.h

#include "pxr/usdImaging/usdImaging/primAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingRepresentedByAncestorPrimAdapter
///
/// Base class for all prim adapters which only want to indicate that an
/// ancestor prim is responsible for them.
/// 
/// Because Hydra 1.0 prim adapter methods are still present, their pure
/// virtuals must be implemented here (even if they will won't be called).
/// 
class UsdImagingRepresentedByAncestorPrimAdapter : public UsdImagingPrimAdapter
{
public:

    using BaseAdapter = UsdImagingPrimAdapter;

    UsdImagingRepresentedByAncestorPrimAdapter()
    : UsdImagingPrimAdapter()
    {}

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    PopulationMode GetPopulationMode() override;

    // ---------------------------------------------------------------------- //
    /// \name Overrides for Pure Virtual Legacy Methods
    // ---------------------------------------------------------------------- //

    SdfPath Populate(
        UsdPrim const& prim,
        UsdImagingIndexProxy* index,
        UsdImagingInstancerContext const*
            instancerContext = nullptr) override;

    void TrackVariability(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        HdDirtyBits* timeVaryingBits,
        UsdImagingInstancerContext const* instancerContext = nullptr)
            const override
    {}

    void UpdateForTime(
        UsdPrim const& prim,
        SdfPath const& cachePath, 
        UsdTimeCode time,
        HdDirtyBits requestedBits,
        UsdImagingInstancerContext const* instancerContext = nullptr)
            const override
    {}

    HdDirtyBits ProcessPropertyChange(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        TfToken const& propertyName) override;

    void MarkDirty(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        HdDirtyBits dirty,
        UsdImagingIndexProxy* index) override
    {}

protected:
    void _RemovePrim(
        SdfPath const& cachePath,
        UsdImagingIndexProxy* index) override
    {}

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_REPRESENTED_BY_ANCESTOR_PRIM_ADAPTER_H
