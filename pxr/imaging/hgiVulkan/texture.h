//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_VULKAN_TEXTURE_H
#define PXR_IMAGING_HGI_VULKAN_TEXTURE_H

#include "pxr/pxr.h"
#include "pxr/base/tf/span.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/resourceTracker.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkan;
class HgiVulkanBuffer;
class HgiVulkanCommandBuffer;
class HgiVulkanDevice;


/// \class HgiVulkanTexture
///
/// Represents a Vulkan GPU texture resource.
///
class HgiVulkanTexture final : public HgiTexture
{
public:
    static const uint32_t NO_PENDING_WRITES = 0;

    HGIVULKAN_API
    ~HgiVulkanTexture() override;

    HGIVULKAN_API
    size_t GetByteSizeOfResource() const override;

    HGIVULKAN_API
    uint64_t GetRawResource() const override;

    /// Creates (on first use) and returns the CPU staging buffer that can be
    /// used to upload new texture data to the image.
    /// After memcpy-ing new data into the returned address the client
    /// must use BlitCmds CopyTextureCpuToGpu to schedule the transfer
    /// from this staging buffer to the GPU texture.
    HGIVULKAN_API
    void* GetCPUStagingAddress();

    /// Returns true if the provided ptr matches the address of staging buffer.
    HGIVULKAN_API
    bool IsCPUStagingAddress(const void* address) const;

    /// Returns the staging buffer.
    HGIVULKAN_API
    HgiVulkanBuffer* GetStagingBuffer() const;

    /// Returns the image of the texture
    HGIVULKAN_API
    VkImage GetImage() const;

    /// Returns the image view of the texture
    HGIVULKAN_API
    VkImageView GetImageView() const;

    /// Returns the image layout of the texture
    HGIVULKAN_API
    VkImageLayout GetImageLayout() const;

    // Returns the allocation info of the texture
    HGIVULKAN_API
    VmaAllocationInfo2 GetAllocationInfo() const;

    /// Returns the device used to create this object.
    HGIVULKAN_API
    HgiVulkanDevice* GetDevice() const;

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIVULKAN_API
    uint64_t & GetInflightBits();

    /// Returns the texture state use for resource tracking.
    HGIVULKAN_API
    HgiVulkanResourceState* GetState();

    /// Schedule a copy of texels from the provided buffer into the texture.
    /// If mipLevel is less than one, all mip levels will be copied from buffer.
    HGIVULKAN_API
    void CopyBufferToTexture(
        HgiVulkanCommandBuffer* cb,
        HgiVulkanBuffer* srcBuffer,
        GfVec3i const& dstTexelOffset = GfVec3i(0),
        int mipLevel = -1);

    /// This function issues a layout change barrier. However, the layout 
    /// transition isn't immediately executed. The command buffer simply 
    /// records the request and executes when in the next submission cycle.
    HGIVULKAN_API
    HgiTextureUsage SubmitLayoutChange(HgiTextureUsage newLayout) override;

    /// Move the image out of the initial VK_IMAGE_LAYOUT_UNDEFINED into the
    /// layout inferred by GetDefaultImageLayout().
    HGIVULKAN_API
    void TransitionFromUndefined(HgiVulkanCommandBuffer* cb);

    /// Returns the layout for a texture based on its usage flags.
    HGIVULKAN_API
    static VkImageLayout GetDefaultImageLayout(HgiTextureUsage usage);

    /// Returns the access flags for a texture based on its usage flags.
    HGIVULKAN_API
    static VkAccessFlags GetDefaultAccessFlags(HgiTextureUsage usage);

protected:
    friend class HgiVulkan;

    HGIVULKAN_API
    HgiVulkanTexture(
        HgiVulkan* hgi,
        HgiTextureDesc const & desc,
        bool optimalTiling,
        bool interop);

    // Texture view constructor to alias another texture's data.
    HGIVULKAN_API
    HgiVulkanTexture(
        HgiVulkan* hgi,
        HgiTextureViewDesc const & desc);

private:
    HgiVulkanTexture() = delete;
    HgiVulkanTexture & operator=(const HgiVulkanTexture&) = delete;
    HgiVulkanTexture(const HgiVulkanTexture&) = delete;

    void CopyMemoryToTexture(
        TfSpan<const std::byte> srcBuffer,
        GfVec3i const& dstTexelOffset = GfVec3i(0),
        int mipLevel = -1);

    VkImage _vkImage;
    VkImageView _vkImageView;
    VmaAllocation _vmaImageAllocation;
    HgiVulkan* _hgi;
    uint64_t _inflightBits;
    std::shared_ptr<HgiVulkanResourceState> _state;
    std::unique_ptr<HgiVulkanBuffer> _stagingBuffer;
    void* _cpuStagingAddress;
    bool _hasHostImageCopy;
    bool _isTextureView;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
