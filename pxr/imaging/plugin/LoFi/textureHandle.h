//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef LOFI_TEXTURE_HANDLE_H
#define LOFI_TEXTURE_HANDLE_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"

#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/types.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using LoFiShaderCodePtr =
    std::weak_ptr<class LoFiShaderCode>;
using LoFiTextureObjectSharedPtr =
    std::shared_ptr<class LoFiTextureObject>;
using LoFiSamplerObjectSharedPtr =
    std::shared_ptr<class LoFiSamplerObject>;

using LoFiTextureHandleSharedPtr =
    std::shared_ptr<class LoFiTextureHandle>;

class LoFiTextureHandleRegistry;

/// \class LoFiTextureHandle
///
/// Represents a texture and sampler that will be allocated and loaded
/// from a texture file during commit, possibly a texture sampler
/// handle and a memory request. It is intended for HdStShaderCode and
/// LoFiShaderCode::AddResourcesFromTextures() is called whenever
/// the underlying texture and sampler gets allocated and (re-)loaded
/// so that the shader code can react to, e.g., changing texture
/// sampler handle for bindless or changing texture metadata such as a
/// field bounding box for volumes.
/// 
class LoFiTextureHandle
{
public:
    /// See LoFiResourceRegistry::AllocateTextureHandle for details.
    LOFI_API
    LoFiTextureHandle(
        LoFiTextureObjectSharedPtr const &textureObject,
        const HdSamplerParameters &samplerParams,
        size_t memoryRequest,
        bool createBindlessHandle,
        LoFiShaderCodePtr const & shaderCode,
        LoFiTextureHandleRegistry *textureHandleRegistry);

    LOFI_API
    ~LoFiTextureHandle();

    /// Get texture object.
    ///
    /// Can be accessed after commit.
    LoFiTextureObjectSharedPtr const &GetTextureObject() const {
        return _textureObject;
    }

    /// Get sampler object.
    ///
    /// Can be accessed after commit.
    LoFiSamplerObjectSharedPtr const &GetSamplerObject() const {
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
    LoFiShaderCodePtr const &GetShaderCode() const {
        return _shaderCode;
    }

    /// Allocate sampler for this handle (not thread-safe).
    ///
    /// This also creates the texture sampler handle (for bindless
    /// textures) and updates it on subsequent calls.
    ///
    LOFI_API
    void ReallocateSamplerIfNecessary();

private:
    LoFiTextureObjectSharedPtr _textureObject;
    LoFiSamplerObjectSharedPtr _samplerObject;
    HdSamplerParameters _samplerParams;
    size_t _memoryRequest;
    bool _createBindlessHandle;
    LoFiShaderCodePtr _shaderCode;
    LoFiTextureHandleRegistry *_textureHandleRegistry;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
