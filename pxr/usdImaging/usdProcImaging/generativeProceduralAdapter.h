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
#ifndef PXR_USD_IMAGING_USD_PROC_IMAGING_GENERATIVE_PROCEDURAL_ADAPTER_H
#define PXR_USD_IMAGING_USD_PROC_IMAGING_GENERATIVE_PROCEDURAL_ADAPTER_H

#include "pxr/usdImaging/usdProcImaging/api.h"
#include "pxr/usdImaging/usdImaging/instanceablePrimAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdProcImagingGenerativeProceduralAdapter
    : public UsdImagingInstanceablePrimAdapter
{
public:
    using BaseAdapter = UsdImagingInstanceablePrimAdapter;

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDPROCIMAGING_API
    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;

    USDPROCIMAGING_API
    TfToken GetImagingSubprimType(UsdPrim const& prim, TfToken const& subprim)
        override;

    USDPROCIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    USDPROCIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType) override;

    // ---------------------------------------------------------------------- //
    /// \name Initialization
    // ---------------------------------------------------------------------- //


    USDPROCIMAGING_API
    SdfPath Populate(
        UsdPrim const& prim,
        UsdImagingIndexProxy* index,
        UsdImagingInstancerContext const*
            instancerContext = nullptr) override;

    USDPROCIMAGING_API
    bool IsSupported(UsdImagingIndexProxy const* index) const override;

    USDPROCIMAGING_API
    void UpdateForTime(
        UsdPrim const& prim,
        SdfPath const& cachePath, 
        UsdTimeCode time,
        HdDirtyBits requestedBits,
        UsdImagingInstancerContext const* 
            instancerContext = nullptr) const override;

    USDPROCIMAGING_API
    VtValue Get(UsdPrim const& prim,
                SdfPath const& cachePath,
                TfToken const& key,
                UsdTimeCode time,
                VtIntArray *outIndices) const override;

    USDPROCIMAGING_API
    HdDirtyBits ProcessPropertyChange(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        TfToken const& propertyName) override;


    USDPROCIMAGING_API
    void MarkDirty(UsdPrim const& prim,
                           SdfPath const& cachePath,
                           HdDirtyBits dirty,
                           UsdImagingIndexProxy* index) override;

    USDPROCIMAGING_API
    void MarkTransformDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index) override;

    USDPROCIMAGING_API
    void MarkVisibilityDirty(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdImagingIndexProxy* index) override;

    USDPROCIMAGING_API
    void TrackVariability(UsdPrim const& prim,
                          SdfPath const& cachePath,
                          HdDirtyBits* timeVaryingBits,
                          UsdImagingInstancerContext const* 
                              instancerContext = nullptr) const override;

protected:
    USDPROCIMAGING_API
    void _RemovePrim(SdfPath const& cachePath,
        UsdImagingIndexProxy* index) override;

private:
    TfToken _GetHydraPrimType(UsdPrim const& prim);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_PROC_IMAGING_GENERATIVE_PROCEDURAL_ADAPTER_H
