//
// Copyright 2017 Pixar
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
#ifndef PXR_USD_IMAGING_USD_IMAGING_DOME_LIGHT_1_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_DOME_LIGHT_1_ADAPTER_H

/// \file usdImaging/domeLight_1Adapter.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/lightAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE


class UsdPrim;

/// \class UsdImagingDomeLight_1Adapter
///
/// Adapter class for lights of type DomeLight_1
///
class UsdImagingDomeLight_1Adapter : public UsdImagingLightAdapter {
public:
    typedef UsdImagingLightAdapter BaseAdapter;

    UsdImagingDomeLight_1Adapter()
        : UsdImagingLightAdapter()
    {}

    USDIMAGING_API
    virtual ~UsdImagingDomeLight_1Adapter();

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;

    USDIMAGING_API
    TfToken GetImagingSubprimType(UsdPrim const& prim, TfToken const& subprim)
        override;

    USDIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
        UsdPrim const& prim, TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    USDIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        UsdImagingPropertyInvalidationType invalidationType) override;

    // ---------------------------------------------------------------------- //
    /// \name Light Params
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    VtValue GetLightParamValue(
        const UsdPrim& prim, const SdfPath& cachePath,
        const TfToken& paramName, UsdTimeCode time) const override;

    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    SdfPath Populate(UsdPrim const& prim,
         UsdImagingIndexProxy* index,
         UsdImagingInstancerContext const* instancerContext = NULL) override;

    USDIMAGING_API
    bool IsSupported(UsdImagingIndexProxy const* index) const override;
    
protected:
    USDIMAGING_API
    void _RemovePrim(SdfPath const& cachePath,
                             UsdImagingIndexProxy* index) override final ;

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DOME_LIGHT_1_ADAPTER_H
