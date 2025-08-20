//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_PULL_BASED_EXECUTOR_ENGINE_H
#define PXR_EXEC_VDF_PULL_BASED_EXECUTOR_ENGINE_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/debugCodes.h"
#include "pxr/exec/vdf/evaluationState.h"
#include "pxr/exec/vdf/executorBufferData.h"
#include "pxr/exec/vdf/executionStats.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/networkUtil.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/output.h"
#include "pxr/exec/vdf/poolChainIndex.h"
#include "pxr/exec/vdf/requiredInputsPredicate.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/smblData.h"
#include "pxr/exec/vdf/vector.h"

#include "pxr/base/tf/bits.h"
#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/mallocTag.h"

#define _VDF_PBEE_TRACE_ON 0
#if _VDF_PBEE_TRACE_ON
#include <iostream>
#endif

PXR_NAMESPACE_OPEN_SCOPE

// The TRACE_SCOPE (and FUNCTION) invocations in this file can be pretty
// expensive. We turn off most of them. However, they are still useful to
// track down performance issues, which is why we have a quick way of enabling
// them, by setting _VDF_PBEE_PROFILING_ON to 1.
#define _VDF_PBEE_PROFILING_ON 0
#if _VDF_PBEE_PROFILING_ON
#define VDF_PBEE_TRACE_FUNCTION TRACE_FUNCTION
#define VDF_PBEE_TRACE_SCOPE TRACE_SCOPE
#else
#define VDF_PBEE_TRACE_FUNCTION()
#define VDF_PBEE_TRACE_SCOPE(name)
#endif

class VdfExecutorInterface;
class VdfExecutorErrorLogger;

// Forward declare the speculation executor engine with equivalent traits to
// this executor engine.
template <typename> class VdfSpeculationExecutorEngine;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfPullBasedExecutorEngine
///
/// \brief This class is a collection  of common functions used by pulled
/// based executors.
///
///

template <typename DataManagerType>
class VdfPullBasedExecutorEngine 
{
public:

    /// The equivalent speculation executor engine. Executor factories can use
    /// this typedef to map from an executor engine to a speculation executor
    /// engine with equivalent traits.
    ///
    typedef
        VdfSpeculationExecutorEngine<DataManagerType>
        SpeculationExecutorEngine;

    /// Constructor.
    ///
    VdfPullBasedExecutorEngine(
        const VdfExecutorInterface &executor, 
        DataManagerType *dataManager) :
        _executor(executor),
        _dataManager(dataManager)
    {}

    /// Executes the given \p schedule with a \p computeRequest and an optional
    /// /p errorLogger.
    ///
    void RunSchedule(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest,
        VdfExecutorErrorLogger *errorLogger) {
        RunSchedule(
            schedule, computeRequest, errorLogger,
            [](const VdfMaskedOutput &, size_t){});
    }

    /// Executes the given \p schedule with a \p computeRequest and an optional
    /// /p errorLogger. Invokes \p callback after evaluation of each uncached
    /// output in the request, and immediatelly after hitting the cache for
    /// cached outputs in the request.
    ///
    template < typename F >
    void RunSchedule(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest,
        VdfExecutorErrorLogger *errorLogger,
        F &&callback);

protected:

    /// The data handle type from the data manager implementation.
    ///
    typedef typename DataManagerType::DataHandle _DataHandle;

    /// This enum describes the stages that a node goes through in execution.
    ///
    enum _ExecutionStage {

        ExecutionStageStart,             // Nodes start in this stage.

        ExecutionStagePreRequisitesDone, // After prerequisites have been
                                         // computed but before rest of 
                                         // inputs have been computed.

        ExecutionStageReadsDone,         // After the reads have finished --
                                         // only needed for speculation engine.

        ExecutionStageCompute,           // Final stage before node computation.

    };


    /// Computes \p node.
    ///
    /// This is the method that ends up calling Compute() on the VdfNode.
    ///
    void _ComputeNode(
        const VdfEvaluationState &state,
        const VdfNode &node,
        bool absorbLockedCache = false);

    /// Causes the outputs with associated inputs in \p node to have 
    /// their data passed through.
    ///
    /// For outputs that don't have associated inputs, the default value
    /// registered for the output's value type is used.
    ///
    /// It is an error to call this method on a node that was computed
    /// with _ComputeNode() -- these two calls are mutually exclusive.
    ///
    /// Returns \c true if any output had data to be passed through, and
    /// false otherwise.
    ///
    bool _PassThroughNode(
        const VdfSchedule &schedule,
        const VdfNode &node,
        bool absorbLockedCache = false);


    /// Helper method to _PrepareReadWriteBuffer that copies the cache from
    /// \p fromOutput to \p toOutput.
    ///
    VdfVector *_CopyCache(
        const VdfOutput &toOutput,
        VdfExecutorBufferData *toBuffer,
        const VdfOutput &fromOutput,
        const VdfMask   &fromMask) const;

    /// Returns the executor running this engine.
    ///
    const VdfExecutorInterface &_GetExecutor() { return _executor; }

    /// Returns the data manager used by this engine.
    ///
    DataManagerType *_GetDataManager() { return _dataManager; }

    /// Fast path for when we know ahead of time the output from which
    /// we wish to pass the buffer (or copy) and it is not necessarily
    /// the one that is directly connected to the output's associated
    /// input.
    /// 
    VdfVector *_PassOrCopySourceOutputBuffer(
        const _DataHandle dataHandle,
        const VdfOutput &output,
        const VdfOutput &source,
        const VdfMask   &inputMask,
        const VdfSchedule &schedule);


    /// Common method for _PrepareReadWriteBuffer and 
    /// _PassOrCopySourceOutputBuffer that attempts to pass the buffer
    /// from \p source to \p output.
    ///
    VdfVector *_PassOrCopyBufferInternal(
        const _DataHandle dataHandle,
        const VdfOutput &output,
        const VdfOutput &source,
        const VdfMask   &inputMask,
        const VdfSchedule &schedule) const;

    /// Prepares a buffer for a read/write output. This method makes sure that
    /// the output buffer has been passed down from the input. If at the input
    /// there is no buffer available for passing, this method will create a
    /// new one.
    ///    
    void _PrepareReadWriteBuffer(
        const _DataHandle dataHandle,
        const VdfInput &input,
        const VdfMask &mask,
        const VdfSchedule &schedule);

    /// Returns true if the output is associative but does not pass the buffer
    /// to another output.  In other words, this returns true if this is the 
    /// last output in the pool chain.
    ///
    inline static bool _IsNotPassing(
        const VdfOutput& output,
        const VdfSchedule::OutputId& outputId,
        const VdfSchedule& schedule)
    {
        return output.GetAssociatedInput() &&
            !schedule.GetPassToOutput(outputId);
    }

private:

    // This struct contains the necessary state to compute an output.
    //
    struct _OutputToExecute {

        // Constructor that takes schedule node and output.
        _OutputToExecute(
            const VdfSchedule::OutputId &outputId,
            const VdfMask &lockedCacheMask,
            bool affective) :
            outputId(outputId),
            stage(ExecutionStageStart),
            lockedCacheMask(lockedCacheMask),
            affective(affective),
            absorbLockedCache(false)
        { }

        // The schedule identifier for the output to execute.
        VdfSchedule::OutputId outputId;

        // The current phase of this output in the execution stack.
        _ExecutionStage stage;

        // Current state of the locked cache
        VdfMask lockedCacheMask;

        // Determines the affective-ness of the output
        bool affective;

        // Absorb the locked cache for SMBL
        bool absorbLockedCache;

    };

    // Helper method to _ExecuteOutput.
    //
    // This method adds \p output to the \p outputs vector.
    // Returns \c true if it added a new output and \c false otherwise.
    //
    bool _PushBackOutput(
        std::vector< _OutputToExecute > *outputs,
        const VdfMask &lockedCacheMask,
        const VdfOutput &output,
        const VdfSchedule &schedule);

    // Executes the given \p output.  
    //
    void _ExecuteOutput(
        const VdfEvaluationState &state,
        const VdfOutput &output, 
        TfBits *executedNodes);

    // Finalize the output buffer after computing or passing through. This
    // sets the computed output mask as well as merges in any data that has
    // been temporarily held on to.
    //
    void _FinalizeComputedOutput(
        const _DataHandle dataHandle,
        const VdfMask &requestMask,
        const bool hasBeenInterrupted,
        const bool extendRequestMask);

    // Update the output stack entry for SMBL. This refreshes the affectiveness
    // flag, the lockedCacheMask and the flag that determines whether the
    // locked cache needs to be absorbed into the executor cache. This method
    // returns true if any of the relevant flags on the stackEntry object have
    // been modified.
    //
    bool _UpdateOutputForSMBL(
        const VdfOutput &output,
        _OutputToExecute *stackEntry,
        const VdfSchedule &schedule);

    // The executor that uses this engine.
    //
    const VdfExecutorInterface &_executor;

    // The data manager for this engine.
    //
    DataManagerType *_dataManager;

    // Acceleration structure used for caching output handles, which may be
    // repeatedly looked up in the same order.
    //
    std::vector<_DataHandle> _dataHandleCache;

};

///////////////////////////////////////////////////////////////////////////////

template<typename DataManagerType>
template<typename F>
void
VdfPullBasedExecutorEngine<DataManagerType>::RunSchedule(
    const VdfSchedule &schedule,
    const VdfRequest &computeRequest,
    VdfExecutorErrorLogger *errorLogger,
    F &&callback)
{
    TRACE_FUNCTION();

    // Make sure the data manager is appropriately sized.
    _dataManager->Resize(*schedule.GetNetwork());

    // Indicates which nodes have been executed.
    TfBits executedNodes(schedule.GetNetwork()->GetNodeCapacity());

    // The persistent evaluation state
    VdfEvaluationState state(_GetExecutor(), schedule, errorLogger);

    // Now execute the uncached, requested outputs.
    VdfRequest::IndexedView requestView(computeRequest);
    for (size_t i = 0; i < requestView.GetSize(); ++i) {
        // Skip outputs not included in the request.
        const VdfMaskedOutput *maskedOutput = requestView.Get(i);
        if (!maskedOutput) {
            continue;
        }

        // Skip outputs that have already been cached. However, we must invoke
        // the callback to notify the client side that evaluation of the
        // requested output has completed.
        const VdfOutput &output = *maskedOutput->GetOutput();
        const VdfMask &mask = maskedOutput->GetMask();
        if (_GetExecutor().GetOutputValue(output, mask)) {
            callback(*maskedOutput, i);
            continue;
        }

        VDF_PBEE_TRACE_SCOPE(
            "VdfPullBasedExecutorEngine<T>::RunSchedule (executing output)");
        _ExecuteOutput(state, output, &executedNodes);

        // If we've been interrupted, bail out.
        if (_GetExecutor().HasBeenInterrupted()) {
            break;
        }

        // Invoke the callback once the output has been evaluated, but only
        // if the executor has not been interrupted.
        else {
            callback(*maskedOutput, i);
        }
    }
}

template<typename DataManagerType>
VdfVector *
VdfPullBasedExecutorEngine<DataManagerType>::_CopyCache(
    const VdfOutput &toOutput,
    VdfExecutorBufferData *toBuffer,
    const VdfOutput &fromOutput,
    const VdfMask   &fromMask) const
{
    TfAutoMallocTag2 tag("Vdf", "VdfPullBasedExecutorEngine<T>::_CopyCache");

    // Note that we must look up the data through the executor, instead of the
    // data manager, because we may have initially received a cache hit by
    // looking up the executor. The data may live in the parent executor, for
    // example, instead of the local data manager.
    const VdfVector *sourceVector = 
        _executor.GetOutputValue(fromOutput, fromMask);

    if (!sourceVector) {
        // CODE_COVERAGE_OFF - We should never hit this
        VDF_FATAL_ERROR(fromOutput.GetNode(), 
                        "No cache for output " + fromOutput.GetDebugName());
        // CODE_COVERAGE_ON
    }

    VdfVector *result = _dataManager->CreateOutputCache(toOutput, toBuffer);
    result->Copy(*sourceVector, fromMask);

    if (VdfExecutionStats* stats = _executor.GetExecutionStats()) {
        stats->LogData(
            VdfExecutionStats::ElementsCopiedEvent, 
            toOutput.GetNode(), 
            fromMask.GetNumSet());
    }

    return result;
}


template<typename DataManagerType>
VdfVector *
VdfPullBasedExecutorEngine<DataManagerType>::_PassOrCopySourceOutputBuffer(
    const _DataHandle dataHandle,
    const VdfOutput &output,
    const VdfOutput &source,
    const VdfMask   &inputMask,
    const VdfSchedule &schedule)
{
    VDF_PBEE_TRACE_FUNCTION();

    // The following block of code makes sure that we touch all the outputs 
    // between the source output and the output that was passed the buffer.
    //
    // XXX: This loop scales with the number of nodes between the two outputs
    //      and can get quite expensive. It's also very cache unfriendly. It
    //      would be great if we could get away without every touching these
    //      outputs.
    //
    const VdfOutput *betweenOutput = VdfGetAssociatedSourceOutput(output);
    while (betweenOutput && betweenOutput != &source) {
        _GetExecutor()._TouchOutput(*betweenOutput);
        betweenOutput = VdfGetAssociatedSourceOutput(*betweenOutput);
    }

    return _PassOrCopyBufferInternal(
        dataHandle, output, source, inputMask, schedule);
}

template<typename DataManagerType>
VdfVector *
VdfPullBasedExecutorEngine<DataManagerType>::_PassOrCopyBufferInternal(
    const _DataHandle dataHandle,
    const VdfOutput &output,
    const VdfOutput &source,
    const VdfMask   &inputMask,
    const VdfSchedule &schedule) const
{
    // Here's where we have the most potential for optimization.  We 
    // can re-use our inputs cache (without any copying) if our input 
    // has one and only one output (and that's us) 
    //
    const _DataHandle sourceHandle =
        _dataManager->GetDataHandle(source.GetId());
    VdfSchedule::OutputId sourceId = schedule.GetOutputId(source);

    VdfVector *result = NULL;

    // If this is the output that 'source' is supposed to pass its buffer 
    // to, do so, otherwise copy.
    if (_dataManager->IsValidDataHandle(sourceHandle) &&
        &output == schedule.GetPassToOutput(sourceId)) {

        // Retrieve the buffer data from the source data handle.
        VdfExecutorBufferData *sourceBuffer =
            _dataManager->GetBufferData(sourceHandle);

        // If the source output does not contain any data, don't even 
        // bother with mung buffer locking or buffer passing.
        if (sourceBuffer->GetExecutorCache() &&
            sourceBuffer->GetExecutorCacheMask().IsAnySet()) {

            // Decide whether mung buffer locking should be in effect.
            // We identify this source output as a likely candidate for buffer
            // locking (keeping its buffer around) if we observe that the 
            // current output has been recently invalidated while the source 
            // output has not. We optimistically "lock" the buffer by copying
            // it instead of passing it, so that during the rest of the current 
            // mung (if any), the source buffer will still have its buffer 
            // intact, and we won't have to visit any of its upstream nodes.
            if (_dataManager->HasInvalidationTimestampMismatch(
                    sourceHandle, dataHandle)) {
                TF_DEBUG(VDF_MUNG_BUFFER_LOCKING)
                        .Msg("Mung buffer locking between outputs "
                             "'%s' and '%s'.\n",
                        source.GetDebugName().c_str(), 
                        output.GetDebugName().c_str());
            }

            // If mung buffers are not supposed to be locked, pass the buffer
            // data from the source output to the destination output.
            else {
    
                // If the source output does not contain all the data that has
                // been requested in the inputMask, we cannot pass buffers.
                // Note, that the requested data being available also implies 
                // that the source output contains the data marked to keep,
                // since the keep mask is always a subset of the request mask.
                // This is verified at scheduling time.
                //
                // We end up in this particular situation if the execution
                // engine has found the data living on a parent executor,
                // i.e. it must be copied before it can be passed to
                // subsequent outputs.
                //
                if (sourceBuffer->GetExecutorCacheMask().Contains(inputMask)) {
                    const VdfMask &keepMask = schedule.GetKeepMask(sourceId);
                    result = _dataManager->PassBuffer(
                        source, sourceBuffer,
                        output, _dataManager->GetBufferData(dataHandle),
                        keepMask);

                    if (VdfExecutionStats* stats = 
                            _executor.GetExecutionStats()) {
                        stats->LogData(
                            VdfExecutionStats::ElementsCopiedEvent, 
                            source.GetNode(), 
                            keepMask.GetNumSet());
                    }
                }
    
                // Note that result can be NULL and we can still end up in
                // _CopyCache.  This can happen when something cached in the 
                // parent executor is read by a speculating executor.
            }
        }
    }

    if (!result) {
        VDF_PBEE_TRACE_SCOPE(
            "VdfPullBasedExecutorEngine<T>::_PassOrCopyBufferInternal "
            "(copying vector)");
        result = _CopyCache(
            output, _dataManager->GetBufferData(dataHandle), source, inputMask);
    }

    return result;
}

template<typename DataManagerType>
void
VdfPullBasedExecutorEngine<DataManagerType>::_PrepareReadWriteBuffer(
    const _DataHandle dataHandle,
    const VdfInput &input,
    const VdfMask &mask,
    const VdfSchedule &schedule)
{
    // Get the output associated with the read/write input.
    const VdfOutput *output = input.GetAssociatedOutput();
    TF_DEV_AXIOM(output);

    // Here's where we have the most potential for optimization.  We 
    // can re-use our inputs cache (without any copying) if our input 
    // has one and only one output (and that's us)
    const size_t numInputNodes = input.GetNumConnections();
    if (numInputNodes == 1 && !input[0].GetMask().IsAllZeros()) {
        _PassOrCopyBufferInternal(
            dataHandle, *output, input[0].GetSourceOutput(), mask, schedule);
        return; 
    }

    // If we have no inputs, provide a fresh new cache.
    _dataManager->CreateOutputCache(
        *output, _dataManager->GetBufferData(dataHandle));
}

template<typename DataManagerType>
bool
VdfPullBasedExecutorEngine<DataManagerType>::_PushBackOutput(
    std::vector< _OutputToExecute > *outputs,
    const VdfMask& lockedCacheMask,
    const VdfOutput &output,
    const VdfSchedule &schedule)
{
    VdfSchedule::OutputId outputId = schedule.GetOutputId(output);

    if (outputId.IsValid()) {
        // Push the output
        outputs->push_back(
            _OutputToExecute(
                outputId, 
                lockedCacheMask,
                schedule.IsAffective(outputId)));
        return true;
    }

    // The output to push is not actually scheduled, which guarantees
    // that is value will never be needed by any computations.  So
    // just skip it.
    return false;
}

template<typename DataManagerType>
bool
VdfPullBasedExecutorEngine<DataManagerType>::_UpdateOutputForSMBL(
    const VdfOutput &output,
    _OutputToExecute *stackEntry,
    const VdfSchedule &schedule)
{
    VDF_PBEE_TRACE_FUNCTION();

    // Retrieve the output data handle.
    const _DataHandle dataHandle = _dataManager->GetDataHandle(output.GetId());
    if (!_dataManager->IsValidDataHandle(dataHandle)) {
        return false;
    }

    // Get the invalidation timestamp at the output.
    const VdfInvalidationTimestamp invalidationTs =
        _dataManager->GetInvalidationTimestamp(dataHandle);

    // If this output has never been invalidated, bail out.
    if (!invalidationTs) {
        return false;
    }

    // If this output was not invalidated during the last invalidation round,
    // do not consider it for sparse mung buffer locking. The first output that
    // is no longer part of the last invalidation round will hold the fully
    // locked mung buffer.
    // Note, we also have to reset the locked cache mask when crossing the
    // timestamp edge. If we ever reach back into a pool chain that has the
    // current invalidation timestamp, we have to start back up with an empty
    // locked cache mask.
    if (invalidationTs != _dataManager->GetInvalidationTimestamp()) {
        if (!stackEntry->lockedCacheMask.IsEmpty()) {
            stackEntry->lockedCacheMask = VdfMask();
            return true;
        }
        return false;
    }

    // Output updated?
    bool updated = false;

    // Append the data sitting at this output to the locked cache mask. This
    // section of the code is responsible for growing the lockedCacheMask as
    // we traverse up the pool chain.
    VdfExecutorBufferData *outputBuffer =
        _dataManager->GetBufferData(dataHandle);
    VdfSMBLData *smblData = _dataManager->GetOrCreateSMBLData(dataHandle);
    const VdfSchedule::OutputId &outputId = stackEntry->outputId;
    const VdfMask &keepMask = schedule.GetKeepMask(outputId);
    if (outputBuffer->GetExecutorCache() && 
        !outputBuffer->GetExecutorCacheMask().IsEmpty() &&
        !keepMask.IsEmpty()) {
        smblData->ExtendLockedCacheMask(
            &stackEntry->lockedCacheMask,
            outputBuffer->GetExecutorCacheMask());
        stackEntry->absorbLockedCache = true;
        updated = true;
    }

    // If the locked cache mask is still empty, than there is no work to do.
    if (stackEntry->lockedCacheMask.IsEmpty()) {
        return false;
    }

    // Before determining the affective-ness of the node, insure that the data
    // indicated by the keep mask is stored in the executor cache, and that
    // any bits not contained in the executor cache are not contained in the
    // locked cache mask. Otherwise, we could be skipping nodes which really
    // need to run in order to provide valid values to be kept.
    if (!keepMask.IsEmpty()) {
        smblData->RemoveUncachedMask(
            &stackEntry->lockedCacheMask,
            outputBuffer->GetExecutorCacheMask(),
            keepMask);
        updated = true;
    }

    // If this node is affective in the schedule, we may be able to get away
    // without computing it, and making it un-affective. We determine whether
    // this is the case by looking at the lockedCacheMask to see if it contains
    // the scheduled affects mask.
    if (stackEntry->affective &&
        !smblData->ComputeAffectiveness(
            stackEntry->lockedCacheMask,
            schedule.GetAffectsMask(outputId))) {
        stackEntry->affective = false;
        return true;
    }

    // Any updates to the output?
    return updated;
}

template<typename DataManagerType>
void
VdfPullBasedExecutorEngine<DataManagerType>::_ExecuteOutput(
    const VdfEvaluationState &state,
    const VdfOutput &output, 
    TfBits *executedNodes)
{
#if _VDF_PBEE_TRACE_ON
    std::cout << "----------------- _ExecuteOutput --------- " << std::endl;
#endif

    // The current schedule
    const VdfSchedule &schedule = state.GetSchedule();

    // Is Sparse Mung Buffer Locking enabled for this round of evaluation?
    //
    // Note that executors that may be interrupted, do not yet support SMBL.
    // After interruption, a buffer that has not been fully passed down the pool
    // chain, may contain garbage data. That same buffer may then get picked up
    // in subsequent evaluation rounds, where it is assumed to be entirely
    // valid.
    const bool enableSMBL =
        schedule.HasSMBL() && !_GetExecutor().GetInterruptionFlag();

    // This is the stack of the outputs currently in the process of execution.
    std::vector< _OutputToExecute > outputsStack;

    // Add the first output to the stack.
    _PushBackOutput(&outputsStack, VdfMask(), output, schedule);

    while (!outputsStack.empty()) {

        // If we've been interrupted, bail out.
        if (_GetExecutor().HasBeenInterrupted()) {
            break;
        }

        // Stack Top State
        VdfSchedule::OutputId outputId = outputsStack.back().outputId;
        bool affective = outputsStack.back().affective;
        VdfMask lockedCacheMask = outputsStack.back().lockedCacheMask;
        bool absorbLockedCache = outputsStack.back().absorbLockedCache;

        // Temporary State
        const VdfMask *requestMask = NULL;
        const VdfOutput *output = NULL;
        const VdfNode &node = *schedule.GetNode(outputId);
        bool added = false;

        switch (outputsStack.back().stage) {

        case ExecutionStageStart:

#if _VDF_PBEE_TRACE_ON
            std::cout << "{ BeginNode(\"" 
                      << node.GetDebugName() << "\");" << std::endl;
#endif

            // We have to compute if 
            //   o The node has not been executed, yet
            //   o The output is dirty
            //   o The cache is empty
            //   o The computed mask doesn't cover what is asked for in the 
            //     schedule.
            output = schedule.GetOutput(outputId);
            requestMask = &schedule.GetRequestMask(outputId);
            if (executedNodes->IsSet(VdfNode::GetIndexFromId(node.GetId())) ||
                _GetExecutor().GetOutputValue(*output, *requestMask)) {

                // Pop off the top of the output stack
                outputsStack.pop_back();

#if _VDF_PBEE_TRACE_ON
                std::cout << " EndNodeFoundCache(); }" << std::endl;
#endif
                continue;
            }

            // Update the output for SMBL. This refreshes the affective-ness
            // flag, the lockedCacheMask and the flag that indicates whether
            // the locked cache should be absorbed into the executor cache.
            if (enableSMBL && Vdf_IsPoolOutput(*output)) {
                // Update the top of the output stack. Since no new outputs
                // have been pushed onto the stack at this point, the top is
                // still the output we are currently executing.
                _OutputToExecute *stackTop = &outputsStack.back();
                if (_UpdateOutputForSMBL(*output, stackTop, schedule)) {
                    affective = stackTop->affective;
                    lockedCacheMask = stackTop->lockedCacheMask;
                    absorbLockedCache = stackTop->absorbLockedCache;
                }
            }

            // The first stage of computation is to execute all the
            // prerequisites for current output.  So we push them on our stack
            // and wait for them to be computed.

            // Mark that we've processed the prerequisites for this output.
            outputsStack.back().stage = ExecutionStagePreRequisitesDone;

            // Push back all the prerequisites if this output will do anything
            if (affective) {
                for (const VdfScheduleInput &input : schedule.GetInputs(node)) {
                    if (input.input->GetSpec().IsPrerequisite()) {
                        added |= _PushBackOutput(
                            &outputsStack, VdfMask(), *input.source, schedule);
                    }
                }
            }

            // If we added inputs then we want to go back to the top of the
            // loop and execute our inputs, otherwise we will fall through to 
            // the next stage.
            if (added) {
                break;
            } // else fall through to the next stage.

        case ExecutionStagePreRequisitesDone:

            // Now that all the prerequisites are done, the second stage
            // of computation is to use the prerequisites to determine what
            // other inputs we need to run to satisfy the current output.

            // Mark that all the inputs have now been processed for the 
            // current output.
            outputsStack.back().stage = ExecutionStageCompute;

            // Note that outputs added are executed in reverse order.  So we
            // push last the nodes that we want to run first.

            // Only run the reads if the output is expected to modify
            // anything.
            if (affective) {

                // Get the list of required inputs based on the prerequisite
                // computations.
                VdfRequiredInputsPredicate inputsPredicate =
                    node.GetRequiredInputsPredicate(VdfContext(state, node));

                // Run the required reads last.
                // Here we try to run the "read" inputs after the "read/write" 
                // inputs.
                if (inputsPredicate.HasRequiredReads()) {
                    for (const VdfScheduleInput &input :
                            schedule.GetInputs(node)) {
                        if (inputsPredicate.IsRequiredRead(*input.input)) {
                            added |= _PushBackOutput(
                                &outputsStack, VdfMask(),
                                *input.source, schedule);
                        }
                    }
                }
            }

            // Run the read/writes first, so that we can maximize the chance of 
            // being able to re-use the kept buffers for speculations.
            for (const VdfScheduleInput &input : schedule.GetInputs(node)) {
                const VdfOutput *assocOutput =
                    input.input->GetAssociatedOutput();
                if (!assocOutput) {
                    continue;
                }

                // Does this output have a pass-through scheduled?
                const VdfSchedule::OutputId &assocOutputId = 
                    schedule.GetOutputId(*assocOutput);
                if (assocOutputId.IsValid()) {
                    if (const VdfOutput *fromBufferOutput =
                            schedule.GetFromBufferOutput(assocOutputId)) {
                        added |= _PushBackOutput(
                            &outputsStack, lockedCacheMask,
                            *fromBufferOutput, schedule);
                        continue;
                    }
                }
                        
                // If the associated output is not scheduled, or it does not
                // have a pass-through scheduled, we need to consider all
                // connected source outputs!
                added |= _PushBackOutput(
                    &outputsStack, lockedCacheMask, *input.source, schedule);
            }

            // If we added inputs then we want to go back to the top of the
            // loop and execute our inputs, otherwise we will fall through to 
            // the next stage.
            if (added) {
                break;
            } // else fall through to the next stage.

        default:

            // Set a bit indicating that this node has been executed.
            executedNodes->Set(VdfNode::GetIndexFromId(node.GetId()));

            // Compute the node.
            if (affective) {
                _ComputeNode(state, node, absorbLockedCache);
#if _VDF_PBEE_TRACE_ON
            std::cout << "ComputedNode(\"" 
                      << node.GetDebugName() 
                      << "\"); }" << std::endl;
#endif

            } else {
                // The node doesn't have any outputs that need to be computed.
                // Skip the node passing through the data for read/write
                // outputs.
                _PassThroughNode(schedule, node, absorbLockedCache);
#if _VDF_PBEE_TRACE_ON
                std::cout << "ComputedNodeInaffective(\"" 
                          << node.GetDebugName() 
                          << "\"); }" << std::endl;
#endif
            }

            // Pop the output off the stack, once we are done with it
            outputsStack.pop_back();
        }
    }
}

template<typename DataManagerType>
void
VdfPullBasedExecutorEngine<DataManagerType>::_ComputeNode(
    const VdfEvaluationState &state,
    const VdfNode &node,
    bool absorbLockedCache) 
{
    VDF_PBEE_TRACE_FUNCTION();

    VdfExecutionStats *stats = _executor.GetExecutionStats();

    VdfExecutionStats::ScopedMallocEvent 
        compute(stats, node, VdfExecutionStats::NodeEvaluateEvent);

    if (stats) {
        stats->LogTimestamp(VdfExecutionStats::NodeDidComputeEvent, node);
    }


    // The current schedule.
    const VdfSchedule &schedule = state.GetSchedule();

    // Clear the acceleration structure for output data lookups.
    _dataHandleCache.clear();

    VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(outputId, schedule, node) {
        const VdfOutput &output = *schedule.GetOutput(outputId);

        // Retrieve the data handle and cache it for accelerated lookup below.
        const _DataHandle dataHandle =
            _dataManager->GetOrCreateDataHandle(output.GetId());
        _dataHandleCache.push_back(dataHandle);

        // Retrieve the buffer data associated with the handle.
        VdfExecutorBufferData *bufferData =
            _dataManager->GetBufferData(dataHandle);

        // If this output still contains data (i.e., invalidation did not
        // remove the cache), it may have been locked and we may want to retain
        // the data to absorb it shortly.
        if (absorbLockedCache ||
            (bufferData->GetExecutorCache() &&
                _IsNotPassing(output, outputId, schedule))) {
            bufferData->RetainExecutorCache(
                output.GetSpec(),
                _dataManager->GetOrCreateSMBLData(dataHandle));
        }

        // Before we compute the output, we have to make sure that all 
        // the recipients of its cache are cleared and that the cache is 
        // reclaimed by output.
        bufferData->ResetExecutorCache();

        // Mark the output as having been touched during evaluation.
        _dataManager->Touch(dataHandle);

        // If this is a read/write output, make sure the buffer has been
        // passed down. We also need to set the computed output mask here,
        // because the node will read input values of read/write inputs
        // directly at this output.
        // Note, that on interruption this mask must be reset!
        if (const VdfInput *ai = output.GetAssociatedInput()) {
            const VdfMask &requestMask = schedule.GetRequestMask(outputId);
            _PrepareReadWriteBuffer(dataHandle, *ai, requestMask, schedule);
            _dataManager->SetComputedOutputMask(bufferData, requestMask);
        }
    }

    // Compute the node
    {
        VDF_PBEE_TRACE_SCOPE(
            "VdfPullBasedExecutorEngine<T>::_ComputeNode "
            "(node callback)");

        node.Compute(VdfContext(state, node));
    }

    // Has the node been interrupted during execution?
    const bool hasBeenInterrupted = _GetExecutor().HasBeenInterrupted();

    // Deallocate temporary buffers which the schedule knows can be deallocated
    // now that this node has run (they will never be read again before they
    // are deallocated due to invalidation).
    if (const VdfOutput* ctd = schedule.GetOutputToClear(node)) {
        // Fetch the data handle directly from _dataManager, rather than
        // through a virtual method, because we only ever want to eagerly clear
        // temporary buffers in our own data manager (never a parent's).
        const _DataHandle dataHandle =
            _dataManager->GetDataHandle(ctd->GetId());
        if (_dataManager->IsValidDataHandle(dataHandle)) {
            _dataManager->GetBufferData(dataHandle)->Reset();
        }
    }

    // We now need to mark the computed parts of our vectors.
    size_t outputIndex = 0;
    VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(outputId, schedule, node) {
        const VdfOutput &output = *schedule.GetOutput(outputId);
        const VdfMask &requestMask = schedule.GetRequestMask(outputId);

        // Retrieve the data handle from the cache.
        const _DataHandle dataHandle = _dataHandleCache[outputIndex++];

        // Retrieve the buffer data associated with the handle.
        VdfExecutorBufferData *bufferData =
            _dataManager->GetBufferData(dataHandle);

        // Check to see if the node did indeed produce values for this
        // output. We don't want to post warnings for missing output values
        // if the node has been interrupted.
        if (!hasBeenInterrupted &&
            !output.GetAssociatedInput() &&
            !bufferData->GetExecutorCache()) {

            // This is an output without an associated input that has
            // no value even though it was requested.  (We know it is
            // requested because otherwise, it wouldn't be in the schedule,
            // because of VdfScheduler::_RemoveTrivialNodes.)
            TF_WARN(
                "No value set for output " + output.GetDebugName() +
                " of type " + output.GetSpec().GetType().GetTypeName() +
                " named " + output.GetName().GetString());

            //XXX: This is not 100% right when we use a single data flow
            //     element to hold multiple values (as we do for shaped
            //     attributes).  FillVector() would need to know that this
            //     is the case and it would need to know the # of values
            //     to package into the output.  This can happen anywhere
            //     in the network, but for now, I only added a workaround
            //     in the EfCopyToPoolNode.
            VdfExecutionTypeRegistry::FillVector(
                output.GetSpec().GetType(),
                requestMask.GetSize(),
                _dataManager->GetOrCreateOutputValueForWriting(
                    output, dataHandle));
        }

        // If the node has been interrupted, make sure to reset the computed
        // output mask: Read/writes will already have their mask set.
        _FinalizeComputedOutput(
            dataHandle, 
            requestMask,
            hasBeenInterrupted,
            _IsNotPassing(output, outputId, schedule));

        // Log stats
        if (stats) {
            const VdfNode& node = output.GetNode();

            stats->LogData(
                VdfExecutionStats::ElementsProcessedEvent,
                node,
                schedule.GetAffectsMask(outputId).GetNumSet());
        }
    }
}

template<typename DataManagerType>
bool 
VdfPullBasedExecutorEngine<DataManagerType>::_PassThroughNode(
    const VdfSchedule &schedule,
    const VdfNode &node,
    bool absorbLockedCache)
{
    VDF_PBEE_TRACE_FUNCTION();

    bool passedThrough = false;

    VdfExecutionStats *stats = _executor.GetExecutionStats();
    VdfExecutionStats::ScopedMallocEvent
        compute(stats, node, VdfExecutionStats::NodeEvaluateEvent);

    VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(outputId, schedule, node) {
        const VdfOutput &output = *schedule.GetOutput(outputId);
        const VdfMask &requestMask = schedule.GetRequestMask(outputId);

        // Retrieve the data handle.
        const _DataHandle dataHandle =
            _dataManager->GetOrCreateDataHandle(output.GetId());

        // Get the buffer data associated with the data handle.
        VdfExecutorBufferData *bufferData =
            _dataManager->GetBufferData(dataHandle);
        
        // If this output still contains data (i.e., invalidation did not
        // remove the cache), it may have been locked and we may want to retain
        // the data to absorb it shortly.
        if (absorbLockedCache ||
            (bufferData->GetExecutorCache() 
                && _IsNotPassing(output, outputId, schedule))) {
            bufferData->RetainExecutorCache(
                output.GetSpec(),
                _dataManager->GetOrCreateSMBLData(dataHandle));
        }

        // Before we pass the output data through, we have to make sure that 
        // all the recipients of its cache are cleared and that the cache is 
        // reclaimed by output.
        bufferData->ResetExecutorCache();

        // Marked the output as having been touched during evaluation, in order
        // for invalidation to consider this output.
        _dataManager->Touch(dataHandle);

        if (const VdfOutput *fromBufferOutput =
            schedule.GetFromBufferOutput(outputId)) {

            _PassOrCopySourceOutputBuffer(
                dataHandle, output, *fromBufferOutput, requestMask, schedule);

            passedThrough = true;

        } else if (const VdfInput *ai = output.GetAssociatedInput()) {

            // We better have one and only one connection on this input
            // connector.  Otherwise we can't pass anything through.
            TF_DEV_AXIOM(output.GetAssociatedInput()->GetNumConnections()==1);

            // If the output has an associated input, pass the data through.
            _PrepareReadWriteBuffer(dataHandle, *ai, requestMask, schedule);
            passedThrough = true;
        }

        // Finalize the computed output, by merging in any temporary data and
        // setting the appropriate computed output mask.
        _FinalizeComputedOutput(
            dataHandle, 
            requestMask,
            false, /* hasBeenInterrupted */
            _IsNotPassing(output, outputId, schedule));
    }

    return passedThrough;
}

template<typename DataManagerType>
void 
VdfPullBasedExecutorEngine<DataManagerType>::_FinalizeComputedOutput(
    const _DataHandle dataHandle,
    const VdfMask &requestMask,
    const bool hasBeenInterrupted,
    const bool extendRequestMask)
{
    // Retrieve the buffer data associated with the data handle.
    VdfExecutorBufferData *bufferData = _dataManager->GetBufferData(dataHandle);

    // Merge in temporary data, if available. Note, we must release the
    // SMBL data despite any possible interruption!
    VdfMask lockedMask = 
        bufferData->ReleaseExecutorCache(_dataManager->GetSMBLData(dataHandle));

    // Has the executor been interrupted? Make sure to reset the computed
    // output mask, so that subsequent cache hits do not return garbage data.
    if (hasBeenInterrupted) {
        _dataManager->SetComputedOutputMask(bufferData, VdfMask());
    }

    // Otherwise, set the computed output mask to the request mask.
    else {
        // If extendRequestMask is set and the cache's mask is non-empty, 
        // copy the bits merge the requestMask and the cacheMask.  Otherwise,
        // set using the standard requestMask.
        _dataManager->SetComputedOutputMask(
            bufferData, 
            extendRequestMask && !lockedMask.IsEmpty() ? 
                lockedMask | requestMask : 
                requestMask);
    }
}

///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
