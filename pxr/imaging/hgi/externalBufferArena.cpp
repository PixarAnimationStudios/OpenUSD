//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/externalBufferArena.h"
#include "pxr/imaging/hgi/hgi.h"

#include "pxr/base/tf/diagnostic.h"

#include <algorithm>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

HgiExternalBufferArena::HgiExternalBufferArena(Hgi *hgi)
    : _hgi(hgi)
{
}

HgiExternalBufferArena::~HgiExternalBufferArena() = default;

HgiExternalBufferSharedPtr
HgiExternalBufferArena::_Register(HgiExternalBufferSharedPtr buffer)
{
    if (!buffer) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(_buffersMutex);
    _buffers.push_back(buffer);
    return buffer;
}

uint64_t
HgiExternalBufferArena::_GetNextBufferHandleId()
{
    return _hgi->GetUniqueId();
}

void
HgiExternalBufferArena::_SetSemaphores(
    HgiSemaphoreSharedPtr appDone,
    HgiSemaphoreSharedPtr hgiDone)
{
    _appDoneSemaphore = std::move(appDone);
    _hgiDoneSemaphore = std::move(hgiDone);
}

bool
HgiExternalBufferArena::CreateExportableSemaphores(
    HgiSemaphoreKind /*kind*/,
    uint64_t *outAppDoneHandle,
    uint64_t *outHgiDoneHandle)
{
    if (outAppDoneHandle) {
        *outAppDoneHandle = 0;
    }
    if (outHgiDoneHandle) {
        *outHgiDoneHandle = 0;
    }
    return false;
}

bool
HgiExternalBufferArena::ImportSemaphores(
    uint64_t /*appDoneHandle*/,
    uint64_t /*hgiDoneHandle*/,
    HgiExternalHandleType /*handleType*/,
    HgiSemaphoreKind /*kind*/)
{
    return false;
}

void
HgiExternalBufferArena::NotifyAppDone(uint64_t value)
{
    std::lock_guard<std::mutex> lock(_syncMutex);
    ++_appDoneEpoch;
    _appDoneValue = value;
}

std::vector<HgiExternalBuffer *>
HgiExternalBufferArena::_GetBufferBarrierList() const
{
    std::vector<HgiExternalBuffer *> buffers;
    std::lock_guard<std::mutex> lock(_buffersMutex);
    buffers.reserve(_buffers.size());
    for (HgiExternalBufferSharedPtr const &buffer : _buffers) {
        buffers.push_back(buffer.get());
    }
    return buffers;
}

void
HgiExternalBufferArena::EncodeAppDoneWait()
{
    // Without an app-done semaphore there is nothing to wait on, but the epoch
    // bookkeeping below still has to run: EncodeHgiDoneSignal() only signals
    // for an epoch recorded here, and an arena configured with hgi-done alone
    // would otherwise never signal.
    if (!_appDoneSemaphore && !_hgiDoneSemaphore) {
        return;
    }

    uint64_t value = 0;
    bool previousEpochWentUnsignalled = false;
    {
        std::lock_guard<std::mutex> lock(_syncMutex);
        // Nothing new published since the last wait: either the application is
        // idle or another render pass in this frame already consumed the
        // signal. Waiting again would block on a signal that is not coming.
        if (_appDoneEpoch == _waitedEpoch) {
            return;
        }
        // A successful hgi-done signal zeroes both counters, so a non-zero
        // _waitedEpoch here means exactly one thing: we waited for an earlier
        // publish and nothing ever signalled that we had finished reading it.
        // Worth saying out loud, because the symptom is a producer that
        // silently blocks -- and the cause is a missing call in the host,
        // which no amount of looking at the producer will reveal.
        if (_waitedEpoch != 0 && !_warnedMissingSignal) {
            _warnedMissingSignal = true;
            previousEpochWentUnsignalled = true;
        }
        _waitedEpoch = _appDoneEpoch;
        value = _appDoneValue;
    }

    // Outside the lock: a diagnostic can run arbitrary delegate code, which
    // has no business reentering the arena while we hold its mutex.
    if (previousEpochWentUnsignalled) {
        TF_WARN("External buffer arena: the consumer read a shared buffer but "
                "nothing signalled hgi-done for the previous publish, so the "
                "producer will block waiting for it. The host must call "
                "Hgi::EndFrame() once per application frame, after that "
                "frame's drawing has been submitted, and it is reaching this "
                "wait without having done so. Warned once per arena.");
    }

    if (_appDoneSemaphore) {
        _appDoneSemaphore->EncodeWait(value, _GetBufferBarrierList());
    } else {
        _AcquireBuffersWithoutWait(_GetBufferBarrierList());
    }
}

void
HgiExternalBufferArena::_AcquireBuffersWithoutWait(
    std::vector<HgiExternalBuffer *> const &)
{
}

bool
HgiExternalBufferArena::EncodeHgiDoneSignal()
{
    if (!_hgiDoneSemaphore) {
        return false;
    }

    uint64_t value = 0;
    {
        std::lock_guard<std::mutex> lock(_syncMutex);
        // Pair with the wait: only signal for an epoch we actually waited for,
        // and only once. A binary semaphore signalled twice with no
        // intervening wait says nothing useful, and the application is waiting
        // for one signal per publish.
        if (_appDoneEpoch == 0 || _waitedEpoch != _appDoneEpoch) {
            return false;
        }
        // Consume the epoch so a second pass in the same frame does not
        // re-signal. A later NotifyAppDone opens the next one.
        _waitedEpoch = _appDoneEpoch = 0;
        value = ++_hgiDoneValue;
    }

    _hgiDoneSemaphore->EncodeSignal(value, _GetBufferBarrierList());
    return true;
}

void
HgiExternalBufferArena::GarbageCollect()
{
    std::vector<HgiExternalBufferSharedPtr> destroy;

    {
        std::lock_guard<std::mutex> lock(_buffersMutex);

        // Stage one: a buffer whose only remaining reference is ours is no
        // longer wanted. Stop counting it as live and stamp the GPU work in
        // flight right now -- but keep the reference, because the CPU letting
        // go says nothing about submitted draws that still name it.
        //
        // Everything retired in this pass shares one stamp: they all stopped
        // being referenced at the same moment, so the work that could still
        // name them is the same work. One batch also keeps the stamp's own
        // lifetime simple -- a GL fence is released exactly once, when its
        // batch is released.
        _PendingDestroyBatch batch;
        for (auto it = _buffers.begin(); it != _buffers.end(); ) {
            if (it->use_count() == 1) {
                batch.buffers.push_back(std::move(*it));
                it = _buffers.erase(it);
            } else {
                ++it;
            }
        }
        if (!batch.buffers.empty()) {
            batch.stamp = _CaptureSubmissionStamp();
            _pendingDestroy.push_back(std::move(batch));
        }

        // Stage two: release the batches whose work has retired. Anything not
        // retired stays for a later pass; lagging a few frames is the safe
        // direction.
        for (auto it = _pendingDestroy.begin();
                it != _pendingDestroy.end(); ) {
            if (_IsSubmissionRetired(it->stamp)) {
                // Move the references out and drop them outside the lock: the
                // destructors run there, and a subclass destructor -- or a
                // keepalive deleter -- has no business reentering the arena
                // while we hold its mutex.
                for (HgiExternalBufferSharedPtr &buffer : it->buffers) {
                    destroy.push_back(std::move(buffer));
                }
                it = _pendingDestroy.erase(it);
            } else {
                ++it;
            }
        }
    }

    destroy.clear();
}

void
HgiExternalBufferArena::_Shutdown()
{
    std::vector<HgiExternalBufferSharedPtr> buffers;
    {
        std::lock_guard<std::mutex> lock(_buffersMutex);
        buffers.swap(_buffers);
        for (_PendingDestroyBatch &batch : _pendingDestroy) {
            for (HgiExternalBufferSharedPtr &buffer : batch.buffers) {
                buffers.push_back(std::move(buffer));
            }
        }
        _pendingDestroy.clear();
    }

    size_t numBuffersStillReferenced = 0;
    for (HgiExternalBufferSharedPtr const &buffer : buffers) {
        if (buffer.use_count() > 1) {
            ++numBuffersStillReferenced;
        }
        buffer->_ReleaseResources();
        buffer->_arena = nullptr;
    }
    buffers.clear();

    size_t numSemaphoresStillReferenced = 0;
    for (HgiSemaphoreSharedPtr *semaphore :
            { &_appDoneSemaphore, &_hgiDoneSemaphore }) {
        if (*semaphore) {
            if (semaphore->use_count() > 1) {
                ++numSemaphoresStillReferenced;
            }
            (*semaphore)->_ReleaseResources();
            semaphore->reset();
        }
    }

    _hgi = nullptr;

    if (numBuffersStillReferenced || numSemaphoresStillReferenced) {
        TF_CODING_ERROR("Hgi was destroyed while %zu external buffer(s) and "
                        "%zu semaphore(s) from its arena were still "
                        "referenced. Their GPU resources have been released; "
                        "release every buffer and semaphore before "
                        "destroying Hgi.",
                        numBuffersStillReferenced,
                        numSemaphoresStillReferenced);
    }
}

HgiExternalBufferArenaUsage
HgiExternalBufferArena::GetUsage() const
{
    HgiExternalBufferArenaUsage usage;
    std::lock_guard<std::mutex> lock(_buffersMutex);
    usage.numBuffers = _buffers.size();
    for (_PendingDestroyBatch const &batch : _pendingDestroy) {
        usage.numPendingDestroy += batch.buffers.size();
    }
    for (HgiExternalBufferSharedPtr const &buffer : _buffers) {
        usage.totalByteSize += buffer->GetByteSize();
    }
    for (_PendingDestroyBatch const &batch : _pendingDestroy) {
        for (HgiExternalBufferSharedPtr const &buffer : batch.buffers) {
            usage.totalByteSize += buffer->GetByteSize();
        }
    }
    return usage;
}

PXR_NAMESPACE_CLOSE_SCOPE
