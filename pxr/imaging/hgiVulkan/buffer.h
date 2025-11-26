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
/// \struct HgiVulkanMappedBufferUniquePointerDeleter
///
/// For use with std::unique_ptr. Unmaps a pointer to host visible memory when
/// the owning pointer is destroyed.
///
struct HgiVulkanMappedBufferUniquePointerDeleter
{
    void operator()([[maybe_unused]] void* memory) const
    {
        vmaUnmapMemory(_vma, _allocation);
    }

    HgiVulkanMappedBufferUniquePointerDeleter() = default;

    HgiVulkanMappedBufferUniquePointerDeleter(VmaAllocator vma,
        VmaAllocation allocation)
        : _vma(vma)
        , _allocation(allocation)
    {
    }

private:
    VmaAllocator _vma;
    VmaAllocation _allocation;
};

using HgiVulkanMappedBufferUniquePointer =
    std::unique_ptr<void, HgiVulkanMappedBufferUniquePointerDeleter>;

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
    static std::unique_ptr<HgiVulkanBuffer> CreateStagingBuffer(
        HgiVulkan* hgi,
        HgiBufferDesc const& desc);

    /// Returns a device local, host writeable pointer to the buffer allocation.
    /// Writing sequentially to this pointer should be the fastest way to write
    /// to device memory.
    /// This should only be called on buffers with usage HgiBufferUsageUpload
    /// or on UMA/ReBAR enabled systems.
    HGIVULKAN_API
    HgiVulkanMappedBufferUniquePointer Map() const;

protected:
    friend class HgiVulkan;

    // Constructor for making buffers
    HGIVULKAN_API
    HgiVulkanBuffer(
        HgiVulkan* hgi,
        HgiBufferDesc const& desc);

private:
    HgiVulkanBuffer() = delete;
    HgiVulkanBuffer & operator=(const HgiVulkanBuffer&) = delete;
    HgiVulkanBuffer(const HgiVulkanBuffer&) = delete;

    HgiVulkan* _hgi;
    VkBuffer _vkBuffer;
    VmaAllocation _vmaAllocation;
    uint64_t _inflightBits;
    std::unique_ptr<HgiVulkanBuffer> _stagingBuffer;
    HgiVulkanMappedBufferUniquePointer _cpuStagingAddress;
    bool _mappable;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
