//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_BUFFER_H
#define PXR_IMAGING_HGIVULKAN_BUFFER_H

#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkan;
class HgiVulkanCommandBuffer;
class HgiVulkanDevice;

///
/// \class HgiVulkanBuffer
///
/// Vulkan implementation of HgiBuffer
///
class HgiVulkanBuffer final : public HgiBuffer
{
public:
    HGIVULKAN_API
    ~HgiVulkanBuffer() override;

    HGIVULKAN_API
    size_t GetByteSizeOfResource() const override;

    HGIVULKAN_API
    uint64_t GetRawResource() const override;

    HGIVULKAN_API
    void* GetCPUStagingAddress() override;

    /// Returns true if the provided ptr matches the address of staging buffer.
    HGIVULKAN_API
    bool IsCPUStagingAddress(const void* address) const;

    /// Returns the vulkan buffer.
    HGIVULKAN_API
    VkBuffer GetVulkanBuffer() const;

    /// Returns the memory allocation
    HGIVULKAN_API
    VmaAllocation GetVulkanMemoryAllocation() const;

    /// Returns the staging buffer.
    HGIVULKAN_API
    HgiVulkanBuffer* GetStagingBuffer() const;

    /// Returns the device used to create this object.
    HGIVULKAN_API
    HgiVulkanDevice* GetDevice() const;

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIVULKAN_API
    uint64_t & GetInflightBits();

    /// Creates a staging buffer.
    /// The caller is responsible for the lifetime (destruction) of the buffer.
    HGIVULKAN_API
    static HgiVulkanBuffer* CreateStagingBuffer(
        HgiVulkanDevice* device,
        HgiBufferDesc const& desc);

protected:
    friend class HgiVulkan;

    // Constructor for making buffers
    HGIVULKAN_API
    HgiVulkanBuffer(
        HgiVulkan* hgi,
        HgiVulkanDevice* device,
        HgiBufferDesc const& desc);

    // Constructor for making staging buffers
    HGIVULKAN_API
    HgiVulkanBuffer(
        HgiVulkanDevice* device,
        VkBuffer vkBuffer,
        VmaAllocation vmaAllocation,
        HgiBufferDesc const& desc);

private:
    HgiVulkanBuffer() = delete;
    HgiVulkanBuffer & operator=(const HgiVulkanBuffer&) = delete;
    HgiVulkanBuffer(const HgiVulkanBuffer&) = delete;

    HgiVulkanDevice* _device;
    VkBuffer _vkBuffer;
    VmaAllocation _vmaAllocation;
    uint64_t _inflightBits;
    HgiVulkanBuffer* _stagingBuffer;
    void* _cpuStagingAddress;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
