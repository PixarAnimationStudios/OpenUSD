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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/capabilities.h"
#include "pxr/imaging/hgiVulkan/commandBuffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/garbageCollector.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/texture.h"
#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

static bool
_CheckFormatSupport(
    VkPhysicalDevice pDevice,
    VkFormat format,
    VkFormatFeatureFlags flags )
{
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(pDevice, format, &props);
    return (props.optimalTilingFeatures & flags) == flags;
}

HgiVulkanTexture::HgiVulkanTexture(
    HgiVulkan* hgi,
    HgiVulkanDevice* device,
    HgiTextureDesc const & desc)
    : HgiTexture(desc)
    , _isTextureView(false)
    , _vkImage(nullptr)
    , _vkImageView(nullptr)
    , _vkImageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
    , _vmaImageAllocation(nullptr)
    , _device(device)
    , _inflightBits(0)
    , _stagingBuffer(nullptr)
    , _cpuStagingAddress(nullptr)
{
    GfVec3i const& dimensions = desc.dimensions;
    bool isDepthBuffer = desc.usage & HgiTextureUsageBitsDepthTarget;

    //
    // Gather image create info
    //

    VkImageCreateInfo imageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

    imageCreateInfo.imageType = HgiVulkanConversions::GetTextureType(desc.type);
    imageCreateInfo.format = HgiVulkanConversions::GetFormat(desc.format);
    imageCreateInfo.mipLevels = desc.mipLevels;
    imageCreateInfo.arrayLayers = desc.layerCount;
    imageCreateInfo.samples = 
        HgiVulkanConversions::GetSampleCount(desc.sampleCount);
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = { (uint32_t) dimensions[0],
                               (uint32_t) dimensions[1],
                               (uint32_t) dimensions[2] };

    imageCreateInfo.usage = HgiVulkanConversions::GetTextureUsage(desc.usage);
    if (imageCreateInfo.usage == 0) {
        TF_CODING_ERROR("Texture usage missing in descriptor");
        imageCreateInfo.usage = 
            HgiTextureUsageBitsColorTarget | 
            HgiTextureUsageBitsShaderRead |
            HgiTextureUsageBitsShaderWrite;
    }

    // XXX VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT could be a useful
    // optimization, but Hgi doesn'tell us if a resource is transient.
    imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | 
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // XXX STORAGE_IMAGE requires VK_IMAGE_USAGE_STORAGE_BIT, but Hgi
    // doesn't tell us if a texture will be used as image load/store.
    if ((desc.usage & HgiTextureUsageBitsShaderRead) ||
            (desc.usage & HgiTextureUsageBitsShaderWrite)) {
        imageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

    VkFormatFeatureFlags formatValidationFlags =
        HgiVulkanConversions::GetFormatFeature(desc.usage);

    if (!_CheckFormatSupport(
            device->GetVulkanPhysicalDevice(),
            imageCreateInfo.format,
            formatValidationFlags)) {
        TF_CODING_ERROR("Image format / usage combo not supported on device");
        return;
    };

    if (imageCreateInfo.tiling != VK_IMAGE_TILING_OPTIMAL && desc.mipLevels>1) {
        TF_WARN("Linear tiled images usually do not support mips");
    }

    //
    // Create image with memory allocated and bound.
    //

    // Equivalent to: vkCreateImage, vkAllocateMemory, vkBindImageMemory
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    TF_VERIFY(
        vmaCreateImage(
            device->GetVulkanMemoryAllocator(),
            &imageCreateInfo,
            &allocInfo,
            &_vkImage,
            &_vmaImageAllocation,
            nullptr) == VK_SUCCESS
    );

    TF_VERIFY(_vkImage, "Failed to create image");

    // Debug label
    if (!_descriptor.debugName.empty()) {
        std::string debugLabel = "Image " + _descriptor.debugName;
        HgiVulkanSetDebugName(
            device,
            (uint64_t)_vkImage,
            VK_OBJECT_TYPE_IMAGE,
            debugLabel.c_str());
    }

    //
    // Create image view
    //

    // Textures are not directly accessed by the shaders and
    // are abstracted by image views containing additional
    // information and sub resource ranges
    VkImageViewCreateInfo view = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.viewType = HgiVulkanConversions::GetTextureViewType(desc.type);
    view.format = imageCreateInfo.format;
    view.components.r = 
        HgiVulkanConversions::GetComponentSwizzle(desc.componentMapping.r);
    view.components.g = 
        HgiVulkanConversions::GetComponentSwizzle(desc.componentMapping.g);
    view.components.b = 
        HgiVulkanConversions::GetComponentSwizzle(desc.componentMapping.b);
    view.components.a = 
        HgiVulkanConversions::GetComponentSwizzle(desc.componentMapping.a);

    // The subresource range describes the set of mip levels (and array layers)
    // that can be accessed through this image view.
    // It's possible to create multiple image views for a single image
    // referring to different (and/or overlapping) ranges of the image.
    // A 'view' must be either depth or stencil, not both, especially when used
    // in a descriptor set. For now we assume we always want the 'depth' aspect.
    view.subresourceRange.aspectMask = isDepthBuffer ?
        VK_IMAGE_ASPECT_DEPTH_BIT /*| VK_IMAGE_ASPECT_STENCIL_BIT*/ :
        VK_IMAGE_ASPECT_COLOR_BIT;

    view.subresourceRange.baseMipLevel = 0;
    view.subresourceRange.baseArrayLayer = 0;
    view.subresourceRange.layerCount = desc.layerCount;
    view.subresourceRange.levelCount = desc.mipLevels;
    view.image = _vkImage;

    TF_VERIFY(
        vkCreateImageView(
            device->GetVulkanDevice(),
            &view,
            HgiVulkanAllocator(),
            &_vkImageView) == VK_SUCCESS
    );

    // Debug label
    if (!_descriptor.debugName.empty()) {
        std::string debugLabel = "ImageView " + _descriptor.debugName;
        HgiVulkanSetDebugName(
            device,
            (uint64_t)_vkImageView,
            VK_OBJECT_TYPE_IMAGE_VIEW,
            debugLabel.c_str());
    }

    //
    // Upload data
    //
    if (desc.initialData && desc.pixelsByteSize > 0) {
        HgiBufferDesc stageDesc;
        stageDesc.byteSize = 
            std::min(GetByteSizeOfResource(), desc.pixelsByteSize);
        stageDesc.initialData = desc.initialData;
        HgiVulkanBuffer* stagingBuffer = 
            HgiVulkanBuffer::CreateStagingBuffer(_device, stageDesc);

        // Schedule transfer from staging buffer to device-local texture
        HgiVulkanCommandQueue* queue = device->GetCommandQueue();
        HgiVulkanCommandBuffer* cb = queue->AcquireResourceCommandBuffer();
        CopyBufferToTexture(cb, stagingBuffer);

        // We don't know if this texture is a static (immutable) or
        // dynamic (animated) texture. We assume that most textures are
        // static and schedule garbage collection of staging resource.
        HgiBufferHandle stagingHandle(stagingBuffer, 0);
        hgi->TrashObject(
            &stagingHandle,
            hgi->GetGarbageCollector()->GetBufferList());
    }

    //
    // Transition image
    //

    // Transition image to default image layout and access,flags.
    // XXX We lack information about how this texture will be used so
    // we have none-optimal assumptions for imageLayout, access and stageFlags.
    VkImageLayout layout = GetDefaultImageLayout(desc.usage);

    if (_vkImageLayout != layout) {
        HgiVulkanCommandQueue* queue = device->GetCommandQueue();
        HgiVulkanCommandBuffer* cb = queue->AcquireResourceCommandBuffer();

        TransitionImageBarrier(
            cb,
            this,
            layout,
            NO_PENDING_WRITES,
            GetDefaultAccessFlags(desc.usage),
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
    }

    _descriptor.initialData = nullptr;
}

HgiVulkanTexture::HgiVulkanTexture(
    HgiVulkan* hgi,
    HgiVulkanDevice* device,
    HgiTextureViewDesc const & desc)
    : HgiTexture(desc.sourceTexture->GetDescriptor())
    , _isTextureView(true)
    , _vkImage(nullptr)
    , _vkImageView(nullptr)
    , _vkImageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
    , _vmaImageAllocation(nullptr)
    , _device(device)
    , _inflightBits(0)
    , _stagingBuffer(nullptr)
    , _cpuStagingAddress(nullptr)
{
    // Update the texture descriptor to reflect the view desc
    _descriptor.debugName = desc.debugName;
    _descriptor.format = desc.format;
    _descriptor.layerCount = desc.layerCount;
    _descriptor.mipLevels = desc.mipLevels;

    HgiVulkanTexture* srcTexture =
        static_cast<HgiVulkanTexture*>(desc.sourceTexture.Get());
    HgiTextureDesc const& srcTexDesc = desc.sourceTexture->GetDescriptor();
    bool isDepthBuffer = srcTexDesc.usage & HgiTextureUsageBitsDepthTarget;

    _vkImage = srcTexture->GetImage();
    _vkImageLayout = srcTexture->GetImageLayout();

    VkImageViewCreateInfo view = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.viewType = HgiVulkanConversions::GetTextureViewType(srcTexDesc.type);
    view.format = HgiVulkanConversions::GetFormat(desc.format);
    view.components.r = HgiVulkanConversions::GetComponentSwizzle(
        srcTexDesc.componentMapping.r);
    view.components.g = HgiVulkanConversions::GetComponentSwizzle(
        srcTexDesc.componentMapping.g);
    view.components.b = HgiVulkanConversions::GetComponentSwizzle(
        srcTexDesc.componentMapping.b);
    view.components.a = HgiVulkanConversions::GetComponentSwizzle(
        srcTexDesc.componentMapping.a);

    view.subresourceRange.aspectMask = isDepthBuffer ?
        VK_IMAGE_ASPECT_DEPTH_BIT /*| VK_IMAGE_ASPECT_STENCIL_BIT*/ :
        VK_IMAGE_ASPECT_COLOR_BIT;

    view.subresourceRange.baseMipLevel = 0;
    view.subresourceRange.baseArrayLayer = 0;
    view.subresourceRange.layerCount = desc.layerCount;
    view.subresourceRange.levelCount = desc.mipLevels;
    view.image = srcTexture->GetImage();

    TF_VERIFY(
        vkCreateImageView(
            device->GetVulkanDevice(),
            &view,
            HgiVulkanAllocator(),
            &_vkImageView) == VK_SUCCESS
    );

    // Debug label
    if (!_descriptor.debugName.empty()) {
        std::string debugLabel = "ImageView " + _descriptor.debugName;
        HgiVulkanSetDebugName(
            device,
            (uint64_t)_vkImageView,
            VK_OBJECT_TYPE_IMAGE_VIEW,
            debugLabel.c_str());
    }
}

HgiVulkanTexture::~HgiVulkanTexture()
{
    if (_cpuStagingAddress && _stagingBuffer) {
        vmaUnmapMemory(
            _device->GetVulkanMemoryAllocator(),
            _stagingBuffer->GetVulkanMemoryAllocation());
        _cpuStagingAddress = nullptr;
    }

    delete _stagingBuffer;
    _stagingBuffer = nullptr;

    if (_vkImageView) {
        vkDestroyImageView(
            _device->GetVulkanDevice(),
            _vkImageView,
            HgiVulkanAllocator());
    }

    // This texture may be a 'TextureView' into another Texture's image.
    // In that case we do not own the image.
    if (!_isTextureView && _vkImage) {
        vmaDestroyImage(
            _device->GetVulkanMemoryAllocator(),
            _vkImage,
            _vmaImageAllocation);
    }
}

size_t
HgiVulkanTexture::GetByteSizeOfResource() const
{
    return _GetByteSizeOfResource(_descriptor);
}

uint64_t
HgiVulkanTexture::GetRawResource() const
{
    return (uint64_t) _vkImage;
}

void*
HgiVulkanTexture::GetCPUStagingAddress()
{
    if (!_stagingBuffer) {

        HgiBufferDesc desc;
        desc.byteSize = GetByteSizeOfResource();
        desc.initialData = nullptr;
        _stagingBuffer = HgiVulkanBuffer::CreateStagingBuffer(_device, desc);
    }

    if (!_cpuStagingAddress) {
        TF_VERIFY(
            vmaMapMemory(
                _device->GetVulkanMemoryAllocator(), 
                _stagingBuffer->GetVulkanMemoryAllocation(), 
                &_cpuStagingAddress) == VK_SUCCESS
        );
    }

    // This lets the client code memcpy into the staging buffer directly.
    // The staging data must be explicitely copied to the device-local
    // GPU buffer via CopyTextureCpuToGpu cmd by the client.
    return _cpuStagingAddress;
}

bool
HgiVulkanTexture::IsCPUStagingAddress(const void* address) const
{
    return (address == _cpuStagingAddress);
}

HgiVulkanBuffer*
HgiVulkanTexture::GetStagingBuffer() const
{
    return _stagingBuffer;
}

VkImage
HgiVulkanTexture::GetImage() const
{
    return _vkImage;
}

VkImageView
HgiVulkanTexture::GetImageView() const
{
    return _vkImageView;
}

VkImageLayout
HgiVulkanTexture::GetImageLayout() const
{
    return _vkImageLayout;
}

HgiVulkanDevice*
HgiVulkanTexture::GetDevice() const
{
    return _device;
}

uint64_t &
HgiVulkanTexture::GetInflightBits()
{
    return _inflightBits;
}

void
HgiVulkanTexture::CopyBufferToTexture(
    HgiVulkanCommandBuffer* cb,
    HgiVulkanBuffer* srcBuffer,
    GfVec3i const& dstTexelOffset,
    int mipLevel)
{
    // Setup buffer copy regions for each mip level

    std::vector<VkBufferImageCopy> bufferCopyRegions;

    const std::vector<HgiMipInfo> mipInfos =
        HgiGetMipInfos(
            _descriptor.format,
            _descriptor.dimensions,
            _descriptor.layerCount,
            srcBuffer->GetDescriptor().byteSize);

    const size_t mipLevels = std::min(
        mipInfos.size(), size_t(_descriptor.mipLevels));

    for (size_t mip = 0; mip < mipLevels; mip++) {
        // Skip this mip if it isn't a mipLevel we want to copy
        if (mipLevel > -1 && (int)mip != mipLevel) {
            continue;
        }

        const HgiMipInfo &mipInfo = mipInfos[mip];
        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask= VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = (uint32_t) mip;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = _descriptor.layerCount;
        bufferCopyRegion.imageExtent.width = mipInfo.dimensions[0];
        bufferCopyRegion.imageExtent.height = mipInfo.dimensions[1];
        bufferCopyRegion.imageExtent.depth = mipInfo.dimensions[2];
        bufferCopyRegion.bufferOffset = mipInfo.byteOffset;
        bufferCopyRegion.imageOffset.x = dstTexelOffset[0];
        bufferCopyRegion.imageOffset.y = dstTexelOffset[1];
        bufferCopyRegion.imageOffset.z = dstTexelOffset[2];
        bufferCopyRegions.push_back(bufferCopyRegion);
    }

    // Transition image so we can copy into it
    TransitionImageBarrier(
        cb,
        this,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Transition tex to this layout
        NO_PENDING_WRITES,                    // No pending writes
        VK_ACCESS_TRANSFER_WRITE_BIT,         // Write access to image
        VK_PIPELINE_STAGE_HOST_BIT,           // Producer stage
        VK_PIPELINE_STAGE_TRANSFER_BIT);      // Consumer stage

    // Copy pixels (all mip levels) from staging buffer to gpu image
    vkCmdCopyBufferToImage(
        cb->GetVulkanCommandBuffer(),
        srcBuffer->GetVulkanBuffer(),
        _vkImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(bufferCopyRegions.size()),
        bufferCopyRegions.data());

    // Transition image to default layout when copy is finished
    VkImageLayout layout = GetDefaultImageLayout(_descriptor.usage);
    VkAccessFlags access = GetDefaultAccessFlags(_descriptor.usage);

    TransitionImageBarrier(
        cb,
        this,
        layout,                              // Transition tex to this
        VK_ACCESS_TRANSFER_WRITE_BIT,        // Pending vkCmdCopyBufferToImage
        access,                              // Shader read access
        VK_PIPELINE_STAGE_TRANSFER_BIT,      // Producer stage
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT); // Consumer stage
}

void
HgiVulkanTexture::TransitionImageBarrier(
    HgiVulkanCommandBuffer* cb,
    HgiVulkanTexture* tex,
    VkImageLayout newLayout,
    VkAccessFlags producerAccess,
    VkAccessFlags consumerAccess,
    VkPipelineStageFlags producerStage,
    VkPipelineStageFlags consumerStage,
    int32_t mipLevel)
{
    HgiTextureDesc const& desc = tex->GetDescriptor();
    bool isDepthBuffer = desc.usage & HgiTextureUsageBitsDepthTarget;

    uint32_t firstMip = mipLevel < 0 ? 0 : (uint32_t)mipLevel;
    uint32_t mipCnt = mipLevel < 0 ? desc.mipLevels : 1;

    VkImageMemoryBarrier barrier[1] = {};
    barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier[0].srcAccessMask = producerAccess; // what producer does / changes.
    barrier[0].dstAccessMask = consumerAccess; // what consumer does / changes.
    barrier[0].oldLayout = tex->GetImageLayout();
    barrier[0].newLayout = newLayout;
    barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier[0].image = tex->GetImage();
    barrier[0].subresourceRange.aspectMask = isDepthBuffer ?
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT :
        VK_IMAGE_ASPECT_COLOR_BIT;
    barrier[0].subresourceRange.baseMipLevel = firstMip;
    barrier[0].subresourceRange.levelCount = mipCnt;
    barrier[0].subresourceRange.layerCount = desc.layerCount;

    // Insert a memory dependency at the proper pipeline stages that will
    // execute the image layout transition.

    vkCmdPipelineBarrier(
        cb->GetVulkanCommandBuffer(),
        producerStage,
        consumerStage,
        0, 0, NULL, 0, NULL, 1,
        barrier);

    tex->_vkImageLayout = newLayout;
}

VkImageLayout
HgiVulkanTexture::GetDefaultImageLayout(HgiTextureUsage usage)
{
    if (usage == 0) {
        TF_CODING_ERROR("Cannot determine image layout from invalid usage.");
    }

    if (usage & HgiTextureUsageBitsShaderWrite) {
        // Assume the ShaderWrite means its a storage image.
        return VK_IMAGE_LAYOUT_GENERAL;
    } else if (usage & HgiTextureUsageBitsShaderRead) {
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    } else if (usage & HgiTextureUsageBitsDepthTarget) {
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    } else if (usage & HgiTextureUsageBitsColorTarget) {
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VkAccessFlags
HgiVulkanTexture::GetDefaultAccessFlags(HgiTextureUsage usage)
{
    if (usage == 0) {
        TF_CODING_ERROR("Cannot determine image layout from invalid usage.");
    }

    if (usage & HgiTextureUsageBitsShaderRead) {
        return VK_ACCESS_SHADER_READ_BIT;
    } else if (usage & HgiTextureUsageBitsDepthTarget) {
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    } else if (usage & HgiTextureUsageBitsColorTarget) {
        return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    return VK_ACCESS_SHADER_READ_BIT;
}

PXR_NAMESPACE_CLOSE_SCOPE
