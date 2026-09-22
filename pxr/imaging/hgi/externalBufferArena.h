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
/// rediscovering per frame whether a given buffer can be shared.
///
/// There is one arena per type per Hgi, which makes ONE PRODUCER PER ARENA a
/// requirement rather than a convention.  Two independent producers sharing an
/// arena share its semaphore pair and its epoch counter, and the epoch counter
/// does not distinguish them: producer A's publish can be consumed by the wait
/// raised on B's behalf, and the signal that follows zeroes the state for
/// both.  On a binary semaphore two publishes against one wait also leave it
/// signalled, so the next frame's wait consumes a stale signal and neither
/// producer is ordered against Hgi at all.  None of this is detected or
/// reported.  An application with two genuinely independent producers needs
/// two Hgis, or has to serialize them into one producer itself.
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
/// That bracket is four operations, and only two of them are the arena's to
/// perform:
///
/// |        | app-done semaphore  | hgi-done semaphore    |
/// | ------ | ------------------- | --------------------- |
/// | signal | the application     | EncodeHgiDoneSignal() |
/// | wait   | EncodeAppDoneWait() | the application       |
///
/// Hgi encodes its two from Hgi::StartFrame() and Hgi::EndFrame()
/// respectively.  The application does its two in its own submission, and
/// reports the signal afterwards with NotifyAppDone().
///
/// The diagonal is the whole explanation: this arena can only encode onto the
/// CONSUMER's queue.  It has no access to the application's, so the two cells
/// on the other diagonal are things the application does for itself, with the
/// semaphores it takes from GetAppDoneHgiSemaphore() /
/// GetHgiDoneHgiSemaphore() or from a backend's native accessors.
///
/// Hence the shape of the API, which is otherwise easy to read as an
/// oversight.  NotifyAppDone() exists because the arena cannot see the
/// application's signal and must not guess: without it there is no way to tell
/// "the application published" from "the application is idle".  There is no
/// counterpart for the application's wait, because nothing here depends on
/// knowing it happened -- and no EncodeHgiDoneWait(), because the arena could
/// not encode one if it wanted to.
///
/// Neither the application nor the consuming renderer encodes the two Hgi
/// cells; both are protected, with Hgi a friend.  Calling them from
/// application code would not fail loudly -- it would consume an epoch, and
/// the frame that needed it would go silently unsynchronized.
///
/// The application's wait is not optional even though nothing here observes
/// it.  Hgi signals hgi-done once per epoch, and a binary semaphore signalled
/// twice with no intervening wait is invalid, so an application that publishes
/// again without waiting corrupts the semaphore's state on top of whatever it
/// does to the buffer.
///
/// Encoding from the frame hooks puts a real obligation on the host in turn:
/// a client that shares buffers must call both, once per application frame,
/// with StartFrame ahead of everything that reads a shared buffer and EndFrame
/// after all of it has been submitted.  A client that does not gets no
/// synchronization whatsoever, and no diagnostic saying so -- the check that
/// would report a missing signal lives inside the wait, which is itself in
/// the hook that was not called.
///
/// A corollary for producers: publish BEFORE StartFrame.  NotifyAppDone()
/// called after it opens an epoch this frame will not wait for, and an epoch
/// that was not waited for is not signalled at EndFrame either, so the
/// producer's next wait for hgi-done slips to the following frame.
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
    ///
    /// Hgi-typed, hence the name: a backend arena may also offer the same
    /// object in its own type system, where the application can use it
    /// directly (HgiVulkanExternalBufferArena::GetAppDoneVkSemaphore).  An
    /// application on the consumer's own device wants that one; this one is
    /// for code that must stay backend-agnostic.
    HgiSemaphoreSharedPtr const &GetAppDoneHgiSemaphore() const {
        return _appDoneSemaphore;
    }

    /// The semaphore Hgi signals when it has finished reading, and the
    /// application waits on before overwriting shared memory.  Null when this
    /// arena needs no sync.
    ///
    /// Hgi-typed; see GetAppDoneHgiSemaphore for the native counterpart.  Note
    /// that Hgi never encodes the application's wait on this -- it has no
    /// access to the application's queue -- so there is deliberately no
    /// EncodeHgiDoneWait().  The application waits in its own submission.
    HgiSemaphoreSharedPtr const &GetHgiDoneHgiSemaphore() const {
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
    /// backend cannot import them, which includes any timeline semaphore, on
    /// every backend; see HgiSemaphoreKind. A caller that cannot proceed
    /// unsynchronized should read false as "copy instead".
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
    // Hgi's half of the bracket. Not public, and not application API: an
    // application signals and waits on its own queue, with the semaphores it
    // gets from GetAppDoneHgiSemaphore() / GetHgiDoneHgiSemaphore(). These two
    // encode onto the CONSUMER's queue, which only Hgi is in a position to do
    // correctly, because only Hgi knows where the frame starts and ends.
    //
    // Calling either from application code would not fail loudly. It would
    // consume an epoch, and the frame that needed it would then silently go
    // unsynchronized -- which is why this is enforced by the compiler rather
    // than by the comment that used to sit here.

    /// Encode a wait on the app-done semaphore ahead of the commands the
    /// consumer is about to record.  Hgi calls this on every arena it owns
    /// from StartFrame(), which is ahead of everything in the frame that
    /// imports, copies out of, or draws with a buffer from this arena.  Does
    /// nothing when there is no semaphore, or when the current epoch has
    /// already been waited for (see the class documentation).
    HGI_API
    void EncodeAppDoneWait();

    /// Encode a signal on the hgi-done semaphore telling the application the
    /// consumer has finished reading this arena's buffers.
    ///
    /// Hgi calls this from EndFrame(), after the frame's drawing has been
    /// submitted. It cannot be encoded any earlier. A directly bound buffer's
    /// reads ARE the draws, and at commit time those have not been recorded
    /// yet, so a signal there claims the reads are finished before they exist.
    /// Nothing inside the consuming renderer can do better: several render
    /// passes run per frame and none of them knows it issued the last one.
    ///
    /// Does nothing when there is no semaphore, or when this epoch was not
    /// waited for -- signalling a binary semaphore twice with no intervening
    /// wait is not meaningful. Returns true only when a signal was actually
    /// encoded, so a caller can skip backend work -- a queue flush, say --
    /// that an application sharing nothing should not pay for every frame.
    HGI_API
    bool EncodeHgiDoneSignal();

    HGI_API
    HgiExternalBufferArena(Hgi *hgi);

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
    // Encodes the bracket from StartFrame/EndFrame. The reverse friendship
    // already exists in hgi.h, so the two classes were coupled before this.
    friend class Hgi;

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

    HgiSemaphoreSharedPtr _appDoneSemaphore;
    HgiSemaphoreSharedPtr _hgiDoneSemaphore;

    // How many times the application has published, bumped by
    // NotifyAppDone(). Only ever compared against _waitedEpoch, never read as
    // a count: together they answer the wait side's one question, is there an
    // unconsumed publish?
    uint64_t _appDoneEpoch = 0;

    // The value the application signalled for the current epoch, carried from
    // NotifyAppDone() through to EncodeWait(). Meaningful for a timeline
    // semaphore; a binary one has no value and ignores it.
    uint64_t _appDoneValue = 0;

    // The epoch EncodeAppDoneWait() last encoded a wait for. Below
    // _appDoneEpoch means a wait is due; equal means the publish has been
    // consumed and a signal is owed.
    //
    // Both this and _appDoneEpoch are zeroed when a signal is emitted, which
    // is what lets "_waitedEpoch != 0 on entry to a wait" mean, precisely,
    // that the previous publish was consumed and never signalled.
    uint64_t _waitedEpoch = 0;

    // The value last signalled on the hgi-done semaphore, incremented per
    // signal so a timeline waiter can tell one frame's signal from the next.
    // Ignored by a binary semaphore.
    uint64_t _hgiDoneValue = 0;

    // One-shot latch for the "nobody signalled hgi-done" diagnostic. Without
    // it the warning would repeat every frame for the life of the arena.
    bool _warnedMissingSignal = false;

    // Guards everything above: NotifyAppDone() is called from the
    // application's thread while the encodes happen on the consumer's.
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
