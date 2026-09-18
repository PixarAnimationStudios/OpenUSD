//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GL_SEMAPHORE_H
#define PXR_IMAGING_HGI_GL_SEMAPHORE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgi/externalBuffer.h"
#include "pxr/imaging/hgi/semaphore.h"

#include <cstdint>
#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiGLSemaphore
///
/// A GL semaphore object (GL_EXT_semaphore) imported from another API's
/// semaphore, and the GL objects that import created.
///
/// GL can only ever *import* a shareable semaphore -- the extension has no way
/// to create one another API could wait on -- so this always wraps a semaphore
/// Vulkan created and exported. Two consequences worth knowing. GL semaphores
/// are binary only: there is no timeline form, which is why binary is the
/// interop lowest common denominator. And glWaitSemaphoreEXT is a command
/// stream operation on the whole context rather than a per-queue one, so its
/// granularity is every command the context has recorded -- fine for Storm,
/// coarse for a producer running several streams.
///
/// The wait and signal take the buffers they must cover explicitly, because
/// that is how the extension establishes coherency for imported memory; the
/// arena passes its own buffers. glSignalSemaphoreEXT also implies a flush.
///
class HgiGLSemaphore final : public HgiSemaphore
{
public:
    /// Import the semaphore named by the OS handle \p externalHandle, which
    /// another API created as exportable. Returns null when the GL
    /// implementation has no GL_EXT_semaphore support, when the handle type is
    /// not one GL can import, or when \p kind is not binary -- GL cannot
    /// import a timeline semaphore, and silently treating one as binary would
    /// wait for the wrong thing.
    ///
    /// Handle ownership follows the platform convention: an fd is taken over
    /// by the import, a Win32 handle stays the caller's to close.
    HGIGL_API
    static std::shared_ptr<HgiGLSemaphore> Import(
        uint64_t externalHandle,
        HgiExternalHandleType handleType,
        HgiSemaphoreKind kind);

    HGIGL_API
    ~HgiGLSemaphore() override;

    HGIGL_API
    void EncodeWait(
        uint64_t value,
        std::vector<HgiExternalBuffer *> const &buffers) override;

    HGIGL_API
    void EncodeSignal(
        uint64_t value,
        std::vector<HgiExternalBuffer *> const &buffers) override;

    /// The GL semaphore object name.
    uint32_t GetSemaphoreId() const {
        return _semaphoreId;
    }

private:
    explicit HgiGLSemaphore(uint32_t semaphoreId);

    // The GL buffer names of \p buffers, which the wait and signal barrier
    // lists are built from. Buffers with no usable GL resource are skipped.
    static std::vector<uint32_t> _GetGLBufferIds(
        std::vector<HgiExternalBuffer *> const &buffers);

    uint32_t _semaphoreId;
};

using HgiGLSemaphoreSharedPtr = std::shared_ptr<HgiGLSemaphore>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HGI_GL_SEMAPHORE_H
