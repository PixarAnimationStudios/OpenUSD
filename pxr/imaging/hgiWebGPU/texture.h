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
    wgpu::TextureFormat _pixelFormat;

    struct StagingData
    {
        wgpu::ImageCopyTexture destination;
        wgpu::TextureDataLayout dataLayout;
        std::vector<uint8_t> initData;
    };
    std::vector<StagingData> _stagingDatas;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
