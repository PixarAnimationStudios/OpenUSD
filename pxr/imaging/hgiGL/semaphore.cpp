//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/semaphore.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

std::shared_ptr<HgiGLImportedSemaphore>
HgiGLImportedSemaphore::Import(
    uint64_t externalHandle,
    HgiExternalHandleType handleType,
    HgiSemaphoreKind kind)
{
    if (!externalHandle) {
        return nullptr;
    }

    // GL_EXT_semaphore has no timeline form. Importing a timeline semaphore as
    // if it were binary would wait for a signal that never comes in that
    // shape, so refuse and let the caller fall back.
    if (kind != HgiSemaphoreKindBinary) {
        TF_WARN("HgiGL cannot import a timeline semaphore; "
                "GL_EXT_semaphore is binary only");
        return nullptr;
    }

    if (!glGenSemaphoresEXT || !glWaitSemaphoreEXT || !glSignalSemaphoreEXT) {
        return nullptr;
    }

    GLuint semaphoreId = 0;
    glGenSemaphoresEXT(1, &semaphoreId);
    if (!semaphoreId) {
        return nullptr;
    }

    switch (handleType) {
#if defined(ARCH_OS_WINDOWS)
    case HgiExternalHandleTypeOpaqueWin32:
        if (!glImportSemaphoreWin32HandleEXT) {
            glDeleteSemaphoresEXT(1, &semaphoreId);
            return nullptr;
        }
        glImportSemaphoreWin32HandleEXT(
            semaphoreId,
            GL_HANDLE_TYPE_OPAQUE_WIN32_EXT,
            reinterpret_cast<void *>(
                static_cast<uintptr_t>(externalHandle)));
        break;
#else
    case HgiExternalHandleTypeOpaqueFd:
        if (!glImportSemaphoreFdEXT) {
            glDeleteSemaphoresEXT(1, &semaphoreId);
            return nullptr;
        }
        // The import takes over the fd; the caller must have dup()ed it if it
        // still needs one.
        glImportSemaphoreFdEXT(
            semaphoreId,
            GL_HANDLE_TYPE_OPAQUE_FD_EXT,
            static_cast<int>(externalHandle));
        break;
#endif
    default:
        glDeleteSemaphoresEXT(1, &semaphoreId);
        return nullptr;
    }

    // An import that failed leaves the name unusable rather than reporting an
    // error, so ask.
    if (glIsSemaphoreEXT && !glIsSemaphoreEXT(semaphoreId)) {
        glDeleteSemaphoresEXT(1, &semaphoreId);
        return nullptr;
    }

    HGIGL_POST_PENDING_GL_ERRORS();

    return std::shared_ptr<HgiGLImportedSemaphore>(new HgiGLImportedSemaphore(semaphoreId));
}

HgiGLImportedSemaphore::HgiGLImportedSemaphore(uint32_t semaphoreId)
    : HgiSemaphore(HgiSemaphoreKindBinary)
    , _semaphoreId(semaphoreId)
{
}

HgiGLImportedSemaphore::~HgiGLImportedSemaphore()
{
    if (_semaphoreId && glDeleteSemaphoresEXT) {
        GLuint semaphoreId = _semaphoreId;
        glDeleteSemaphoresEXT(1, &semaphoreId);
        _semaphoreId = 0;
    }
    HGIGL_POST_PENDING_GL_ERRORS();
}

std::vector<uint32_t>
HgiGLImportedSemaphore::_GetGLBufferIds(
    std::vector<HgiExternalBuffer *> const &buffers)
{
    std::vector<uint32_t> bufferIds;
    bufferIds.reserve(buffers.size());
    for (HgiExternalBuffer *buffer : buffers) {
        if (!buffer) {
            continue;
        }
        const HgiBufferHandle handle = buffer->GetBuffer();
        if (!handle) {
            continue;
        }
        if (const uint64_t raw = handle->GetRawResource()) {
            bufferIds.push_back(static_cast<uint32_t>(raw));
        }
    }
    return bufferIds;
}

void
HgiGLImportedSemaphore::EncodeWait(
    uint64_t /*value*/,
    std::vector<HgiExternalBuffer *> const &buffers)
{
    if (!_semaphoreId || !glWaitSemaphoreEXT) {
        return;
    }

    // Binary semaphore: there is no value to wait for. The buffer list is what
    // makes the producer's writes visible to us, so pass every buffer this
    // wait covers.
    const std::vector<uint32_t> bufferIds = _GetGLBufferIds(buffers);
    glWaitSemaphoreEXT(
        _semaphoreId,
        static_cast<GLuint>(bufferIds.size()),
        bufferIds.empty() ? nullptr : bufferIds.data(),
        0, nullptr, nullptr);

    HGIGL_POST_PENDING_GL_ERRORS();
}

void
HgiGLImportedSemaphore::EncodeSignal(
    uint64_t /*value*/,
    std::vector<HgiExternalBuffer *> const &buffers)
{
    if (!_semaphoreId || !glSignalSemaphoreEXT) {
        return;
    }

    const std::vector<uint32_t> bufferIds = _GetGLBufferIds(buffers);
    // This also implies a flush, which is what makes the signal reach the
    // server rather than sitting in the client's command buffer.
    glSignalSemaphoreEXT(
        _semaphoreId,
        static_cast<GLuint>(bufferIds.size()),
        bufferIds.empty() ? nullptr : bufferIds.data(),
        0, nullptr, nullptr);

    HGIGL_POST_PENDING_GL_ERRORS();
}

PXR_NAMESPACE_CLOSE_SCOPE
