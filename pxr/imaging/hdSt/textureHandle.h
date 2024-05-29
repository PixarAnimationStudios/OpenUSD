//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    HDST_API
    void ReallocateSamplerIfNecessary();

    /// Get whether bindless texture handles are enabled.
    ///
    bool UseBindlessHandles() const;

private:
    HdStTextureObjectSharedPtr _textureObject;
    HdStSamplerObjectSharedPtr _samplerObject;
    HdSamplerParameters _samplerParams;
    size_t _memoryRequest;
    HdStShaderCodePtr _shaderCode;
    HdSt_TextureHandleRegistry *_textureHandleRegistry;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
