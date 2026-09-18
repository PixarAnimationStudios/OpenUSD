//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_EXTERNAL_BUFFER_ARENA_H
#define PXR_IMAGING_HGI_EXTERNAL_BUFFER_ARENA_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/externalBuffer.h"
#include "pxr/imaging/hgi/semaphore.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;

/// \struct HgiExternalBufferArenaUsage
///
/// Diagnostics for one arena: how many buffers it is keeping alive, how much
/// memory they account for, and how many are waiting on the GPU to retire
/// before they can be destroyed.
struct HgiExternalBufferArenaUsage
{
    size_t numBuffers = 0;
    size_t numPendingDestroy = 0;
    size_t totalByteSize = 0;
};

/// \class HgiExternalBufferArena
///
/// Creates, owns and reclaims the HgiExternalBuffers for one source of
/// application-allocated GPU memory, and owns the single semaphore pair that
/// orders that application's access against Hgi's.
///
/// Obtain one from Hgi::GetExternalBufferArena(), which creates it on demand
/// and returns null when this Hgi cannot interop with the requested arena
/// type.  That null *is* the negotiation step: the application and Hgi settle
/// on an interop format once, when the arena is created, rather than
/// rediscovering per frame whether a given buffer can be shared.  A source
/// device gets its own arena, so handles from two devices never mix.
///
/// \section Sync
///
/// There is one semaphore pair per arena, not per buffer.  The application
/// signals "app done" after finishing its writes and before calling into Hgi;
/// Hgi waits on it before the first thing it does that reads shared memory,
/// and signals "Hgi done" afterwards, which the application waits on before
/// touching shared memory again.  The granularity is therefore the whole
/// arena: the application cannot touch *any* buffer in it until Hgi is done,
/// even buffers Hgi never looked at.  That costs some overlap and buys a great
/// deal of simplicity.
///
/// The wait and signal are encoded by the *consumer's commit*, not by
/// Hgi::StartFrame() / EndFrame().  Those are documented as optional, are
/// driven by whichever hdx tasks happen to be in the task list, and are
/// emitted more than once per application frame by clients that run several
/// render passes or a pick pass -- so a mandatory wait placed there either
/// never runs or runs more times than the application signalled.
///
/// Repetition is handled here rather than being left to the application to get
/// right.  NotifyAppDone() opens an epoch; EncodeAppDoneWait() encodes a wait
/// only for an epoch it has not waited for yet, so N render passes in one
/// application frame produce one wait, and an idle application that published
/// nothing produces none -- which matters, because waiting on a binary
/// semaphore nobody is going to signal hangs the frame.
///
class HgiExternalBufferArena
{
public:
    HGI_API
    virtual ~HgiExternalBufferArena();

    /// The Hgi that consumes buffers from this arena.
    Hgi *GetHgi() const {
        return _hgi;
    }

    /// The application-side device or context this arena's buffers come from,
    /// as a uint64 cast of the native device or context pointer.  This is the
    /// arena's identity along with its type: pointer identity is what actually
    /// decides whether a native handle means anything here, which a physical
    /// device UUID cannot -- two logical devices on one GPU share a UUID and
    /// hand out unrelated handles.
    uint64_t GetRawSourceDevice() const {
        return _rawSourceDevice;
    }

    /// Allocate a buffer out of this arena.  The arena owns and frees it; the
    /// returned reference co-owns it.  Use this when Hgi, rather than the
    /// application, should own the allocation -- it sidesteps the question of
    /// who may delete a shared buffer entirely.  Returns null on failure.
    HGI_API
    virtual HgiExternalBufferSharedPtr AllocateBuffer(
        size_t byteSize,
        HgiBufferUsage usage,
        std::string const &debugName = std::string()) = 0;

    /// The semaphore the application signals when it has finished writing, and
    /// Hgi waits on.  Null when this arena needs no sync -- the usual case when
    /// the application and Hgi share one context, where command order already
    /// sequences the accesses.
    HgiSemaphoreSharedPtr const &GetAppDoneSemaphore() const {
        return _appDoneSemaphore;
    }

    /// The semaphore Hgi signals when it has finished reading, and the
    /// application waits on before overwriting shared memory.  Null when this
    /// arena needs no sync.
    HgiSemaphoreSharedPtr const &GetHgiDoneSemaphore() const {
        return _hgiDoneSemaphore;
    }

    /// Import the application's exported semaphore pair, replacing whatever
    /// this arena was using. \p appDoneHandle is signalled by the application
    /// when its writes are done, \p hgiDoneHandle by Hgi when its reads are;
    /// either may be 0 for "none".
    ///
    /// This is where the application and Hgi settle on a synchronization
    /// format, once, rather than rediscovering per frame whether it will work.
    /// Returns false -- and leaves the arena unsynchronized -- when this
    /// backend cannot import them, which includes a timeline semaphore handed
    /// to an OpenGL consumer, since GL_EXT_semaphore is binary only. A caller
    /// that cannot proceed unsynchronized should read false as "copy instead".
    ///
    /// Default: unsupported.
    HGI_API
    virtual bool ImportSemaphores(
        uint64_t appDoneHandle,
        uint64_t hgiDoneHandle,
        HgiExternalHandleType handleType,
        HgiSemaphoreKind kind);

    /// Called by the application after it has signalled the app-done semaphore
    /// (or, with a timeline semaphore, after signalling \p value), to tell the
    /// arena there is a new signal for Hgi to wait on.  Without this the arena
    /// cannot distinguish "the application published something" from "the
    /// application is idle", and must assume idle rather than risk a wait that
    /// never completes.
    ///
    /// Thread safety: safe to call from any thread.
    HGI_API
    void NotifyAppDone(uint64_t value = 0);

    /// Encode a wait on the app-done semaphore ahead of the commands the
    /// consumer is about to record.  Call this from the consumer's commit,
    /// before importing, copying out of, or drawing with any buffer from this
    /// arena.  Does nothing when there is no semaphore, or when the current
    /// epoch has already been waited for (see the class documentation).
    HGI_API
    void EncodeAppDoneWait();

    /// Encode a signal on the hgi-done semaphore after the commands the
    /// consumer just recorded.  Call this from the consumer's commit, after
    /// the import / copy / draw.  Does nothing when there is no semaphore, or
    /// when this epoch was not waited for -- signalling a binary semaphore
    /// twice with no intervening wait is not meaningful.
    HGI_API
    void EncodeHgiDoneSignal();

    /// Reclaim buffers nobody is using any more.
    ///
    /// Two stages, and the second is the one that matters.  A buffer whose only
    /// remaining reference is the arena's is moved to a pending list, stamped
    /// with the GPU work in flight at that moment.  It is destroyed on a later
    /// pass, once that work has retired -- because a reference count reaching
    /// zero only says the CPU has let go, and destroying a buffer a submitted
    /// draw still names is a use-after-free.
    ///
    /// The arena releases its own reference only here, and only after the
    /// retire test.  That is deliberate and worth relying on: it makes
    /// expiry of a weak reference a sound "the GPU is done with this too"
    /// signal, which it would not be if the reference were dropped as soon as
    /// the count fell.
    ///
    /// Thread safety: not thread safe.  Call on the thread that owns the
    /// device, at a commit or frame boundary.
    HGI_API
    void GarbageCollect();

    /// Diagnostics; see HgiExternalBufferArenaUsage.
    HGI_API
    HgiExternalBufferArenaUsage GetUsage() const;

protected:
    HGI_API
    HgiExternalBufferArena(Hgi *hgi, uint64_t rawSourceDevice);

    /// Take ownership of \p buffer and return the caller's co-owning
    /// reference.  Subclasses call this from AllocateBuffer and from their own
    /// adopt / register entry points, which are typed on native handles and so
    /// cannot live on this base class.
    HGI_API
    HgiExternalBufferSharedPtr _Register(HgiExternalBufferSharedPtr buffer);

    /// Set the arena's semaphore pair.  Subclasses call this during
    /// construction; leaving them null means "this arena needs no sync".
    HGI_API
    void _SetSemaphores(
        HgiSemaphoreSharedPtr appDone,
        HgiSemaphoreSharedPtr hgiDone);

    /// A process-unique id for constructing the HgiBufferHandle that wraps an
    /// external buffer. Drawn from Hgi's own counter on purpose: HgiHandle
    /// equality is id-only, so a separate counter would restart at 1 and hand
    /// out ids that make two distinct buffers compare equal in a renderer's
    /// binding and aggregation caches.
    HGI_API
    uint64_t _GetNextBufferHandleId();

    /// Capture an opaque stamp naming the GPU work currently in flight that
    /// could still name a buffer from this arena -- a Vulkan submission index,
    /// a GL fence sync.
    HGI_API
    virtual uint64_t _CaptureSubmissionStamp() = 0;

    /// Whether the work named by \p stamp has completed.  Must not block; a
    /// stamp that is not retired yet is simply retested on the next
    /// GarbageCollect().  Implementations should release any resource the
    /// stamp holds (a GL sync object) when returning true.
    HGI_API
    virtual bool _IsSubmissionRetired(uint64_t stamp) = 0;

private:
    HgiExternalBufferArena() = delete;
    HgiExternalBufferArena(const HgiExternalBufferArena &) = delete;
    HgiExternalBufferArena & operator=(
        const HgiExternalBufferArena &) = delete;

    // The external buffers whose memory the arena's waits and signals must
    // cover. GL needs these listed explicitly; see HgiSemaphore.
    std::vector<HgiExternalBuffer *> _GetBufferBarrierList() const;

    // The buffers one GarbageCollect() pass stopped counting as live, and the
    // stamp naming the GPU work that could still have been reading them at
    // that moment. Batched per pass rather than per buffer so the stamp's own
    // lifetime is unambiguous -- see GarbageCollect.
    struct _PendingDestroyBatch
    {
        std::vector<HgiExternalBufferSharedPtr> buffers;
        uint64_t stamp = 0;
    };

    Hgi *_hgi;
    uint64_t _rawSourceDevice;

    HgiSemaphoreSharedPtr _appDoneSemaphore;
    HgiSemaphoreSharedPtr _hgiDoneSemaphore;

    // The epoch the application last published, the epoch Hgi last waited
    // for, and the value Hgi last signalled. Guarded by _syncMutex because
    // NotifyAppDone comes from the application's thread while the encodes
    // happen on the consumer's.
    uint64_t _appDoneEpoch = 0;
    uint64_t _appDoneValue = 0;
    uint64_t _waitedEpoch = 0;
    uint64_t _hgiDoneValue = 0;
    mutable std::mutex _syncMutex;

    // Every buffer this arena owns, and those waiting on the GPU before they
    // can be destroyed. Guarded by _buffersMutex: buffers are created while
    // the consumer syncs, which may be parallel, and reclaimed from the
    // device thread.
    std::vector<HgiExternalBufferSharedPtr> _buffers;
    std::vector<_PendingDestroyBatch> _pendingDestroy;
    mutable std::mutex _buffersMutex;
};

using HgiExternalBufferArenaSharedPtr =
    std::shared_ptr<HgiExternalBufferArena>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HGI_EXTERNAL_BUFFER_ARENA_H
