//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/externalBufferArena.h"
#include "pxr/imaging/hgiGL/semaphore.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

bool
HgiGLExternalBufferArena::IsSupportedBy(Hgi *hgi)
{
    return hgi && hgi->GetAPIName() == HgiTokens->OpenGL;
}

HgiGLExternalBufferArena::HgiGLExternalBufferArena(
    Hgi *hgi,
    uint64_t rawSourceDevice)
    : HgiExternalBufferArena(hgi, rawSourceDevice)
{
    // No semaphores by default: the common case is a producer sharing our GL
    // context, where command order already sequences the accesses. A producer
    // that needs more calls ImportSemaphores.
}

HgiGLExternalBufferArena::~HgiGLExternalBufferArena() = default;

HgiExternalBufferSharedPtr
HgiGLExternalBufferArena::AllocateBuffer(
    size_t byteSize,
    HgiBufferUsage usage,
    std::string const &debugName)
{
    return _Register(HgiGLExternalBuffer::_CreateAllocated(
        this, _GetNextBufferHandleId(), byteSize, usage, debugName));
}

HgiExternalBufferSharedPtr
HgiGLExternalBufferArena::RegisterBuffer(
    uint32_t bufferId,
    size_t byteSize,
    HgiBufferUsage usage)
{
    return _Register(HgiGLExternalBuffer::_CreateAdopted(
        this, _GetNextBufferHandleId(), bufferId, byteSize, usage,
        /*takeOwnership*/ false));
}

HgiExternalBufferSharedPtr
HgiGLExternalBufferArena::AdoptBuffer(
    uint32_t bufferId,
    size_t byteSize,
    HgiBufferUsage usage)
{
    return _Register(HgiGLExternalBuffer::_CreateAdopted(
        this, _GetNextBufferHandleId(), bufferId, byteSize, usage,
        /*takeOwnership*/ true));
}

HgiExternalBufferSharedPtr
HgiGLExternalBufferArena::ImportBuffer(
    HgiGLImportBufferDesc const &desc)
{
    return _Register(HgiGLExternalBuffer::_CreateImported(
        this, _GetNextBufferHandleId(), desc));
}

bool
HgiGLExternalBufferArena::ImportSemaphores(
    uint64_t appDoneHandle,
    uint64_t hgiDoneHandle,
    HgiExternalHandleType handleType,
    HgiSemaphoreKind kind)
{
    HgiSemaphoreSharedPtr appDone;
    HgiSemaphoreSharedPtr hgiDone;

    if (appDoneHandle) {
        appDone = HgiGLSemaphore::Import(appDoneHandle, handleType, kind);
        if (!appDone) {
            return false;
        }
    }
    if (hgiDoneHandle) {
        hgiDone = HgiGLSemaphore::Import(hgiDoneHandle, handleType, kind);
        if (!hgiDone) {
            return false;
        }
    }

    _SetSemaphores(std::move(appDone), std::move(hgiDone));
    return true;
}

uint64_t
HgiGLExternalBufferArena::_CaptureSubmissionStamp()
{
    // A fence placed here completes once everything recorded before it has
    // executed -- which is exactly the work that could still be reading the
    // buffers this pass is retiring.
    //
    // Deleting a GL buffer would not strictly need this: glDeleteBuffers is
    // already deferred by the driver until the GPU is done. Recycling one
    // does, and that is the case worth protecting -- overwriting bytes a
    // submitted draw still reads is a hazard GL does not track for us.
    const GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    // Flush, or the fence may never signal. A sync object's status is not
    // required to advance until the commands before it have been flushed, and
    // _IsSubmissionRetired polls with glGetSynciv, which -- unlike
    // glClientWaitSync with GL_SYNC_FLUSH_COMMANDS_BIT -- does not flush. An
    // unflushed fence would leave the buffer pinned for good.
    glFlush();

    HGIGL_POST_PENDING_GL_ERRORS();
    return reinterpret_cast<uint64_t>(sync);
}

bool
HgiGLExternalBufferArena::_IsSubmissionRetired(uint64_t stamp)
{
    const GLsync sync = reinterpret_cast<GLsync>(stamp);
    if (!sync) {
        // No fence: nothing to wait for.
        return true;
    }

    // Poll; never block. A stamp that is not done yet is simply retested on a
    // later pass, which costs a few frames of lag and no stalls.
    GLint status = GL_UNSIGNALED;
    glGetSynciv(sync, GL_SYNC_STATUS, 1, nullptr, &status);
    if (status != GL_SIGNALED) {
        return false;
    }

    glDeleteSync(sync);
    HGIGL_POST_PENDING_GL_ERRORS();
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
