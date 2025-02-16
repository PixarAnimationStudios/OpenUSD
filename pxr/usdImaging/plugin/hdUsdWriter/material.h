//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

#ifndef HD_USD_WRITER_MATERIAL_H
#define HD_USD_WRITER_MATERIAL_H

#include "pxr/usdImaging/plugin/hdUsdWriter/utils.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/api.h"

#include "pxr/pxr.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/base/js/json.h"


PXR_NAMESPACE_OPEN_SCOPE

class HdUsdWriterMaterial : public HdMaterial
{
public:
    HF_MALLOC_TAG_NEW("new HdUsdWriterMaterial");

    /// Constructor.
    ///
    ///   \param id Path to the material.
    HDUSDWRITER_API
    HdUsdWriterMaterial(const SdfPath& id);

    /// Synchronizes state from the delegate to this object.
    /// @param[in, out]  dirtyBits: On input specifies which state is
    ///                             is dirty and can be pulled from the scene
    ///                             delegate.
    ///                             On output specifies which bits are still
    ///                             dirty and were not cleaned by the sync.
    ///
    HDUSDWRITER_API
    void Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits) override;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HDUSDWRITER_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Serialize the primitive to USD.
    ///
    ///   \param stage Reference to HdUsdWriter Stage Proxy.
    HDUSDWRITER_API
    virtual void SerializeToUsd(const UsdStagePtr &stage);

private:
    // This class does not support copying.
    HdUsdWriterMaterial(const HdUsdWriterMaterial&) = delete;
    HdUsdWriterMaterial& operator=(const HdUsdWriterMaterial&) = delete;

    HdUsdWriterOptional<HdMaterialNetworkMap> _materialNetworkMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif