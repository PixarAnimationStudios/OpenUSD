//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/externalBuffer.h"
#include "pxr/imaging/hgiVulkan/externalBufferArena.h"
#include "pxr/imaging/hgiVulkan/hgi.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiVulkanExternalBuffer::HgiVulkanExternalBuffer(
    HgiVulkanExternalBufferArena *arena,
    uint64_t handleId,
    HgiVulkanBuffer *buffer,
    size_t byteSize,
    bool destroysAdoptedBuffer)
    : HgiExternalBuffer(arena, byteSize)
    , _hgiVulkan(static_cast<HgiVulkan *>(arena->GetHgi()))
    , _buffer(HgiBufferHandle(buffer, handleId))
    , _destroysAdoptedBuffer(destroysAdoptedBuffer)
{
}

HgiVulkanExternalBuffer::~HgiVulkanExternalBuffer()
{
    HgiVulkanBuffer *buffer =
        static_cast<HgiVulkanBuffer *>(_buffer.Get());
    if (!buffer) {
        return;
    }

    // Read the handle out before the wrapper goes, for the one case where we
    // have to destroy it ourselves; see _destroysAdoptedBuffer. Deleting the
    // wrapper is what frees an allocated or imported buffer, so asking for
    // both would be a double free.
    const VkBuffer vkBuffer =
        _destroysAdoptedBuffer ? buffer->GetVulkanBuffer() : VK_NULL_HANDLE;

    delete buffer;
    _buffer = HgiBufferHandle();

    if (vkBuffer != VK_NULL_HANDLE) {
        HgiVulkanDevice *device = _hgiVulkan->GetPrimaryDevice();
        vkDestroyBuffer(device->GetVulkanDevice(), vkBuffer,
            HgiVulkanAllocator());
    }
}

HgiBufferHandle
HgiVulkanExternalBuffer::GetBuffer() const
{
    return _buffer;
}

VkBuffer
HgiVulkanExternalBuffer::GetVulkanBuffer() const
{
    HgiVulkanBuffer *buffer =
        static_cast<HgiVulkanBuffer *>(_buffer.Get());
    return buffer ? buffer->GetVulkanBuffer() : VK_NULL_HANDLE;
}

HgiExternalBufferSharedPtr
HgiVulkanExternalBuffer::_CreateAllocated(
    HgiVulkanExternalBufferArena *arena,
    uint64_t handleId,
    size_t byteSize,
    HgiBufferUsage usage,
    std::string const &debugName)
{
    if (byteSize == 0) {
        return nullptr;
    }

    HgiVulkan *hgiVulkan = static_cast<HgiVulkan *>(arena->GetHgi());

    HgiBufferDesc desc;
    desc.byteSize = byteSize;
    desc.usage = usage;
    desc.debugName = debugName.empty()
        ? "HgiVulkanExternalBuffer" : debugName;

    // Exportable: an application in another API has to be able to import this
    // memory, since the point of allocating here is to hand it out.
    HgiVulkanBuffer *buffer =
        new HgiVulkanBuffer(hgiVulkan, desc, /*interop*/ true);
    if (!buffer->GetVulkanBuffer()) {
        delete buffer;
        return nullptr;
    }

    // The exportable wrapper is VMA-backed and frees both the VkBuffer and its
    // memory when deleted, so there is nothing left for us to destroy.
    std::shared_ptr<HgiVulkanExternalBuffer> external(
        new HgiVulkanExternalBuffer(
            arena, handleId, buffer, byteSize,
            /*destroysAdoptedBuffer*/ false));

    // Describe the allocation for whoever imports it. VMA suballocates, so the
    // block size and the offset within it both matter -- an importer that
    // allocates only byteSize gets a different allocation, not this one.
    HgiVulkanDevice *device = hgiVulkan->GetPrimaryDevice();
    VmaAllocationInfo2 allocInfo = {};
    vmaGetAllocationInfo2(
        device->GetVulkanMemoryAllocator(),
        buffer->GetVulkanMemoryAllocation(),
        &allocInfo);

    HgiVulkanExternalBufferExportInfo &info = external->_exportInfo;
    info.memoryBlockSize = allocInfo.blockSize;
    info.memoryOffset = allocInfo.allocationInfo.offset;
    info.dedicated = allocInfo.dedicatedMemory;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    info.handleType = HgiExternalHandleTypeOpaqueWin32;
    info.externalHandle = static_cast<uint64_t>(
        reinterpret_cast<uintptr_t>(device->GetWin32HandleForMemory(
            allocInfo.allocationInfo.deviceMemory)));
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    info.handleType = HgiExternalHandleTypeOpaqueFd;
    info.externalHandle = static_cast<uint64_t>(
        device->GetFdForMemory(allocInfo.allocationInfo.deviceMemory));
#endif

    return external;
}

HgiExternalBufferSharedPtr
HgiVulkanExternalBuffer::_CreateAdopted(
    HgiVulkanExternalBufferArena *arena,
    uint64_t handleId,
    VkBuffer vkBuffer,
    size_t byteSize,
    HgiBufferUsage usage,
    bool takeOwnership)
{
    if (vkBuffer == VK_NULL_HANDLE || byteSize == 0) {
        return nullptr;
    }

    HgiVulkan *hgiVulkan = static_cast<HgiVulkan *>(arena->GetHgi());

    // Non-owning wrapper: binding only. Who destroys the VkBuffer is
    // takeOwnership's business, handled in our destructor.
    HgiVulkanBuffer *buffer =
        new HgiVulkanBuffer(hgiVulkan, vkBuffer, byteSize, usage);

    // takeOwnership is the only route where this object destroys the VkBuffer:
    // the adopting wrapper above frees nothing, by design, so that a buffer the
    // application still uses is never freed out from under it.
    return HgiExternalBufferSharedPtr(new HgiVulkanExternalBuffer(
        arena, handleId, buffer, byteSize,
        /*destroysAdoptedBuffer*/ takeOwnership));
}

HgiExternalBufferSharedPtr
HgiVulkanExternalBuffer::_CreateImported(
    HgiVulkanExternalBufferArena *arena,
    uint64_t handleId,
    HgiVulkanImportBufferDesc const &desc)
{
    if (!desc.externalHandle || desc.byteSize == 0 ||
            desc.memoryBlockSize == 0) {
        TF_WARN("Incomplete HgiVulkanImportBufferDesc: handle=%llu "
                "byteSize=%zu memoryBlockSize=%zu",
                static_cast<unsigned long long>(desc.externalHandle),
                desc.byteSize, desc.memoryBlockSize);
        return nullptr;
    }

    HgiVulkan *hgiVulkan = static_cast<HgiVulkan *>(arena->GetHgi());

    HgiVulkanBuffer *buffer = new HgiVulkanBuffer(hgiVulkan, desc);
    if (!buffer->GetVulkanBuffer()) {
        // The import failed and reported why; the caller copies instead.
        delete buffer;
        return nullptr;
    }

    // The import wrapper frees its own VkBuffer and releases its reference to
    // the producer's memory when deleted, so there is nothing extra here.
    return HgiExternalBufferSharedPtr(new HgiVulkanExternalBuffer(
        arena, handleId, buffer, desc.byteSize,
        /*destroysAdoptedBuffer*/ false));
}

PXR_NAMESPACE_CLOSE_SCOPE
