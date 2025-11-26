//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

static HgiTextureUsage
_VkImageLayoutToHgiTextureUsage(VkImageLayout usage);

HgiVulkanTexture::HgiVulkanTexture(
    HgiVulkan* hgi,
    HgiTextureDesc const & desc,
    bool optimalTiling,
    bool interop)
    : HgiTexture(desc)
    , _vkImage(nullptr)
    , _vkImageView(nullptr)
    , _vkImageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
    , _vmaImageAllocation(nullptr)
    , _hgi(hgi)
    , _inflightBits(0)
    , _stagingBuffer(nullptr)
    , _cpuStagingAddress(nullptr)
    , _hasHostImageCopy(false)
    , _isTextureView(false)
{
    HgiVulkanDevice* device = hgi->GetPrimaryDevice();
    const bool hostImageCopyDesired = !interop;
    HgiVulkanFormatInfo formatInfo = hgi->GetCapabilities()->GetFormatInfo(
        device, desc.type, desc.format, desc.usage,
            optimalTiling, hostImageCopyDesired);

    _hasHostImageCopy = formatInfo.hostImageCopyOptimal;

    VkImageCreateInfo imageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageCreateInfo.imageType = formatInfo.type;
    imageCreateInfo.format = formatInfo.format;
    imageCreateInfo.mipLevels = desc.mipLevels;
    imageCreateInfo.arrayLayers = desc.layerCount;
    imageCreateInfo.flags = formatInfo.createFlags;
    imageCreateInfo.usage = formatInfo.usage;
    imageCreateInfo.samples = 
        HgiVulkanConversions::GetSampleCount(desc.sampleCount);
    imageCreateInfo.tiling = optimalTiling ?
        VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = { static_cast<uint32_t>(desc.dimensions[0]),
                               static_cast<uint32_t>(desc.dimensions[1]),
                               static_cast<uint32_t>(desc.dimensions[2]) };

    VkExternalMemoryImageCreateInfo exportInfo =
        { VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO };
    exportInfo.pNext = nullptr;
    exportInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_AUTO;
    if (interop) {
        exportInfo.pNext = imageCreateInfo.pNext;
        imageCreateInfo.pNext = &exportInfo;
    }

    //
    // Create image with memory allocated and bound.
    //

    // Equivalent to: vkCreateImage, vkAllocateMemory, vkBindImageMemory
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.pool = interop ?
        device->GetVMAPoolForInterop(imageCreateInfo) : VK_NULL_HANDLE;
    HGIVULKAN_VERIFY_VK_RESULT(
        vmaCreateImage(
            device->GetVulkanMemoryAllocator(),
            &imageCreateInfo,
            &allocInfo,
            &_vkImage,
            &_vmaImageAllocation,
            nullptr)
    );

    TF_VERIFY(_vkImage, "Failed to create image");

    // Debug label
    if (!desc.debugName.empty()) {
        std::string debugLabel = "Image " + desc.debugName;
        HgiVulkanSetDebugName(
            device,
            static_cast<uint64_t>(reinterpret_cast<uintptr_t>(_vkImage)),
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
    if (desc.usage & HgiTextureUsageBitsDepthTarget) {
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (desc.usage & HgiTextureUsageBitsStencilTarget) {
            view.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    view.subresourceRange.baseMipLevel = 0;
    view.subresourceRange.baseArrayLayer = 0;
    view.subresourceRange.layerCount = desc.layerCount;
    view.subresourceRange.levelCount = desc.mipLevels;
    view.image = _vkImage;

    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateImageView(
            device->GetVulkanDevice(),
            &view,
            HgiVulkanAllocator(),
            &_vkImageView)
    );

    // Debug label
    if (!desc.debugName.empty()) {
        std::string debugLabel = "ImageView " + desc.debugName;
        HgiVulkanSetDebugName(
            device,
            static_cast<uint64_t>(
                reinterpret_cast<uintptr_t>(_vkImageView)),
            VK_OBJECT_TYPE_IMAGE_VIEW,
            debugLabel.c_str());
    }

    //
    // Upload data & transition image
    //
    const VkImageLayout newLayout = GetDefaultImageLayout(desc.usage);
    if (_hasHostImageCopy
        && hgi->GetCapabilities()->SupportsMemoryToTextureCopy(newLayout)) {

        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask =
            HgiVulkanConversions::GetImageAspectFlag(desc.usage);
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        VkHostImageLayoutTransitionInfoEXT hostImageLayoutTransitionInfo{
            VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO_EXT};
        hostImageLayoutTransitionInfo.image = _vkImage;
        hostImageLayoutTransitionInfo.oldLayout = _vkImageLayout;
        hostImageLayoutTransitionInfo.newLayout = newLayout;
        hostImageLayoutTransitionInfo.subresourceRange = subresourceRange;

        // With VK_EXT_host_image_copy we transition to the final desired layout
        // first, then we do the copy. No need to go through a transfer layout.
        HGIVULKAN_VERIFY_VK_RESULT(device->vkTransitionImageLayoutEXT(
            device->GetVulkanDevice(), 1, &hostImageLayoutTransitionInfo));
        _vkImageLayout = newLayout;

        if (desc.initialData && desc.pixelsByteSize > 0) {
            CopyMemoryToTexture(
                { static_cast<const std::byte*>(desc.initialData),
                    desc.pixelsByteSize });
        }
    } else {
        HgiVulkanCommandQueue* queue = device->GetCommandQueue();
        HgiVulkanCommandBuffer* cb = queue->AcquireResourceCommandBuffer();
        if (desc.initialData && desc.pixelsByteSize > 0) {
            HgiBufferDesc stageDesc;
            stageDesc.usage = HgiBufferUsageUpload;
            stageDesc.byteSize =
                std::min(GetByteSizeOfResource(), desc.pixelsByteSize);
            stageDesc.initialData = desc.initialData;
            std::unique_ptr<HgiVulkanBuffer> stagingBuffer =
                HgiVulkanBuffer::CreateStagingBuffer(hgi, stageDesc);

            // Schedule transfer from staging buffer to device-local texture.
            // This will also do the necessary final desired layout transitions.
            CopyBufferToTexture(cb, stagingBuffer.get());

            // We don't know if this texture is a static (immutable) or
            // dynamic (animated) texture. We assume that most textures are
            // static and schedule garbage collection of staging resource.
            HgiBufferHandle stagingHandle(stagingBuffer.release(), 0);
            hgi->TrashObject(
                &stagingHandle,
                hgi->GetGarbageCollector()->GetBufferList());
        } else {
            // Just transition to the final desired layout.
            LayoutBarrier(
                cb,
                _vkImageLayout,
                newLayout,
                NO_PENDING_WRITES,
                GetDefaultAccessFlags(desc.usage),
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        }
    }

    _descriptor.initialData = nullptr;
}

HgiVulkanTexture::HgiVulkanTexture(
    HgiVulkan* hgi,
    HgiTextureViewDesc const & desc)
    : HgiTexture(desc.sourceTexture->GetDescriptor())
    , _vkImage(nullptr)
    , _vkImageView(nullptr)
    , _vkImageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
    , _vmaImageAllocation(nullptr)
    , _hgi(hgi)
    , _inflightBits(0)
    , _stagingBuffer(nullptr)
    , _cpuStagingAddress(nullptr)
    , _hasHostImageCopy(false)
    , _isTextureView(true)
{
    HgiVulkanDevice* device = hgi->GetPrimaryDevice();
    // Update the texture descriptor to reflect the view desc
    _descriptor.debugName = desc.debugName;
    _descriptor.format = desc.format;
    _descriptor.layerCount = desc.layerCount;
    _descriptor.mipLevels = desc.mipLevels;

    HgiVulkanTexture* srcTexture =
        static_cast<HgiVulkanTexture*>(desc.sourceTexture.Get());
    HgiTextureDesc const& srcTexDesc = desc.sourceTexture->GetDescriptor();
    bool const isDepthBuffer = 
        srcTexDesc.usage & HgiTextureUsageBitsDepthTarget;

    _vkImage = srcTexture->GetImage();
    _vkImageLayout = srcTexture->GetImageLayout();

    VkImageViewCreateInfo view = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.viewType = HgiVulkanConversions::GetTextureViewType(srcTexDesc.type);
    view.format = HgiVulkanConversions::GetFormat(desc.format, isDepthBuffer);
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

    view.subresourceRange.baseMipLevel = desc.sourceFirstMip;
    view.subresourceRange.baseArrayLayer = desc.sourceFirstLayer;
    view.subresourceRange.layerCount = desc.layerCount;
    view.subresourceRange.levelCount = desc.mipLevels;
    view.image = srcTexture->GetImage();

    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateImageView(
            device->GetVulkanDevice(),
            &view,
            HgiVulkanAllocator(),
            &_vkImageView)
    );

    // Debug label
    if (!_descriptor.debugName.empty()) {
        std::string debugLabel = "ImageView " + _descriptor.debugName;
        HgiVulkanSetDebugName(
            device,
            static_cast<uint64_t>(reinterpret_cast<uintptr_t>(_vkImageView)),
            VK_OBJECT_TYPE_IMAGE_VIEW,
            debugLabel.c_str());
    }
}

HgiVulkanTexture::~HgiVulkanTexture()
{
    HgiVulkanDevice* device = _hgi->GetPrimaryDevice();
    if (_cpuStagingAddress && _stagingBuffer) {
        vmaUnmapMemory(
            device->GetVulkanMemoryAllocator(),
            _stagingBuffer->GetVulkanMemoryAllocation());
        _cpuStagingAddress = nullptr;
    }

    _stagingBuffer = nullptr;

    if (_vkImageView) {
        vkDestroyImageView(
            device->GetVulkanDevice(),
            _vkImageView,
            HgiVulkanAllocator());
    }

    // This texture may be a 'TextureView' into another Texture's image.
    // In that case we do not own the image.
    if (!_isTextureView && _vkImage) {
        vmaDestroyImage(
            device->GetVulkanMemoryAllocator(),
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
        desc.usage = HgiBufferUsageUpload;
        desc.byteSize = GetByteSizeOfResource();
        desc.initialData = nullptr;
        desc.debugName = "Staging Buffer for " + _descriptor.debugName;
        _stagingBuffer = HgiVulkanBuffer::CreateStagingBuffer(_hgi, desc);
    }

    if (!_cpuStagingAddress) {
        HGIVULKAN_VERIFY_VK_RESULT(
            vmaMapMemory(
                _hgi->GetPrimaryDevice()->GetVulkanMemoryAllocator(), 
                _stagingBuffer->GetVulkanMemoryAllocation(), 
                &_cpuStagingAddress)
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
    return _stagingBuffer.get();
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

VmaAllocationInfo2
HgiVulkanTexture::GetAllocationInfo() const
{
    VmaAllocationInfo2 info;
    vmaGetAllocationInfo2(
        _hgi->GetPrimaryDevice()->GetVulkanMemoryAllocator(),
        _vmaImageAllocation,
        &info);
    return info;
}

HgiVulkanDevice*
HgiVulkanTexture::GetDevice() const
{
    return _hgi->GetPrimaryDevice();
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
    const std::vector<HgiMipInfo> mipInfos =
        HgiGetMipInfos(
            _descriptor.format,
            _descriptor.dimensions,
            _descriptor.layerCount,
            srcBuffer->GetDescriptor().byteSize);

    const uint32_t baseMip = mipLevel > -1 ? mipLevel : 0;
    const uint32_t mipLevels = mipLevel > -1 ?
        1 : std::min(static_cast<int>(mipInfos.size()),
            static_cast<int>(_descriptor.mipLevels));

    std::vector<VkBufferImageCopy> bufferCopyRegions;
    bufferCopyRegions.reserve(mipLevels);
    for (uint32_t mip = baseMip; mip < (baseMip + mipLevels); mip++) {
        const HgiMipInfo &mipInfo = mipInfos[mip];
        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask =
            HgiVulkanConversions::GetImageAspectFlag(_descriptor.usage);
        bufferCopyRegion.imageSubresource.mipLevel = mip;
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
    LayoutBarrier(
        cb,
        GetImageLayout(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        NO_PENDING_WRITES,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    // Copy pixels from staging buffer to gpu image
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

    LayoutBarrier(
        cb,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        layout,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        access,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
}

void
HgiVulkanTexture::CopyMemoryToTexture(
    TfSpan<const std::byte> srcBuffer,
    GfVec3i const& dstTexelOffset,
    int mipLevel)
{
    // Setup buffer copy regions for each mip level
    const std::vector<HgiMipInfo> mipInfos =
        HgiGetMipInfos(
            _descriptor.format,
            _descriptor.dimensions,
            _descriptor.layerCount,
            srcBuffer.size());

    const uint32_t baseMip = mipLevel > -1 ? mipLevel : 0;
    const uint32_t mipLevels = mipLevel > -1 ?
        1 : std::min(static_cast<int>(mipInfos.size()),
            static_cast<int>(_descriptor.mipLevels));

    std::vector<VkMemoryToImageCopyEXT> bufferCopyRegions;
    for (uint32_t mip = baseMip; mip < (baseMip + mipLevels); mip++) {
        const HgiMipInfo &mipInfo = mipInfos[mip];
        VkMemoryToImageCopyEXT bufferCopyRegion{
            VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY_EXT};
        bufferCopyRegion.imageSubresource.aspectMask =
            HgiVulkanConversions::GetImageAspectFlag(_descriptor.usage);
        bufferCopyRegion.imageSubresource.mipLevel = mip;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = _descriptor.layerCount;
        bufferCopyRegion.imageExtent.width = mipInfo.dimensions[0];
        bufferCopyRegion.imageExtent.height = mipInfo.dimensions[1];
        bufferCopyRegion.imageExtent.depth = mipInfo.dimensions[2];
        bufferCopyRegion.pHostPointer = srcBuffer.data() + mipInfo.byteOffset;
        bufferCopyRegion.imageOffset.x = dstTexelOffset[0];
        bufferCopyRegion.imageOffset.y = dstTexelOffset[1];
        bufferCopyRegion.imageOffset.z = dstTexelOffset[2];
        bufferCopyRegions.push_back(bufferCopyRegion);
    }

    VkCopyMemoryToImageInfoEXT copyMemoryToImageInfo{
        VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO_EXT};
    copyMemoryToImageInfo.dstImage = _vkImage;
    copyMemoryToImageInfo.dstImageLayout = _vkImageLayout;
    copyMemoryToImageInfo.regionCount =
        static_cast<uint32_t>(bufferCopyRegions.size());
    copyMemoryToImageInfo.pRegions = bufferCopyRegions.data();

    // Immediately copy pixels from memory directly to gpu image
    HgiVulkanDevice* device = _hgi->GetPrimaryDevice();
    HGIVULKAN_VERIFY_VK_RESULT(
        device->vkCopyMemoryToImageEXT(
            device->GetVulkanDevice(), &copyMemoryToImageInfo));
}

HgiTextureUsage
HgiVulkanTexture::SubmitLayoutChange(HgiTextureUsage newLayout)
{
    const VkImageLayout oldVkLayout = GetImageLayout();
    const VkImageLayout newVkLayout =
        HgiVulkanTexture::GetDefaultImageLayout(newLayout);

    if (oldVkLayout == newVkLayout) {
        return _VkImageLayoutToHgiTextureUsage(oldVkLayout);
    }

    HgiVulkanCommandQueue* queue = _hgi->GetPrimaryDevice()->GetCommandQueue();
    HgiVulkanCommandBuffer* cb = queue->AcquireResourceCommandBuffer();

    // The following cases are based on few initial assumptions to provide
    // an infrastructure for access mask selection based on layouts.
    // Feel free to update depending on need and use cases.
    VkAccessFlags srcAccessMask = VK_ACCESS_NONE;
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    switch (oldVkLayout) {
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        srcAccessMask = VK_ACCESS_HOST_WRITE_BIT |
            VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;
    }

    VkAccessFlags dstAccessMask = VK_ACCESS_NONE;
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    switch (newVkLayout) {
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;
    }

    LayoutBarrier(
        cb,
        oldVkLayout,
        newVkLayout,
        srcAccessMask,
        dstAccessMask,
        srcStageMask,
        dstStageMask);

    return _VkImageLayoutToHgiTextureUsage(oldVkLayout);
}

void
HgiVulkanTexture::LayoutBarrier(
    HgiVulkanCommandBuffer* cb,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags producerAccess,
    VkAccessFlags consumerAccess,
    VkPipelineStageFlags producerStage,
    VkPipelineStageFlags consumerStage,
    int32_t mipLevel)
{
    HgiTextureDesc const& desc = GetDescriptor();

    const uint32_t firstMip = mipLevel < 0 ? 0 : mipLevel;
    const uint32_t mipCount = mipLevel < 0 ? VK_REMAINING_MIP_LEVELS : 1u;

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = producerAccess;
    barrier.dstAccessMask = consumerAccess;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = GetImage();
    barrier.subresourceRange.aspectMask =
        HgiVulkanConversions::GetImageAspectFlag(desc.usage);
    barrier.subresourceRange.baseMipLevel = firstMip;
    barrier.subresourceRange.levelCount = mipCount;
    barrier.subresourceRange.layerCount = desc.layerCount;

    // Insert a memory dependency at the proper pipeline stages that will
    // execute the image layout transition.

    vkCmdPipelineBarrier(
        cb->GetVulkanCommandBuffer(),
        producerStage,
        consumerStage,
        0,
        0, NULL,
        0, NULL,
        1, &barrier);

    _vkImageLayout = newLayout;
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
    // Prioritize attachment layouts over shader read layout. Some textures 
    // might have both usages.
    } else if (usage & HgiTextureUsageBitsColorTarget) {
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    } else if (usage & HgiTextureUsageBitsDepthTarget ||
               usage & HgiTextureUsageBitsStencilTarget) {
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    } else if (usage & HgiTextureUsageBitsShaderRead) {
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VkAccessFlags
HgiVulkanTexture::GetDefaultAccessFlags(HgiTextureUsage usage)
{
    if (usage == 0) {
        TF_CODING_ERROR("Cannot determine image layout from invalid usage.");
    }

    VkAccessFlags flags = VK_ACCESS_NONE;
    if (usage & HgiTextureUsageBitsShaderRead) {
        flags |= VK_ACCESS_SHADER_READ_BIT;
    }
    if (usage & HgiTextureUsageBitsDepthTarget) {
        flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    } else if (usage & HgiTextureUsageBitsColorTarget) {
        flags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
   
    return flags;
}

static HgiTextureUsage
_VkImageLayoutToHgiTextureUsage(VkImageLayout usage)
{
    switch (usage) {
    case VK_IMAGE_LAYOUT_GENERAL:
        return HgiTextureUsageBitsShaderWrite;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return HgiTextureUsageBitsColorTarget;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        return HgiTextureUsageBitsDepthTarget;
    case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
        return HgiTextureUsageBitsStencilTarget;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return HgiTextureUsageBitsDepthTarget |
            HgiTextureUsageBitsStencilTarget;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return HgiTextureUsageBitsShaderRead;
    default:
        TF_CODING_ERROR("Unsupported VkImageLayout %d", usage);
        return 0;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
