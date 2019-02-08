//
// Copyright 2019 Pixar
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
#ifndef USDSKELIMAGING_SKELROOTADAPTER_H
#define USDSKELIMAGING_SKELROOTADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdSkelImaging/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingSkelRootAdapter
///
/// The SkelRoot adapter exists for two reasons:
/// (a) Registering the SkeletonAdapter to handle processing of any skinned
///     prim under a SkelRoot prim.
///     The UsdSkel schema requires that ANY skinned prim lives under a 
///     SkelRoot.
/// (b) Getting the skeleton that deforms each skinned prim, which is stored
///     in the SkeletonAdapter (the latter is stateful).
/// Both of these happen during Populate(..)
///
class UsdSkelImagingSkelRootAdapter : public UsdImagingPrimAdapter {
public:
    using BaseAdapter = UsdImagingPrimAdapter;

    UsdSkelImagingSkelRootAdapter()
        : BaseAdapter()
    {}

    USDSKELIMAGING_API
    virtual ~UsdSkelImagingSkelRootAdapter();

    // ---------------------------------------------------------------------- //
    /// \name Initialization
    // ---------------------------------------------------------------------- //
    USDSKELIMAGING_API
    SdfPath
    Populate(const UsdPrim& prim,
             UsdImagingIndexProxy* index,
             const UsdImagingInstancerContext*
                 instancerContext=nullptr) override;

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //

    /// Thread Safe.
    USDSKELIMAGING_API
    void TrackVariability(const UsdPrim& prim,
                          const SdfPath& cachePath,
                          HdDirtyBits* timeVaryingBits,
                          const UsdImagingInstancerContext* 
                             instancerContext = nullptr) const override;

    /// Thread Safe.
    USDSKELIMAGING_API
    void UpdateForTime(const UsdPrim& prim,
                       const SdfPath& cachePath, 
                       UsdTimeCode time,
                       HdDirtyBits requestedBits,
                       const UsdImagingInstancerContext*
                           instancerContext=nullptr) const override;

    // ---------------------------------------------------------------------- //
    /// \name Change Processing
    // ---------------------------------------------------------------------- //

    USDSKELIMAGING_API
    HdDirtyBits ProcessPropertyChange(const UsdPrim& prim,
                                      const SdfPath& cachePath,
                                      const TfToken& propertyName) override;

    USDSKELIMAGING_API
    void MarkDirty(const UsdPrim& prim,
                   const SdfPath& cachePath,
                   HdDirtyBits dirty,
                   UsdImagingIndexProxy* index) override;

protected:
    USDSKELIMAGING_API
    void _RemovePrim(const SdfPath& cachePath,
                     UsdImagingIndexProxy* index) override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKELIMAGING_SKELROOTADAPTER_H
