//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_TASK_CYCLE_DETECTOR_H
#define PXR_EXEC_EXEC_TASK_CYCLE_DETECTOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/base/arch/align.h"

#include <tbb/enumerable_thread_specific.h>

#include <atomic>
#include <cstdint>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class Exec_CompilationState;

/// Detects when Exec_CompilationTasks are blocked by a task cycle.
///
/// Task cycles are formed when tasks directly or indirectly wait on themselves
/// to complete, thus preventing the task graph from completing.
///
/// The Exec_TaskCycleDetector maintains counters for the number of tasks that
/// are blocked and unblocked. Worker threads in the thread pool continually
/// execute unblocked tasks until there are no more more unblocked tasks to run.
/// If there remain any blocked tasks after all unblocked tasks have executed,
/// then at least one task-cycle must exist among the blocked tasks.
///
/// Note that the Exec_TaskCycleDetector will not report a task cycle while
/// other tasks are capable of making progress. If a task cycle is established
/// early in the compilation process, it will not be reported until all other
/// tasks have completed or have become blocked.
///
/// TODO: When a task cycle is detected, the Exec_TaskCycleDetector writes a
/// message to stderr and aborts the process to prevent a hang. In the future,
/// we will inpsect the task graph and break the task cycle(s) (e.g. by
/// inserting VdfSpeculationNodes), so that compilation can complete.
///
class Exec_TaskCycleDetector
{
public:
    explicit Exec_TaskCycleDetector(Exec_CompilationState *compilationState);
    
    ~Exec_TaskCycleDetector();

    Exec_TaskCycleDetector(const Exec_TaskCycleDetector &) = delete;
    Exec_TaskCycleDetector &operator=(const Exec_TaskCycleDetector &) = delete;

    /// \name Worker-thread tracking
    ///
    /// Each worker thread signals to the Exec_TaskCycleDetector when it's busy
    /// executing tasks.
    ///
    /// Each call to BeginThreadBusy must have a corresponding call to
    /// EndThreadBusy when the thread has completed its work. Pairs of
    /// BeginThreadBusy / EndThreadBusy may be nested within each other, in
    /// which case, only the outer-most pair of calls actually marks the thread
    /// busy/not-busy.
    /// 
    /// @{

    /// Declares that the calling thread is now busy executing tasks.
    void BeginThreadBusy();

    /// Declares that the calling thread has completed executing tasks.
    ///
    /// This function performs cycle-detection if the caller is the last busy
    /// thread. A cycle is detected if all remaining tasks are blocked.
    ///
    void EndThreadBusy();

    /// An RAII helper class that invokes BeginThreadBusy and EndThreadBusy.
    class BusyScope
    {
    public:
        BusyScope(const BusyScope &) = delete;
        BusyScope &operator=(const BusyScope &) = delete;

        /// Calls BeginThreadBusy.
        BusyScope(Exec_TaskCycleDetector *taskCycleDetector)
            : _taskCycleDetector(taskCycleDetector) {
            _taskCycleDetector->BeginThreadBusy();
        }

        /// Calls EndThreadBusy.
        ~BusyScope() {
            _taskCycleDetector->EndThreadBusy();
        }

    private:
        Exec_TaskCycleDetector *const _taskCycleDetector;
    };

    /// Returns an RAII object that marks the thread busy until the object is
    /// destroyed.
    ///
    BusyScope NewBusyScope() {
        return BusyScope(this);
    }

    /// @}

    /// \name Task state tracking
    ///
    /// As worker threads execute tasks, they notify the Exec_TaskCycleDetector
    /// as tasks are created/destroyed, and when they become blocked/unblocked.
    ///
    /// Each method in this section must be called while the worker thread is
    /// busy - that is, between calls to BeginThreadBusy and EndThreadBusy.
    ///
    /// @{

    /// Declares that a new task has been created.
    ///
    /// The new task is assumed to be unblocked.
    ///
    void CreateTask();

    /// Declares that a task has completed and will be destroyed.
    ///
    /// The task is assumed to already be unblocked.
    ///
    void DestroyTask();

    /// Declares that a previously unblocked task is now blocked.
    void BlockTask();

    /// Declares that a previously blocked task is now unblocked.
    void UnblockTask();

    /// @}

private:
    // Each thread maintains thread-local counters which are flushed to the
    // shared counters after executing a block of tasks.
    //
    struct _ThreadData
    {
        int32_t blockedTasks = 0;
        int32_t unblockedTasks = 0;
        
        // Number of nested BeginThreadBusy/EndThreadBusy sections for the
        // thread. A value > 0 means the thread is busy. A value of 0 means the
        // thread is not busy.
        //
        uint32_t nestedBusy = 0;
    };

    // The number of busy threads and number of unblocked tasks are two 32-bit
    // values packed into a single atomic 64-bit value.
    // 
    // Together, these two counters indicate if the task graph is capable of
    // making progress: If there are 0 busy threads, but >0 unblocked tasks,
    // then another thread is about to become busy. If there are 0 unblocked
    // tasks, but >0 busy threads, then another thread is still running. Only
    // when both values reach 0 do we determine that the task graph is
    // incapable of making further progress.
    //
    class _BusyThreadsAndUnblockedTasks
    {
    public:
        // Initializes both counters to 0.
        _BusyThreadsAndUnblockedTasks();

        // Atomically increments each counter by a different value.
        void Add(
            int32_t busyThreads,
            int32_t unblockedTasks,
            std::memory_order memoryOrder = std::memory_order_seq_cst);

        // Atomically increments each counter by a different value, and returns
        // their original values prior to the increment.
        //
        std::pair<int32_t, int32_t> FetchAdd(
            int32_t busyThreads,
            int32_t unblockedTasks,
            std::memory_order memoryOrder = std::memory_order_seq_cst);

        // Atomically loads both counter values.
        std::pair<int32_t, int32_t> Load(
            std::memory_order memoryOrder = std::memory_order_seq_cst);

    private:
        alignas(ARCH_CACHE_LINE_SIZE) std::atomic<uint64_t> _packed;
    };

private:
    // The cycle detector needs a back-pointer to the compilation state, so that
    // it can interrupt compilation when a cycle is detected.
    Exec_CompilationState *const _compilationState;

    // This flag is set to true once a cycle has been detected. It ensures that
    // we never detect a cycle more than once in the same round of compilation.
    std::atomic<bool> _isCycleDetected = false;

    // Thread-local counters.
    tbb::enumerable_thread_specific<_ThreadData> _threadData;

    // Shared counters.
    _BusyThreadsAndUnblockedTasks _busyThreadsAndUnblockedTasks;
    alignas(ARCH_CACHE_LINE_SIZE) std::atomic<int32_t> _blockedTasks;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif