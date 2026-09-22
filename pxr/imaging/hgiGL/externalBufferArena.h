//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GL_EXTERNAL_BUFFER_ARENA_H
#define PXR_IMAGING_HGI_GL_EXTERNAL_BUFFER_ARENA_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgiGL/externalBuffer.h"
#include "pxr/imaging/hgi/externalBufferArena.h"

#include <cstdint>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;

/// \class HgiGLExternalBufferArena
///
/// The arena through which an application shares GPU buffers with an HgiGL
/// consumer. Obtain one from Hgi::GetExternalBufferArena:
///
/// \code
///     auto arena = hgi->GetExternalBufferArena<HgiGLExternalBufferArena>();
///     if (!arena) { /* interop unavailable -- copy instead */ }
/// \endcode
///
/// \section Direction
///
/// An arena belongs to the *consuming* backend, which is what keeps hgiGL and
/// hgiVulkan independent of each other. So this one class covers both sources
/// GL can consume:
///
/// | producer | how | notes |
/// | -------- | --- | ----- |
/// | OpenGL   | RegisterBuffer / AdoptBuffer | passthrough; nothing imported |
/// | Vulkan   | ImportBuffer | GL imports the exported allocation |
///
/// The reverse direction -- an OpenGL producer feeding a Vulkan consumer -- has
/// no entry here and none in the Vulkan arena either, because it is impossible
/// rather than unimplemented: OpenGL cannot export an allocation for another
/// API to import. A Vulkan-backed Storm asked for a GL arena gets null from
/// Hgi::GetExternalBufferArena and must copy through the CPU.
///
/// \section Ownership
///
/// RegisterBuffer and AdoptBuffer differ only in who deletes the GL name, and
/// the difference matters. A pooled buffer that an application still draws
/// with and recycles on its own schedule must be *registered*: deleting its
/// name from here would be a double free, and GL may hand the freed name back
/// out for an unrelated allocation. AdoptBuffer is for a buffer the
/// application is genuinely handing over and will never touch again.
///
/// Registration is the weaker contract of the two, and deliberately so for
/// now: the application promises the buffer outlives Hgi's use of it, and
/// nothing here enforces that promise. SetKeepalive covers it for an
/// application whose buffer object is reference counted. An application that
/// pools and overwrites its buffers needs a post-retire release signal
/// instead, which this API does not have yet -- so such a producer must not
/// recycle a registered buffer while any draw that named it may still be in
/// flight. Allocating out of the arena sidesteps the question entirely.
///
/// \section Sync
///
/// Sharing one GL context with the producer needs no semaphores at all: the
/// commands run in order, which is what a semaphore would have established.
/// That is the common case and the arena's default. A producer on a *different*
/// GL context still needs synchronization even inside a share group -- a share
/// group makes objects visible but does not order commands -- and a producer
/// that writes through compute, image stores or transform feedback owes a
/// glMemoryBarrier of its own, since GL's automatic hazard tracking does not
/// cover incoherent writes.
///
/// A Vulkan producer supplies semaphores it exported, which
/// ImportSemaphores() brings in; from then on the ordinary arena bracket
/// applies.
///
class HgiGLExternalBufferArena final : public HgiExternalBufferArena
{
public:
    /// Whether \p hgi can consume external buffers through this arena, i.e.
    /// whether it is an HgiGL. See Hgi::GetExternalBufferArena.
    HGIGL_API
    static bool IsSupportedBy(Hgi *hgi);

    /// Constructed by Hgi::GetExternalBufferArena.
    HGIGL_API
    HgiGLExternalBufferArena(Hgi *hgi);

    HGIGL_API
    ~HgiGLExternalBufferArena() override;

    HGIGL_API
    HgiExternalBufferSharedPtr AllocateBuffer(
        size_t byteSize,
        HgiBufferUsage usage,
        std::string const &debugName = std::string()) override;

    /// Share the existing GL buffer \p bufferId WITHOUT transferring
    /// ownership: the arena binds and reads it, and never deletes it. This is
    /// the entry point for an application-owned, pooled viewport buffer. See
    /// the ownership notes above for the contract it carries.
    HGIGL_API
    HgiExternalBufferSharedPtr RegisterBuffer(
        uint32_t bufferId,
        size_t byteSize,
        HgiBufferUsage usage);

    /// Hand the existing GL buffer \p bufferId over to the arena, which will
    /// delete it once nothing references it and the GPU has retired the work
    /// that named it. Only correct for a buffer the application will never
    /// touch again; use RegisterBuffer otherwise.
    HGIGL_API
    HgiExternalBufferSharedPtr AdoptBuffer(
        uint32_t bufferId,
        size_t byteSize,
        HgiBufferUsage usage);

    /// Import the foreign allocation described by \p desc and bind a GL buffer
    /// over it. Returns null when this GL implementation cannot import
    /// external memory, or the import fails, in which case the caller copies
    /// instead.
    HGIGL_API
    HgiExternalBufferSharedPtr ImportBuffer(
        HgiGLImportBufferDesc const &desc);

    /// Import the producer's exported semaphore pair, replacing whatever this
    /// arena was using. \p appDoneHandle is signalled by the producer when its
    /// writes are done, \p hgiDoneHandle is signalled by Hgi when its reads
    /// are. Either may be 0 for "none".
    ///
    /// Returns false, and leaves the arena unsynchronized, when GL cannot
    /// import them -- which is also the answer for a timeline semaphore, since
    /// GL_EXT_semaphore is binary only. Callers that cannot proceed without
    /// synchronization should treat false as "copy instead".
    HGIGL_API
    bool ImportSemaphores(
        uint64_t appDoneHandle,
        uint64_t hgiDoneHandle,
        HgiExternalHandleType handleType,
        HgiSemaphoreKind kind) override;

protected:
    HGIGL_API
    uint64_t _CaptureSubmissionStamp() override;

    HGIGL_API
    bool _IsSubmissionRetired(uint64_t stamp) override;

private:
    HgiGLExternalBufferArena() = delete;
    HgiGLExternalBufferArena(
        const HgiGLExternalBufferArena &) = delete;
    HgiGLExternalBufferArena & operator=(
        const HgiGLExternalBufferArena &) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HGI_GL_EXTERNAL_BUFFER_ARENA_H
