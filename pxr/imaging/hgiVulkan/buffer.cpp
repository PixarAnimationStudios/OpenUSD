//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/commandBuffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/garbageCollector.h"
#include "pxr/imaging/hgiVulkan/hgi.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiVulkanBuffer::HgiVulkanBuffer(
    HgiVulkan* hgi,
    HgiBufferDesc const& desc)
    : HgiBuffer(desc)
    , _hgi(hgi)
    , _vkBuffer(nullptr)
    , _vmaAllocation(nullptr)
    , _inflightBits(0)
    , _cpuStagingAddress(nullptr)
    , _mappable(false)
{
    if (_descriptor.byteSize == 0) {
        TF_CODING_ERROR("The size of buffer [%p] is zero.", this);
        return;
    }
    HgiVulkanDevice* device = hgi->GetPrimaryDevice();
    VmaAllocator vma = device->GetVulkanMemoryAllocator();

    VkBufferCreateInfo bi = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = _descriptor.byteSize;
    bi.usage = HgiVulkanConversions::GetBufferUsage(_descriptor.usage);
    bi.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // gfx queue only

    // Create buffer with memory allocated and bound.
    // Equivalent to: vkCreateBuffer, vkAllocateMemory, vkBindBufferMemory
    const bool isUploadBuffer = _descriptor.usage & HgiBufferUsageUpload;
    VmaAllocationCreateInfo ai = {};
    if (isUploadBuffer) {
        ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        ai.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        ai.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    } else {
        ai.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        if (_descriptor.initialData) {
            // This flag combination allows us to avoid staging copies on device
            // memory if the driver signals that this wouldn't hurt performance.
            ai.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
        }
    }

    const bool isUMA = hgi->GetCapabilities()->
        IsSet(HgiDeviceCapabilitiesBitsUnifiedMemory);
    if (isUMA) {
        ai.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        ai.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    _mappable = isUploadBuffer | isUMA;

    HGIVULKAN_VERIFY_VK_RESULT(
        vmaCreateBuffer(vma, &bi, &ai, &_vkBuffer, &_vmaAllocation, 0));

    // Debug label
    if (!_descriptor.debugName.empty()) {
        HgiVulkanSetDebugName(
            device,
            (uint64_t)_vkBuffer,
            VK_OBJECT_TYPE_BUFFER,
            _descriptor.debugName.c_str());
    }

    if (_descriptor.initialData) {
        VkMemoryPropertyFlags allocatedFlags;
        vmaGetAllocationMemoryProperties(vma, _vmaAllocation, &allocatedFlags);
        if (allocatedFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            HGIVULKAN_VERIFY_VK_RESULT(
                vmaCopyMemoryToAllocation(vma, _descriptor.initialData,
                    _vmaAllocation, 0, _descriptor.byteSize));
        } else {
            // Use a 'staging buffer' to schedule uploading the 'initialData' to
            // the device-local GPU buffer.
            HgiBufferDesc stagingDesc = _descriptor;
            stagingDesc.usage = HgiBufferUsageUpload;
            if (!stagingDesc.debugName.empty()) {
                stagingDesc.debugName =
                    "Staging Buffer for " + stagingDesc.debugName;
            }

            auto stagingBuffer =  CreateStagingBuffer(_hgi, stagingDesc);
            VkBuffer vkStagingBuf = stagingBuffer->GetVulkanBuffer();

            HgiVulkanCommandQueue* queue = device->GetCommandQueue();
            HgiVulkanCommandBuffer* cb = queue->AcquireResourceCommandBuffer();
            VkCommandBuffer vkCmdBuf = cb->GetVulkanCommandBuffer();

            // Copy data from staging buffer to device-local buffer.
            VkBufferCopy copyRegion = {};
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = 0;
            copyRegion.size = stagingDesc.byteSize;
            vkCmdCopyBuffer(vkCmdBuf, vkStagingBuf, _vkBuffer, 1, &copyRegion);

            VkBufferMemoryBarrier memoryBarrier {
                VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
            memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
            memoryBarrier.dstAccessMask =
                VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            memoryBarrier.buffer = _vkBuffer;
            memoryBarrier.offset = 0;
            memoryBarrier.size = stagingDesc.byteSize;
            vkCmdPipelineBarrier(
                vkCmdBuf,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0,
                0, nullptr,
                1, &memoryBarrier,
                0, nullptr);

            // We don't know if this buffer is a static (immutable) or
            // dynamic (animated) buffer. We assume that most buffers are
            // static and schedule garbage collection of staging resource.
            HgiBufferHandle stagingHandle(stagingBuffer.release(), 0);
            hgi->TrashObject(
                &stagingHandle,
                hgi->GetGarbageCollector()->GetBufferList());
        }
    }

    _descriptor.initialData = nullptr;
}

HgiVulkanBuffer::~HgiVulkanBuffer()
{
    _cpuStagingAddress = nullptr;
    _stagingBuffer = nullptr;

    vmaDestroyBuffer(
        _hgi->GetPrimaryDevice()->GetVulkanMemoryAllocator(),
        _vkBuffer,
        _vmaAllocation);
}

size_t
HgiVulkanBuffer::GetByteSizeOfResource() const
{
    return _descriptor.byteSize;
}

uint64_t
HgiVulkanBuffer::GetRawResource() const
{
    return (uint64_t) _vkBuffer;
}

void*
HgiVulkanBuffer::GetCPUStagingAddress()
{
    if (!_cpuStagingAddress) {
        if (_mappable) {
            _cpuStagingAddress = Map();
        } else {
            HgiBufferDesc stagingDesc = _descriptor;
            stagingDesc.usage = HgiBufferUsageUpload;
            stagingDesc.debugName = "Staging Buffer for: " + 
                (stagingDesc.debugName.empty() ?
                    "Unknown" : stagingDesc.debugName);
            stagingDesc.initialData = nullptr;
            
            _stagingBuffer = CreateStagingBuffer(_hgi, stagingDesc);
            _cpuStagingAddress = _stagingBuffer->Map();
        }
    }
    return _cpuStagingAddress.get();
}

bool
HgiVulkanBuffer::IsCPUStagingAddress(const void* address) const
{
    return address == _cpuStagingAddress.get();
}

VkBuffer
HgiVulkanBuffer::GetVulkanBuffer() const
{
    return _vkBuffer;
}

VmaAllocation
HgiVulkanBuffer::GetVulkanMemoryAllocation() const
{
    return _vmaAllocation;
}

HgiVulkanBuffer*
HgiVulkanBuffer::GetStagingBuffer() const
{
    return _stagingBuffer.get();
}

HgiVulkanDevice*
HgiVulkanBuffer::GetDevice() const
{
    return _hgi->GetPrimaryDevice();
}

uint64_t &
HgiVulkanBuffer::GetInflightBits()
{
    return _inflightBits;
}

HgiVulkanMappedBufferUniquePointer
HgiVulkanBuffer::Map() const
{
    TF_VERIFY(_mappable);
    VmaAllocator vma = _hgi->GetPrimaryDevice()->GetVulkanMemoryAllocator();
    void* memory = nullptr;
    HGIVULKAN_VERIFY_VK_RESULT(vmaMapMemory(vma, _vmaAllocation, &memory));
    return HgiVulkanMappedBufferUniquePointer(memory, {vma, _vmaAllocation});
}

std::unique_ptr<HgiVulkanBuffer>
HgiVulkanBuffer::CreateStagingBuffer(
    HgiVulkan* hgi,
    HgiBufferDesc const& desc)
{
    TF_VERIFY(desc.usage & HgiBufferUsageUpload);
    return std::unique_ptr<HgiVulkanBuffer>(
        new HgiVulkanBuffer(hgi, desc));
}

PXR_NAMESPACE_CLOSE_SCOPE
