//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/scheduleInvalidator.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/debugCodes.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"

#include "pxr/base/trace/trace.h"

#include <mutex>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

static bool _IsNodeInSet(const TfBits &set, const VdfNode &node)
{
    const VdfIndex index = VdfNode::GetIndexFromId(node.GetId());
    return index < set.GetSize() && set.IsSet(index);
}

Vdf_ScheduleInvalidator::~Vdf_ScheduleInvalidator()
{
    InvalidateAll();
}

void
Vdf_ScheduleInvalidator::InvalidateAll()
{
    // Early bail out if the schedules are already clear.
    if (_schedules.empty()) {
        return;
    }

    TRACE_FUNCTION();

    TF_DEBUG(VDF_SCHEDULING).
        Msg("[Vdf] Clearing all %zu schedules.\n", _schedules.size()); 

    _nodeFilter.clear();
    _nodeFilterState.store(0, std::memory_order_release);

    for (auto &[schedule, entry] : _schedules) {
        if (entry.alive.load()) {
            // Clear calls Unregister()
            schedule->Clear();
        }
    }
    _schedules.clear();
}

void 
Vdf_ScheduleInvalidator::InvalidateContainingNode(const VdfNode *node)
{
    // Early bail out if the schedules are already clear.
    if (_schedules.empty()) {
        return;
    }

    TRACE_FUNCTION();

    // Filter out nodes that can't affect any schedules.
    if (!_IsNodeInAnySchedule(*node)) {
        return;
    }

    size_t numCleared = 0;
    for (auto &[schedule, entry] : _schedules) {
        if (!entry.alive.load() || !_IsNodeInSet(entry.scheduledNodes, *node)) {
            continue;
        }
        
        std::lock_guard<tbb::spin_mutex> lock(entry.lock);
        if (entry.alive.load()) {
            // Clear calls Unregister(), which sets `alive = false`
            schedule->Clear();
            ++numCleared;
        }
    }

    if (numCleared && TfDebug::IsEnabled(VDF_SCHEDULING)) {
        TfDebug::Helper().
            Msg("[Vdf] InvalidateContainingNode: %s\n"
                "[Vdf] ... cleared %zu schedules, have %zu entries.\n",
                node->GetDebugName().c_str(), numCleared, _schedules.size());
    }
}

void 
Vdf_ScheduleInvalidator::UpdateForAffectsMaskChange(VdfOutput *output)
{
    // Early bail out if the schedules are already clear.
    if (_schedules.empty()) {
        return;
    }

    TRACE_FUNCTION();

    // Filter out nodes that can't affect any schedules.
    const VdfNode &node = output->GetNode();
    if (!_IsNodeInAnySchedule(node)) {
        return;
    }

    size_t numCleared = 0;
    for (auto &[schedule, entry] : _schedules) {
        if (!entry.alive.load() || !_IsNodeInSet(entry.scheduledNodes, node)) {
            continue;
        }

        std::lock_guard<tbb::spin_mutex> lock(entry.lock);
        if (!entry.alive.load()) {
            continue;
        }

        if (!VdfScheduler::UpdateAffectsMaskForOutput(schedule, *output)) {
            // Clear calls Unregister(), which set `alive = false`
            schedule->Clear();
            ++numCleared;
        }
    }

    if (numCleared && TfDebug::IsEnabled(VDF_SCHEDULING)) {
        TfDebug::Helper().
            Msg("[Vdf] UpdateSchedulesForAffectsMaskChange: %s\n"
                "[Vdf] ... cleared %zu schedules, have %zu entries.\n",
                output->GetDebugName().c_str(), numCleared, _schedules.size());
    }
}

void 
Vdf_ScheduleInvalidator::UpdateForConnectionChange(
    const VdfConnection *connection)
{
    // Early bail out if the schedules are already clear.
    if (_schedules.empty()) {
        return;
    }

    TRACE_FUNCTION();

    const VdfNode &targetNode = connection->GetTargetNode();
    if (!_IsNodeInAnySchedule(targetNode)) {
        return;
    }

    size_t numCleared = 0;
    for (auto &[schedule, entry] : _schedules) {
        if (!entry.alive.load() || 
                !_IsNodeInSet(entry.scheduledNodes, targetNode)) {
            continue;
        }

        std::lock_guard<tbb::spin_mutex> lock(entry.lock);
        if (!entry.alive.load()) {
            continue;
        }

        VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(oid, *schedule, targetNode) {
            if (!TF_VERIFY(oid.IsValid())) {
                continue;
            }

            const VdfOutput *output = schedule->GetOutput(oid);
            const VdfMask &requestMask = schedule->GetRequestMask(oid);
            const VdfMask::Bits dependencyMask =
                targetNode.ComputeInputDependencyMask(
                    VdfMaskedOutput(
                        const_cast<VdfOutput *>(output), requestMask),
                    *connection);

            if (!dependencyMask.AreAllUnset()) {
                // Clear calls Unregister(), which set `alive = false`
                schedule->Clear();
                ++numCleared;
                break;
            }
        }
    }

    if (numCleared && TfDebug::IsEnabled(VDF_SCHEDULING)) {
        TfDebug::Helper().
            Msg("[Vdf] UpdateSchedulesForConnectionChange: %s\n"
                "[Vdf] ... cleared %zu schedules, have %zu entries.\n",
                connection->GetDebugName().c_str(),
                numCleared, _schedules.size());
    }
}

void
Vdf_ScheduleInvalidator::Register(VdfSchedule *schedule)
{
    TRACE_FUNCTION();

    const auto [iterator, inserted] = _schedules.emplace(
        std::piecewise_construct, 
            std::forward_as_tuple(schedule), 
            std::forward_as_tuple());

    if (!iterator->second.alive.exchange(true)) {
        iterator->second.scheduledNodes = schedule->GetScheduledNodeBits();
        _MergeScheduleIntoNodeFilter(*schedule);
    }
}

void
Vdf_ScheduleInvalidator::Unregister(VdfSchedule *schedule)
{
    TRACE_FUNCTION();

    const auto iterator = _schedules.find(schedule);
    if (iterator == _schedules.end()) {
        return;
    }

    // Concurrently removing entries from the schedule map is not supported, so
    // we instead tombstone the entry. If we succeed in doing so, i.e., we are
    // the first ones to set `alive = false`, we will also remove the scheduled
    // nodes from the reference counted array.
    // 
    // Note, tombstoning instead of erasing entries here risks leaving cruft in
    // the map. The risk is small, since we expect the memory allocator to alias
    // VdfSchedule pointers, resulting in resurrected entries, but there is no
    // such guarantee. To address this in the long-run, we should be assigning
    // VdfIds to VdfSchedules, which would lead to re-use/aliasing of previously
    // used indices by design.
    if (iterator->second.alive.exchange(false)) {
        _RemoveScheduleFromNodeFilter(*schedule);
    }
}

void
Vdf_ScheduleInvalidator::_GrowNodeFilter(size_t newSize)
{
    // Note, we can replace this with zero-allocating the _nodeFilter along with
    // using std::atomic_ref, once it becomes available with C++20.

    // Use the most significant bit in the state variable to indicate whether
    // the array is currently growing. 
    static constexpr size_t IsGrowingBit =
        size_t(1) << (sizeof(size_t) * 8 - 1);

    // Read the current size from the state variable, indicating the number of
    // constructed entries in _nodeFilter. If this size is already sufficient to
    // accommodate newSize, we can bail out here without performing expensive
    // atomic writes.
    size_t currentState = _nodeFilterState.load(std::memory_order_acquire);
    size_t currentSize = currentState & (~IsGrowingBit);
    if (currentSize >= newSize) {
        return;
    }

    // If there aren't enough constructed entries, check whether the "growing"
    // flag is set, indicating that new entries are currently under construction
    // on a different thread. If that's the case, let's spin wait until the
    // array is done growing.
    while ((currentState & IsGrowingBit) > 0) {
        currentState = _nodeFilterState.load(std::memory_order_acquire);
        currentSize = currentState & (~IsGrowingBit);
        if (currentSize >= newSize) {
            return;
        }

        std::this_thread::yield();
    }

    // The array is not concurrently growing, so let's try to atomically set
    // the "growing" flag. If we're successful in doing so, we lock out other
    // threads from growing the array at the same time.
    while (!_nodeFilterState.compare_exchange_strong(
        currentSize, currentSize | IsGrowingBit)) {
        currentSize &= (~IsGrowingBit);
        if (currentSize >= newSize) {
            return;
        }

        std::this_thread::yield();
    }

    // We hold exclusive access to the "growing" flag, so we can go ahead and
    // do the growing.
    // 
    // grow_to_at_least is thread-safe, *except* it will not synchronize
    // invoking the constructors on the newly added entries with concurrent
    // access to these new entries.
    _nodeFilter.grow_to_at_least(newSize);

    // Now, let's write back the new size to the _nodeFilterState, while at the
    // same time resetting the "growing" flag.
    const size_t prevState = _nodeFilterState.exchange(newSize);
    TF_VERIFY(prevState == (currentSize | IsGrowingBit));
}

inline void
Vdf_ScheduleInvalidator::_MergeScheduleIntoNodeFilter(
    const VdfSchedule &schedule)
{
    TRACE_FUNCTION();

    _GrowNodeFilter(schedule.GetNetwork()->GetNodeCapacity());

    const TfBits &scheduledNodeSet = schedule.GetScheduledNodeBits();
    for (const size_t i : scheduledNodeSet.GetAllSetView()) {
        ++_nodeFilter[i];
    }
}

inline void
Vdf_ScheduleInvalidator::_RemoveScheduleFromNodeFilter(
    const VdfSchedule &schedule)
{
    // Exit early when there are no nodes in the filter or if no schedules have
    // actually been removed.  This can happen, for example, when
    // Vdf_ScheduleInvalidator::InvalidateAll is called.  In that case, both
    // _schedules and _nodeFilter are empty, so there's no need to iterate over
    // the schedule's node set.
    if (_nodeFilter.empty()) {
        return;
    }

    TRACE_FUNCTION();

    const TfBits &scheduledNodeSet = schedule.GetScheduledNodeBits();
    for (const size_t i : scheduledNodeSet.GetAllSetView()) {
        --_nodeFilter[i];
    }
}

inline bool
Vdf_ScheduleInvalidator::_IsNodeInAnySchedule(const VdfNode &node) const
{
    const VdfIndex index = VdfNode::GetIndexFromId(node.GetId());
    return index < _nodeFilter.size() && _nodeFilter[index] != 0;
}

PXR_NAMESPACE_CLOSE_SCOPE
