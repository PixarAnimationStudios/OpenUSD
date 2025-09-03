//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SCHEDULE_INVALIDATOR_H
#define PXR_EXEC_VDF_SCHEDULE_INVALIDATOR_H

#include "pxr/pxr.h"

#include "pxr/base/arch/align.h"
#include "pxr/base/tf/bits.h"

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>
#include <tbb/spin_mutex.h>

#include <atomic>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

class VdfConnection;
class VdfNode;
class VdfOutput;
class VdfSchedule;

/// Collects schedules and invalidates them when relevant changes to the
/// topology of the VdfNetwork are made.
/// 
/// Schedules must first be registered with this invalidator before they
/// receive invalidation. The VdfNetwork is on the hook for calling the
/// notification methods on this class when relevant network edits are made.
/// 
/// The invalidator supports concurrent registration and unregistration of
/// schedules, as well as concurrent invalidation. However, it does not support
/// registering/unregistering while concurrently invalidating.
///
class Vdf_ScheduleInvalidator 
{
public:
    Vdf_ScheduleInvalidator() : _nodeFilterState(0) {}

    /// All the registered schedules are invalidated when the manager is
    /// destructed.
    ///
    ~Vdf_ScheduleInvalidator();

    /// Invalidates (i.e., calls Clear() on) all registered schedules.
    ///
    void InvalidateAll();

    /// Invalidates (i.e., calls Clear() on) all the registered schedules that
    /// contain \p node.
    ///
    void InvalidateContainingNode(const VdfNode *node);

    /// Updates schedules that contain \p output for an affects mask change.
    /// Will invalidate and clear schedules, if this can't be done.
    ///
    void UpdateForAffectsMaskChange(VdfOutput *output);

    /// Updates schedules that contain \p connection for an added or removed
    /// connection. Will invalidate and clear schedules, if this can't be done.
    ///
    void UpdateForConnectionChange(const VdfConnection *connection);

    /// Adds a schedule to the invalidator, making sure it will receive proper
    /// invalidation going forward.
    /// 
    void Register(VdfSchedule *schedule);

    /// Removes the schedule from the invalidator. When this call returns, the
    /// provided schedule will not longer receive invalidation.
    ///
    void Unregister(VdfSchedule *schedule);

private:
    // If needed, grows the _nodeFilter array to accommodate at least newSize
    // entries.
    void _GrowNodeFilter(size_t newSize);

    // Union schedule's nodes into the node filter.
    void _MergeScheduleIntoNodeFilter(const VdfSchedule &schedule);

    // Remove schedule's nodes from the node filter.
    void _RemoveScheduleFromNodeFilter(const VdfSchedule &schedule);

    // Returns true if node is contained in at least one schedule, according
    // to _nodeFilter.
    bool _IsNodeInAnySchedule(const VdfNode &node) const;

private:
    // Holds a prefilter that lets us know if we have any schedule that
    // could be affected by the node at the corresponding index.
    tbb::concurrent_vector<std::atomic<uint32_t>> _nodeFilter;

    // Stores the size of _nodeFilter along with a bit that indicates whether
    // the concurrent_vector is currently growing. If the vector is currently
    // growing, we need to synchronize on construction of the newly added
    // entries.
    std::atomic<size_t> _nodeFilterState;

    // Represents a schedule entry in the map
    struct _ScheduleEntry {
        _ScheduleEntry() : alive(false) {}

        // This mutex protects the VdfSchedule. The schedule pointer must not
        // be dereferenced without holding this lock.
        alignas(ARCH_CACHE_LINE_SIZE) tbb::spin_mutex lock;

        // Indicates whether the entry is alive in the map. If this is false,
        // the schedule pointer must not be dereferenced. It will be invalid.
        std::atomic<bool> alive;

        // A copy of the bitset that indicates which nodes in the network are
        // included in the schedule. This data can be accessed regardless of
        // whether the entry is alive or not. The data can also be read from
        // concurrently, but it must not be mutated outside of reviving
        // tombstoned entries in Register().
        TfBits scheduledNodes;
    };
    static_assert(sizeof(_ScheduleEntry) == ARCH_CACHE_LINE_SIZE);

    // The map of all existing schedules that reference this network.
    using _SchedulesMap =
        tbb::concurrent_unordered_map<VdfSchedule*, _ScheduleEntry>;
    _SchedulesMap _schedules;

};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
