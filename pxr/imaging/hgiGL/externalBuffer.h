//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GL_EXTERNAL_BUFFER_H
#define PXR_IMAGING_HGI_GL_EXTERNAL_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/externalBuffer.h"

#include <cstddef>
#include <cstdint>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class HgiGLExternalBufferArena;

/// \struct HgiGLImportBufferDesc
///
/// A foreign memory allocation to import into OpenGL and bind a buffer into,
/// so HgiGL can read memory another API allocated. The producer exports the
/// allocation (vkGetMemoryWin32HandleKHR / vkGetMemoryFdKHR) and describes it
/// with this.
///
/// This describes the *allocation*, not a buffer object: GL has no way to
/// interpret a foreign API's buffer handle, only its memory.
struct HgiGLImportBufferDesc
{
    /// OS-shareable handle naming the memory allocation. On Windows the caller
    /// keeps ownership and should close it afterwards; on Linux the import
    /// takes over the fd, so pass a dup() if it is needed again.
    uint64_t externalHandle = 0;

    /// How to interpret externalHandle. Never inferred from the value -- a
    /// Win32 NT handle and an fd are both small integers.
    HgiExternalHandleType handleType = HgiGetPlatformExternalHandleType();

    /// Size of the whole memory block the handle names. GL imports the block
    /// entire, even when only a window of it is used here.
    size_t memoryBlockSize = 0;

    /// Offset of the buffer within that block.
    size_t memoryOffset = 0;

    /// Size of the buffer to bind at memoryOffset.
    size_t byteSize = 0;

    /// Whether the producer made a dedicated allocation. Must match how it was
    /// allocated or the import fails.
    bool dedicated = false;

    /// What the consumer may bind the resulting buffer for.
    HgiBufferUsage usage = 0;

    std::string debugName;
};

/// \class HgiGLExternalBuffer
///
/// An HgiExternalBuffer whose consumer is OpenGL. Covers the two ways GL can
/// get at memory it did not allocate:
///
/// * Passthrough, when the producer is also OpenGL and shares the context or
///   share group. There is nothing to import -- a GL buffer name already means
///   something here -- so this is the cheapest form of sharing there is, and
///   the one a GL-based viewport handing buffers to Storm uses.
///
/// * Import, when the producer is another API (Vulkan) that exported its
///   allocation. GL creates a memory object, imports the handle into it, and
///   binds a buffer over it. The import is persistent: create it once and
///   reuse it, which is what the arena's ownership of these objects buys.
///
/// Import lives here, in the consuming backend, rather than in the producer's.
/// That keeps hgiGL and hgiVulkan free of each other -- neither has to link
/// the other to make VK-to-GL sharing work -- and it puts the GL objects in
/// the library that knows how to destroy them.
///
class HgiGLExternalBuffer final : public HgiExternalBuffer
{
public:
    HGIGL_API
    ~HgiGLExternalBuffer() override;

    HGIGL_API
    HgiBufferHandle GetBuffer() const override;

    /// The GL buffer name, whether adopted or created by an import.
    uint32_t GetBufferId() const {
        return _bufferId;
    }

private:
    friend class HgiGLExternalBufferArena;

    // Allocate a new GL buffer the arena owns and frees.
    static HgiExternalBufferSharedPtr _CreateAllocated(
        HgiGLExternalBufferArena *arena,
        uint64_t handleId,
        size_t byteSize,
        HgiBufferUsage usage,
        std::string const &debugName);

    // Wrap the existing GL buffer name \p bufferId. \p takeOwnership decides
    // who deletes it: false leaves it with the application, which is the only
    // correct answer for a pooled buffer the application still draws with and
    // recycles on its own schedule.
    static HgiExternalBufferSharedPtr _CreateAdopted(
        HgiGLExternalBufferArena *arena,
        uint64_t handleId,
        uint32_t bufferId,
        size_t byteSize,
        HgiBufferUsage usage,
        bool takeOwnership);

    // Import a foreign allocation and bind a GL buffer over it. Returns null
    // when the GL implementation cannot import, or the import fails.
    static HgiExternalBufferSharedPtr _CreateImported(
        HgiGLExternalBufferArena *arena,
        uint64_t handleId,
        HgiGLImportBufferDesc const &desc);

    HgiGLExternalBuffer(
        HgiGLExternalBufferArena *arena,
        uint64_t handleId,
        uint32_t bufferId,
        size_t byteSize,
        HgiBufferUsage usage,
        bool ownsBufferId,
        uint32_t memoryObjectId);

    HgiGLExternalBuffer() = delete;
    HgiGLExternalBuffer(const HgiGLExternalBuffer &) = delete;
    HgiGLExternalBuffer & operator=(const HgiGLExternalBuffer &) = delete;

    uint32_t _bufferId;
    // The GL memory object an import created, 0 otherwise. Owned: it is ours
    // even though the allocation behind it is not.
    uint32_t _memoryObjectId;
    // Whether _bufferId is ours to delete.
    bool _ownsBufferId;
    // Wraps _bufferId for the consumer. Non-owning with respect to the GL
    // name; this object decides deletion.
    HgiBufferHandle _buffer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HGI_GL_EXTERNAL_BUFFER_H
