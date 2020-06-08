//
// Copyright 2020 Pixar
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
#ifndef PXR_IMAGING_HD_ST_DYNAMIC_UV_TEXTURE_OBJECT_H
#define PXR_IMAGING_HD_ST_DYNAMIC_UV_TEXTURE_OBJECT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hdSt/textureObject.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HgiTextureDesc;
class HdSt_TextureObjectRegistry;

/// \class HdStDynamicUvTextureObject
///
/// A uv texture that is managed but not populated by the Storm texture system.
///
/// Clients can allocate an instance of this class through
/// HdStResourceRegistry::AllocateTextureHandle or AllocateTextureObject
/// by passing an HdStTextureIdentifier with a
/// HdStDynamicUvSubtextureIdentifier.
///
/// The client can allocate the GPU resource with CreateTexture and populate it
/// by either giving data in the HgiTextureDesc or binding the texture as target
/// for a computation or render.
///
/// Bindless texture sampler handles can only be created correctly if
/// a client has created the texture before the texture commit phase
/// is finished.
///
class HdStDynamicUvTextureObject final : public HdStUvTextureObject
{
public:
    HDST_API
    HdStDynamicUvTextureObject(
        const HdStTextureIdentifier &textureId,
        HdSt_TextureObjectRegistry *textureObjectRegistry);

    HDST_API
    ~HdStDynamicUvTextureObject() override;

    /// Allocate GPU resource using the texture descriptor. Populate
    /// if data are given in the descriptor.
    /// 
    HDST_API
    void CreateTexture(const HgiTextureDesc &desc);

    /// Get the handle to the actual GPU resource.
    ///
    /// Only valid after CreateTexture has been called.
    ///
    HDST_API
    HgiTextureHandle const &GetTexture() const override {
        return _gpuTexture;
    }

    /// Just returns HdWrapNoOpinion.
    ///
    HDST_API
    const std::pair<HdWrap, HdWrap> &GetWrapParameters() const override;

    /// Always returns true - so that samplers for this texture are
    /// created.
    ///
    HDST_API
    bool IsValid() const override;

protected:
    HDST_API
    void _Load() override;

    HDST_API
    void _Commit() override;

private:
    HgiTextureHandle _gpuTexture;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
