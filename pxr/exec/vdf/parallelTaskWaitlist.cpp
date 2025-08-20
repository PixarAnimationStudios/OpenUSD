//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/parallelTaskWaitlist.h"

#include "pxr/base/arch/threads.h"
#include "pxr/base/work/taskGraph.h"

#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

// A sentinel used to atomically plug a waiting queue. The presence of this tag
// signals that the list is closed, and that all the waiting tasks have been
// notified.
static VdfParallelTaskWaitlist::Node * const _NotifiedTag =
    reinterpret_cast<VdfParallelTaskWaitlist::Node *>(uintptr_t(-1));

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

struct VdfParallelTaskWaitlist::Node {

    Node(WorkTaskGraph::BaseTask *t, Node *n) : task(t), next(n) {}

    // The waiting task.
    WorkTaskGraph::BaseTask *task;

    // The next node in the queue.
    Node *next;
};

VdfParallelTaskWaitlist::VdfParallelTaskWaitlist(size_t numReserved)
{
    // Reserve some entries on the allocator for waiting nodes.
    if (numReserved) {
        _allocator.reserve(numReserved);
    }
}

VdfParallelTaskWaitlist::~VdfParallelTaskWaitlist() = default;

void
VdfParallelTaskWaitlist::Rewind()
{
    // The allocator is used as scratch pad memory, so we have to rewind it
    // everytime all the lists have been processed.
    _allocator.clear();
}

bool
VdfParallelTaskWaitlist::WaitOn(
    HeadPtr *headPtr, WorkTaskGraph::BaseTask *successor)
{
    // Get the head of the waiting queue.
    Node *headNode = headPtr->load(std::memory_order_acquire);

    // If the dependent is done, we can return immediately.
    if (headNode == _NotifiedTag) {
        return false;
    }

    // Exponentially back off on the atomic free head under high contention.
    _AtomicBackoff backoff;

    // Increment the reference count of the successor task to indicate that
    // it has one more unfulfilled dependency.
    successor->AddChildReference();

    // Allocate a new node to be added to the waiting queue.
    Node *newHead = _AllocateNode(successor, headNode);

    // Atomically set the new waiting task as the head of the queue. If the CAS
    // fails, fix up the pointer to the next entry and retry.
    while (!headPtr->compare_exchange_weak(headNode, newHead)) {
        // If in the meantime the dependency has been satisfied, we can no
        // longer queue up the waiting task, because there is no guarantee that
        // another thread has not already signaled all the queued up tasks.
        // Instead, we immediately signal the successor and bail out.
        if (headNode == _NotifiedTag) {
            successor->RemoveChildReference();
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
VdfParallelTaskWaitlist::CloseAndNotify(
    HeadPtr *headPtr, WorkTaskGraph *taskGraph)
{
    // Get the the head of the waiting queue and replace it with the
    // notified tag to indicate that this queue is now closed.
    Node *headNode = headPtr->exchange(_NotifiedTag);

    // If the queue was already closed, return false.
    if (headNode == _NotifiedTag) {
        return false;
    }

    // Iterate over all the entries in the queue to notify the waiting tasks.
    while (headNode) {
        // Spawn the waiting task if its reference count reaches 0. If the
        // reference count is greater than 0, the task still has unfulfilled
        // dependencies and will be spawn later when the last dependency has
        // been fulfilled.
        if (headNode->task->RemoveChildReference() == 0) {
            taskGraph->RunTask(headNode->task);
        }

        // Move on to the next entry in the queue.
        headNode = headNode->next;
    };

    return true;
}

VdfParallelTaskWaitlist::Node *
VdfParallelTaskWaitlist::_AllocateNode(
    WorkTaskGraph::BaseTask *task, Node *next)
{
    return &(*_allocator.emplace_back(task, next));
}

PXR_NAMESPACE_CLOSE_SCOPE
