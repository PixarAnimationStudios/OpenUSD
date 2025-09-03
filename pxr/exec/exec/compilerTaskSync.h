//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_COMPILER_TASK_SYNC_H
#define PXR_EXEC_EXEC_COMPILER_TASK_SYNC_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/outputKey.h"

#include "pxr/base/tf/hash.h"

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>

#include <atomic>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

class Exec_CompilationTask;
class Exec_CompiledOutputCache;
class WorkDispatcher;

/// Instances of this class are used to synchronize compilation task graphs.
/// 
/// Tasks can claim dependent output keys for processing, and depending on the
/// returned result are on the hook for processing the claimed output key, or
/// will be notified when a task previously claiming the same output key
/// is completed.
/// 
/// The lifetime of instances of this class is expected to be limited to one
/// round of compilation.
///
class Exec_CompilerTaskSync
{
public:
    explicit Exec_CompilerTaskSync(WorkDispatcher &dispatcher);

    Exec_CompilerTaskSync(const Exec_CompilerTaskSync &) = delete;
    Exec_CompilerTaskSync &operator=(const Exec_CompilerTaskSync &) =
        delete;

    ~Exec_CompilerTaskSync();

    struct WaitlistNode;

    /// Run a concurrent compilation task on the work dispatcher.
    void Run(Exec_CompilationTask *task) const;

    /// The different results claiming an output key can return.
    enum class ClaimResult {
        Done,       /// The task is already done.

        Wait,       /// Another task is currently processing the output key and
                    /// the claimant will be notified once it is done.
                    
        Claimed     /// The output key has been successfully claimed, and the
                    /// claimant is on the hook for completing the work.
    };

    /// Attempts to claim the output \p key for processing, and returns
    /// whether the attempt was successful.
    /// 
    /// This method will increment the dependency count of the \p task, if the
    /// output key has already been claimed and \p task needs to wait for the
    /// results. Once the dependency is fulfilled, the \p task will be notified
    /// by decrementing its dependency count, and if it reaches zero the \p task
    /// will automatically be spawned.
    ///
    ClaimResult Claim(
        const Exec_OutputKey::Identity &key,
        Exec_CompilationTask *task);

    /// Marks the task associated with the output \p key done.
    /// 
    /// This method will notify any tasks depending on \p key by decrementing
    /// their dependency counts, and spawning them if their dependency count
    /// reaches 0.
    ///
    void MarkDone(const Exec_OutputKey::Identity &key);

private:
    // Registers \p task as waiting on the list denoted by \p headPtr. The
    // method will return \c false if the list is already closed and task does
    // not need to wait. Returns \c true if the task is now successfully waiting
    // for the list to be closed.
    //
    bool _WaitOn(
        std::atomic<WaitlistNode*> *headPtr,
        Exec_CompilationTask *task);

    // Closes the list denoted by \p headPtr, and notifies any tasks that are
    // waiting on this list. Returns \c false if the list had already been
    // closed prior to calling CloseAndNotify().
    //
    bool _CloseAndNotify(std::atomic<WaitlistNode*> *headPtr);

    // Allocate a new node for a waiting queue.
    WaitlistNode *_AllocateNode(Exec_CompilationTask *task, WaitlistNode *next);

private:
    // The various states a task can be in.
    enum _TaskState : uint8_t {
        _TaskStateUnclaimed,
        _TaskStateClaimed,
        _TaskStateDone
    };

    // Entries in the map always begin life as unclaimed tasks with no
    // nodes on their waitlist.
    struct _Entry {
        _Entry() : state(_TaskStateUnclaimed), waiting(nullptr) {}
        std::atomic<uint8_t> state;
        std::atomic<WaitlistNode*> waiting;
    };

    // The map of tasks that have been claimed during this round of
    // compilation.
    using _ClaimedTasks =
        tbb::concurrent_unordered_map<Exec_OutputKey::Identity, _Entry, TfHash>;
    _ClaimedTasks _claimedTasks;

    // A simple vector that serves as a way of scratch-allocating new
    // waiting nodes.
    tbb::concurrent_vector<WaitlistNode> _allocator;

    // Work dispatcher for running tasks that have all their dependencies
    // fulfilled.
    WorkDispatcher &_dispatcher;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif