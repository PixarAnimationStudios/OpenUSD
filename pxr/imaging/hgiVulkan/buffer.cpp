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
    if (!_descriptor.debugName.empty() && HgiVulkanIsDebugEnabled()) {
        HgiVulkanSetDebugName(
            device,
            (uint64_t)_vkBuffer,
            VK_OBJECT_TYPE_BUFFER,
            _descriptor.debugName.c_str());

        vmaSetAllocationName(device->GetVulkanMemoryAllocator(),
            _vmaAllocation, _descriptor.debugName.c_str());
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

            VkBufferMemoryBarrier memoryBarrier =
                 GetBarrier(VK_ACCESS_MEMORY_WRITE_BIT,
                    VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);

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

HgiVulkanBuffer::HgiVulkanBuffer(
    HgiVulkan* hgi,
    VkBuffer existingBuffer,
    size_t byteSize,
    HgiBufferUsage usage)
    : HgiBuffer(HgiBufferDesc())
    , _hgi(hgi)
    , _vkBuffer(existingBuffer)
    , _vmaAllocation(nullptr)
    , _inflightBits(0)
    , _cpuStagingAddress(nullptr)
    , _mappable(false)
    , _isExternal(true)
{
    // Non-owning wrapper around a buffer the producer owns. No allocation of
    // our own; the descriptor is just enough for binding queries.
    _descriptor.byteSize = byteSize;
    _descriptor.usage = usage;
}

HgiVulkanBuffer::HgiVulkanBuffer(
    HgiVulkan* hgi,
    HgiBufferDesc const& desc,
    bool /*interop*/)
    : HgiBuffer(desc)
    , _hgi(hgi)
    , _vkBuffer(nullptr)
    , _vmaAllocation(nullptr)
    , _inflightBits(0)
    , _cpuStagingAddress(nullptr)
    , _mappable(false)
    , _isExternal(false)          // owning: we allocated the memory
{
    HgiVulkanDevice* device = hgi->GetPrimaryDevice();
    VmaAllocator vma = device->GetVulkanMemoryAllocator();

    VkBufferCreateInfo bi = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = _descriptor.byteSize;
    bi.usage = HgiVulkanConversions::GetBufferUsage(_descriptor.usage);
    bi.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Make the memory exportable so another GPU API can import and alias it.
    VkExternalMemoryBufferCreateInfo externalInfo =
        { VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO };
    externalInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_AUTO;
    externalInfo.pNext = bi.pNext;
    bi.pNext = &externalInfo;

    VmaAllocationCreateInfo ai = {};
    ai.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    ai.pool = device->GetVMAPoolForInterop(bi);

    HGIVULKAN_VERIFY_VK_RESULT(
        vmaCreateBuffer(vma, &bi, &ai, &_vkBuffer, &_vmaAllocation, 0));

    _descriptor.initialData = nullptr;
}

// Picks a memory type that the buffer accepts and that is device local. For
// imported memory this must be a type the exporting allocation also used;
// requesting DEVICE_LOCAL matches how CreateInteropBuffer allocates
// (VMA_MEMORY_USAGE_GPU_ONLY), so the compatible-type sets intersect.
static uint32_t
_SelectDeviceLocalMemoryType(HgiVulkanDevice* device, uint32_t typeBits)
{
    VkPhysicalDeviceMemoryProperties const& props =
        device->GetDeviceCapabilities().vkMemoryProperties;
    for (uint32_t i = 0; i < props.memoryTypeCount; ++i) {
        if (!(typeBits & (1u << i))) {
            continue;
        }
        if (props.memoryTypes[i].propertyFlags &
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            return i;
        }
    }
    // No device-local candidate: fall back to the first acceptable type.
    for (uint32_t i = 0; i < props.memoryTypeCount; ++i) {
        if (typeBits & (1u << i)) {
            return i;
        }
    }
    return UINT32_MAX;
}

HgiVulkanBuffer::HgiVulkanBuffer(
    HgiVulkan* hgi,
    HgiExternalMemoryBufferDesc const& desc)
    : HgiBuffer(HgiBufferDesc())
    , _hgi(hgi)
    , _vkBuffer(nullptr)
    , _vmaAllocation(nullptr)
    , _inflightBits(0)
    , _cpuStagingAddress(nullptr)
    , _mappable(false)
    , _isExternal(false)          // owning, but not via VMA
{
    _descriptor.byteSize = desc.byteSize;
    _descriptor.usage = desc.usage;
    _descriptor.debugName = desc.debugName;

    HgiVulkanDevice* device = hgi->GetPrimaryDevice();
    VkDevice vkDevice = device->GetVulkanDevice();

    VkBufferCreateInfo bi = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bi.size = desc.byteSize;
    bi.usage = HgiVulkanConversions::GetBufferUsage(desc.usage);
    bi.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // The buffer must declare the same handle type the memory was exported
    // with, or binding the imported memory to it is invalid.
    VkExternalMemoryBufferCreateInfo externalInfo =
        { VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO };
    externalInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_AUTO;
    externalInfo.pNext = bi.pNext;
    bi.pNext = &externalInfo;

    if (vkCreateBuffer(vkDevice, &bi, HgiVulkanAllocator(), &_vkBuffer)
            != VK_SUCCESS) {
        _vkBuffer = nullptr;
        return;
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(vkDevice, _vkBuffer, &memReqs);

    const uint32_t memoryTypeIndex =
        _SelectDeviceLocalMemoryType(device, memReqs.memoryTypeBits);
    if (memoryTypeIndex == UINT32_MAX) {
        TF_WARN("No memory type accepts the imported interop buffer");
        vkDestroyBuffer(vkDevice, _vkBuffer, HgiVulkanAllocator());
        _vkBuffer = nullptr;
        return;
    }

    // Import the producer's allocation. The whole block is (re)allocated -- the
    // import aliases it rather than copying -- and the buffer is bound at its
    // offset within the block.
    VkMemoryAllocateInfo mai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.allocationSize = desc.memoryBlockSize;
    mai.memoryTypeIndex = memoryTypeIndex;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkImportMemoryWin32HandleInfoKHR importInfo =
        { VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR };
    importInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_AUTO;
    importInfo.handle =
        reinterpret_cast<HANDLE>(static_cast<uintptr_t>(desc.externalHandle));
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    VkImportMemoryFdInfoKHR importInfo =
        { VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR };
    importInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_AUTO;
    importInfo.fd = static_cast<int>(desc.externalHandle);
#endif
#if defined(VK_USE_PLATFORM_WIN32_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
    importInfo.pNext = mai.pNext;
    mai.pNext = &importInfo;
#endif

    // A dedicated allocation can only be imported as dedicated, and the
    // resource it is dedicated to must be named at allocation time.
    VkMemoryDedicatedAllocateInfo dedicatedInfo =
        { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
    if (desc.dedicated) {
        dedicatedInfo.buffer = _vkBuffer;
        dedicatedInfo.pNext = mai.pNext;
        mai.pNext = &dedicatedInfo;
    }

    if (vkAllocateMemory(vkDevice, &mai, HgiVulkanAllocator(),
            &_vkImportedMemory) != VK_SUCCESS) {
        TF_WARN("Failed to import external memory handle %llu (%zu bytes)",
                static_cast<unsigned long long>(desc.externalHandle),
                desc.memoryBlockSize);
        _vkImportedMemory = VK_NULL_HANDLE;
        vkDestroyBuffer(vkDevice, _vkBuffer, HgiVulkanAllocator());
        _vkBuffer = nullptr;
        return;
    }

    if (vkBindBufferMemory(vkDevice, _vkBuffer, _vkImportedMemory,
            desc.memoryOffset) != VK_SUCCESS) {
        TF_WARN("Failed to bind imported memory at offset %zu",
                desc.memoryOffset);
        vkFreeMemory(vkDevice, _vkImportedMemory, HgiVulkanAllocator());
        _vkImportedMemory = VK_NULL_HANDLE;
        vkDestroyBuffer(vkDevice, _vkBuffer, HgiVulkanAllocator());
        _vkBuffer = nullptr;
        return;
    }

    if (!_descriptor.debugName.empty() && HgiVulkanIsDebugEnabled()) {
        HgiVulkanSetDebugName(
            device,
            (uint64_t)_vkBuffer,
            VK_OBJECT_TYPE_BUFFER,
            _descriptor.debugName.c_str());
    }

    // The memory was last written outside this device's queue family, and the
    // buffer is VK_SHARING_MODE_EXCLUSIVE, so ownership has to be acquired from
    // VK_QUEUE_FAMILY_EXTERNAL before first use. Skipping this is the classic
    // "works on one vendor, corrupts on another" interop bug. One acquire per
    // import suffices: the producer re-releases to EXTERNAL each time it writes
    // (for a GL producer, glSignalSemaphoreEXT's buffer list does that).
    //
    // Queued rather than recorded here. Recording needs the resource command
    // buffer, which is main-thread only, and a consumer imports during its Sync,
    // which runs in parallel -- so recording inline fails the queue's thread
    // check on whichever prims a worker thread happens to pick up.
    device->GetCommandQueue()->AddPendingQueueFamilyAcquire(_vkBuffer);
}

HgiVulkanBuffer::~HgiVulkanBuffer()
{
    _cpuStagingAddress = nullptr;
    _stagingBuffer = nullptr;

    // Adopted (external) buffers are non-owning: never free the producer's
    // VkBuffer/memory.
    if (_isExternal) {
        return;
    }

    // Imported buffers own their VkBuffer and their imported memory reference,
    // but have no VmaAllocation to hand back.
    if (_vkImportedMemory != VK_NULL_HANDLE) {
        VkDevice vkDevice = _hgi->GetPrimaryDevice()->GetVulkanDevice();
        vkDestroyBuffer(vkDevice, _vkBuffer, HgiVulkanAllocator());
        vkFreeMemory(vkDevice, _vkImportedMemory, HgiVulkanAllocator());
        return;
    }

    // A failed import leaves nothing to release.
    if (!_vkBuffer) {
        return;
    }

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

VkBufferMemoryBarrier HgiVulkanBuffer::GetBarrier(
    VkAccessFlags srcAccess,
    VkAccessFlags dstAccess) const
{
    VkBufferMemoryBarrier bufferBar =
        { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr };
    bufferBar.buffer = GetVulkanBuffer();
    bufferBar.offset = 0;
    bufferBar.size = GetByteSizeOfResource();
    bufferBar.srcAccessMask = srcAccess;
    bufferBar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBar.dstAccessMask = dstAccess;
    bufferBar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    return bufferBar;
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
