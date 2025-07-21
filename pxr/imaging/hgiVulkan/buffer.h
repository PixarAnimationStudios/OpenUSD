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
/// \struct HgiVulkanUmaUniquePointerDeleter
///
/// For use with std::unique_ptr. Unmaps a pointer to host visible memory when
/// the owning pointer is destroyed.
///
struct HgiVulkanMappedMemoryUniquePointerDeleter
{
    void operator()([[maybe_unused]] void* memory) const
    {
        vmaUnmapMemory(_vma, _allocation);
    }

    HgiVulkanMappedMemoryUniquePointerDeleter() = default;

    HgiVulkanMappedMemoryUniquePointerDeleter(VmaAllocator vma,
        VmaAllocation allocation)
        : _vma(vma)
        , _allocation(allocation)
    {
    }

private:
    VmaAllocator _vma{};
    VmaAllocation _allocation{};
};

using HgiVulkanMappedMemoryUniquePointer =
    std::unique_ptr<void, HgiVulkanMappedMemoryUniquePointerDeleter>;

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
    HgiVulkanBuffer* GetStagingBuffer();

    /// Returns the device used to create this object.
    HGIVULKAN_API
    HgiVulkanDevice* GetDevice() const;

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIVULKAN_API
    uint64_t & GetInflightBits();

    /// Returns a device local, host writeable pointer to the buffer allocaiton,
    /// if UMA or equivalent like ReBAR is available. Returns null otherwise.
    /// Writing sequentially to this pointer should be the fastest way to write
    /// to device memory.
    HGIVULKAN_API
    HgiVulkanMappedMemoryUniquePointer GetUmaPointer() const;

    /// Creates a staging buffer.
    /// The caller is responsible for the lifetime (destruction) of the buffer.
    HGIVULKAN_API
    static std::unique_ptr<HgiVulkanBuffer> CreateStagingBuffer(
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
    std::unique_ptr<HgiVulkanBuffer> _stagingBuffer;
    HgiVulkanMappedMemoryUniquePointer _cpuStagingAddress;
    bool _isUma;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
