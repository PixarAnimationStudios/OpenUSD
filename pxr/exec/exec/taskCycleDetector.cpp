//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/taskCycleDetector.h"

#include "pxr/exec/exec/compilationState.h"
#include "pxr/exec/exec/validationError.h"

#include "pxr/base/tf/diagnostic.h"

#include <atomic>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

// Calculates the amount that's added to the 64-bit packed counter in order to
// increase the number of busyThreads by \p busyThreads and increase the number
// of unblockedTasks by \p unblockedTasks. The parameters may be negative.
//
static uint64_t
_PackIncrementAmount(int32_t busyThreads, int32_t unblockedTasks);

// Unpacks the packed 64-bit value into a pair containing the number of busy
// threads and the number of unblocked tasks.
//
static std::pair<int32_t, int32_t>
_Unpack(uint64_t packed);

Exec_TaskCycleDetector::Exec_TaskCycleDetector(
    Exec_CompilationState *const compilationState)
    : _compilationState(compilationState)
    , _blockedTasks(0)
{}

Exec_TaskCycleDetector::~Exec_TaskCycleDetector()
{
    // Once all tasks have completed, all counters should be 0.
    const auto [busyThreads, unblockedTasks] =
        _busyThreadsAndUnblockedTasks.Load(std::memory_order_acquire);
    const int32_t blockedTasks = _blockedTasks.load(std::memory_order_relaxed);

    TF_VERIFY(
        busyThreads == 0 && blockedTasks == 0 && unblockedTasks == 0,
        "Counts are not all 0: Busy threads=%d, Blocked Tasks=%d, "
        "Unblocked Tasks=%d",
        busyThreads, blockedTasks, unblockedTasks);
}

void
Exec_TaskCycleDetector::BeginThreadBusy()
{
    _ThreadData &threadData = _threadData.local();
    
    // If this is a nested call to BeginThreadBusy, then the thread is already
    // busy.
    if (++threadData.nestedBusy > 1) {
        return;
    }

    // This thread is now busy.
    _busyThreadsAndUnblockedTasks.Add(1, 0, std::memory_order_release);
    threadData.blockedTasks = 0;
    threadData.unblockedTasks = 0;
}

void
Exec_TaskCycleDetector::EndThreadBusy()
{
    _ThreadData &threadData = _threadData.local();

    // If this is a nested call to EndThreadBusy, then the thread is still busy.
    if (--threadData.nestedBusy > 0) {
        return;
    }

    // This thread is no longer busy. First, flush the number of blocked tasks
    // accumulated by this thread.
    _blockedTasks.fetch_add(
        threadData.blockedTasks, std::memory_order_acquire);

    // Next, decrement the number of busy threads, while also flushing the
    // number of unblocked tasks accumulated by this thread. FetchAdd returns
    // the values of these counters prior to the addition.
    const auto [prevBusyThreads, prevUnblockedTasks] =
        _busyThreadsAndUnblockedTasks.FetchAdd(
            -1, threadData.unblockedTasks, std::memory_order_release);

    // Get the current numbers of busy threads and unblocked tasks.
    const int32_t busyThreads = prevBusyThreads - 1;
    const int32_t unblockedTasks =
        prevUnblockedTasks + threadData.unblockedTasks;

    // If there remain any busy threads or unblocked tasks, then the task graph
    // is capable of making further progress, and it's too early to check for
    // task cycles.
    if (busyThreads != 0 || unblockedTasks != 0) {
        return;
    }

    // The task graph is incapable of making further progress. Only the last
    // busy thread will encounter this. If no tasks are blocked, then all work
    // has completed.
    const int32_t blockedTasks = _blockedTasks.load(std::memory_order_acquire);
    if (blockedTasks == 0) {
        return;
    }

    // There remains at least one blocked task, so there must be a task cycle.

    // TODO: Currently, when we detect a task cycle, we interrupt compilation,
    // which should break all task cycles and prevent new cycles from being
    // created. Therefore, a single round of compilation should not detect
    // task cycles more than once. In the future, we may allow compilation to
    // continue after breaking the task cycles, which may establish additional
    // task cycles in the same round of compilation. When that happens, we can
    // remove this verify.
    const bool wasCycleAlreadyDetected =
        _isCycleDetected.exchange(true, std::memory_order_relaxed);
    TF_VERIFY(!wasCycleAlreadyDetected);

    TF_ERROR(ExecValidationErrorType::DataDependencyCycle,
        "Interrupting exec compilation due to one or more data depencency "
        "cycles. Computed values may not be accurate until the cycles are "
        "resolved.");
    _compilationState->GetInterruptState().Interrupt();
}

void
Exec_TaskCycleDetector::CreateTask()
{
    _ThreadData &threadData = _threadData.local();
    ++threadData.unblockedTasks;
}

void
Exec_TaskCycleDetector::DestroyTask()
{
    _ThreadData &threadData = _threadData.local();
    --threadData.unblockedTasks;
}

void
Exec_TaskCycleDetector::BlockTask()
{
    _ThreadData &threadData = _threadData.local();
    ++threadData.blockedTasks;
    --threadData.unblockedTasks;
}

void
Exec_TaskCycleDetector::UnblockTask()
{
    _ThreadData &threadData = _threadData.local();
    ++threadData.unblockedTasks;
    --threadData.blockedTasks;
}

Exec_TaskCycleDetector::
_BusyThreadsAndUnblockedTasks::_BusyThreadsAndUnblockedTasks()
    // For each signed 32-bit value X, we actually store an unsigned 32-bit
    // value Y biased by 2^31: (Y = X + 2^31). This ensures we never store a
    // negative value in the counter, and this allows us to subtract arbitrary
    // values from either (or both) counters without a carry of the lower 32-
    // bits affecting the upper 32-bits.
    //   0x00000000 represents the minimuim (most-negative) value.
    //   0x80000000 represents 0.
    //   0xffffffff represents the maximum (most-positive) value.
    : _packed(0x80000000'80000000)
{}

void
Exec_TaskCycleDetector::_BusyThreadsAndUnblockedTasks::Add(
    const int32_t busyThreads,
    const int32_t unblockedTasks,
    const std::memory_order memoryOrder)
{
    const uint64_t incrementAmount =
        _PackIncrementAmount(busyThreads, unblockedTasks);
    _packed.fetch_add(incrementAmount, memoryOrder);
}

std::pair<int32_t, int32_t> 
Exec_TaskCycleDetector::_BusyThreadsAndUnblockedTasks::FetchAdd(
    const int32_t busyThreads,
    const int32_t unblockedTasks,
    const std::memory_order memoryOrder)
{
    const uint64_t incrementAmount =
        _PackIncrementAmount(busyThreads, unblockedTasks);
    const uint64_t packed = _packed.fetch_add(incrementAmount, memoryOrder);
    return _Unpack(packed);
}

std::pair<int32_t, int32_t> 
Exec_TaskCycleDetector::_BusyThreadsAndUnblockedTasks::Load(
    const std::memory_order memoryOrder)
{
    const uint64_t packed = _packed.load(memoryOrder);
    return _Unpack(packed);
}

uint64_t
_PackIncrementAmount(const int32_t busyThreads, const int32_t unblockedTasks)
{
    // Each additional unblockedTask (lower 32-bits) increases the packed value
    // by 1. Each additional busyThread (upper 32-bits) increases the packed
    // value by 2^32. Note, this works even if the deltas are negative.
    return (static_cast<uint64_t>(busyThreads) << 32) +
        (static_cast<uint64_t>(unblockedTasks));
}

std::pair<int32_t, int32_t>
_Unpack(const uint64_t packed)
{
    // Extracting the upper/lower 32-bits yields an unsigned value Y = X + 2^31.
    // To obtain the intended value X, we need to subtract 2^31 from this
    // unsigned value.
    constexpr static uint32_t BIAS = 1 << 31;

    const int32_t busyThreads =
        static_cast<int32_t>(((packed >> 32) & 0xffffffff) - BIAS);

    const int32_t unblockedTasks =
        static_cast<int32_t>((packed & 0xffffffff) - BIAS);

    return {busyThreads, unblockedTasks};
}

PXR_NAMESPACE_CLOSE_SCOPE
