//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgiGL/buffer.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/externalBuffer.h"
#include "pxr/imaging/hgiGL/externalBufferArena.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLExternalBuffer::HgiGLExternalBuffer(
    HgiGLExternalBufferArena *arena,
    uint64_t handleId,
    uint32_t bufferId,
    size_t byteSize,
    HgiBufferUsage usage,
    bool ownsBufferId,
    uint32_t memoryObjectId)
    : HgiExternalBuffer(arena, byteSize)
    , _bufferId(bufferId)
    , _memoryObjectId(memoryObjectId)
    , _ownsBufferId(ownsBufferId)
{
    HgiBufferDesc desc;
    desc.byteSize = byteSize;
    desc.usage = usage;
    desc.debugName = "HgiGLExternalBuffer";
    // Adopting: the HgiGLBuffer wrapper never deletes the name, so deletion
    // stays a decision of this object (see _ownsBufferId).
    _buffer = HgiBufferHandle(new HgiGLBuffer(desc, bufferId), handleId);
}

HgiGLExternalBuffer::~HgiGLExternalBuffer()
{
    // Tear the consumer-facing wrapper down first: nothing may name the GL
    // objects after this point.
    if (HgiGLBuffer *buffer =
            static_cast<HgiGLBuffer *>(_buffer.Get())) {
        delete buffer;
    }
    _buffer = HgiBufferHandle();

    if (_ownsBufferId && _bufferId) {
        glDeleteBuffers(1, &_bufferId);
    }
    _bufferId = 0;

    // The memory object is ours even when the allocation it names is not:
    // deleting it releases our reference to the producer's memory, not the
    // memory.
    if (_memoryObjectId && glDeleteMemoryObjectsEXT) {
        glDeleteMemoryObjectsEXT(1, &_memoryObjectId);
    }
    _memoryObjectId = 0;

    HGIGL_POST_PENDING_GL_ERRORS();
}

HgiBufferHandle
HgiGLExternalBuffer::GetBuffer() const
{
    return _buffer;
}

HgiExternalBufferSharedPtr
HgiGLExternalBuffer::_CreateAllocated(
    HgiGLExternalBufferArena *arena,
    uint64_t handleId,
    size_t byteSize,
    HgiBufferUsage usage,
    std::string const &debugName)
{
    if (byteSize == 0) {
        return nullptr;
    }

    GLuint bufferId = 0;
    glCreateBuffers(1, &bufferId);
    if (!bufferId) {
        return nullptr;
    }
    glNamedBufferData(bufferId, byteSize, nullptr, GL_DYNAMIC_DRAW);
    if (!debugName.empty()) {
        HgiGLObjectLabel(GL_BUFFER, bufferId, debugName);
    }

    HGIGL_POST_PENDING_GL_ERRORS();

    return HgiExternalBufferSharedPtr(new HgiGLExternalBuffer(
        arena, handleId, bufferId, byteSize, usage,
        /*ownsBufferId*/ true, /*memoryObjectId*/ 0));
}

HgiExternalBufferSharedPtr
HgiGLExternalBuffer::_CreateAdopted(
    HgiGLExternalBufferArena *arena,
    uint64_t handleId,
    uint32_t bufferId,
    size_t byteSize,
    HgiBufferUsage usage,
    bool takeOwnership)
{
    if (!bufferId || byteSize == 0) {
        return nullptr;
    }

    return HgiExternalBufferSharedPtr(new HgiGLExternalBuffer(
        arena, handleId, bufferId, byteSize, usage,
        takeOwnership, /*memoryObjectId*/ 0));
}

HgiExternalBufferSharedPtr
HgiGLExternalBuffer::_CreateImported(
    HgiGLExternalBufferArena *arena,
    uint64_t handleId,
    HgiGLImportBufferDesc const &desc)
{
    if (!desc.externalHandle || desc.byteSize == 0 ||
            desc.memoryBlockSize == 0) {
        return nullptr;
    }
    if (!glCreateMemoryObjectsEXT || !glNamedBufferStorageMemEXT) {
        return nullptr;
    }

    GLuint memoryObjectId = 0;
    glCreateMemoryObjectsEXT(1, &memoryObjectId);
    if (!memoryObjectId) {
        return nullptr;
    }

    // A dedicated allocation has to be imported as dedicated; the parameter is
    // not advisory, and mismatching it fails the import.
    if (desc.dedicated && glMemoryObjectParameterivEXT) {
        const GLint dedicated = GL_TRUE;
        glMemoryObjectParameterivEXT(
            memoryObjectId, GL_DEDICATED_MEMORY_OBJECT_EXT, &dedicated);
    }

    bool imported = false;
    switch (desc.handleType) {
#if defined(ARCH_OS_WINDOWS)
    case HgiExternalHandleTypeOpaqueWin32:
        if (glImportMemoryWin32HandleEXT) {
            // Win32 import does not transfer the handle; the caller closes it.
            glImportMemoryWin32HandleEXT(
                memoryObjectId,
                desc.memoryBlockSize,
                GL_HANDLE_TYPE_OPAQUE_WIN32_EXT,
                reinterpret_cast<void *>(
                    static_cast<uintptr_t>(desc.externalHandle)));
            imported = true;
        }
        break;
#else
    case HgiExternalHandleTypeOpaqueFd:
        if (glImportMemoryFdEXT) {
            // The import takes over the fd.
            glImportMemoryFdEXT(
                memoryObjectId,
                desc.memoryBlockSize,
                GL_HANDLE_TYPE_OPAQUE_FD_EXT,
                static_cast<int>(desc.externalHandle));
            imported = true;
        }
        break;
#endif
    default:
        break;
    }

    if (!imported) {
        glDeleteMemoryObjectsEXT(1, &memoryObjectId);
        return nullptr;
    }

    GLuint bufferId = 0;
    glCreateBuffers(1, &bufferId);
    if (!bufferId) {
        glDeleteMemoryObjectsEXT(1, &memoryObjectId);
        return nullptr;
    }
    glNamedBufferStorageMemEXT(
        bufferId, desc.byteSize, memoryObjectId, desc.memoryOffset);

    if (!desc.debugName.empty()) {
        HgiGLObjectLabel(GL_BUFFER, bufferId, desc.debugName);
    }

    HGIGL_POST_PENDING_GL_ERRORS();

    return HgiExternalBufferSharedPtr(new HgiGLExternalBuffer(
        arena, handleId, bufferId, desc.byteSize, desc.usage,
        /*ownsBufferId*/ true, memoryObjectId));
}

PXR_NAMESPACE_CLOSE_SCOPE
