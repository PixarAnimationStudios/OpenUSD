//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/compilerTaskSyncBase.h"

#include "pxr/exec/exec/compilationTask.h"

#include "pxr/base/arch/threads.h"
#include "pxr/base/work/dispatcher.h"

#include <functional>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

Exec_CompilerTaskSyncBase::_WaitlistNode * const
Exec_CompilerTaskSyncBase::_NotifiedTag =
    reinterpret_cast<Exec_CompilerTaskSyncBase::_WaitlistNode *>(uintptr_t(-1));

namespace {

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

struct Exec_CompilerTaskSyncBase::_WaitlistNode {
    _WaitlistNode(Exec_CompilationTask *t, _WaitlistNode *n)
        : task(t), next(n) {}

    // The waiting task.
    Exec_CompilationTask *task;

    // The next node in the queue.
    _WaitlistNode *next;
};

Exec_CompilerTaskSyncBase::Exec_CompilerTaskSyncBase(WorkDispatcher &dispatcher)
    : _dispatcher(dispatcher)
{}

Exec_CompilerTaskSyncBase::~Exec_CompilerTaskSyncBase() = default;

Exec_CompilerTaskSyncBase::ClaimResult
Exec_CompilerTaskSyncBase::_Claim(
    _Waitlist *const waitlist,
    Exec_CompilationTask *task)
{
    // If the task associated with this waitlist is already done, return here.
    uint8_t state = waitlist->state.load(std::memory_order_acquire);
    if (state == _TaskStateDone) {
        return ClaimResult::Done;
    }

    // If the task has not been claimed yet, attempt to claim it by CAS and
    // return the result. Note if the caller claims this key, then the provided
    // task is *not* added to the waitlist.
    if (state == _TaskStateUnclaimed &&
        waitlist->state.compare_exchange_strong(state, _TaskStateClaimed)) {
        return ClaimResult::Claimed;
    }

    // If we get here, the task has already been claimed, or the CAS failed and
    // another task got to claim it just before we did. In this case, wait on
    // the task completion. If we fail to wait on the task, it completed just
    // as we were about to wait and we can consider it done!
    return _WaitOn(&waitlist->waiting, task)
        ? ClaimResult::Wait
        : ClaimResult::Done;
}

Exec_CompilerTaskSyncBase::WaitResult
Exec_CompilerTaskSyncBase::_WaitOn(
    _Waitlist *const waitlist,
    Exec_CompilationTask *task)
{
    // If the task associated with this waitlist is already done, return here.
    const uint8_t state = waitlist->state.load(std::memory_order_acquire);
    if (state == _TaskStateDone) {
        return WaitResult::Done;
    }

    // Atomically add task to the waitlist, or return Done if the waitlist has
    // been closed.
    return _WaitOn(&waitlist->waiting, task)
        ? WaitResult::Wait
        : WaitResult::Done;
}

bool
Exec_CompilerTaskSyncBase::_MarkDone(_Waitlist *const waitlist)
{
    // Set the state to done.
    const uint8_t previousState = waitlist->state.exchange(_TaskStateDone);
    if (previousState == _TaskStateDone) {
        return false;
    }

    // Close the waiting queue and notify all waiting tasks. We expect to be
    // the first to close the queue.
    const bool closed = _CloseAndNotify(&waitlist->waiting);
    TF_VERIFY(closed);
    return true;
}

bool
Exec_CompilerTaskSyncBase::_WaitOn(
    std::atomic<_WaitlistNode*> *headPtr,
    Exec_CompilationTask *task)
{
    // Get the head of the waiting queue.
    _WaitlistNode *headNode = headPtr->load(std::memory_order_acquire);

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
    _WaitlistNode *newHead = _AllocateNode(task, headNode);

    // Atomically set the new waiting task as the head of the queue. If the CAS
    // fails, fix up the pointer to the next node and retry.
    while (!headPtr->compare_exchange_weak(headNode, newHead)) {
        // If in the meantime the dependency has been satisfied, we can no
        // longer queue up the waiting task, because there is no guarantee that
        // another thread has not already signaled all the queued up tasks.
        // Instead, we immediately signal the task and bail out.
        if (headNode == _NotifiedTag) {
            task->RemoveDependency();
            return false;
        }

        // Fix up the pointer to the next node, with the up-to-date head of the
        // queue.
        newHead->next = headNode;

        // Backoff on the atomic under high contention.
        backoff.Pause();
    }

    // Task is now successfully waiting.
    return true;
}

bool
Exec_CompilerTaskSyncBase::_CloseAndNotify(std::atomic<_WaitlistNode*> *headPtr)
{
    // Get the the head of the waiting queue and replace it with the
    // notified tag to indicate that this queue is now closed.
    _WaitlistNode *headNode = headPtr->exchange(_NotifiedTag);

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
            _dispatcher.Run(std::ref(*headNode->task));
        }

        // Move on to the next node in the waitlist.
        headNode = headNode->next;
    };

    return true;
}

Exec_CompilerTaskSyncBase::_WaitlistNode *
Exec_CompilerTaskSyncBase::_AllocateNode(
    Exec_CompilationTask *task,
    _WaitlistNode *next)
{
    return &(*_allocator.emplace_back(task, next));
}

PXR_NAMESPACE_CLOSE_SCOPE
