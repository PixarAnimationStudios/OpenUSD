//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_SEMAPHORE_H
#define PXR_IMAGING_HGI_SEMAPHORE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"

#include <cstdint>
#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HgiExternalBuffer;

/// \enum HgiSemaphoreKind
///
/// Flavour of a cross-queue synchronization primitive.
///
/// Binary is the interop lowest common denominator: OpenGL can only create or
/// import binary semaphores, because GL_EXT_semaphore has no timeline form.
///
/// Timeline is currently NOT IMPLEMENTED by any backend, including Vulkan.
/// The enumerator exists because the interface -- and
/// HgiExternalBufferArena's value bookkeeping behind it -- is designed for it,
/// but HgiVulkanSemaphore encodes through the command queue's pending wait and
/// signal lists, and those carry no values.  Every creation and import path
/// therefore refuses a timeline kind rather than returning a semaphore that
/// cannot be submitted correctly.  Supporting it means teaching the queue to
/// carry values, not changing anything here.
enum HgiSemaphoreKind
{
    HgiSemaphoreKindBinary = 0,
    HgiSemaphoreKindTimeline,
};

/// \class HgiSemaphore
///
/// Wraps a cross-queue synchronization object used to order an application's
/// access to a shared buffer against Hgi's.  This is deliberately not a
/// general-purpose sync primitive: an intra-Storm primitive (a split barrier,
/// say) has different needs and belongs in its own hierarchy.
///
/// Waits and signals are *encoded into the command stream* -- they never block
/// the host.  Vulkan appends to the queue's pending wait/signal list, and GL
/// issues glWaitSemaphoreEXT / glSignalSemaphoreEXT, which are server-side
/// operations on the context's command stream (the signal also implies a
/// flush).  So the same bracket maps onto both APIs.
///
/// The buffer list exists because GL needs it: glWaitSemaphoreEXT and
/// glSignalSemaphoreEXT take explicit buffer and texture barrier arrays, and
/// that is how the external-object extension establishes memory coherency for
/// the imported objects.  Vulkan ignores the list -- its memory visibility
/// comes from the semaphore alone.  Callers pass the external buffers whose
/// memory the wait or signal must cover; HgiExternalBufferArena does this for
/// its own buffers.
///
class HgiSemaphore
{
public:
    HGI_API
    virtual ~HgiSemaphore();

    /// Which flavour of semaphore this is.
    HgiSemaphoreKind GetKind() const {
        return _kind;
    }

    /// Queue a wait on this semaphore ahead of the commands Hgi is about to
    /// record, so they do not run before the signaller is done.  Does not
    /// block the calling thread.
    ///
    /// For a timeline semaphore \p value is the value to wait for; binary
    /// semaphores ignore it.  \p buffers are the external buffers whose memory
    /// this wait must make visible (GL only; see the class documentation).
    HGI_API
    virtual void EncodeWait(
        uint64_t value,
        std::vector<HgiExternalBuffer *> const &buffers) = 0;

    /// Queue a signal on this semaphore after the commands Hgi has recorded,
    /// so a waiter learns when they have completed.  Does not block.
    ///
    /// For a timeline semaphore \p value is the value to signal; binary
    /// semaphores ignore it.  \p buffers are as for EncodeWait.
    HGI_API
    virtual void EncodeSignal(
        uint64_t value,
        std::vector<HgiExternalBuffer *> const &buffers) = 0;

protected:
    HGI_API
    explicit HgiSemaphore(HgiSemaphoreKind kind);

private:
    HgiSemaphore() = delete;
    HgiSemaphore(const HgiSemaphore &) = delete;
    HgiSemaphore & operator=(const HgiSemaphore &) = delete;

    HgiSemaphoreKind _kind;
};

using HgiSemaphoreSharedPtr = std::shared_ptr<HgiSemaphore>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HGI_SEMAPHORE_H
