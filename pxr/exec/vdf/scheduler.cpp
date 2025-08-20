//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/scheduler.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/error.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/networkUtil.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/rootNode.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/sparseInputTraverser.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/denseHashMap.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/withScopedParallelism.h"

#include "pxr/base/trace/trace.h"

#include <tbb/concurrent_vector.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Threshold for # of outputs on a node to use the
// ComputeInputDependencyRequest() API.
#define NODE_OUTPUT_THRESHOLD 100

// structure for poolOutputQueue. 
typedef
    std::map<VdfPoolChainIndex, VdfMaskedOutput,std::greater<VdfPoolChainIndex>>
    _IndexToMaskedOutputMap;

// Map from output pointer to scheduled output index.
typedef
    TfDenseHashMap<
        VdfMaskedOutput,
        VdfScheduleInputDependencyUniqueIndex,
        VdfMaskedOutput::Hash>
    _OutputToIndexMap;

namespace {

// This structure describes an invocation of a node. It feed the task graph
// scheduling algorithm.
//
struct _InvocationNode {

    // Constructor.
    //
    _InvocationNode(
        uint32_t nidx,
        VdfScheduleTaskIndex iidx,
        VdfScheduleTaskNum inum,
        VdfScheduleTaskIndex itidx,
        VdfScheduleTaskNum ktidx) :
        scheduleNodeIndex(nidx),
        invocationIndex(iidx),
        invocationNum(inum),
        inputsTaskIndex(itidx),
        keepTaskIndex(ktidx)
    {}

    // Index into the array of scheduled nodes.
    //
    uint32_t scheduleNodeIndex;

    // Index of this invocation.
    //
    VdfScheduleTaskIndex invocationIndex;

    // Number of invocations produced by this node.
    //
    VdfScheduleTaskNum invocationNum;

    // Index to the inputs task, if any.
    //
    VdfScheduleTaskIndex inputsTaskIndex;

    // Index to the keep task, if any.
    //
    VdfScheduleTaskNum keepTaskIndex;
};

// Each node invocation has a number of bitsets associated with it.
// Invocations are only produced for nodes with one output, so the bitsets are
// relevant to that one output, only.
//
struct _InvocationBitsets {

    // The requested bits in this invocation.
    //
    TfCompressedBits requested;

    // The affected bits in this invocation. Empty if none are affected.
    //
    TfCompressedBits affected;

    // The kept bits in this invocation. Empty if none are affected.
    //
    TfCompressedBits kept;
};

// A structure holding a number of invocations. One of these is created per
// point pool chain.
//
struct _Invocations {

    // Constructor.
    //
    _Invocations() :
        numInputsTasks(0),
        numKeepTasks(0)
    {}

    // The invocation nodes.
    //
    tbb::concurrent_vector<_InvocationNode> nodes;

    // The bitets associated with each invocation.
    //
    tbb::concurrent_vector<_InvocationBitsets> bitsets;

    // The number of inputs tasks that must be generated for this invocation.
    //
    VdfScheduleTaskNum numInputsTasks;

    // The number of keep tasks that must be generated for this invocation.
    //
    VdfScheduleTaskNum numKeepTasks;
};

// A structue used to gather and sort input dependencies for each
// scheduled node.
//
struct _NodeDependencies {

    // The read/write dependencies
    //
    std::vector<const VdfScheduleInput *> rws;

    // The prereq dependencies.
    //
    std::vector<const VdfScheduleInput *> prereqs;

    // The read dependencies.
    //
    std::vector<const VdfScheduleInput *> reads;
};

}

static inline void
_AppendToMask(VdfMask::Bits *out, const VdfMask::Bits &in) {
    if (out->GetSize() == 0) {
        *out = in;
    } else {
        *out |= in;
    }
}

static bool
_IsTargetOutputRequested(
    const VdfSchedule &schedule,
    const VdfOutput &output,
    const VdfConnection &inputConnection,
    VdfMask::Bits *dependencyMask)
{
    // Only care about passing through to target node outputs that 
    // are also scheduled
    const VdfSchedule::OutputId &outputId = schedule.GetOutputId(output);
    if (!outputId.IsValid()) {
        return false;
    }

    // If the target node output has an empty request mask, skip it
    const VdfMask &requestMask = schedule.GetRequestMask(outputId);
    if (requestMask.IsEmpty() || requestMask.IsAllZeros()) {
        return false;
    }

    // If the output has no dependency on the input connection, skip it
    *dependencyMask = 
        inputConnection.GetTargetNode().ComputeInputDependencyMask(
            VdfMaskedOutput(const_cast<VdfOutput*>(&output), requestMask), 
            inputConnection);

    if (dependencyMask->AreAllUnset()) {
        return false;
    }

    return true;
}

// Make sure that all read/write outputs are requested on all scheduled nodes.
//
// Even if a read/write output has not been requested through the supplied
// VdfMaskedOutputVector, we need to make sure to provide a read/write
// buffer for the output. The node callback itself has no knowledge of what
// has been requested and may want to write a value to the read/write
// output. Thus, we mark it as requested.
//
static void
_ScheduleUnrequestedReadWrites(
    const VdfScheduleNode &schedNode,
    VdfSchedule *schedule)
{
    // Already have all outputs on this node scheduled?
    if (schedNode.outputs.size() == schedNode.node->GetNumOutputs()) {
        return;
    }
    
    // Iterate over all the outputs on the node, including the ones
    // that have not been scheduled.
    TF_FOR_ALL (o, schedNode.node->GetOutputsIterator()) {
        const VdfInput *ai = o->second->GetAssociatedInput();

        // Ignore outputs without associated inputs or unconnected
        // associated inputs.
        if (!ai || !ai->GetNumConnections()) {
            continue;
        }

        VdfSchedule::OutputId oid = schedule->GetOrCreateOutputId(*o->second);

        // If this output is already requested, we can skip it.
        if (!schedule->GetRequestMask(oid).IsEmpty()) {
            continue;
        }

        // Build a request mask from all the input connection masks.
        VdfMask requestMask;
        TF_FOR_ALL (c, ai->GetConnections()) {
            requestMask.SetOrAppend((*c)->GetMask());
        }
        TF_VERIFY(!requestMask.IsEmpty());

        // Mark the output as requested.
        schedule->SetRequestMask(oid, requestMask);
    }
}

// Computes the passToOutput and keepMask for each output of \p schedNode.
//
static void 
_SetBufferPassDataForOutputs(
    VdfScheduleNode *schedNode, 
    const VdfSchedule &schedule)
{
    // If the node does not support buffer passing, because it manages its own
    // buffers, we can return early.
    if (VdfRootNode::IsARootNode(*schedNode->node)) {
        return;
    }

    TF_FOR_ALL(o, schedNode->outputs) {
        // If the output is not requested, there is no need to compute the
        // keep mask.
        if (o->requestMask.IsEmpty()) {
            continue;
        }

        size_t currMaxPopCount = 0;
        const VdfConnection *currMaxConnection = NULL;

        // Find the connection whose mask's population count is bigger than 
        // the mask on any other connection.
        TF_FOR_ALL(c, o->output->GetConnections()) {

            // Only care about passing through to read/writes.
            const VdfOutput *assocOutput = 
                (*c)->GetTargetInput().GetAssociatedOutput();
            if (!assocOutput) {
                continue;
            }

            // Only care about passing through to outputs that are requested
            VdfMask::Bits inputDependencyMask;
            bool isTargetOutputRequested = 
                _IsTargetOutputRequested(
                    schedule, *assocOutput, **c, &inputDependencyMask);

            if (!isTargetOutputRequested) {
                continue;
            }

            // XXX: The code below makes it so that we pass the data along the
            //      connection with the biggest connection mask. This is not
            //      ideal, since we really should be passing the data along
            //      the connection with the most data requested! This
            //      way, we can also avoid keeping redundant data at this
            //      output. Unfortunately, doing so resulted in a few
            //      regressions.
            //
            // const size_t numSet = inputDependencyMask.GetNumSet();

            const size_t numSet = (*c)->GetMask().GetNumSet();
            if (numSet > currMaxPopCount) {
                currMaxConnection = (*c);
                currMaxPopCount = numSet;
            }
        }

        // No connection found, this output doesn't pass its buffer, move on.
        if (currMaxPopCount == 0 || currMaxConnection == NULL) {
            continue;
        }

        // Run through again gathering a union mask of all but the
        // currMaxConnection.
        VdfMask::Bits unionBits(o->requestMask.GetSize());
        TF_FOR_ALL(c, o->output->GetConnections()) {

            if ( *c != currMaxConnection ) {

                // Iterate over all outputs on the target node to determine
                // the which data on the input connection the target output
                // depends on.
                TF_FOR_ALL(o, (*c)->GetTargetNode().GetOutputsIterator()) {
                    const VdfOutput *output = o->second;

                    // Determine whether the target output is requested and
                    // obtain its dependency mask based on the incoming
                    // connection
                    VdfMask::Bits inputDependencyMask;
                    bool isTargetOutputRequested = 
                        _IsTargetOutputRequested(
                            schedule, *output, **c, &inputDependencyMask);

                    // The keep mask will need to be appended with the bits
                    // of the dependency mask, i.e. the data that the input
                    // connection supplies and that contributes to the
                    // the requested output we are currently looking at
                    if (isTargetOutputRequested) {
                        _AppendToMask(&unionBits, inputDependencyMask);
                    }
                }

            }

        }

        // Now AND that with the our scheduled mask, and that becomes the
        // subset mask to copy.
        if (TF_VERIFY(currMaxConnection)) {
            // We are only interested in bits overlapping with the request mask.
            unionBits &= o->requestMask.GetBits();

            // If what we are keeping is the entirety of the request mask,
            // there is no point in first passing the data, and then copying
            // all of it back to the source output. Instead, prevent it from
            // being passed down in the first place.
            if (unionBits == o->requestMask.GetBits()) {
                continue;
            }

            // Assign the union of the kept bits to the keep mask.
            o->keepMask = unionBits.IsAnySet()
                ? VdfMask(unionBits) 
                : VdfMask();
            
            o->passToOutput = 
                currMaxConnection->GetTargetInput().GetAssociatedOutput();
        }
    }
}

void 
VdfScheduler::_ScheduleBufferPasses(
    const VdfRequest &request,
    VdfSchedule *schedule)
{
    TRACE_FUNCTION();

    // Make sure that all read/writes on each scheduled node are requested
    TF_FOR_ALL (node, schedule->GetScheduleNodeVector()) {
        _ScheduleUnrequestedReadWrites(*node, schedule);
    }

    // Make sure that information for buffer passing and keeping is set up
    TF_FOR_ALL(node, schedule->GetScheduleNodeVector()) {
        _SetBufferPassDataForOutputs(&(*node), *schedule);
    }

    // In order to avoid passing buffers for outputs, which are requested, we
    // set the keep mask to include the whole request mask at each output in
    // the request.
    // Instead, we could just set the keep mask to the mask in the request,
    // meaning that we would only keep what's really been requested at that
    // output. If a subsequent request, with a new schedule, however, asks for
    // the same output again, keeping the whole request mask increases our
    // chances of being able to re-use that output cache!
    TF_FOR_ALL(i, request) {
        const VdfSchedule::OutputId &outputId = 
            schedule->GetOutputId(*(i->GetOutput()));
        TF_DEV_AXIOM(outputId.IsValid());
        schedule->SetKeepMask(outputId, schedule->GetRequestMask(outputId));
    }

    // Dump statistics about the number of copies scheduled
    #if 0    
    schedule->DumpBufferCopyStats();
    #endif
}


// Finds the first source output that feeds into \p output that has
// any affect in the current request.
static const VdfOutput * 
_FindPrevAffectiveOutput(
    const VdfOutput *output,
    TfHashSet<const VdfOutput*, TfHash> *visitedOutputs,
    const VdfSchedule &schedule)
{
    // Traverse until we find an affective (or a terminal) node.
    while (output) {
        // The current output should always be valid in the schedule
        const VdfSchedule::OutputId &currentOutputId =
            schedule.GetOutputId(*output);
        if (!TF_VERIFY(currentOutputId.IsValid())) {
            return NULL;
        }

        // If the current output does not have an associated input, i.e. it
        // won't have its data passed down, it is always considered affective.
        const VdfInput *assocInput = output->GetAssociatedInput();
        if (!assocInput) {
            return output;
        }

        // If there are no more input connections to traverse, we consider this
        // node affective, because this is as far as we can seek up while
        // passing through.
        const bool hasInputConnection =
            assocInput->GetConnections().size() == 1;
        if (!hasInputConnection) {
            return output;
        }

        // If the only incoming connection has an all-zeros mask, bail here. We
        // are not going to pass a buffer at all, in this case.
        if ((*assocInput)[0].GetMask().IsAllZeros()) {
            return output;
        }

        // If the current output has an affects mask, it is obviously
        // affective, so return the current output.
        const VdfMask &affectsMask = schedule.GetAffectsMask(currentOutputId);
        if (affectsMask.IsAnySet()) {
            return output;
        }

        // If there are any scheduled reads connected to this output, we cannot
        // simply pass through it, because we need to copy the kept bits back
        // to this output. If this is the case, we consider the current
        // output affective.
        const VdfMask &keepMask = schedule.GetKeepMask(currentOutputId);
        if (!keepMask.IsEmpty() || 
            schedule.GetPassToOutput(currentOutputId) == NULL) {
            return output;
        }

        // Lastly, we can seek ahead to the next associated output to figure
        // out if it has more than one read/write connection. If this is the
        // case, we found an output where multiple branches of the pool
        // converge, as it is the case with nodes just above parallel movers.
        // We cannot pass through nodes where the pool converges, unless we
        // copy (keep) the entire buffer, which we try to avoid here.
        const VdfOutput &nextOutput = (*assocInput)[0].GetSourceOutput();

        // Count the number of connected read/writes.
        const VdfConnectionVector &connections = nextOutput.GetConnections();
        if (connections.size() > 1) {
            size_t numNextReadWrites = 0;
            TF_FOR_ALL (it, connections) {
                const VdfConnection *c = *it;
                if (c->GetTargetInput().GetAssociatedOutput()) {
                    ++numNextReadWrites;
                }

                // More than one connected read/write? Cannot pass through
                // the current output.
                if (numNextReadWrites > 1) {
                    return output;
                }
            }
        }

        // The current output has been cleared for pass through, so we can
        // add it to the set of visited outputs.
        if (!visitedOutputs->insert(output).second) {
            return output;
        }

        // Continue the traversal in the input direction by setting the 
        // output to the source output on the connection.
        output = &nextOutput;
    }

    // No more outputs
    return output;
}

// Recursive helper for _ScheduleForPassThroughs.
static void 
_SchedulePassThroughForOutput(
    const VdfOutput *output,
    TfHashSet<const VdfOutput*, TfHash> *visitedOutputs,
    VdfSchedule *schedule)
{
    while (output) {
        // If this output has already been visited, bail out
        if (!visitedOutputs->insert(output).second) {
            return;
        }

        // Retrieve the current output's id from the schedule
        const VdfSchedule::OutputId &outputId = schedule->GetOutputId(*output);
        if (!outputId.IsValid()) {
            return;
        }

        // Find the output directly above
        const VdfOutput *immediateOutput =
            VdfGetAssociatedSourceOutput(*output);
        if (!immediateOutput) {
            return;
        }

        // Never pass through all-zeros connection masks
        if ((*output->GetAssociatedInput())[0].GetMask().IsAllZeros()) {
            return;
        }

        // Retrieve the immediate output's id from the schedule.
        const VdfSchedule::OutputId &immediateOutputId =
            schedule->GetOutputId(*immediateOutput);
        if (!immediateOutputId.IsValid()) {
            return;
        }

        // For this node, find the output to pass from
        const VdfOutput *passFromOutput =
            _FindPrevAffectiveOutput(
                immediateOutput, visitedOutputs, *schedule);
        if (passFromOutput && passFromOutput != immediateOutput) {
            // Get output id for the pass-from output
            const VdfSchedule::OutputId &passFromOutputId =
                schedule->GetOutputId(*passFromOutput);

            // We can only schedule pass-throughs for unaffective nodes. The
            // executor engines do not (yet) support passing to an affective
            // node.
            if (TF_VERIFY(immediateOutputId.IsValid()) &&
                TF_VERIFY(passFromOutputId.IsValid())) {
                // Tell the source output to get its buffer from the first
                // output that we've found that will provide an affected value
                schedule->SetFromBufferOutput(
                    immediateOutputId, passFromOutput);

                // For the passFromOutput, make sure to set the pass-to-output
                // to the immediate output that we will be passing to, now.
                if (schedule->GetPassToOutput(passFromOutputId)) {
                    schedule->SetPassToOutput(
                        passFromOutputId, immediateOutput);
                }
            }
        }

        // Move on to the next output in the chain, which is the output we are
        // passing from.
        output = passFromOutput;
    }
}

// Schedule the outputs from which buffers should be passed. This helps ensure
// that potentially large portions of the network that won't have any effect in
// this schedule are skipped when passing buffers.
void
VdfScheduler::_ScheduleForPassThroughs(
    const VdfRequest& request,
    VdfSchedule *schedule,
    const PoolPriorityVector &sortedPoolOutputs)
{
    TRACE_FUNCTION();

    // Visited outputs for cycle detection. Using this hash set, we can also
    // make sure to schedule every pool chain branch exactly once. There may be
    // multiple entry points into the pool, but since we start with the lowest
    // entry, every subsequent, higher entry will already be added to the
    // set of visited outputs.
    VdfOutputPtrSet visitedOutputs;

    // Process the queue of point pool outputs, starting with the lowest pool
    // output (which has the greatest pool chain index; pool outputs must
    // already be sorted!)
    TF_FOR_ALL(i, sortedPoolOutputs) {
        const VdfOutput *output = i->second;

        // Schedule pass throughs for the point pool branch as identified by
        // the point pool output retrieved from the queue. The function below
        // will bail out early, if the branch has already been scheduled for
        // pass throughs.
        _SchedulePassThroughForOutput(output, &visitedOutputs, schedule);
    }
}

// Produces a compressed bitset from an input bitset, by simple leaving bits
// within a certain range (the partition) flipped on, and unsetting all other
// bits. The partition size is driven by the grain size.
//
static TfCompressedBits
_ComputePartitionSubset(
    const uint32_t index,
    const uint32_t grainSize,
    const TfCompressedBits &bits)
{
    TfCompressedBits result;

    // The range of the bits to leave flipped on.
    const uint32_t partitionFirst = index * grainSize;
    const uint32_t partitionLast  = partitionFirst + grainSize - 1;

    // Iterate over all the platforms in the input bitset.
    using View = TfCompressedBits::PlatformsView;
    View platforms = bits.GetPlatformsView();

    for (View::const_iterator it=platforms.begin(), e=platforms.end(); it != e;
         ++it) {
        const uint32_t platformSize = it.GetPlatformSize();

        // Append unset platforms to the resulting bitset for any platform,
        // which is unset in the input bitset, or which is beyond the range
        // of the bits to leave flipped on.
        if (!it.IsSet() ||
            (*it + platformSize) <= partitionFirst ||
            *it > partitionLast) {
            result.Append(platformSize, false);
        }

        // For any platform that is set in the input bitset, append a set
        // platform to the resulting bitset, but make sure to trim the platform
        // to the range of the partition, as determined by the grain size.
        else {
            int32_t leadingZeros = partitionFirst - *it;
            leadingZeros = leadingZeros > 0 ? leadingZeros : 0;

            int32_t trailingZeros = *it + platformSize - partitionLast - 1;
            trailingZeros = trailingZeros > 0 ? trailingZeros : 0;

            int32_t numOnes = platformSize - leadingZeros - trailingZeros;
            
            result.Append(leadingZeros, false);
            result.Append(numOnes, true);
            result.Append(trailingZeros, false);
        }
    }

    TF_VERIFY(result.GetSize() == bits.GetSize());

    return result;
}

// For any partition that has bits set in the input bitset, flip on a bit in
// the output bitset. Note, the output bitset size = bits.GetSize() / grainSize.
//
static void
_GatherOccupiedPartitions(
    const uint32_t grainSize,
    const TfCompressedBits &bits,
    TfBits *occupied)
{
    using View = TfCompressedBits::PlatformsView;
    View platforms = bits.GetPlatformsView();

    for (View::const_iterator it=platforms.begin(), e=platforms.end(); it != e;
         ++it) {
        if (it.IsSet()) {
            const uint32_t platformFirst = *it;
            const uint32_t platformLast =
                platformFirst + it.GetPlatformSize() - 1;

            const uint32_t partitionFirst = platformFirst / grainSize;
            const uint32_t partitionLast = platformLast / grainSize;

            for (uint32_t p = partitionFirst; p <= partitionLast; ++p) {
                occupied->Set(p);
            }
        }
    }
}

// Compute the bitset for a given invocation from the request, affects and
// keep masks, as well as the partitions. 
//
static void
_ComputeInvocationBitsets(
    const VdfMask &requestMask,
    const VdfMask &affectsMask,
    const VdfMask &keepMask,
    const bool isAffective,
    const uint32_t numPartitions,
    const uint32_t grainSize,
    const VdfScheduleTaskIndex invocationIndex,
    const VdfScheduleTaskNum invocationNum,
    _Invocations *invocations)
{
    TRACE_FUNCTION();

    // Iterate over all partitions to check for overlap with the masks.
    uint32_t offset = 0;
    for (uint32_t i = 0; i < numPartitions; ++i) {

        // Which bits are requested in this partition?
        TfCompressedBits requested(
            _ComputePartitionSubset(i, grainSize, requestMask.GetBits()));

        // If there are no requested bits in this partition, bail out. Both
        // the affects mask and keep mask will be a subset of the request mask.
        if (requested.AreAllUnset()) {
            continue;
        }

        // The invocation masks.
        _InvocationBitsets *bitsets =
            &invocations->bitsets[invocationIndex + offset];

        // Assign the requested bits.
        bitsets->requested.Swap(requested);

        // Which bits are affected in this partition?
        if (isAffective) {
            bitsets->affected =
                _ComputePartitionSubset(i, grainSize, affectsMask.GetBits());
        }

        // Which bits are kept in this partition?
        if (!keepMask.IsEmpty()) {
            bitsets->kept =
                _ComputePartitionSubset(i, grainSize, keepMask.GetBits());
        }

        // Increment the invocation index counter.
        ++offset;
    }

    TF_VERIFY(offset == invocationNum);
}

// Given an output, find the output is sources its buffer from.
//
static const VdfOutput *
_FindFromBufferOutput(
    const VdfSchedule &schedule,
    const VdfOutput &output,
    VdfSchedule::OutputId oid)
{
    // Determine the next output in the pool chain.
    if (const VdfOutput *from = schedule.GetFromBufferOutput(oid)) {
        return from;
    }

    // If the current output does not have a from buffer source, find the next
    // associated output.
    const VdfOutput *source = VdfGetAssociatedSourceOutput(output);
    if (!source) {
        return nullptr;
    }

    // Determine whether that output is passing to the current output.
    VdfSchedule::OutputId fromid = schedule.GetOutputId(*source);
    if (!fromid.IsValid() || schedule.GetPassToOutput(fromid) != &output) {
        return nullptr;
    }

    return source;
}

// Given an output, find the next pool output in the pool chain.
//
static const VdfOutput *
_FindNextPoolOutput(
    const VdfSchedule &schedule,
    const VdfOutput &output,
    VdfSchedule::OutputId oid)
{
    // Only consider outputs as long as they are part of the pool chain.
    const VdfOutput *from = _FindFromBufferOutput(schedule, output, oid);
    return from && Vdf_IsPoolOutput(*from) ? from : nullptr;
}

// Creates node invocations for each node encountered along the pool chain
// terminating in \p output.
//
static void
_CreatePoolInvocations(
    const VdfOutput *output,
    const VdfSchedule *schedule,
    std::atomic<bool> visitedNodes[],
    _Invocations *invocations,
    std::vector<uint8_t> *hasInvocations,
    std::atomic<VdfScheduleTaskNum> *numPoolNodes,
    std::atomic<VdfScheduleTaskNum> *numPoolInvocations,
    std::atomic<VdfScheduleTaskNum> *numPoolInputsTasks,
    std::atomic<VdfScheduleTaskNum> *numPoolKeepTasks,
    WorkDispatcher *dispatcher)
{
    TRACE_FUNCTION();

    // Get the output id for the first output in the pool chain.
    const VdfSchedule::OutputId firstOid = schedule->GetOutputId(*output);

    // Determine the size of the request mask on the first output. This is
    // also the size for each one of the partitions.
    const size_t partitionSize = schedule->GetRequestMask(firstOid).GetSize();

    // The grain size for each partition. Currently, this grain size is
    // hardcoded here. The current size has been empirically determined to work
    // well over a broad range of networks. Note, that the grain size must
    // always be >= 5, and be divisible by 5, so not to split up packed
    // transforms in the point pool.
    //
    // XXX: In the future, we should take attribute boundaries into account
    //      when generating the different partitions. Currently, that strategy
    //      doesn't seem to speed things up a whole lot.
    //
    const size_t grainSize = 500;
    static_assert(grainSize >= 5, "grainSize cannot be smaller than 5.");
    static_assert(grainSize % 5 == 0, "grainSize must be divisible by 5.");

    // The number of partitions we need to generate.
    const size_t numPartitions = (partitionSize + grainSize - 1) / grainSize;

    // If there is only one partition to be generated, we do not need to
    // create node invocations. Bail out instead.
    if (numPartitions <= 1) {
        return;
    }

    // The current index for the next inputs, and keep task.
    VdfScheduleTaskIndex inputsTaskIndex = 0;
    VdfScheduleTaskIndex keepTaskIndex = 0;

    // Reserve some space in the invocations arrays.
    const size_t numReservedNodes = 1000;
    invocations->nodes.reserve(numReservedNodes);
    invocations->bitsets.reserve(numReservedNodes);

    // A bitset that indicates which partitions are occupied given some mask.
    TfBits occupied(numPartitions);

    // Visit every output in the pool chain, in order to determine the
    // partitioning of the data vectors.
    while (output) {

        // All nodes in the pool chain should only have one output, and
        // that output should be a pool output.
        const VdfNode &node = output->GetNode();
        if (!TF_VERIFY(node.GetNumOutputs() == 1) ||
            !TF_VERIFY(Vdf_IsPoolOutput(*output))) {
            break;
        }

        // Make sure this is a scheduled output.
        VdfSchedule::OutputId oid = schedule->GetOutputId(*output);
        if (!oid.IsValid()) {
            break;
        }

        // Only consider this output, if it passes its data.
        if (!schedule->GetPassToOutput(oid)) {
            break;
        }

        // The schedule node index.
        const int scheduleNodeIndex = schedule->GetScheduleNodeIndex(oid);
        TF_VERIFY(
            scheduleNodeIndex >= 0 &&
            static_cast<unsigned int>(scheduleNodeIndex) <
                schedule->GetScheduleNodeVector().size());

        // Bail out if this node has already been visited.
        if (visitedNodes[scheduleNodeIndex].exchange(true)) {
            break;
        }

        // Determine how many invocations this node will create.
        const VdfMask &requestMask = schedule->GetRequestMask(oid);
        occupied.ClearAll();
        _GatherOccupiedPartitions(grainSize, requestMask.GetBits(), &occupied);
        const VdfScheduleTaskNum numInvocations = occupied.GetNumSet();
        TF_VERIFY(numInvocations > 0);

        // Is this node affective?
        const bool isAffective = schedule->IsAffective(oid);

        // Keep mask.
        const VdfMask &keepMask = schedule->GetKeepMask(oid);

        // The invocation index.
        TF_VERIFY(invocations->bitsets.size() <
                std::numeric_limits<VdfScheduleTaskIndex>::max());
        const VdfScheduleTaskIndex invocationIndex =
            invocations->bitsets.size();

        // Add entries to the node invocations structure.
        invocations->nodes.emplace_back(
            scheduleNodeIndex, invocationIndex, numInvocations,
            isAffective ? inputsTaskIndex++ : VdfScheduleTaskInvalid,
            !keepMask.IsEmpty() ? keepTaskIndex++ : VdfScheduleTaskInvalid);
        invocations->bitsets.grow_by(numInvocations);

        // Concurrently build invocations for this node.
        dispatcher->Run(
            std::bind(
                &_ComputeInvocationBitsets,
                std::cref(requestMask),
                std::cref(schedule->GetAffectsMask(oid)),
                std::cref(keepMask),
                isAffective,
                numPartitions,
                grainSize,
                invocationIndex,
                numInvocations,
                invocations));

        // Mark this node as having invocations.
        TF_VERIFY(scheduleNodeIndex >= 0 &&
            static_cast<unsigned int>(scheduleNodeIndex) <
                hasInvocations->size());
        TF_VERIFY((*hasInvocations)[scheduleNodeIndex] == 0);
        (*hasInvocations)[scheduleNodeIndex] = 1;

        // Move on to the next output.
        output = _FindNextPoolOutput(*schedule, *output, oid);
    }

    // Set the number of inputs, and keep tasks.
    invocations->numInputsTasks = inputsTaskIndex;
    invocations->numKeepTasks = keepTaskIndex;

    // Account for the number of nodes, invocations, inputs tasks, and
    // keep tasks created for this pool chain.
    numPoolNodes->fetch_add(
        invocations->nodes.size(), std::memory_order_relaxed);
    numPoolInvocations->fetch_add(
        invocations->bitsets.size(), std::memory_order_relaxed);
    numPoolInputsTasks->fetch_add(
        invocations->numInputsTasks, std::memory_order_relaxed);
    numPoolKeepTasks->fetch_add(
        invocations->numKeepTasks, std::memory_order_relaxed);
}

// Produces tasks for each node invocation.
//
static void
_CreateInvocationTasks(
    const _Invocations &invocations,
    VdfScheduleTaskIndex offsetNodeIndex,
    VdfScheduleTaskIndex offsetInvocationIndex,
    VdfScheduleTaskIndex offsetInputsTaskIndex,
    VdfScheduleTaskIndex offsetKeepTaskIndex,
    std::vector<VdfScheduleNodeTasks> *nodeToComputeTasks,
    std::vector<VdfScheduleTaskIndex> *nodeToKeepTasks,
    Vdf_DefaultInitVector<VdfScheduleComputeTask> *computeTasks,
    size_t f,
    size_t l)
{
    TRACE_FUNCTION();

    // For each node, generate tasks.
    for (size_t i = f; i != l; ++i) {

        // Schedule node index.
        uint32_t scheduleNodeIndex = invocations.nodes[i].scheduleNodeIndex;

        // Generate an inputs task?
        const VdfScheduleTaskIndex inputsTaskIndex =
            invocations.nodes[i].inputsTaskIndex;

        // Generate the compute tasks.
        const VdfScheduleTaskNum invocationNum =
            invocations.nodes[i].invocationNum;
        for (VdfScheduleTaskIndex j = 0; j < invocationNum; ++j) {
            const VdfScheduleTaskIndex invocationIndex =
                invocations.nodes[i].invocationIndex + j;

            // The affected bits.
            const TfCompressedBits &affected = 
                invocations.bitsets[invocationIndex].affected;
            const bool isAffective =
                affected.GetSize() && !affected.AreAllUnset();

            // The kept bits.
            const TfCompressedBits &kept =
                invocations.bitsets[invocationIndex].kept;
            const bool hasKeep =
                kept.GetSize() && !kept.AreAllUnset();

            // Get the compute task.
            TF_VERIFY((offsetInvocationIndex + invocationIndex) <
                computeTasks->size());
            VdfScheduleComputeTask *computeTask =
                &(*computeTasks)[offsetInvocationIndex + invocationIndex];

            // Set the relevant data on the compute task.
            computeTask->invocationIndex =
                offsetInvocationIndex + invocationIndex;

            // Not every compute task also has an inputs task. We try to avoid
            // creating inputs tasks for nodes that aren't affective and
            // therefore don't consume any inputs.
            computeTask->inputsTaskIndex =
                isAffective && !VdfScheduleTaskIsInvalid(inputsTaskIndex)
                    ? offsetInputsTaskIndex + inputsTaskIndex
                    : VdfScheduleTaskInvalid;

            // For each compute task that invokes the same node, we only create
            // one prep task. The single prep task is shared between all the
            // invocations, because we only need to prepare a node once per
            // evaluation iteration.
            computeTask->prepTaskIndex = offsetNodeIndex + i;

            // Assign the compute task flags.
            computeTask->flags.isAffective = isAffective;
            computeTask->flags.hasKeep = hasKeep;
        }

        // Set the node-to-tasks map entry.
        const VdfScheduleTaskIndex computeTaskIndex =
            offsetInvocationIndex + invocations.nodes[i].invocationIndex;
        TF_VERIFY(scheduleNodeIndex < nodeToComputeTasks->size());
        VdfScheduleNodeTasks *nodeToComputeTask =
            &(*nodeToComputeTasks)[scheduleNodeIndex];
        nodeToComputeTask->taskId = computeTaskIndex;
        nodeToComputeTask->taskNum = invocationNum;

        // Generate a keep task?
        const VdfScheduleTaskIndex keepTaskIndex =
            invocations.nodes[i].keepTaskIndex;
        if (!VdfScheduleTaskIsInvalid(keepTaskIndex)) {
            // Set the node-to-tasks map entry.
            TF_VERIFY(scheduleNodeIndex < nodeToKeepTasks->size());
            (*nodeToKeepTasks)[scheduleNodeIndex] =
                offsetKeepTaskIndex + keepTaskIndex;
        }
    }
}

// Inserts node invocations into the schedule from each one of the node
// invocation bitsets produced during an early stage of scheduling.
//
static void
_CreateInvocations(
    const _Invocations &invocations,
    const size_t offsetInvocationIndex,
    Vdf_DefaultInitVector<VdfScheduleNodeInvocation> *nodeInvocations)
{
    TRACE_FUNCTION();

    // Iterate over the bitsets created for each invocation, and
    // add those to the schedule as node invocations.
    //
    // XXX: Note that the construction of masks from bitsets contends
    //      on a global lock.
    //
    for (size_t i = 0; i < invocations.bitsets.size(); ++i) {

        // Obtains the bitset corresponding to this invocation.
        const _InvocationBitsets &bitsets = invocations.bitsets[i];
        TF_VERIFY(!bitsets.requested.AreAllUnset());

        // Retrieve the node invocation from the schedule.
        const size_t invocationIndex = offsetInvocationIndex + i;
        TF_VERIFY(invocationIndex < nodeInvocations->size());
        VdfScheduleNodeInvocation *nodeInvocation = 
            &(*nodeInvocations)[invocationIndex];

        // Apply the request mask.
        nodeInvocation->requestMask = VdfMask(bitsets.requested);

        // Set the affects mask, if not all-zeros.
        if (!bitsets.affected.AreAllUnset()) {
            nodeInvocation->affectsMask = VdfMask(bitsets.affected);
        }

        // Set the keep mask, if not all zeros.
        if (!bitsets.kept.AreAllUnset()) {
            nodeInvocation->keepMask = VdfMask(bitsets.kept);
        }
    }
}

// Insert invocations and tasks into the schedule.
//
static void
_CreateInvocationsAndTasks(
    const _Invocations &invocations,
    VdfScheduleTaskIndex offsetNodeIndex,
    VdfScheduleTaskIndex offsetInvocationIndex,
    VdfScheduleTaskIndex offsetInputsTaskIndex,
    VdfScheduleTaskIndex offsetKeepTaskIndex,
    std::vector<VdfScheduleNodeTasks> *nodeToComputeTasks,
    std::vector<VdfScheduleTaskIndex> *nodeToKeepTasks,
    Vdf_DefaultInitVector<VdfScheduleNodeInvocation> *nodeInvocations,
    Vdf_DefaultInitVector<VdfScheduleComputeTask> *computeTasks,
    WorkDispatcher *dispatcher)
{
    TRACE_FUNCTION();

    // Generate invocations for this node
    dispatcher->Run(
        std::bind(
            &_CreateInvocations,
            std::cref(invocations),
            offsetInvocationIndex,
            nodeInvocations));

    // Concurrently, generate tasks for this node
    WorkParallelForN(invocations.nodes.size(), 
        std::bind(
            &_CreateInvocationTasks,
            std::cref(invocations),
            offsetNodeIndex,
            offsetInvocationIndex,
            offsetInputsTaskIndex,
            offsetKeepTaskIndex,
            nodeToComputeTasks,
            nodeToKeepTasks,
            computeTasks,
            std::placeholders::_1,
            std::placeholders::_2));
}

// Returns true if the given node has at least one prereq and at least one
// read dependency.
//
static bool
_HasPrereqsAndReads(const VdfSchedule &schedule, const VdfNode &node)
{
    bool hasPrereq = false;
    bool hasRead = false;

    for (const std::pair<TfToken, VdfInput *> &i : node.GetInputsIterator()) {
        const VdfInput *input = i.second;

        // Found a new prereq input?
        if (!hasPrereq && input->GetSpec().IsPrerequisite()) {
            for (const VdfConnection *c : input->GetConnections()) {
                VdfSchedule::OutputId oid =
                    schedule.GetOutputId(c->GetSourceOutput());
                if (oid.IsValid()) {
                    hasPrereq = true;
                    break;
                }
            }
        }

        // Found a new read input?
        else if (!hasRead && !input->GetAssociatedOutput()) {
            for (const VdfConnection *c : input->GetConnections()) {
                VdfSchedule::OutputId oid =
                    schedule.GetOutputId(c->GetSourceOutput());
                if (oid.IsValid()) {
                    hasRead = true;
                    break;
                }
            }
        }

        // If we found at least one prereq, and at least one read,
        // we can bail out early.
        if (hasPrereq && hasRead) {
            return true;
        }
    }

    // No reads, or no prereqs.
    return false;
}

// Insert tasks into the schedule for any node that has only a single
// invocation (i.e. non pool chain nodes).
//
static void
_CreateSingularTasks(
    const VdfSchedule &schedule,
    const std::vector<uint8_t> &hasInvocations,
    VdfScheduleTaskIndex offsetComputeTaskIndex,
    VdfScheduleTaskIndex offsetInputsTaskIndex,
    std::vector<VdfScheduleNodeTasks> *nodeToComputeTasks,
    Vdf_DefaultInitVector<VdfScheduleComputeTask> *computeTasks,
    VdfScheduleTaskNum *numInputsTasks)
{
    TRACE_FUNCTION();

    const VdfSchedule::ScheduleNodeVector &scheduleNodes =
        schedule.GetScheduleNodeVector();

    for (size_t i = 0; i < scheduleNodes.size(); ++i) {

        // Ignore nodes with multiple invocations.
        if (hasInvocations[i] > 0) {
            continue;
        }

        // Is this node affective?
        const bool isAffective = scheduleNodes[i].affective;

        // Do we need to create an inputs task? Note, that we only ever
        // create inputs tasks for nodes that have at least one prereq and
        // at least one read. Otherwise, there is no point in running the
        // reads concurrently with the prereqs, and a separate task is
        // therefore not required.
        VdfScheduleTaskIndex inputsTaskIndex = VdfScheduleTaskInvalid;
        if (isAffective && !scheduleNodes[i].node->IsSpeculationNode() &&
            _HasPrereqsAndReads(schedule, *scheduleNodes[i].node)) {
            inputsTaskIndex = offsetInputsTaskIndex++;
        }

        // Create the compute task.
        const VdfScheduleTaskIndex computeTaskIndex = offsetComputeTaskIndex++;

        TF_VERIFY(computeTaskIndex < computeTasks->size());
        VdfScheduleComputeTask *computeTask =
            &(*computeTasks)[computeTaskIndex];

        computeTask->invocationIndex = VdfScheduleTaskInvalid;
        computeTask->inputsTaskIndex = inputsTaskIndex;
        computeTask->prepTaskIndex = VdfScheduleTaskInvalid;

        // Note, nodes with only a single invocation never have prep tasks.
        // Since there is only one compute task associated with such nodes,
        // there is only one task that can ever prep that node in the first
        // place.

        // Is this task affective?
        computeTask->flags.isAffective = isAffective;

        // Does this task keep any data on any one of its outputs?
        computeTask->flags.hasKeep = false;
        for (const VdfScheduleOutput &so : scheduleNodes[i].outputs) {
            if (!so.keepMask.IsEmpty()) {
                computeTask->flags.hasKeep = true;
                break;
            }
        }

        // Note, we do not create separate keep tasks for nodes with only
        // a single invocation (and therefore only a single compute task),
        // because the single compute task can assume the responsibility of
        // keeping the relevant data in this case.

        TF_VERIFY(i < nodeToComputeTasks->size());
        VdfScheduleNodeTasks *nodeToComputeTask = &(*nodeToComputeTasks)[i];
        nodeToComputeTask->taskId = computeTaskIndex;
        nodeToComputeTask->taskNum = 1;
    }

    // The total number of compute tasks should be equal to the size of the
    // compute task array, at this point.
    TF_VERIFY(offsetComputeTaskIndex == computeTasks->size());

    // Store the total number of inputs tasks created.
    *numInputsTasks = offsetInputsTaskIndex;
}

// Gather all dependencies (read/writes, prereqs and reads) for a single
// scheduled node.
//
static void
_GatherNodeDependencies(
    const VdfSchedule &schedule,
    const VdfScheduleNode &scheduleNode, 
    _NodeDependencies *dependencies)
{
    const VdfNode &node = *scheduleNode.node;

    // Speculation nodes have no dependencies with respect to scheduling
    // the dependency task graph.
    if (node.IsSpeculationNode()) {
        return;
    }

    // Is this node affective?
    const bool isAffective = scheduleNode.affective;

    // For each input on the node.
    for (const VdfScheduleInput &scheduleInput : schedule.GetInputs(node)) {
        const VdfInput &input = *scheduleInput.input;

        // Is this a read/write?
        const bool isRw = input.GetAssociatedOutput();

        // Prereq
        if (isAffective && input.GetSpec().IsPrerequisite()) {
            TF_VERIFY(!isRw);
            dependencies->prereqs.emplace_back(&scheduleInput);
        }

        // Read
        else if (isAffective && !isRw) {
            dependencies->reads.emplace_back(&scheduleInput);
        }

        // Read/Write
        else if (isRw) {
            TF_VERIFY(input.GetNumConnections() <= 1);
            dependencies->rws.emplace_back(&scheduleInput);
        }
    }
}

// Gather dependencies for schedule nodes within a given range.
//
static void
_GatherNodeDependenciesInRange(
    const VdfSchedule &schedule,
    std::vector<_NodeDependencies> *nodeToDependencies,
    size_t f,
    size_t l)
{
    const VdfSchedule::ScheduleNodeVector &nodes = 
        schedule.GetScheduleNodeVector(); 

    for (size_t i = f; i != l; ++i) {
        _NodeDependencies *dependencies = &(*nodeToDependencies)[i];
        _GatherNodeDependencies(schedule, nodes[i], dependencies);
    }
}

// Gather dependencies for all schedule nodes.
//
static void
_GatherAllNodeDependencies(
    const VdfSchedule &schedule,
    std::vector<_NodeDependencies> *nodeToDependencies)
{
    // The number of scheduled nodes.
    const size_t numScheduledNodes = schedule.GetScheduleNodeVector().size();

    // Prepare the array that tracks node dependencies.
    nodeToDependencies->resize(numScheduledNodes);

    // Gather up all the node dependencies.
    WorkParallelForN(
        numScheduledNodes,
        std::bind(
            &_GatherNodeDependenciesInRange,
            std::cref(schedule),
            nodeToDependencies,
            std::placeholders::_1,
            std::placeholders::_2));
}

static VdfScheduleInputDependencyUniqueIndex
_GetOrCreateUniqueInputDependencyIndex(
    const VdfOutput &output,
    const VdfMask &mask,
    _OutputToIndexMap *uniqueIndices)
{
    return uniqueIndices->insert(
        std::make_pair(
            VdfMaskedOutput(const_cast<VdfOutput *>(&output), mask),
            uniqueIndices->size())).first->second;
}

// Establish task dependencies for a single scheduled source output.
//
static void
_EstablishTaskDependency(
    const VdfSchedule &schedule,
    const VdfSchedule::OutputId fromOutputId,
    const bool isPassTo,
    const VdfMask &dependencyMask,
    std::vector<VdfScheduleInputDependency> *inputDependencies,
    _OutputToIndexMap *uniqueIndices,
    VdfScheduleTaskIndex *startHint)
{
    // Get the source output and node.
    const VdfOutput &output = *schedule.GetOutput(fromOutputId);
    const VdfNode &node = output.GetNode();

    // If the source output doesn't pass its data to the output establishing
    // this dependency, we need to check if there is a keep task at the source
    // end, i.e. we may need to establish a dependency on that keep task.
    if (!isPassTo) {
        const VdfMask &keepMask = schedule.GetKeepMask(fromOutputId);
        if (!keepMask.IsEmpty()) {
            if (!keepMask.Overlaps(dependencyMask)) {
                return;
            }

            const VdfScheduleTaskIndex keepTaskIndex =
                schedule.GetKeepTaskIndex(node);
            if (!VdfScheduleTaskIsInvalid(keepTaskIndex)) {
                // XXX: It is safe to generate narrower input dependencies by
                //      intersecting the keepMask with the dependencyMask.
                //      However, doing so will generate more unique indices,
                //      which in turn will result in more cache lookups during
                //      evaluation.
                inputDependencies->emplace_back(
                    VdfScheduleInputDependency {
                        _GetOrCreateUniqueInputDependencyIndex(
                            output, keepMask, uniqueIndices),
                        output,
                        keepMask,
                        keepTaskIndex,
                        0
                    });
                return;
            }
        }
    }

    // If we are not sourcing the data from a keep task, we need to establish
    // dependencies on one or more compute tasks. Those are the compute tasks
    // associated with the node at the source.
    const VdfSchedule::TaskIdRange computeTasks =
        schedule.GetComputeTaskIds(node);

    // Get the last set bit in the dependency mask. We'll use it to skip
    // all irrelevant request masks on nodes with multiple compute tasks. The
    // compute tasks are partitioned in ascending bit order, therefore we can
    // bail out once we found the last overlapping partition.
    const size_t lastRelevantBit = dependencyMask.GetLastSet();

    // Look at each compute task produced by the source node, and check whether
    // that task produces data that overlaps with our dependency mask. If that's
    // the case, we need to establish a dependency on that task.
    // Note, that we find the first task that overlaps, as well as the last
    // task that overlaps and then establish dependencies on all tasks in
    // between. Currently, it is generally true that there is a contiguous
    // range of tasks that will be overlapping.
    VdfScheduleTaskId computeTaskBegin =
        std::numeric_limits<VdfScheduleTaskId>::max();
    VdfScheduleTaskId computeTaskEnd =
        std::numeric_limits<VdfScheduleTaskId>::min();

    VdfScheduleTaskId computeTaskId = *computeTasks.begin();
    VdfScheduleTaskId end = computeTaskId + computeTasks.size();

    // startHint will be 0 unless we have an offset to apply.
    computeTaskId += *startHint;
    TF_VERIFY(computeTaskId < end);

    for (; computeTaskId < end; computeTaskId++) {
        // Get the compute task for this computeTaskId.
        const VdfScheduleComputeTask &computeTask = 
            schedule.GetComputeTask(computeTaskId);

        // Any compute task without an associated invocation is dependent
        // by default. Since we know we are dependent on the source node, and
        // it has only one task, we've just found it.
        bool isDependent =
            VdfScheduleTaskIsInvalid(computeTask.invocationIndex);

        // If this task isn't dependent by default, we need to figure out if
        // its request mask overlaps with our dependency mask.
        if (!isDependent) {
            const VdfMask &requestMask =
                schedule.GetRequestMask(computeTask.invocationIndex);

            // Since the request masks are partitioned, and partitions are
            // sorted in ascending bit order, we can bail out once we found
            // the last relevant partition. This saves us from potentially
            // calling VdfMask::Overlaps a bunch more times.
            if (requestMask.GetFirstSet() > lastRelevantBit) {
                *startHint = computeTaskId - *computeTasks.begin();
                break;
            }

            // We are dependent on this compute task if its request mask
            // overlaps with the specified dependency mask.
            isDependent = requestMask.Overlaps(dependencyMask);
        }

        // Update the first and last task found, if we are indeed dependent
        // on this one.
        if (isDependent) {
            computeTaskBegin = std::min(computeTaskBegin, computeTaskId);
            computeTaskEnd = std::max(computeTaskEnd, computeTaskId);
        }
    }

    // Set the task indices on the input dependency.
    const VdfMask &fromRequestMask = schedule.GetRequestMask(fromOutputId);
    TF_VERIFY(computeTaskBegin <= computeTaskEnd);
    inputDependencies->emplace_back(
        VdfScheduleInputDependency {
            _GetOrCreateUniqueInputDependencyIndex(
                output, fromRequestMask, uniqueIndices),
            output,
            fromRequestMask,
            computeTaskBegin,
            computeTaskEnd - computeTaskBegin + 1
        });
}

// Establish input dependencies for read/write connections.
//
static std::pair<VdfScheduleTaskIndex, VdfScheduleTaskNum>
_EstablishReadWriteDependencies(
    const VdfSchedule &schedule,
    const VdfScheduleTaskIndex invocationIndex,
    const std::vector<const VdfScheduleInput *>::const_iterator begin,
    const std::vector<const VdfScheduleInput *>::const_iterator end,
    std::vector<VdfScheduleInputDependency> *inputDependencies,
    _OutputToIndexMap *uniqueIndices,
    VdfScheduleTaskIndex *startHint)
{
    TF_VERIFY(inputDependencies->size() <
        std::numeric_limits<VdfScheduleTaskIndex>::max());
    const VdfScheduleTaskIndex index = inputDependencies->size();

    std::vector<const VdfScheduleInput *>::const_iterator it = begin;
    for (; it != end; ++it) {
        const VdfScheduleInput *scheduleInput = *it;

        // Get the associated output.
        const VdfOutput &ao = *scheduleInput->input->GetAssociatedOutput();
        const VdfSchedule::OutputId aoid = schedule.GetOutputId(ao);

        // Get the from buffer output, if any.
        const VdfOutput *from = schedule.GetFromBufferOutput(aoid);

        // Get the source output.
        const VdfOutput &source = from 
            ? *from
            : *scheduleInput->source;            
        const VdfSchedule::OutputId &sourceId =
            schedule.GetOutputId(source);

        // Are we passing the buffer from the source?
        const bool isPassTo = from
            ? true
            : schedule.GetPassToOutput(sourceId) == &ao;

        // Get the request mask. Note, that if we are looking at a node
        // invocation, we use the request mask from that invocation. This is
        // so that when we establish the read/write dependency, we only
        // establish dependencies on compute tasks that produce values in our
        // invocation request mask. This algorithm is essentially what does the
        // strip-mining!
        const VdfMask &requestMask =
            !VdfScheduleTaskIsInvalid(invocationIndex)
                ? schedule.GetRequestMask(invocationIndex)
                : schedule.GetRequestMask(aoid);

        // Establish the task dependency.
        _EstablishTaskDependency(
            schedule,
            sourceId,
            isPassTo,
            requestMask,
            inputDependencies,
            uniqueIndices,
            startHint);
    }

    const VdfScheduleTaskNum num = inputDependencies->size() - index;
    return std::make_pair(index, num);
}

// Establish input dependencies for the read (or prereq) connections.
//
static std::pair<VdfScheduleTaskIndex, VdfScheduleTaskNum>
_EstablishReadDependencies(
    const VdfSchedule &schedule,
    const std::vector<const VdfScheduleInput *>::const_iterator begin,
    const std::vector<const VdfScheduleInput *>::const_iterator end,
    std::vector<VdfScheduleInputDependency> *inputDependencies,
    _OutputToIndexMap *uniqueIndices)
{
    TF_VERIFY(inputDependencies->size() <
        std::numeric_limits<VdfScheduleTaskIndex>::max());
    const VdfScheduleTaskIndex index = inputDependencies->size();

    std::vector<const VdfScheduleInput *>::const_iterator it = begin;
    for (; it != end; ++it) {
        const VdfScheduleInput *scheduleInput = *it;

        // Get the source output.
        const VdfSchedule::OutputId sourceId =
            schedule.GetOutputId(*scheduleInput->source);

        VdfScheduleTaskIndex startHint = 0;

        // Establish the task dependency.
        _EstablishTaskDependency(
            schedule,
            sourceId,
            /* isPassTo = */ false,
            scheduleInput->mask,
            inputDependencies,
            uniqueIndices,
            &startHint);
    }

    const VdfScheduleTaskNum num = inputDependencies->size() - index;
    return std::make_pair(index, num);
}

// Insert input dependencies for each schedule node.
//
static size_t
_InsertInputDependencies(
    VdfSchedule *schedule,
    const std::vector<_NodeDependencies> &nodeToDependencies,
    Vdf_DefaultInitVector<VdfScheduleComputeTask> *computeTasks,
    Vdf_DefaultInitVector<VdfScheduleInputsTask> *inputsTasks,
    std::vector<VdfScheduleInputDependency> *inputDependencies)
{
    TRACE_FUNCTION();

    // Reserve some storage space for the input dependencies array. This is a
    // guesstimate.
    inputDependencies->reserve(computeTasks->size() + inputsTasks->size());

    // Maps outputs to their assigned sequential indices.
    _OutputToIndexMap uniqueIndices;

    // Iterate over all schedule nodes, and produce the input dependencies.
    VdfSchedule::ScheduleNodeVector &scheduleNodes =
        schedule->GetScheduleNodeVector();
    for (size_t i = 0; i < scheduleNodes.size(); ++i) {

        // The schedule node and VdfNode.
        const VdfScheduleNode &scheduleNode = scheduleNodes[i];
        const VdfNode &node = *scheduleNode.node;

        // The per-node dependencies, gathered earlier.
        const _NodeDependencies &nodeDependencies = nodeToDependencies[i];

        // Find all the compute tasks for the given node.
        const VdfSchedule::TaskIdRange computeTaskIndexRange =
            schedule->GetComputeTaskIds(node);
        if (computeTaskIndexRange.empty()) {
            continue;
        }

        VdfScheduleTaskIndex startHint = 0;

        // For all the compute tasks associated with this node, produce
        // read/write input dependencies. We produce these first, because
        // during evaluation those will be read from memory, first!
        VdfScheduleTaskIndex inputsTaskIndex = VdfScheduleTaskInvalid;
        for (const VdfScheduleTaskIndex cti : computeTaskIndexRange) {
            VdfScheduleComputeTask *computeTask = &(*computeTasks)[cti];

            // Not all invocations of a node have an inputs task, but all the
            // one that do must have the same one! Store that inputs task
            // index for later.
            if (!VdfScheduleTaskIsInvalid(computeTask->inputsTaskIndex)) {
                TF_VERIFY(
                    VdfScheduleTaskIsInvalid(inputsTaskIndex) ||
                    inputsTaskIndex == computeTask->inputsTaskIndex); 
                inputsTaskIndex = computeTask->inputsTaskIndex;
            }

            // Insert the read/write dependencies.
            std::pair<VdfScheduleTaskIndex, VdfScheduleTaskNum> rwIndices =
                _EstablishReadWriteDependencies(
                    *schedule,
                    computeTask->invocationIndex,
                    nodeDependencies.rws.begin(),
                    nodeDependencies.rws.end(),
                    inputDependencies,
                    &uniqueIndices,
                    &startHint);

            // Read/writes are always required.
            computeTask->requiredsIndex = rwIndices.first;
            computeTask->requiredsNum = rwIndices.second;
        }

        // If there isn't an inputs task, but the node has more than a single
        // invocation, we are done. The read/writes is all we need!
        if (VdfScheduleTaskIsInvalid(inputsTaskIndex) &&
            computeTaskIndexRange.size() > 1) {
            continue;
        }

        // Get the inputs task, if there is one.
        VdfScheduleInputsTask *inputsTask = nullptr;
        if (!VdfScheduleTaskIsInvalid(inputsTaskIndex)) {
            inputsTask = &(*inputsTasks)[inputsTaskIndex];
        }

        // Insert input dependencies for prereqs.
        std::pair<VdfScheduleTaskIndex, VdfScheduleTaskNum> prereqIndices =
            _EstablishReadDependencies(
                *schedule,
                nodeDependencies.prereqs.begin(),
                nodeDependencies.prereqs.end(),
                inputDependencies,
                &uniqueIndices);

        // Insert input dependencies for reads.
        std::pair<VdfScheduleTaskIndex, VdfScheduleTaskNum> readIndices =
            _EstablishReadDependencies(
                *schedule,
                nodeDependencies.reads.begin(),
                nodeDependencies.reads.end(),
                inputDependencies,
                &uniqueIndices);

        // If there is an inputs task, synchronize it on the prereqs and reads.
        // We consider all the reads optional, i.e. dependent on the values of
        // the prereqs. During evaluation, those may be required... or not.
        if (inputsTask) {
            inputsTask->inputDepIndex = prereqIndices.first;
            inputsTask->prereqsNum = prereqIndices.second;
            inputsTask->optionalsNum = readIndices.second;
        }

        // Otherwise, add the prereqs and reads to the compute task. They are
        // required at this point, because only inputs tasks are clever enough
        // to run prereqs and optionals concurrently with required inputs.
        else {
            TF_VERIFY(computeTaskIndexRange.size() == 1);
            const VdfScheduleTaskIndex computeTaskIndex =
                *computeTaskIndexRange.begin();
            VdfScheduleComputeTask *computeTask =
                &(*computeTasks)[computeTaskIndex];
            computeTask->requiredsNum +=
                (prereqIndices.second + readIndices.second);
        }
    }

    // Assign the unique indices to all scheduled output.
    for (size_t i = 0; i < scheduleNodes.size(); ++i) {
        for (VdfScheduleOutput &scheduleOutput : scheduleNodes[i].outputs) {
            // We currently only read the unique index when passing buffers, so
            // we can avoid a bunch of work if the output does not pass its
            // buffer.
            if (!scheduleOutput.passToOutput) {
                continue;
            }

            _OutputToIndexMap::const_iterator it = uniqueIndices.find(
                VdfMaskedOutput(
                    const_cast<VdfOutput *>(scheduleOutput.output),
                    scheduleOutput.requestMask));

            // Outputs in the request, as well as outputs that are skipped due
            // to from-buffer passing, won't be pulled in as dependencies via
            // a connection and will thus not have a unique index assigned.
            if (it != uniqueIndices.end()) {
                scheduleOutput.uniqueIndex = it->second;
            }
        }
    }

    // Return the number of dependency indices created.
    return uniqueIndices.size();
}

// Generate tasks and invocations for all nodes in the schedule.
//
void
VdfScheduler::_GenerateTasks(
    VdfSchedule *schedule,
    const PoolPriorityVector &sortedPoolOutputs)
{
    TRACE_FUNCTION();

    // The number of scheduled nodes.
    const size_t numScheduledNodes = schedule->_nodes.size();

    // Schedule node index to boolean value indicating whether the
    // scheduled node has multiple invocations.
    // Note, this is NOT a std::vector<bool>, because we will be modifying the
    // entries concurrently, and therefore can't use the std::vector<bool>
    // bitset specialization!
    std::vector<uint8_t> hasInvocations(numScheduledNodes, 0);

    // The array of per-pool-chain invocations. Note, that this vector will
    // be appended to, while invocations are being generated concurrently.
    tbb::concurrent_vector<_Invocations> allInvocations;

    // Account for the number of nodes, invocations and inputs tasks requested
    // on behalf of any of the nodes in a pool chain.
    std::atomic<VdfScheduleTaskNum> numPoolNodes(0);
    std::atomic<VdfScheduleTaskNum> numPoolInvocations(0);
    std::atomic<VdfScheduleTaskNum> numPoolInputsTasks(0);
    std::atomic<VdfScheduleTaskNum> numPoolKeepTasks(0);

    // Keep track of visited nodes, such that the same pool chain will not be
    // entered multiple times.
    std::unique_ptr<std::atomic<bool>[]> visitedNodes(
        new std::atomic<bool>[numScheduledNodes]);
    char *const visitedNodesPtr = reinterpret_cast<char *>(visitedNodes.get());
    memset(visitedNodesPtr, 0,
           sizeof(std::atomic<bool>) * numScheduledNodes);

    // The dispatcher to run the concurrent computations on.
    WorkDispatcher dispatcher;

    // For each distinct pool chain, create a set of invocations for the
    // nodes in the pool chain.
    for (size_t i = 0; i < sortedPoolOutputs.size(); ++i) {
        const VdfOutput &output = *sortedPoolOutputs[i].second;

        // Is this output at the end of a pool chain? If not, skip ahead
        // to the next output.
        const VdfSchedule::OutputId oid = schedule->GetOutputId(output);
        if (schedule->GetPassToOutput(oid)) {
            continue;
        }

        // Get the first output that passes its buffer, and start creating
        // node invocations along the pool chain.
        if (const VdfOutput *from =
                _FindNextPoolOutput(*schedule, output, oid)) {
            dispatcher.Run(
                std::bind(
                    &_CreatePoolInvocations,
                    from,
                    schedule,
                    visitedNodes.get(),
                    &(*allInvocations.emplace_back()),
                    &hasInvocations,
                    &numPoolNodes,
                    &numPoolInvocations,
                    &numPoolInputsTasks,
                    &numPoolKeepTasks,
                    &dispatcher));
        }
    }

    // Make sure that the arrays for node-to-task inversions are properly sized.
    const VdfScheduleNodeTasks defaultNodeTasks = { 0, 0 };
    schedule->_nodesToComputeTasks.resize(numScheduledNodes, defaultNodeTasks);
    schedule->_nodesToKeepTasks.resize(
        numScheduledNodes, VdfScheduleTaskInvalid);

    // Before proceeding, Wait until all pool chains have been processed.
    dispatcher.Wait();

    // Make sure that the arrays in the schedule are properly sized.
    const size_t numScheduleComputeTasks =
        numScheduledNodes - numPoolNodes + numPoolInvocations;
    schedule->_nodeInvocations.resize(numPoolInvocations);
    schedule->_computeTasks.resize(numScheduleComputeTasks);
    schedule->_inputsTasks.resize(
        numScheduledNodes - numPoolNodes + numPoolInputsTasks);
    schedule->_numKeepTasks = numPoolKeepTasks;
    schedule->_numPrepTasks = numPoolNodes;

    // Create tasks for the different invocations.
    VdfScheduleTaskIndex offsetNodeIndex = 0;
    VdfScheduleTaskIndex offsetInvocationIndex = 0;
    VdfScheduleTaskIndex offsetInputsTaskIndex = 0;
    VdfScheduleTaskIndex offsetKeepTaskIndex = 0;
    for (const _Invocations &invocations : allInvocations) {
        dispatcher.Run(
            std::bind(
                &_CreateInvocationsAndTasks,
                std::cref(invocations),
                offsetNodeIndex,
                offsetInvocationIndex,
                offsetInputsTaskIndex,
                offsetKeepTaskIndex,
                &schedule->_nodesToComputeTasks,
                &schedule->_nodesToKeepTasks,
                &schedule->_nodeInvocations,
                &schedule->_computeTasks,
                &dispatcher));

        // Offset the indices into the array for each chain of invocations.
        offsetNodeIndex += invocations.nodes.size();
        offsetInvocationIndex += invocations.bitsets.size();
        offsetInputsTaskIndex += invocations.numInputsTasks;
        offsetKeepTaskIndex += invocations.numKeepTasks;
    }

    // Create tasks for all nodes with singular invocations.
    VdfScheduleTaskNum numInputsTasks = 0;
    dispatcher.Run(
        std::bind(
            &_CreateSingularTasks,
            std::cref(*schedule),
            std::cref(hasInvocations),
            offsetInvocationIndex,
            offsetInputsTaskIndex,
            &schedule->_nodesToComputeTasks,
            &schedule->_computeTasks,
            &numInputsTasks));

    // Make sure that all tasks and invocations have been created.
    dispatcher.Wait();

    // Resize the inputs tasks array to fit the number of inputs tasks created.
    // We may end up creating a smaller number of tasks than initially assumed.
    TF_VERIFY(numInputsTasks <= schedule->_inputsTasks.size());
    schedule->_inputsTasks.resize(numInputsTasks);
}

// Schedule the task graph by producing invocations, tasks and dependencies
// between tasks.
//
void
VdfScheduler::_ScheduleTaskGraph(
    VdfSchedule *schedule,
    const PoolPriorityVector &sortedPoolOutputs)
{
    TRACE_FUNCTION();

    // An isolated work dispatcher for doing some of the task graph
    // generation in parallel.
    WorkWithScopedParallelism([&]() {
            WorkDispatcher dispatcher;

            // Generate compute, input and keep tasks for all the scheduled
            // nodes.
            dispatcher.Run(
                std::bind(&_GenerateTasks,
                          schedule, std::cref(sortedPoolOutputs)));

            // Gather dependencies for all scheduled nodes.
            std::vector<_NodeDependencies> nodeToDependencies;
            dispatcher.Run(
                std::bind(
                    &_GatherAllNodeDependencies,
                    std::cref(*schedule),
                    &nodeToDependencies));
            
            // Wait until all tasks have been created, and all dependencies
            // have been gathered.
            dispatcher.Wait();

            // Insert all the input dependencies into the schedule.
            const size_t numUniqueInputDeps = _InsertInputDependencies(
                schedule,
                nodeToDependencies,
                &schedule->_computeTasks,
                &schedule->_inputsTasks,
                &schedule->_inputDeps);
            
            // Set the number of output indices created.
            schedule->_numUniqueInputDeps = numUniqueInputDeps;
            
        });
}

static bool
_AssignLockMaskForOutput(
    const VdfOutput &output, 
    VdfSchedule *schedule)
{
    // If the output is not part of the schedule, or not requested, then there
    // is no point in assigning a lock mask for sparse mung buffer locking.
    // If the output is not affective, bail out.
    VdfSchedule::OutputId outputId = schedule->GetOutputId(output);
    if (!outputId.IsValid() || 
        !schedule->IsAffective(outputId)) {
        return false;
    }

    // Retrieve the output's affects mask from the schedule. We use it do
    // determine which bits in the mask have become un-affective at the next
    // output. Note, that the affects mask in the schedule is already a subset
    // of the request mask.
    const VdfMask &affectsMask = schedule->GetAffectsMask(outputId);

    // Initialize an empty mask with the size of the affects mask at the
    // current output. Eventually, we will set all the bits affected at the
    // output, so that we can determine which bits to lock at this output.
    VdfMask affectedAtNext(affectsMask.GetSize());

    // Find the next affective output, which this output will be passing its
    // data to. If there is no next affective output, we will not add any bits
    // to the affectedAtNext mask and simply lock all bits at the current
    // output. Doing so allows us to lock the data at the requested outputs.
    const VdfOutput *nextAffectedOutput = &output;
    VdfSchedule::OutputId nextAffectedId = outputId;
    while (nextAffectedOutput) {
        // If this output is not passing its data, bail out and lock everything.
        nextAffectedOutput = schedule->GetPassToOutput(nextAffectedId);
        if (!nextAffectedOutput) {
            break;
        }

        // If the next output is affective, store its affects mask and bail out,
        // otherwise continue searching for the next affective output.
        nextAffectedId = schedule->GetOutputId(*nextAffectedOutput);
        if (TF_VERIFY(nextAffectedId.IsValid()) &&
            schedule->IsAffective(nextAffectedId)) {
            affectedAtNext = schedule->GetAffectsMask(nextAffectedId);
            break;
        }
    }

    // If the next output is on a mover with more than one output, we must lock
    // everything in order to guarantee that no incorrect data ever flows into
    // this next mover. The reason is that the non-pool output may depend on
    // any bits of the associated pool input, i.e. all of the incoming data
    // must be correct.
    // XXX: Currently we do not expect any mover with more than one output!
    if (nextAffectedOutput && 
        !TF_VERIFY(nextAffectedOutput->GetNode().GetNumOutputs() == 1)) {
        affectedAtNext = VdfMask(affectsMask.GetSize());
    }

    // Compute the lock mask by taking the affects mask at the current output,
    // and leaving any bits turned one, which are no longer affected at the
    // target output.
    VdfMask lockMask = affectsMask - affectedAtNext;

    // Add the locked bits to the keep mask
    if (lockMask.IsAnySet()) {
        VdfMask keepMask = schedule->GetKeepMask(outputId);
        keepMask.SetOrAppend(lockMask);
        schedule->SetKeepMask(outputId, keepMask);

        // Locked some data
        return true;
    }

    // Did not lock any data
    return false;
}

void
VdfScheduler::_ComputeLockMasks(
    const VdfRequest &request, 
    VdfSchedule *schedule,
    const VdfScheduler::PoolPriorityVector &sortedPoolOutputs)
{
    TRACE_FUNCTION();

    // Enable Sparse Mung Buffer Locking (SMBL) in the schedule?
    bool enableSMBL = false;

    // For each pool output found, assign the lock masks
    TF_FOR_ALL(i, sortedPoolOutputs) {
        const VdfOutput *output = i->second;
        enableSMBL |= _AssignLockMaskForOutput(*output, schedule);
    }

    // Enable sparse mung buffer locking if data has been locked
    schedule->SetHasSMBL(enableSMBL);
}

void 
VdfScheduler::_ApplyAffectsMasks(VdfSchedule *schedule)
{
    TRACE_FUNCTION();

    for (VdfScheduleNode &so : schedule->GetScheduleNodeVector()) {
        _ApplyAffectsMasksForNode(&so);
    }
}

bool
VdfScheduler::_ApplyAffectsMasksForNode(VdfScheduleNode *node)
{
    const bool wasAffective = node->affective;

    node->affective = false;

    // If the node manages its own buffers we leave 'affective' at false which
    // will result in the node not being run (while the outputs are still 
    // scheduled). 

    if (VdfRootNode::IsARootNode(*node->node)) {
        return !wasAffective;
    }

    for (VdfScheduleOutput &so : node->outputs) {
        // For outputs that have an associated input connector, and for
        // those where an affects mask has been set, AND the affects mask
        // with the request mask.
        const VdfOutput *output = so.output;
        if (output && output->GetAssociatedInput()) {
            const VdfMask *affectsMask = output->GetAffectsMask();
            so.affectsMask = affectsMask && !so.requestMask.IsEmpty()
                ? so.requestMask & *affectsMask
                : so.requestMask;
        }

        // Given the affects masks, mark each scheduled node as
        // "affective" or not.  Note that if an output has no associated
        // input (unlike the .pool outputs on movers), it is said to be
        // "affective."
        node->affective |=
            !output->GetAssociatedInput() || so.affectsMask.IsAnySet();
    }
    
    // Return false, if there was any change in state that requires full
    // re-scheduling.
    return wasAffective == node->affective;
}

void
VdfScheduler::_UpdateAffectsMaskForInvocation(
    VdfSchedule *schedule,
    VdfScheduleNode *node)
{
    // With parallel evaluation disabled, we can bail out.
    if (!VdfIsParallelEvaluationEnabled()) {
        return;
    }

    TRACE_FUNCTION();

    // If this node does not have any invocations, we can bail out.
    const VdfSchedule::TaskIdRange taskIds =
        schedule->GetComputeTaskIds(*node->node);
    if (taskIds.empty()) {
        return;
    }

    // Get the inputs task by linearly searching over the compute tasks.
    VdfScheduleTaskIndex inputsTaskIndex = VdfScheduleTaskInvalid;
    for (const VdfScheduleTaskId taskId : taskIds) {
        const VdfScheduleComputeTask &computeTask =
            schedule->GetComputeTask(taskId);
        if (!VdfScheduleTaskIsInvalid(computeTask.inputsTaskIndex)) {
            inputsTaskIndex = computeTask.inputsTaskIndex;
            break;
        }
    }

    // Get the new affects mask from the single output.
    const VdfMask &newAffectsMask = node->outputs.front().affectsMask;

    // For each invocation, determine whether it is still affective or not.
    for (const VdfScheduleTaskId taskId : taskIds) {
        VdfScheduleComputeTask *computeTask = &schedule->_computeTasks[taskId];
        
        // If this compute task is not for a node invocation, there is nothing
        // to update.
        const VdfScheduleTaskIndex invocationIndex =
            computeTask->invocationIndex;
        if (VdfScheduleTaskIsInvalid(invocationIndex)) {
            continue;
        }

        // Retrieve the invocation from the schedule.
        VdfScheduleNodeInvocation *invocation =
            &schedule->_nodeInvocations[invocationIndex];

        // Compute the new invocation affects mask, and toggle the affective
        // flag as well as the inputs task index based on whether the invocation
        // is affective or not.
        const VdfMask newInvocationAffectsMask =
            invocation->requestMask & newAffectsMask;
        if (newInvocationAffectsMask.IsAllZeros()) {
            invocation->affectsMask = VdfMask();
            computeTask->flags.isAffective = false;
            computeTask->inputsTaskIndex = VdfScheduleTaskInvalid;
        } else {
            invocation->affectsMask = newInvocationAffectsMask;
            computeTask->flags.isAffective = true;
            computeTask->inputsTaskIndex = inputsTaskIndex;
        }
    }
}

bool
VdfScheduler::_UpdateAffectsMasksForNode(
    VdfSchedule *schedule,
    VdfScheduleNode *node)
{
    // Apply the new affects masks to the node, but if this does not succeed
    // indicate that the schedule cannot be updated.
    if (!_ApplyAffectsMasksForNode(node)) {
        return false;
    }

    // Update affects masks for any node invocations, if they exist.
    _UpdateAffectsMaskForInvocation(schedule, node);

    // We were able to successfully update the schedule.
    return true;
}

void
VdfScheduler::_UpdateLockMaskForNode(
    VdfSchedule *schedule, 
    VdfScheduleNode *node)
{
    // Find the pool output and re-assign the lock mask
    TF_FOR_ALL (o, node->outputs) {
        if (o->output && Vdf_IsPoolOutput(*o->output)) {
            // Update the lock mask for the output
            _AssignLockMaskForOutput(*o->output, schedule);

            // Find the output, which will be passing its data to this output.
            // We also need to update the lock mask there. This is because the
            // lock mask at that output depends on the affects mask at the
            // pass-to output, which has just been modified.
            if (o->fromBufferOutput) {
                // Now, update the lock mask for the from-buffer output
                _AssignLockMaskForOutput(*o->fromBufferOutput, schedule);
            }
        }
    }
 }

static VdfConnectionAndMaskVector
_FindInputs(const VdfMaskedOutput &maskedOutput)
{
    VdfConnectionAndMaskVector dependencies;

    // Gather up all the read inputs, but only if the output is affective as
    // determined by the affects mask (or lack thereof).
    const VdfOutput *output = maskedOutput.GetOutput();
    const VdfMask &mask = maskedOutput.GetMask();
    const VdfMask *affectsMask = output->GetAffectsMask();
    if (!affectsMask || affectsMask->Overlaps(mask)) {
        dependencies = output->GetNode().ComputeInputDependencyMasks(
            maskedOutput, /* skipAssociatedInputs = */ true);
    }

    // Add associated inputs with the full request mask (ignoring
    // sparse dependencies) so that we have buffers to write into.
    if (const VdfInput *associatedInput = output->GetAssociatedInput()) {
        const VdfConnectionVector &connections =
            associatedInput->GetConnections();

        // If there is more than one node connected on the input, something
        // went horribly wrong. We do not support this case.
        if (connections.size() > 1) {
            VDF_FATAL_ERROR(
                output->GetNode(),
                "Multiple inputs found on " + associatedInput->GetDebugName()
                + " associated with output " + output->GetDebugName()
                + ".  The system doesn't know how to pass the data through.");
        }

        else if (!connections.empty()) {
            VdfConnection *c = connections.front();
            if (c->GetMask().IsAnySet()) {
                dependencies.emplace_back(c, mask);
            }
        }
    }

    return dependencies;
}

static VdfConnectionAndMaskVector
_FindInputs(const VdfMaskedOutputVector &maskedOutputs)
{
    if (maskedOutputs.empty()) {
        return VdfConnectionAndMaskVector();
    }

    const VdfNode &node = maskedOutputs.front().GetOutput()->GetNode();
    return node.ComputeInputDependencyRequest(maskedOutputs);
}

static void
_AddInputs(
    VdfConnectionAndMaskVector &&dependencies,
    VdfSchedule *schedule,
    std::vector<VdfMaskedOutput> *stack)
{
    // The read/write outputs appear last in the dependencies array, and we also
    // want to traverse those last. Consequently, we need to push them onto the
    // stack first. Iterate in reverse order to do just that.
    const VdfConnectionAndMaskVector::reverse_iterator end =
        dependencies.rend();
    VdfConnectionAndMaskVector::reverse_iterator it =
        dependencies.rbegin();
    for (; it != end; ++it) {
        const VdfConnection *connection = it->first;
        schedule->AddInput(*connection, it->second);
        stack->emplace_back(
            &connection->GetNonConstSourceOutput(),
            std::move(it->second));
    }
}

static bool
_SetRequestMask(
    VdfSchedule *schedule, 
    const VdfSchedule::OutputId &outputId,
    const VdfMask &newMask)
{
    bool addedNewBits = false;

    const VdfMask &requestMask = schedule->GetRequestMask(outputId);

    if (requestMask.IsEmpty())  {
        schedule->SetRequestMask(outputId, newMask);
        addedNewBits = true;
    } else if (TF_VERIFY(!newMask.IsEmpty())) {
        // If the existing mask has all of its bits already set in the
        // currently accumulated mask, then there is no new information added.
        addedNewBits = !requestMask.Contains(newMask);
        // Mask already exists, or the new request mask in.
        if (addedNewBits) {
            schedule->SetRequestMask(outputId, requestMask | newMask);
        }
    }
    return addedNewBits;
}

static void 
_ProcessImmediateStack(
   std::vector<VdfMaskedOutput> *stack,
   VdfSchedule *schedule,
   _IndexToMaskedOutputMap *poolOutputQueue,
   VdfScheduler::PoolPriorityVector *poolOutputs,
   VdfScheduler::NodeToRequestMap *deferredInputsToAdd)
{
    TF_DEV_AXIOM(deferredInputsToAdd);

    // Process all the outputs that don't need to wait (i.e. those which are not
    // pool).
    //
    while (!stack->empty()) {

        VdfMaskedOutput maskedOutput = std::move(stack->back());
        stack->pop_back();

        const VdfOutput &output = *maskedOutput.GetOutput();
        const VdfNode &node     = output.GetNode();

        // If we encounter an output that has an affects mask, move it to the
        // poolOutputQueue for later processing.
        if (Vdf_IsPoolOutput(output)) {

            // The poolOutputQueue is a priority queue such that nodes
            // further down the pool chain are processed first. We achieve
            // this by ordering with greater-than on the pool chain index.
            const VdfPoolChainIndex poolChainIndex = 
                node.GetNetwork().GetPoolChainIndex(output);

            std::pair<_IndexToMaskedOutputMap::iterator, bool> i = 
                poolOutputQueue->try_emplace(
                    poolChainIndex, std::move(maskedOutput));

            if (i.second) {
                // Add the output to the poolOutputs vector. Later in this
                // function, the vector will be sorted by pool chain index,
                // i.e. downstream outputs will be on the front of the vector.
                poolOutputs->emplace_back(poolChainIndex, &output);
            } else {

                // We've already seen this output in this traversal.
                // Grab the mask at the previous point and OR it in 
                // to our current position, and update the index.
                // Doing this will make it so that the first time this output
                // is popped off for processing, we will process it with a 
                // fuller mask, reducing the need for multiple traversals.
                //
                VdfMaskedOutput &affectsMaskedOutput = i.first->second;

                VdfMask mask = 
                    affectsMaskedOutput.GetMask() | maskedOutput.GetMask();

                TF_DEV_AXIOM(affectsMaskedOutput.GetOutput() == 
                             maskedOutput.GetOutput());

                affectsMaskedOutput.SetMask(std::move(mask));
            }

        } else {

            const VdfSchedule::OutputId &outputId = 
                schedule->GetOrCreateOutputId(output);
            TF_DEV_AXIOM(outputId.IsValid());

            // Append to our current request mask and note if we added any new
            // entries.
            const bool addedNewRequest = 
                _SetRequestMask(schedule, outputId, maskedOutput.GetMask());

            // Skip speculation nodes, they cause cycles and do their own
            // scheduling.
            if (node.IsSpeculationNode()) {
                continue;
            }

            // If we've added new entries, we need to do some further processing.
            // Otherwise, we're done.
            if (addedNewRequest) {

                // If the node has multiple outputs and the output in question
                // is not associated and doesn't have an affects mask we queue
                // it up for later vectorized processing.  This helps cases
                // like sharing nodes to schedule quickly (those nodes can
                // have thousands of outputs).
                
                //XXX: We need to be careful what we actually add to 
                //     the delayed deferredInputsToAdd map.  This is because if
                //     we add too much, we might offsets we gain from 
                //     the pool ordering optimizations.
                const bool addInputsVectorized =
                    node.GetNumOutputs() > NODE_OUTPUT_THRESHOLD &&
                    !output.GetAffectsMask() &&
                    !output.GetAssociatedInput();

                if (addInputsVectorized) {
                    (*deferredInputsToAdd)[&node].push_back(
                        std::move(maskedOutput));
                } else {
                    _AddInputs(_FindInputs(maskedOutput), schedule, stack);
                }
            }
        }
    }
}


static inline bool
_PoolChainIndexGreaterThan(
    const std::pair<VdfPoolChainIndex, const VdfOutput *> &lhs,
    const std::pair<VdfPoolChainIndex, const VdfOutput *> &rhs)
{
    return lhs.first > rhs.first;
}

void
VdfScheduler::_InitializeRequestMasks(
    const VdfRequest &request,
    VdfSchedule *schedule,
    PoolPriorityVector *poolOutputs)
{
    TRACE_FUNCTION();

    if (!TF_VERIFY(!request.IsEmpty())) {
        return;
    }

    // Stack to contain the outputs that need processing before the outputs
    // with an affects mask.
    std::vector<VdfMaskedOutput> stack;
    stack.reserve(request.GetSize());

    // The outputs that have affects mask and need to be processed after all
    // the nodes under them (contained in 'stack').
    // The poolOutputQueue is a priority queue with the priority being the index
    // returned by the VdfPoolChainIndexer.
    _IndexToMaskedOutputMap poolOutputQueue;

    // Initialize the stack with the outputs from the request. Verify that all
    // requested outputs come from the same network.
    const VdfNetwork *network = request.GetNetwork();
    TF_FOR_ALL(i, request) {
        if (TF_VERIFY(&i->GetOutput()->GetNode().GetNetwork() == network)) {

            stack.push_back(*i);
        }
    }

    // A map from node to request, used when multiple outputs are requested from
    // a node in one go.
    VdfScheduler::NodeToRequestMap deferredInputsToAdd;

    // Now process all the remaining outputs before we process another
    // output with an affects mask.
    _ProcessImmediateStack(
        &stack, schedule, &poolOutputQueue, poolOutputs, &deferredInputsToAdd);

    while (!deferredInputsToAdd.empty() || !poolOutputQueue.empty()) {

        while (!poolOutputQueue.empty()) {
    
            // Get the first item of the poolOutputQueue which will be the item
            // with the lowest point pool index.
            _IndexToMaskedOutputMap::iterator topIter = poolOutputQueue.begin();
            VdfMaskedOutput maskedOutput = topIter->second;
            poolOutputQueue.erase(topIter);
    
            const VdfOutput &output = *maskedOutput.GetOutput();
            const VdfSchedule::OutputId &outputId = 
                schedule->GetOrCreateOutputId(output);
            if (!TF_VERIFY(outputId.IsValid())) {
                continue;
            }
    
            // Add the current mask to the request and note if the request has
            // changed at all.
            bool addedNewRequest =
                _SetRequestMask(schedule, outputId, maskedOutput.GetMask());
    
            // The pool is never output from a VdfSpeculationNode.
            if (!TF_VERIFY(!output.GetNode().IsSpeculationNode())) {
                continue;
            }
    
            // We only have work to do when we added new bits to the request
            // mask.
            if (addedNewRequest) {

                // Now add our inputs to be processed.
                _AddInputs(_FindInputs(maskedOutput), schedule, &stack);
            }

            // Now process all the remaining outputs before we process another
            // output with an affects mask.
            _ProcessImmediateStack(
                &stack, schedule, &poolOutputQueue, poolOutputs,
                &deferredInputsToAdd);
        }

        if (!deferredInputsToAdd.empty()) {
        
            // Use the vectorized API to compute all inputs dependencies for
            // all scheduled outputs of the given node in one single call.
            // This helps if the node can provide those dependencies quickly
            // (like the Mf_ExecSharingNode).

            for (const auto &e : deferredInputsToAdd) {
                _AddInputs(_FindInputs(e.second), schedule, &stack);
            }
            deferredInputsToAdd.clear();

            // Now process all the remaining outputs before we process another
            // node with multiple outputs.
            _ProcessImmediateStack(
                &stack, schedule, &poolOutputQueue, poolOutputs,
                &deferredInputsToAdd);
        }
    }

    // Calls to _AddInputs in the traversal above collect scheduled inputs
    // without checking for duplicates or merging masks.  We defer this
    // deduplication because, even though merging immediately in
    // VdfSchedule::AddInput is efficient for nodes with few inputs, deferred
    // sorting and merging is an overall gain when sharing nodes are involved
    // because they can have thousands of scheduled inputs.
    schedule->DeduplicateInputs();

    // Sort using greater-than so that outputs further down in the network
    // appear at the front of the vector.  Clients expect pool outputs in
    // bottom-up order.
    std::sort(
        poolOutputs->begin(), poolOutputs->end(),
        _PoolChainIndexGreaterThan);
    poolOutputs->erase(
        std::unique(poolOutputs->begin(), poolOutputs->end()),
        poolOutputs->end());
}

void
VdfScheduler::_MarkSmallSchedule(VdfSchedule *schedule)
{
    // If we have a small schedule, just remove the _nodesToIndexMap,
    // flag it, and call it a day.
    constexpr int smallScheduleSize = 32;
    if (schedule->_nodes.size() <= smallScheduleSize) {

        schedule->_isSmallSchedule = true;
        TfReset(schedule->_nodesToIndexMap);

    } else if (!TF_VERIFY(!schedule->_isSmallSchedule)) {
        // Schedule isSmallSchedule should already have been set to false.
        // We shouldn't hit this.
        schedule->_isSmallSchedule = false;
    }
}

static void
_TopologicallySort(
    const VdfRequest &request,
    VdfSchedule *schedule)
{
    // Prime the working stack with the given requested outputs.
    std::vector<std::pair<const VdfOutput *, bool>> stack;
    stack.reserve(request.GetSize());
    for (const VdfMaskedOutput &mo : request) {
        stack.emplace_back(mo.GetOutput(), false);
    }

    // Process the stack.
    while (!stack.empty()) {
        const VdfOutput &output = *stack.back().first;
        const VdfNode &node = output.GetNode();
        bool *addSelf = &stack.back().second;

        // Append this output to the sorted result immediately if we've
        // already processed all its dependencies.
        if (*addSelf || schedule->IsScheduled(node)) {
            schedule->GetOrCreateOutputId(output);
            stack.pop_back();
            continue;
        }

        // Before traversing the top output's dependencies, mark that
        // when we return to this point in the stack, we need to process
        // the output itself.
        *addSelf = true;

        // If this is a speculation node, do not traverse its inputs.
        if (node.IsSpeculationNode()) {
            continue;
        }

        // Traverse the inputs.
        for (const std::pair<TfToken, VdfInput *> &input :
                node.GetInputsIterator()) {
            const VdfConnectionVector &connections =
                input.second->GetConnections();
            for(const VdfConnection *c : connections) {
                const VdfOutput &sourceOutput = c->GetSourceOutput();
                stack.emplace_back(&sourceOutput, false);
            }
        }
    }
}

void
VdfScheduler::Schedule(
    const VdfRequest &request, VdfSchedule *schedule, bool topologicallySort)
{
    TRACE_FUNCTION();

    TfAutoMallocTag2 tag("Vdf", "VdfScheduler::Schedule");

    schedule->Clear();

    // It's a valid schedule, it's just empty.
    if (request.IsEmpty()) {
        _SetScheduleValid(schedule, nullptr);
        return;
    }

    // Initialize the size of the network we're dealing with.
    const VdfNetwork *network = request.GetNetwork();
    schedule->InitializeFromNetwork(*network);

    // If we've been asked to schedule in topological order, sort  the nodes
    // before we start scheduling.
    if (topologicallySort) {
        _TopologicallySort(request, schedule);
    }

    // Initialize all the request masks.
    PoolPriorityVector poolOutputs;
    _InitializeRequestMasks(request, schedule, &poolOutputs);

    // Schedule the buffer-passing.
    _ScheduleBufferPasses(request, schedule);

    // Set the affects masks so that they only affect the things in the
    // request.
    _ApplyAffectsMasks(schedule);

    // This call fills in the passToOutput to speed up the passing of
    // buffers by skipping all the outputs in between that have no effect.
    // This needs to happen AFTER all the keep masks have been set up
    // correctly.
    _ScheduleForPassThroughs(request, schedule, poolOutputs);

    // Schedule node tasks.
    if (VdfIsParallelEvaluationEnabled()) {
        _ScheduleTaskGraph(schedule, poolOutputs);
    }

    // Determine if this is a small schedule.
    _MarkSmallSchedule(schedule);

    // Set the request.
    schedule->SetRequest(request);

    // The schedule is done and is now valid.
    _SetScheduleValid(schedule, network);
}

bool
VdfScheduler::UpdateAffectsMaskForOutput(
    VdfSchedule *schedule,
    const VdfOutput &output)
{
    if (!TF_VERIFY(
            output.GetAssociatedInput() &&
            output.GetAffectsMask())) {
        return true;
    }

    // If the output is not scheduled, we don't need to update anything.
    VdfSchedule::OutputId outputId = schedule->GetOutputId(output);
    if (!outputId.IsValid()) {
        return true;
    }

    // If the affects mask does not overlap with the request mask in the
    // schedule, we don't need to update anything.
    const VdfMask &requestMask = schedule->GetRequestMask(outputId);
    const bool isAffective = output.GetAffectsMask()->Overlaps(requestMask);
    if (!isAffective) {
        return true;
    }

    // Find all the outputs connected to the input dependencies.
    VdfConnectionAndMaskVector dependencies = _FindInputs(
        VdfMaskedOutput(const_cast<VdfOutput *>(&output), requestMask));

    // Check if the dependencies are already all scheduled. The dependencies
    // may have changed, since they are dependent on the affects mask.
    for (const VdfConnectionAndMask &dependency : dependencies) {
        const VdfOutput &sourceOutput = dependency.first->GetSourceOutput();
        VdfSchedule::OutputId dependencyOutputId =
            schedule->GetOutputId(sourceOutput);

        // Already scheduled. We can update the schedule as long as the request
        // mask already contains the dependency.
        const VdfMask &mask = dependency.second;
        if (dependencyOutputId.IsValid()) {
            const VdfMask &dependencyRequestMask =
                schedule->GetRequestMask(dependencyOutputId);
            if (!dependencyRequestMask.Contains(mask)) {
                return false;
            }
        }

        // Not scheduled. We can't update the schedule and need to rebuild it
        // instead.
        else {
            return false;
        }
    }

    // We cannot update the schedule, if the owning node is not included in
    // the schedule (i.e. it is a trivial node.) In that case, the state of
    // affective-ness has changed for sure.
    const int scheduleNodeIdx =
        schedule->_GetScheduleNodeIndex(output.GetNode());
    if (scheduleNodeIdx < 0) {
        return false;
    }

    // Retrieve the schedule node.
    VdfScheduleNode *scheduleNode =
        &schedule->GetScheduleNodeVector()[scheduleNodeIdx];

    // Update the affects masks for the node.
    if (!_UpdateAffectsMasksForNode(schedule, scheduleNode)) {
        return false;
    }

    // Update the lock masks for the node.
    if (schedule->HasSMBL()) {
        _UpdateLockMaskForNode(schedule, scheduleNode);
    }

    // We were able to successfully update the schedule. There is no need for it
    // to be rebuilt.
    return true;
}

void 
VdfScheduler::_SetScheduleValid(
    VdfSchedule *schedule, 
    const VdfNetwork *network)
{
    TF_DEV_AXIOM(schedule);
    schedule->_SetIsValidForNetwork(network);
}

PXR_NAMESPACE_CLOSE_SCOPE
