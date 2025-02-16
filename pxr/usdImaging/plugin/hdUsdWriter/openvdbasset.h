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

#ifndef HD_USD_WRITER_OVD_ASSET_H
#define HD_USD_WRITER_OVD_ASSET_H

#include "pxr/usdImaging/plugin/hdUsdWriter/utils.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/api.h"

#include "pxr/imaging/hd/field.h"
#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Class to handle synchronizing openvdb assets.
class HdUsdWriterOpenvdbAsset : public HdField
{
public:
    /// Constructor.
    ///
    ///   \param id Path to the OpenVdb asset.
    HDUSDWRITER_API
    explicit HdUsdWriterOpenvdbAsset(const SdfPath& id);

    HDUSDWRITER_API
    virtual ~HdUsdWriterOpenvdbAsset() = default;

    /// Syncs the OpenVDB asset from the scene delegate.
    ///
    ///   \param sceneDelegate Pointer to the Hydra scene delegate.
    ///   \param renderParam
    ///   \param dirtyBits
    HDUSDWRITER_API
    void Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits) override;

    /// Gets the initial dirty bit mask for the primitive.
    ///
    ///   \return Initial dirty bit mask.
    HDUSDWRITER_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Serialize the primitive to USD.
    ///
    ///   \param stage Reference to HdUsdWriter Stage Proxy.
    HDUSDWRITER_API
    virtual void SerializeToUsd(const UsdStagePtr &stage);

protected:
    HdUsdWriterOptional<SdfAssetPath> _filePath;
    HdUsdWriterOptional<GfMatrix4d> _transform;
    HdUsdWriterOptional<TfToken> _fieldName;
    HdUsdWriterOptional<int> _fieldIndex;
    HdUsdWriterOptional<TfToken> _fieldDataType;
    HdUsdWriterOptional<TfToken> _vectorDataRoleHint;
    HdUsdWriterOptional<TfToken> _fieldClass;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif