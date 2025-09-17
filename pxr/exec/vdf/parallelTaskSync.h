//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_PARALLEL_TASK_SYNC_H
#define PXR_EXEC_VDF_PARALLEL_TASK_SYNC_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/parallelTaskWaitlist.h"

#include "pxr/base/work/taskGraph.h"

#include <atomic>
#include <cstdint>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfParallelTaskSync
///
/// \brief Instances of this class are used to synchronize dynamic, acyclic 
///        task graphs, allowing tasks to claim dependents for processing.
///        Methods on this class are thread-safe unless specifically called out
///        to not be thread-safe.
///
class VdfParallelTaskSync
{
public:
    /// Noncopyable.
    ///
    VdfParallelTaskSync(const VdfParallelTaskSync &) = delete;
    VdfParallelTaskSync &operator=(const VdfParallelTaskSync &) = delete;

    /// Constructor.
    ///
    VdfParallelTaskSync(WorkTaskGraph *taskGraph) 
        : _waitlists(1000)
        , _num(0)
        , _taskGraph(taskGraph)
    {}

    /// Resets the state of all tasks in the graph. Ensures that \p num
    /// entries are available for use.
    /// 
    /// It is not thread-safe to call this method on the same instance from
    /// multiple threads.
    ///
    VDF_API
    void Reset(const size_t num);

    /// The different states a task can be in.
    ///
    enum class State {
        Done,       /// The task is already done.

        Wait,       /// The task is currently running, the claimant must wait
                    /// for the task to complete.

        Claimed     /// The task has been successfully claimed. The claimant
                    /// can go ahead and process the task.
    };

    /// Claims the task \p idx for processing, and returns the new task state.
    ///
    /// This method will automatically increment the reference count of the
    /// \p successor, if the task has already been claimed, and will cause
    /// the reference count of \p successor to be automatically decremented as
    /// soon as the task completes.
    ///
    inline State Claim(const size_t idx, WorkTaskGraph::BaseTask *successor);

    /// Mark the task \p idx as done. 
    ///
    /// This method will notify any tasks depending on \p idx about the
    /// completion of \p idx.
    ///
    inline void MarkDone(const size_t idx);

private:

    // The different states a task can be in. 
    enum _TaskState : uint8_t {
        _TaskStateUnclaimed,
        _TaskStateClaimed,
        _TaskStateDone
    };

    // A byte-array indicating the state of each task.
    std::unique_ptr<std::atomic<uint8_t>[]> _state;

    // A pointer to the waiting queue head for each task.
    std::unique_ptr<VdfParallelTaskWaitlist::HeadPtr[]> _waiting;

    // The waitlist instance for managing the queues.
    VdfParallelTaskWaitlist _waitlists;

    // The number of tasks in this graph.
    size_t _num;

    // The task graph for running pending tasks. 
    WorkTaskGraph *_taskGraph;

};

///////////////////////////////////////////////////////////////////////////////

VdfParallelTaskSync::State
VdfParallelTaskSync::Claim(const size_t idx, WorkTaskGraph::BaseTask *successor)
{
    // Get the current task state.
    uint8_t state = _state[idx].load(std::memory_order_acquire);

    // If the task has completed, bail out.
    if (state == _TaskStateDone) {
        return State::Done;
    }

    // If the task has not been claimed, yet, attempt to atomically claim it
    // now. Return if this succeeds.
    else if (state == _TaskStateUnclaimed && 
        _state[idx].compare_exchange_strong(state, _TaskStateClaimed)) {
        return State::Claimed;
    }
    
    // If we have to wait, try to enqueue in the waiting list, but bail
    // out if the task completes while attempting to do so.
    return _waitlists.WaitOn(&_waiting[idx], successor)
        ? State::Wait
        : State::Done;
}

void
VdfParallelTaskSync::MarkDone(const size_t idx)
{
    // Mark the task done in the state array.
    _state[idx].store(_TaskStateDone, std::memory_order_release);

    // Close the corresponding wait list and notify all waiting tasks.
    _waitlists.CloseAndNotify(&_waiting[idx], _taskGraph);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
