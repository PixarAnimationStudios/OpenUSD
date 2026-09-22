//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiWebGPU/texture.h"
#include "pxr/imaging/hgiWebGPU/buffer.h"
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"

PXR_NAMESPACE_OPEN_SCOPE

static void
_ApplyComponentSwizzle(const wgpu::Device& device,
    const HgiComponentMapping& cm, wgpu::TextureViewDescriptor& viewDesc,
    wgpu::TextureComponentSwizzleDescriptor& swizzleDesc)
{
    if (!device.HasFeature(wgpu::FeatureName::TextureComponentSwizzle)) {
        return;
    }
    if (cm.r == HgiComponentSwizzleR && cm.g == HgiComponentSwizzleG &&
        cm.b == HgiComponentSwizzleB && cm.a == HgiComponentSwizzleA) {
        return;
    }
    swizzleDesc.swizzle.r = HgiWebGPUConversions::GetComponentSwizzle(cm.r);
    swizzleDesc.swizzle.g = HgiWebGPUConversions::GetComponentSwizzle(cm.g);
    swizzleDesc.swizzle.b = HgiWebGPUConversions::GetComponentSwizzle(cm.b);
    swizzleDesc.swizzle.a = HgiWebGPUConversions::GetComponentSwizzle(cm.a);
    viewDesc.nextInChain = &swizzleDesc;
}

HgiWebGPUTexture::HgiWebGPUTexture(HgiWebGPU* hgi, HgiTextureDesc const& desc)
    : HgiTexture(desc)
    , _textureHandle(nullptr)
    , _textureView(nullptr)
    , _isTextureView(false)
{
    wgpu::TextureDescriptor descriptor;
    // TODO: setting TextureBinding since renderAttachment texture could be used
    // as binding in a following pass
    descriptor.usage = wgpu::TextureUsage::CopySrc |
        wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;
    descriptor.format = HgiWebGPUConversions::GetPixelFormat(desc.format);
    descriptor.label = desc.debugName.c_str();

    // Override usage if necessary
    if (desc.usage & HgiTextureUsageBitsColorTarget) {
        descriptor.usage |= wgpu::TextureUsage::RenderAttachment |
            wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
    } else if (desc.usage & HgiTextureUsageBitsDepthTarget) {
        descriptor.format =
            HgiWebGPUConversions::GetDepthOrStencilTextureFormat(
                desc.usage, desc.format);
        descriptor.usage |= wgpu::TextureUsage::RenderAttachment |
            wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
    }

    if (desc.usage & HgiTextureUsageBitsShaderRead) {
        descriptor.usage |= wgpu::TextureUsage::TextureBinding;
    }
    if (desc.usage & HgiTextureUsageBitsShaderWrite) {
        descriptor.usage |= wgpu::TextureUsage::StorageBinding;
    }

    switch (desc.type) {
    case HgiTextureType1D:
        descriptor.dimension = wgpu::TextureDimension::e1D;
        descriptor.size.width = desc.dimensions[0];
        break;
    case HgiTextureType2D:
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = desc.dimensions[0];
        descriptor.size.height = desc.dimensions[1];
        break;
    case HgiTextureType3D:
        descriptor.dimension = wgpu::TextureDimension::e3D;
        descriptor.size.width = desc.dimensions[0];
        descriptor.size.height = desc.dimensions[1];
        descriptor.size.depthOrArrayLayers = desc.dimensions[2];
        break;
    case HgiTextureTypeCubemap:
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = desc.dimensions[0];
        descriptor.size.height = desc.dimensions[1];
        descriptor.size.depthOrArrayLayers = 6;
        break;
    case HgiTextureType1DArray:
        // WebGPU doesn't support 1D array, so we use 2D array with height 1.
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = desc.dimensions[0];
        descriptor.size.height = 1;
        descriptor.size.depthOrArrayLayers = desc.layerCount;
        break;
    case HgiTextureType2DArray:
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = desc.dimensions[0];
        descriptor.size.height = desc.dimensions[1];
        descriptor.size.depthOrArrayLayers = desc.layerCount;
        break;
    default:
        TF_CODING_ERROR("Unsupported desc.type");
    }

    // Integer formats don't support multisampling in WebGPU
    const wgpu::TextureSampleType sampleType =
        HgiWebGPUConversions::GetTextureSampleType(desc.format);
    const bool isIntegerFormat =
        sampleType == wgpu::TextureSampleType::Sint ||
        sampleType == wgpu::TextureSampleType::Uint;
    if (isIntegerFormat) {
        descriptor.sampleCount = 1;
        _descriptor.sampleCount = HgiSampleCount1;
    } else {
        descriptor.sampleCount = desc.sampleCount;
    }

    // This can be less than desc.mipLevels
    const std::vector<HgiMipInfo> mipInfos = HgiGetMipInfos(
        desc.format, desc.dimensions, desc.layerCount, desc.pixelsByteSize);
    descriptor.mipLevelCount = desc.mipLevels;

    _textureHandle = hgi->GetPrimaryDevice().CreateTexture(&descriptor);

    if (desc.initialData && desc.pixelsByteSize > 0) {
        const size_t perPixelSize = HgiGetDataSizeOfFormat(desc.format);
        const auto initialData =
            reinterpret_cast<const char*>(desc.initialData);

        const uint32_t mipLevelCount =
            std::min(static_cast<uint32_t>(mipInfos.size()),
                static_cast<uint32_t>(_descriptor.mipLevels));

        for (size_t mip = 0; mip < mipLevelCount; mip++) {
            const HgiMipInfo& mipInfo = mipInfos[mip];

            wgpu::TexelCopyTextureInfo destination;
            destination.texture = _textureHandle;
            destination.mipLevel = mip;

            wgpu::Extent3D writeSize;
            writeSize.width = static_cast<uint32_t>(mipInfo.dimensions[0]);
            writeSize.height = static_cast<uint32_t>(mipInfo.dimensions[1]);
            writeSize.depthOrArrayLayers =
                desc.layerCount > 1 ? desc.layerCount : mipInfo.dimensions[2];

            wgpu::TexelCopyBufferLayout dataLayout;
            dataLayout.bytesPerRow = writeSize.width * perPixelSize;
            dataLayout.rowsPerImage = writeSize.height;

            wgpu::Queue queue = hgi->GetQueue();
            queue.WriteTexture(&destination, initialData + mipInfo.byteOffset,
                writeSize.depthOrArrayLayers * mipInfo.byteSizePerLayer,
                &dataLayout, &writeSize);
        }
    }

    // create the texture view
    wgpu::TextureViewDescriptor textureViewDesc;
    textureViewDesc.format = descriptor.format;
    textureViewDesc.dimension =
        HgiWebGPUConversions::GetTextureViewDimension(desc.type);
    textureViewDesc.mipLevelCount = descriptor.mipLevelCount;
    textureViewDesc.arrayLayerCount = (desc.type == HgiTextureType3D) ?
        1 : descriptor.size.depthOrArrayLayers;

    wgpu::TextureComponentSwizzleDescriptor swizzleDesc;
    _ApplyComponentSwizzle(hgi->GetPrimaryDevice(), desc.componentMapping,
        textureViewDesc, swizzleDesc);
    _textureView = _textureHandle.CreateView(&textureViewDesc);

    // Cubemaps need a 2DArray view for writable storage texture bindings
    // since WebGPU doesn't support writable cube views.
    if (desc.type == HgiTextureTypeCubemap) {
        wgpu::TextureViewDescriptor storageViewDesc = textureViewDesc;
        storageViewDesc.dimension = wgpu::TextureViewDimension::e2DArray;
        _storageTextureView = _textureHandle.CreateView(&storageViewDesc);
    }
}

HgiWebGPUTexture::HgiWebGPUTexture(
    HgiWebGPU* hgi, HgiTextureViewDesc const& desc)
    : HgiTexture(desc.sourceTexture->GetDescriptor())
    , _textureHandle(nullptr)
    , _storageTextureView(nullptr)
    , _isTextureView(true)
{
    HgiWebGPUTexture* srcTexture =
        static_cast<HgiWebGPUTexture*>(desc.sourceTexture.Get());

    _descriptor.debugName = desc.debugName;
    _descriptor.format = desc.format;
    _descriptor.layerCount = desc.layerCount;
    _descriptor.mipLevels = desc.mipLevels;

    _textureHandle = srcTexture->GetTextureHandle();

    // create the texture view
    wgpu::TextureViewDescriptor textureViewDesc;
    if (_descriptor.usage & HgiTextureUsageBitsDepthTarget) {
        textureViewDesc.format =
            HgiWebGPUConversions::GetDepthOrStencilTextureFormat(
                _descriptor.usage, desc.format);
    } else {
        textureViewDesc.format =
            HgiWebGPUConversions::GetPixelFormat(desc.format);
    }
    textureViewDesc.dimension =
        HgiWebGPUConversions::GetTextureViewDimension(_descriptor.type);
    textureViewDesc.baseMipLevel = desc.sourceFirstMip;
    textureViewDesc.mipLevelCount = desc.mipLevels;
    textureViewDesc.baseArrayLayer = desc.sourceFirstLayer;
    textureViewDesc.arrayLayerCount = desc.layerCount;

    wgpu::TextureComponentSwizzleDescriptor swizzleDesc;
    _ApplyComponentSwizzle(hgi->GetPrimaryDevice(),
        srcTexture->GetDescriptor().componentMapping, textureViewDesc,
        swizzleDesc);
    _textureView = _textureHandle.CreateView(&textureViewDesc);

    if (_descriptor.type == HgiTextureTypeCubemap) {
        wgpu::TextureViewDescriptor storageViewDesc = textureViewDesc;
        storageViewDesc.dimension = wgpu::TextureViewDimension::e2DArray;
        _storageTextureView = _textureHandle.CreateView(&storageViewDesc);
    }
}

HgiWebGPUTexture::~HgiWebGPUTexture()
{
    if (_textureHandle && !_isTextureView) {
        _textureHandle.Destroy();
    }
}

size_t
HgiWebGPUTexture::GetByteSizeOfResource() const
{
    return _GetByteSizeOfResource(_descriptor);
}

uint64_t
HgiWebGPUTexture::GetRawResource() const
{
    return reinterpret_cast<uintptr_t>(_textureHandle.Get());
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

wgpu::TextureView
HgiWebGPUTexture::GetStorageTextureView() const
{
    return _storageTextureView ? _storageTextureView : _textureView;
}

HgiTextureUsage
HgiWebGPUTexture::SubmitLayoutChange(HgiTextureUsage newLayout)
{
    return 0;
}

PXR_NAMESPACE_CLOSE_SCOPE
