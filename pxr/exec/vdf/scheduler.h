//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SCHEDULER_H
#define PXR_EXEC_VDF_SCHEDULER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/poolChainIndex.h"

#include "pxr/base/tf/hashmap.h"

#include <atomic>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfMaskedOutput;
using VdfMaskedOutputVector = std::vector<VdfMaskedOutput>;
class VdfNetwork;
class VdfNode;
class VdfRequest;
class VdfSchedule;
class VdfScheduleNode;
class VdfScheduleOutput;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfScheduler
///
/// \brief Used to make a VdfSchedule.
///
class VdfScheduler 
{
public:
    // Vector of priorities for pool outputs.
    typedef std::vector<
        std::pair<VdfPoolChainIndex, const VdfOutput *>
    > PoolPriorityVector;

    /// A map from VdfNode * to VdfMaskedOutputVector.
    using NodeToRequestMap =
        TfHashMap<const VdfNode *, VdfMaskedOutputVector, TfHash>;

    /// Constructs a scheduler.
    ///
    /// If \p include is non-null, the pointed-to atomic flag is checked during
    /// scheduling to determine if the client has requested scheduling to be
    /// interrupted. If scheduling is interrupted, CreateSchedule attempts to
    /// return quickly (on the order of a few ms), leaving the resulting
    /// schedule in an unknown state.
    ///
    VDF_API
    VdfScheduler(const std::atomic_bool *interruptionFlag);

    /// Generates a schedule.
    ///
    VDF_API
    static void Schedule(const VdfRequest &request, VdfSchedule *schedule,
        bool topologicallySort);

    /// Generates a schedule.
    ///
    /// This method supports optional scheduling interruption.
    ///
    /// Returns `false` if scheduling was interrupted, and `true` otherwise.
    ///
    VDF_API
    bool CreateSchedule(const VdfRequest &request, VdfSchedule *schedule,
        bool topologicallySort);

    /// Update \p schedule after the affects mask changed on \p output.
    ///
    VDF_API
    static bool UpdateAffectsMaskForOutput(
        VdfSchedule *schedule,
        const VdfOutput &output);

protected:
    /// Returns `true` if the scheduler has been interrupted, if the scheduler
    /// supports interruption.
    ///
    /// If interruption is not supported, i.e.  no interruption flag has been
    /// set, this will always return `false`.
    ///
    bool _HasBeenInterrupted() const {
        return _interruptionFlag &&
            _interruptionFlag->load(std::memory_order_relaxed);
    }

    /// Method to signal that a \p schedule is done being built and that it is
    /// now valid for the given \p network.
    ///
    VDF_API
    static void _SetScheduleValid(
        VdfSchedule *schedule, 
        const VdfNetwork *network);

    /// Initializes the request masks for all the outputs that will be computed
    /// as a result of \p request.
    ///
    /// \p poolOutputs is an output parameter. The vector will contain
    /// all the pool outputs scheduled with request masks, sorted in reverse
    /// order of pool chain index, i.e. the pool output furthest downstream
    /// will be at the front of the vector.
    ///
    /// Returns `false` if scheduling has been interrupted, or if an error has
    /// been detected.
    ///
    VDF_API
    bool _InitializeRequestMasks(
        const VdfRequest &request,
        VdfSchedule *schedule,
        PoolPriorityVector *poolOutputs);

    /// Marks the schedule as small if it is indeed small.
    VDF_API
    static void _MarkSmallSchedule(VdfSchedule *schedule);

    /// Method to schedule the buffer passes and the "keep" masks for the 
    /// scheduled nodes.
    ///
    /// Schedulers that care about performance will want to call this after
    /// all the outputs have gone through the _ScheduleOutput method above.
    ///
    /// Returns `false` if scheduling has been interrupted.
    ///
    VDF_API
    bool _ScheduleBufferPasses(
        const VdfRequest &request,
        VdfSchedule *schedule);

    /// Schedule the outputs from which buffers should be passed. This helps
    /// ensure that potentially large portions of the network that won't have
    /// any effect in this schedule are skipped when passing buffers.
    ///
    /// \p sortedPoolOutputs contains all the pool outputs in the schedule.
    /// Callers are responsible for ensuring that \p sortedPoolOutputs are in
    /// descending order of their respective pool chain index, i.e. the pool
    /// output furthest downstream will be at the front of the vector.
    ///
    /// Returns `false` if scheduling has been interrupted.
    ///
    VDF_API
    bool _ScheduleForPassThroughs(
        const VdfRequest &request,
        VdfSchedule *schedule,
        const PoolPriorityVector &sortedPoolOutputs);

    /// Generate tasks for the scheduled task graph. The task graph is used
    /// by the parallel evaluation engine.
    ///
    /// Returns `false` if scheduling has been interrupted.
    ///
    VDF_API
    void _GenerateTasks(
        VdfSchedule *schedule,
        const PoolPriorityVector &sortedPoolOutputs);

    /// Schedule the task graph for multi-threaded munging. This will generate
    /// tasks and invocations, as well as dependencies between them.
    ///
    /// Returns `false` if scheduling has been interrupted.
    ///
    VDF_API
    bool _ScheduleTaskGraph(
        VdfSchedule *schedule,
        const PoolPriorityVector &sortedPoolOutputs);

    /// Setup the lock masks require for sparse mung buffer locking for all the
    /// outputs in the pool chain.
    ///
    /// \p sortedPoolOutputs contains all the pool outputs in the schedule.
    /// Callers are responsible for ensuring that \p sortedPoolOutputs are in
    /// descending order of their respective pool chain index, i.e. the pool
    /// output furthest downstream will be at the front of the vector.
    ///
    VDF_API
    static void _ComputeLockMasks(
        const VdfRequest &request, 
        VdfSchedule *schedule,
        const PoolPriorityVector &sortedPoolOutputs);

    /// Applies the affects mask to the schedule.
    ///
    /// Returns `false` if scheduling has been interrupted.
    ///
    VDF_API
    bool _ApplyAffectsMasks(VdfSchedule *schedule);

    /// Applies the affects masks to the scheduled node.
    ///
    /// This mask is the ANDing of the request mask and the affects mask (if
    /// any).  It is used by iterators to quickly skip only to the elements
    /// that are affected by the node.  It is an optional mask, and if it
    /// doesn't exist, the scheduled affects mask will simply be request mask
    /// (a super set of the truly affected elements).
    ///
    VDF_API
    static bool _ApplyAffectsMasksForNode(VdfScheduleNode *node);

    /// Updates the affects mask on an existing scheduled node invocation.
    ///
    VDF_API
    static void _UpdateAffectsMaskForInvocation(
        VdfSchedule *schedule,
        VdfScheduleNode *node);

    /// Updates schedule for \p node if affects mask changed.
    ///
    VDF_API
    static bool _UpdateAffectsMasksForNode(
        VdfSchedule *schedule,
        VdfScheduleNode *node);

    /// Updates schedule for \p node if the lock mask changed.
    ///
    VDF_API
    static void _UpdateLockMaskForNode(
        VdfSchedule *schedule,
        VdfScheduleNode *node);

private:

    // Used to optionally support scheduling interruption.
    const std::atomic_bool *_interruptionFlag = nullptr;
};

///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
