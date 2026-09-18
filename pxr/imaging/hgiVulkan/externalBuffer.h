//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_EXTERNAL_BUFFER_H
#define PXR_IMAGING_HGIVULKAN_EXTERNAL_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"
#include "pxr/imaging/hgi/externalBuffer.h"

#include <cstddef>
#include <cstdint>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkan;
class HgiVulkanExternalBufferArena;

/// \struct HgiVulkanExternalBufferExportInfo
///
/// Describes the memory behind a buffer the arena allocated, so an
/// application in another API can import and alias the same memory. The
/// counterpart of HgiVulkanImportBufferDesc, from the other side.
///
/// Produced only for buffers from
/// HgiVulkanExternalBufferArena::AllocateBuffer, which allocates exportable;
/// registered and imported buffers leave it at its defaults.
struct HgiVulkanExternalBufferExportInfo
{
    /// OS-shareable handle naming the memory, or 0 when the buffer is not
    /// exportable. On Windows the recipient should close it when done; on
    /// Linux the importer takes over the fd.
    uint64_t externalHandle = 0;

    /// How to interpret externalHandle.
    HgiExternalHandleType handleType = HgiExternalHandleTypeOpaqueWin32;

    /// Size of the whole memory block, and the buffer's offset within it --
    /// both needed to import the memory elsewhere.
    size_t memoryBlockSize = 0;
    size_t memoryOffset = 0;

    /// Whether this is a dedicated allocation; the importer has to match it.
    bool dedicated = false;
};

/// \class HgiVulkanExternalBuffer
///
/// An HgiExternalBuffer whose consumer is Vulkan. Covers the three ways a
/// Vulkan consumer can reach memory it did not allocate itself:
///
/// * Passthrough, when the producer allocated on the same logical device. A
///   VkBuffer already means something here, so there is nothing to import.
///   This is the only route where the *handle* rather than the memory is
///   shared, and the only one that needs the two devices to be the same
///   object -- which is why the arena is keyed on the producer's device.
///
/// * Import, when the producer's memory lives on another device or was
///   allocated by another API that exported it. Vulkan allocates with an
///   import chain and binds a new VkBuffer into the producer's memory.
///
/// * Allocation, when it is simpler for Hgi to own the memory and let the
///   application write into it. The allocation is made exportable, so the
///   application can import it through whichever API it uses; see
///   GetExportInfo.
///
class HgiVulkanExternalBuffer final : public HgiExternalBuffer
{
public:
    HGIVULKAN_API
    ~HgiVulkanExternalBuffer() override;

    HGIVULKAN_API
    HgiBufferHandle GetBuffer() const override;

    /// The underlying VkBuffer, or VK_NULL_HANDLE if creation failed.
    HGIVULKAN_API
    VkBuffer GetVulkanBuffer() const;

    /// How an application in another API can import this buffer's memory.
    /// Meaningful only for an allocated buffer; see the struct.
    HgiVulkanExternalBufferExportInfo const &GetExportInfo() const {
        return _exportInfo;
    }

private:
    friend class HgiVulkanExternalBufferArena;

    // Allocate an exportable buffer the arena owns and frees, and fill in the
    // export info an application needs to import it.
    static HgiExternalBufferSharedPtr _CreateAllocated(
        HgiVulkanExternalBufferArena *arena,
        uint64_t handleId,
        size_t byteSize,
        HgiBufferUsage usage,
        std::string const &debugName);

    // Wrap the existing \p vkBuffer, which must belong to this arena's source
    // device. \p takeOwnership decides who destroys it; false -- the default
    // for a pooled application buffer -- leaves it with the application.
    static HgiExternalBufferSharedPtr _CreateAdopted(
        HgiVulkanExternalBufferArena *arena,
        uint64_t handleId,
        VkBuffer vkBuffer,
        size_t byteSize,
        HgiBufferUsage usage,
        bool takeOwnership);

    // Import a foreign allocation and bind a VkBuffer over it. Returns null
    // when the import fails.
    static HgiExternalBufferSharedPtr _CreateImported(
        HgiVulkanExternalBufferArena *arena,
        uint64_t handleId,
        HgiVulkanImportBufferDesc const &desc);

    HgiVulkanExternalBuffer(
        HgiVulkanExternalBufferArena *arena,
        uint64_t handleId,
        HgiVulkanBuffer *buffer,
        size_t byteSize,
        bool destroysAdoptedBuffer);

    HgiVulkanExternalBuffer() = delete;
    HgiVulkanExternalBuffer(const HgiVulkanExternalBuffer &) = delete;
    HgiVulkanExternalBuffer & operator=(
        const HgiVulkanExternalBuffer &) = delete;

    HgiVulkan *_hgiVulkan;
    HgiBufferHandle _buffer;
    // Whether destroying this object must also destroy the VkBuffer ITSELF --
    // which is true in exactly one case, an adopted buffer whose ownership the
    // application transferred to us.
    //
    // It is not an ownership claim; it says who performs the destroy. An
    // allocated or imported buffer is equally ours, but its HgiVulkanBuffer
    // wrapper frees it (through VMA, or vkDestroyBuffer plus vkFreeMemory), so
    // deleting the wrapper is enough and doing more would be a double free. An
    // adopting wrapper deliberately frees nothing, which leaves the job here.
    bool _destroysAdoptedBuffer;
    HgiVulkanExternalBufferExportInfo _exportInfo;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HGIVULKAN_EXTERNAL_BUFFER_H
