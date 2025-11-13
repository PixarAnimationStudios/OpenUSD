//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_COMPILER_TASK_SYNC_BASE_H
#define PXR_EXEC_EXEC_COMPILER_TASK_SYNC_BASE_H

#include "pxr/pxr.h"

#include <tbb/concurrent_vector.h>

#include <atomic>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

class Exec_CompilationTask;
class WorkDispatcher;

/// A base class that implements all non-template functionality common to
/// each template instantiation of Exec_CompilerTaskSync.
///
class Exec_CompilerTaskSyncBase
{
public:
    /// The different results for claiming a key.
    enum class ClaimResult {
        /// The task is already done.
        Done,

        /// Another task is currently processing the key and the claimant will
        /// be notified once it is done.
        Wait,

        /// The key has been successfully claimed, and the claimant is on the
        /// hook for completing the work.
        Claimed,
    };

    /// The different results for waiting on a key.
    enum class WaitResult {
        /// The task is already done.
        Done,

        /// The task is not yet marked done.
        Wait,
    };

protected:
    struct _WaitlistNode;

    /// The constructor is protected, so that it can only be invoked by derived
    /// classes.
    ///
    explicit Exec_CompilerTaskSyncBase(WorkDispatcher &dispatcher);

    Exec_CompilerTaskSyncBase(const Exec_CompilerTaskSyncBase &) = delete;
    Exec_CompilerTaskSyncBase &operator=(const Exec_CompilerTaskSyncBase &) =
        delete;

    ~Exec_CompilerTaskSyncBase();

    /// Entries in the map always begin life as unclaimed tasks with no
    /// nodes on their waitlist.
    ///
    /// Derived classes should not concern themselves with the contents of the
    /// _Waitlist structure, but its contents must be known to be stored in a
    /// map.
    ///
    struct _Waitlist {
        _Waitlist() : state(_TaskStateUnclaimed), waiting(nullptr) {}
        std::atomic<uint8_t> state;
        std::atomic<_WaitlistNode*> waiting;
    };

    /// Attempts to claim the \p waitlist, and returns whether the attempt was
    /// successful.
    ///
    /// This method will increment the dependency count of the \p task, if the
    /// waitlist has already been claimed and \p task needs to wait for the
    /// results. Once the dependency is fulfilled, the \p task will be notified
    /// by decrementing its dependency count, and if it reaches zero the \p task
    /// will automatically be spawned.
    ///
    ClaimResult _Claim(_Waitlist *waitlist, Exec_CompilationTask *task);

    /// Marks the task associated with \p waitlist as done.
    /// 
    /// This method will notify any tasks depending on \p waitlist by
    /// decrementing their dependency counts, and spawning them if their
    /// dependency count reaches 0. The waitlist need not already be claimed.
    ///
    void _MarkDone(_Waitlist *waitlist);

    /// Establishes that \p task depends on the task associated with
    /// \p waitlist.
    ///
    /// Unlike Claim, if the task for \p waitlist has not been claimed, the
    /// caller is *not* responsible for creating that task. In that case, a new
    /// waitlist is created for \p waitlist if necessary, and the \p task is
    /// added to it.
    ///
    WaitResult _WaitOn(_Waitlist *waitlist, Exec_CompilationTask *task);

private:
    // Registers \p task as waiting on the list denoted by \p headPtr. The
    // method will return \c false if the list is already closed and task does
    // not need to wait. Returns \c true if the task is now successfully waiting
    // for the list to be closed.
    //
    bool _WaitOn(
        std::atomic<_WaitlistNode*> *headPtr,
        Exec_CompilationTask *task);

    // Closes the list denoted by \p headPtr, and notifies any tasks that are
    // waiting on this list. Returns \c false if the list had already been
    // closed prior to calling CloseAndNotify().
    //
    bool _CloseAndNotify(std::atomic<_WaitlistNode*> *headPtr);

    // Allocate a new node for a waiting queue.
    _WaitlistNode *_AllocateNode(
        Exec_CompilationTask *task,
        _WaitlistNode *next);

private:
    // The various states a task can be in.
    enum _TaskState : uint8_t {
        _TaskStateUnclaimed,
        _TaskStateClaimed,
        _TaskStateDone
    };

    // A sentinel used to atomically plug a waiting queue. The presence of this
    // tag signals that the list is closed, and that all the waiting tasks have
    // been notified.
    static _WaitlistNode *const _NotifiedTag;

    // A simple vector that serves as a way of scratch-allocating new
    // waiting nodes.
    tbb::concurrent_vector<_WaitlistNode> _allocator;

    // Work dispatcher for running tasks that have all their dependencies
    // fulfilled.
    WorkDispatcher &_dispatcher;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif