//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_TEXTURE_H
#define PXR_IMAGING_HGI_WEBGPU_TEXTURE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgi/texture.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiWebGPU;

/// \class HgiWebGPUTexture
///
/// Represents a WebGPU GPU texture resource.
///
class HgiWebGPUTexture final : public HgiTexture {
public:
    HGIWEBGPU_API
    ~HgiWebGPUTexture() override;

    HGIWEBGPU_API
    size_t GetByteSizeOfResource() const override;

    /// This hgi transition helper returns the WebGPU resource as uint64_t
    /// for external clients.
    HGIWEBGPU_API
    uint64_t GetRawResource() const override;

    /// Returns the handle to the WebGPU texture.
    HGIWEBGPU_API
    wgpu::Texture GetTextureHandle() const;

    HGIWEBGPU_API
    wgpu::TextureView GetTextureView() const;

    /// Returns an e2DArray view for cubemap textures, used when binding as a
    /// writable storage texture (WebGPU doesn't support writable Cube views).
    /// For non-cubemap textures this is the same as GetTextureView().
    HGIWEBGPU_API
    wgpu::TextureView GetStorageTextureView() const;

    /// This function does not do anything. There is no support for explicit
    /// layout transition in non-explicit APIs like OpenGL. Hence this function
    /// simply returns void.
    HGIWEBGPU_API
    HgiTextureUsage SubmitLayoutChange(HgiTextureUsage newLayout) override;

protected:
    friend class HgiWebGPU;

    HGIWEBGPU_API
    HgiWebGPUTexture(HgiWebGPU *hgi,
                    HgiTextureDesc const & desc);
    
    HGIWEBGPU_API
    HgiWebGPUTexture(HgiWebGPU *hgi,
                    HgiTextureViewDesc const & desc);
    
private:
    HgiWebGPUTexture() = delete;
    HgiWebGPUTexture & operator=(const HgiWebGPUTexture&) = delete;
    HgiWebGPUTexture(const HgiWebGPUTexture&) = delete;

    wgpu::Texture _textureHandle;
    wgpu::TextureView _textureView;
    wgpu::TextureView _storageTextureView;
    bool _isTextureView;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
