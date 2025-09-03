//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/compilerTaskSync.h"

#include "pxr/exec/exec/compilationTask.h"
#include "pxr/exec/exec/compiledOutputCache.h"

#include "pxr/base/arch/threads.h"
#include "pxr/base/work/dispatcher.h"

#include <functional>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

// A sentinel used to atomically plug a waiting queue. The presence of this tag
// signals that the list is closed, and that all the waiting tasks have been
// notified.
static Exec_CompilerTaskSync::WaitlistNode * const _NotifiedTag =
    reinterpret_cast<Exec_CompilerTaskSync::WaitlistNode *>(uintptr_t(-1));

namespace {

//
// Instances of this class can be used to back off from atomic variables
// that are under high contention (as determined by repeatedly failing CAS)
//
class _AtomicBackoff {
public:
    _AtomicBackoff() : _counter(1) {}

    // This method introduces a pause after a failed CAS.
    void Pause() {
        // Back off by exponentially increasing a spin wait interval, up to
        // a predetermined number of iterations (should be roughly equal to
        // the cost of a context switch).
        const uint32_t maxSpinCount = 16;
        if (_counter < maxSpinCount) {
            for (size_t i = 0; i < _counter; ++i) {
                ARCH_SPIN_PAUSE();
            }
            _counter *= 2;
        }

        // Force a context switch under very high contention.
        else {
            std::this_thread::yield();
        }
    }

private:
    uint32_t _counter;
};

}

struct Exec_CompilerTaskSync::WaitlistNode {
    WaitlistNode(Exec_CompilationTask *t, WaitlistNode *n) : task(t), next(n) {}

    // The waiting task.
    Exec_CompilationTask *task;

    // The next node in the queue.
    WaitlistNode *next;
};

Exec_CompilerTaskSync::Exec_CompilerTaskSync(WorkDispatcher &dispatcher)
    : _dispatcher(dispatcher)
{}

Exec_CompilerTaskSync::~Exec_CompilerTaskSync() = default;

void
Exec_CompilerTaskSync::Run(Exec_CompilationTask *const task) const
{
    _dispatcher.Run(std::ref(*task));
}

Exec_CompilerTaskSync::ClaimResult
Exec_CompilerTaskSync::Claim(
    const Exec_OutputKey::Identity &key,
    Exec_CompilationTask *task)
{
    // Add the key to the map. If another task got to claiming it first, it's
    // expected and safe for the key to already have an entry.
    const auto &[iterator, inserted] = _claimedTasks.emplace(
        std::piecewise_construct, 
            std::forward_as_tuple(key),
            std::forward_as_tuple());
    _Entry *const entry = &iterator->second;

    // If the task associated with this output is already done, return here.
    uint8_t state = entry->state.load(std::memory_order_acquire);
    if (state == _TaskStateDone) {
        return ClaimResult::Done;
    }

    // If the task has not been claimed yet, attempt to claim it by CAS and
    // return the result.
    else if (state == _TaskStateUnclaimed &&
        entry->state.compare_exchange_strong(state, _TaskStateClaimed)) {
        return ClaimResult::Claimed;
    }

    // If we get here, the task has already been claimed, or the CAS failed and
    // another task got to claim it just before we did. In this case, wait on
    // the task completion. If we fail to wait on the task, it completed just
    // as we were about to wait and we can consider it done!
    const ClaimResult claimResult = _WaitOn(&entry->waiting, task)
        ? ClaimResult::Wait
        : ClaimResult::Done;
    return claimResult;
}

void
Exec_CompilerTaskSync::MarkDone(const Exec_OutputKey::Identity &key)
{
    // Note, some of these TF_VERIFYs can be safely relaxed if we later
    // want to mark tasks done from tasks that aren't the original claimaints.

    // We expect the publishing task to have previously claimed this key, so
    // there should already be an entry in the map.
    const auto iterator = _claimedTasks.find(key);
    if (!TF_VERIFY(iterator != _claimedTasks.end())) {
        return;
    }
    _Entry *const entry = &iterator->second;

    // Set the state to done. We expect this to transition from the claimed
    // state.
    const uint8_t previousState = entry->state.exchange(_TaskStateDone);
    TF_VERIFY(previousState == _TaskStateClaimed);

    // Close the waiting queue and notify all waiting tasks. We expect to be
    // the first to close the queue.
    const bool closed = _CloseAndNotify(&entry->waiting);
    TF_VERIFY(closed);   
}

bool
Exec_CompilerTaskSync::_WaitOn(
    std::atomic<WaitlistNode*> *headPtr,
    Exec_CompilationTask *task)
{
    // Get the head of the waiting queue.
    WaitlistNode *headNode = headPtr->load(std::memory_order_acquire);

    // If the dependent is done, we can return immediately.
    if (headNode == _NotifiedTag) {
        return false;
    }

    // Exponentially back off on the atomic free head under high contention.
    _AtomicBackoff backoff;

    // Increment the dependency count of the task to indicate that it has one
    // more unfulfilled dependency.
    task->AddDependency();

    // Allocate a new node to be added to the waiting queue.
    WaitlistNode *newHead = _AllocateNode(task, headNode);

    // Atomically set the new waiting task as the head of the queue. If the CAS
    // fails, fix up the pointer to the next entry and retry.
    while (!headPtr->compare_exchange_weak(headNode, newHead)) {
        // If in the meantime the dependency has been satisfied, we can no
        // longer queue up the waiting task, because there is no guarantee that
        // another thread has not already signaled all the queued up tasks.
        // Instead, we immediately signal the task and bail out.
        if (headNode == _NotifiedTag) {
            task->RemoveDependency();
            return false;
        }

        // Fix up the pointer to the next entry, with the up-to-date head of
        // the queue.
        newHead->next = headNode;

        // Backoff on the atomic under high contention.
        backoff.Pause();
    }

    // Task is now successfully waiting.
    return true;
}

bool
Exec_CompilerTaskSync::_CloseAndNotify(std::atomic<WaitlistNode*> *headPtr)
{
    // Get the the head of the waiting queue and replace it with the
    // notified tag to indicate that this queue is now closed.
    WaitlistNode *headNode = headPtr->exchange(_NotifiedTag);

    // If the queue was already closed, return false.
    if (headNode == _NotifiedTag) {
        return false;
    }

    // Iterate over all the entries in the queue to notify the waiting tasks.
    while (headNode) {
        // Spawn the waiting task if its dependency count reaches 0. If the
        // dependency count is greater than 0, the task still has unfulfilled
        // dependencies and will be spawn later when the last dependency has
        // been fulfilled.
        if (headNode->task->RemoveDependency() == 0) {
            Run(headNode->task);
        }

        // Move on to the next entry in the queue.
        headNode = headNode->next;
    };

    return true;
}

Exec_CompilerTaskSync::WaitlistNode *
Exec_CompilerTaskSync::_AllocateNode(
    Exec_CompilationTask *task,
    WaitlistNode *next)
{
    return &(*_allocator.emplace_back(task, next));
}

PXR_NAMESPACE_CLOSE_SCOPE
