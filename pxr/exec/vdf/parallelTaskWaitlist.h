//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_PARALLEL_TASK_WAITLIST_H
#define PXR_EXEC_VDF_PARALLEL_TASK_WAITLIST_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"

#include "pxr/base/work/taskGraph.h"

#include <tbb/concurrent_vector.h>

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

/// VdfParallelTaskWaitlist
/// 
/// This class manages lists of tasks waiting on uncompleted work. One instance
/// of this class can manage multiple independent queues denoted by separate
/// VdfParallelTaskWaitlist::HeadPtr instances.
/// 
/// The client is expected to instantiate one or more heads, and then use the
/// methods WaitOn(), to wait on completion of the work denoted by those heads
/// respectively. Once the work has been completed, CloseAndNotify() can be
/// called to close the waiting list denoted by the respective head, and
/// simultanously notify all the currently waiting tasks to continue their
/// execution - assuming their task reference count reaches 0. Tasks with
/// reference counts greater than 0 are still waiting on other, unfulfilled
/// dependencies.
/// 
/// The client is expected to call Rewind() once all heads have been closed and
/// notified. This ensures that the internal state of this class has been reset,
/// and its allocated memory does not grow past invocations of Rewind().
///
class VdfParallelTaskWaitlist
{
public:
    /// Represents a node in one of the waiting queues.
    ///
    struct Node;
    /// This type denotes the head of an independent waitlist. Clients are
    /// expected to instantiate this one of these for each independent list.
    ///
    using HeadPtr = std::atomic<Node*>;

    /// Noncopyable.
    ///
    VdfParallelTaskWaitlist(const VdfParallelTaskWaitlist &) = delete;
    VdfParallelTaskWaitlist &operator=(const VdfParallelTaskWaitlist &) =
        delete;

    /// Constructor.
    /// 
    /// Reserves \p numReserved waiting nodes as an optimization that can
    /// eliminate many smaller allocations for when the approximate size of the
    /// waiting lists is known ahead of time.
    ///
    VDF_API
    explicit VdfParallelTaskWaitlist(size_t numReserved = 0);

    /// Destructor.
    ///
    VDF_API
    ~VdfParallelTaskWaitlist();

    /// Rewind the internal state and ensure that internally allocated memory
    /// does not grow beyond this point.
    ///
    VDF_API
    void Rewind();

    /// Registers \p successor as waiting on the list denoted by \p headPtr.
    /// The method will return \c false if the list is already closed and
    /// successor does not need to wait. Returns \c true if the successor is
    /// now successfully waiting for the list to be closed.
    ///
    VDF_API
    bool WaitOn(HeadPtr *headPtr, WorkTaskGraph::BaseTask *successor);

    /// Closes the list denoted by \p headPtr, and notifies any tasks that are
    /// waiting on this list. Returns \c false if the list had already been
    /// closed prior to calling CloseAndNotify().
    ///
    VDF_API
    bool CloseAndNotify(HeadPtr *headPtr, WorkTaskGraph *taskGraph);

private:
    // Allocate a new node for a waiting queue.
    //
    VDF_API
    Node *_AllocateNode(WorkTaskGraph::BaseTask *task, Node *next);

    // A simple vector that serves as a way of scratch-allocating new
    // waiting nodes.
    //
    tbb::concurrent_vector<Node> _allocator;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
