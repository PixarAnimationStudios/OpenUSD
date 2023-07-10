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
#include "pxr/imaging/hgiWebGPU/buffer.h"
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/texture.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPUTexture::HgiWebGPUTexture(HgiWebGPU *hgi, HgiTextureDesc const & desc)
    : HgiTexture(desc)
    , _textureHandle(nullptr)
    , _textureView(nullptr)
{
    wgpu::TextureDescriptor descriptor;
    // TODO: setting TextureBinding since renderAttachment texture could be use as binding in a following pass
    descriptor.usage = wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;
    descriptor.format = HgiWebGPUConversions::GetPixelFormat(desc.format);
    descriptor.label = desc.debugName.c_str();

    // Override usage if necessary
    if (desc.usage & HgiTextureUsageBitsColorTarget) {
        descriptor.usage |= wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
    } else if (desc.usage & HgiTextureUsageBitsDepthTarget) {
        descriptor.format = HgiWebGPUConversions::GetDepthOrStencilTextureFormat(desc.usage, desc.format);
        descriptor.usage |= wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
    }
    _pixelFormat = descriptor.format;

    if (desc.usage & HgiTextureUsageBitsShaderRead) {
        descriptor.usage |= wgpu::TextureUsage::TextureBinding;
    }
    if (desc.usage & HgiTextureUsageBitsShaderWrite) {
        descriptor.usage |= wgpu::TextureUsage::StorageBinding;
    }

    descriptor.size.width = desc.dimensions[0];
    descriptor.size.height = desc.dimensions[1];
    descriptor.size.depthOrArrayLayers = desc.dimensions[2];
    descriptor.dimension = desc.dimensions[1] > 1 ? ( desc.dimensions[2] > 1 ? wgpu::TextureDimension::e3D : wgpu::TextureDimension::e2D) : wgpu::TextureDimension::e1D;
    descriptor.sampleCount = desc.sampleCount;
    descriptor.mipLevelCount = desc.mipLevels;

    _textureHandle = hgi->GetPrimaryDevice().CreateTexture(&descriptor);

    _stagingDatas.resize(_stagingDatas.size() + 1);

    if (desc.initialData && desc.pixelsByteSize > 0) {
        size_t perPixelSize = HgiGetDataSizeOfFormat(desc.format);

        // upload each provide mip level
        const std::vector<HgiMipInfo> mipInfos =
            HgiGetMipInfos(
                desc.format,
                desc.dimensions,
                desc.layerCount,
                desc.pixelsByteSize);
        const size_t mipLevels = std::min(
            mipInfos.size(), size_t(desc.mipLevels));
        const char * const initialData = reinterpret_cast<const char *>(
            desc.initialData);

        for (size_t mip = 0; mip < mipLevels; mip++) {
            if (desc.type != HgiTextureType2D)
            {
                TF_CODING_ERROR("Not implemented for types other than HgiTextureType2D");
                continue;
            }
            const HgiMipInfo &mipInfo = mipInfos[mip];

            const size_t width = mipInfo.dimensions[0];
            const size_t height = mipInfo.dimensions[1];
            const size_t depth = 1;
            const size_t bytesPerRow = perPixelSize * width;

            _stagingDatas.resize(_stagingDatas.size() + 1);
            auto &stagingData = *_stagingDatas.rbegin();

            wgpu::ImageCopyTexture &destination = stagingData.destination;
            destination.texture = _textureHandle;
            destination.mipLevel = mip;
            destination.origin = { 0, 0, 0 };

            wgpu::TextureDataLayout &dataLayout = stagingData.dataLayout;
            dataLayout.bytesPerRow = bytesPerRow;
            dataLayout.rowsPerImage = height;

            wgpu::Extent3D writeSize = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), static_cast<uint32_t>(depth) };

            wgpu::Queue queue = hgi->GetQueue();
            queue.WriteTexture(&destination, initialData + mipInfo.byteOffset, bytesPerRow * height, &dataLayout, &writeSize);
        }
    }

    // create the texture view
    wgpu::TextureViewDescriptor textureViewDesc;
    textureViewDesc.format = _pixelFormat;
    textureViewDesc.dimension = _descriptor.dimensions[1] > 1 ? ( _descriptor.dimensions[2] > 1 ? wgpu::TextureViewDimension::e3D : wgpu::TextureViewDimension::e2D) : wgpu::TextureViewDimension::e1D;
    textureViewDesc.mipLevelCount = desc.mipLevels;
    textureViewDesc.arrayLayerCount = desc.layerCount;
	_textureView = _textureHandle.CreateView(&textureViewDesc);
}

HgiWebGPUTexture::HgiWebGPUTexture(HgiWebGPU *hgi, HgiTextureViewDesc const & desc)
    : HgiTexture(desc.sourceTexture->GetDescriptor())
    , _textureHandle(nullptr)
{
    HgiWebGPUTexture* srcTexture =
        static_cast<HgiWebGPUTexture*>(desc.sourceTexture.Get());

    _descriptor.format = desc.format;
    _descriptor.layerCount = desc.layerCount;
    _descriptor.mipLevels = desc.mipLevels;
    
    // TODO: probably make a *copy* of this resource rather than just a shallow copy of the handle
    _textureHandle = srcTexture->GetTextureHandle();

    // create the texture view
    wgpu::TextureViewDescriptor textureViewDesc;
    textureViewDesc.format = srcTexture->_pixelFormat;
    textureViewDesc.dimension = _descriptor.dimensions[1] > 1 ? ( _descriptor.dimensions[2] > 1 ? wgpu::TextureViewDimension::e3D : wgpu::TextureViewDimension::e2D) : wgpu::TextureViewDimension::e1D;
	textureViewDesc.mipLevelCount = desc.mipLevels;
    textureViewDesc.arrayLayerCount = desc.layerCount;
	_textureView = _textureHandle.CreateView(&textureViewDesc);
}

HgiWebGPUTexture::~HgiWebGPUTexture()
{
    if (_textureHandle) {
        _textureHandle.Destroy();
        _textureHandle = nullptr;
    }
    _textureView = nullptr;
}

size_t
HgiWebGPUTexture::GetByteSizeOfResource() const
{
    return _GetByteSizeOfResource(_descriptor);
}

uint64_t
HgiWebGPUTexture::GetRawResource() const
{
    return (uint64_t) _textureHandle.Get();
}

wgpu::Texture
HgiWebGPUTexture::GetTextureHandle() const
{
    return _textureHandle;
}

wgpu::TextureView
HgiWebGPUTexture::GetTextureView() const
{
    return _textureView;
}

PXR_NAMESPACE_CLOSE_SCOPE
