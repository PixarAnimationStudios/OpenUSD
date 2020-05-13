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
#ifndef HD_ST_TEXTURE_HANDLE_H
#define HD_ST_TEXTURE_HANDLE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/types.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HdStShaderCodePtr =
    std::weak_ptr<class HdStShaderCode>;
using HdStTextureObjectSharedPtr =
    std::shared_ptr<class HdStTextureObject>;
using HdStSamplerObjectSharedPtr =
    std::shared_ptr<class HdStSamplerObject>;

using HdStTextureHandleSharedPtr =
    std::shared_ptr<class HdStTextureHandle>;

class HdSt_TextureHandleRegistry;

/// \class HdStTextureHandle
///
/// Represents a texture and sampler that will be allocated and loaded
/// from a texture file during commit, possibly a texture sampler
/// handle and a memory request. It is intended for HdStShaderCode and
/// HdStShaderCode::AddResourcesFromTextures() is called whenever
/// the underlying texture and sampler gets allocated and (re-)loaded
/// so that the shader code can react to, e.g., changing texture
/// sampler handle for bindless or changing texture metadata such as a
/// field bounding box for volumes.
/// 
class HdStTextureHandle
{
public:
    /// See HdStResourceRegistry::AllocateTextureHandle for details.
    HDST_API
    HdStTextureHandle(
        HdStTextureObjectSharedPtr const &textureObject,
        const HdSamplerParameters &samplerParams,
        size_t memoryRequest,
        bool createBindlessHandle,
        HdStShaderCodePtr const & shaderCode,
        HdSt_TextureHandleRegistry *textureHandleRegistry);

    HDST_API
    ~HdStTextureHandle();

    /// Get texture object.
    ///
    /// Can be accessed after commit.
    HdStTextureObjectSharedPtr const &GetTextureObject() const {
        return _textureObject;
    }

    /// Get sampler object.
    ///
    /// Can be accessed after commit.
    HdStSamplerObjectSharedPtr const &GetSamplerObject() const {
        return _samplerObject;
    }
 
    /// Get sampler parameters.
    ///
    HdSamplerParameters const &GetSamplerParameters() const {
        return _samplerParams;
    }

    /// Get how much memory this handle requested for the texture.
    ///
    size_t GetMemoryRequest() const {
        return _memoryRequest;
    }

    /// Get the shader code associated with this handle.
    ///
    HdStShaderCodePtr const &GetShaderCode() const {
        return _shaderCode;
    }

    /// Allocate sampler for this handle (not thread-safe).
    ///
    /// This also creates the texture sampler handle (for bindless
    /// textures) and updates it on subsequent calls.
    ///
    HDST_API
    void ReallocateSamplerIfNecessary();

private:
    HdStTextureObjectSharedPtr _textureObject;
    HdStSamplerObjectSharedPtr _samplerObject;
    HdSamplerParameters _samplerParams;
    size_t _memoryRequest;
    bool _createBindlessHandle;
    HdStShaderCodePtr _shaderCode;
    HdSt_TextureHandleRegistry *_textureHandleRegistry;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
