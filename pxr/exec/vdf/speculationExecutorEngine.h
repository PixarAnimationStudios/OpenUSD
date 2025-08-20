//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SPECULATION_EXECUTOR_ENGINE_H
#define PXR_EXEC_VDF_SPECULATION_EXECUTOR_ENGINE_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/pullBasedExecutorEngine.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/dataManagerVector.h"
#include "pxr/exec/vdf/evaluationState.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/requiredInputsPredicate.h"
#include "pxr/exec/vdf/speculationExecutorBase.h"

#define _VDF_SEE_TRACE_ON 0
#if _VDF_SEE_TRACE_ON
#include <iostream>
#endif

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorErrorLogger;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfSpeculationExecutorEngine
///
/// \brief This class provides an executor engine to the speculation executor.
///
/// \remark This class inherits from VdfPullBasedExecutorEngine only to
/// share code.  It is not meant to behave polymorphically.
///
template <typename DataManagerType>
class VdfSpeculationExecutorEngine : 
    public VdfPullBasedExecutorEngine<DataManagerType>
{
    // Base type definition
    typedef
        VdfPullBasedExecutorEngine<DataManagerType>
        Base;

public:

    /// Constructs an engine used by the speculation executor.
    ///
    VdfSpeculationExecutorEngine(
        const VdfSpeculationExecutorBase &speculationExecutor, 
        DataManagerType *dataManager) :
        VdfPullBasedExecutorEngine<DataManagerType>(
            speculationExecutor, dataManager),
        _writeBackExecutor(const_cast<VdfExecutorInterface *>(
            speculationExecutor.GetNonSpeculationParentExecutor())) {
        TF_VERIFY(_writeBackExecutor);
    }

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
    template <typename F>
    void RunSchedule(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest,
        VdfExecutorErrorLogger *errorLogger,
        F &&callback);

private:
    // MSVC errors out if Base::ExecutionStageStart is accessed in the
    // _OutputToExecute constructor, because the enumerator is protected in the
    // base class. GCC and clang do not consider this an error. To work around
    // this issue, we loft the enumerator into the namespace of the derived
    // class where _OutputToExecute() has the proper privileges across all
    // compilers.
    static constexpr typename Base::_ExecutionStage _ExecutionStageStart =
        Base::ExecutionStageStart;

    // This struct contains the necessary state to compute an output.
    //
    struct _OutputToExecute {

        // Constructor that takes schedule node and output.
        _OutputToExecute(const VdfSchedule::OutputId &outputId) :
            outputId(outputId),
            stage(_ExecutionStageStart),
            numPushed(0),
            inputsSpeculate(false)
        { }

        // The schedule identifier for the output to execute.
        VdfSchedule::OutputId outputId;

        // The current phase of this output in the execution stack.
        typename Base::_ExecutionStage stage;

        // The number of inputs that this output is waiting on.
        int numPushed;

        // Whether or not our read inputs speculate.
        bool inputsSpeculate;
    };

    // This method adds \p output to the \p outputs vector.
    // Returns \c true if it added a new output and \c false otherwise.
    //
    bool _PushBackOutputForSpeculation(
        std::vector< _OutputToExecute > *outputs,
        const VdfOutput &output,
        const VdfSchedule &schedule);

    // Method that makes sure that data is available for the given \p output 
    // before returning.
    //
    void _ExecuteOutputForSpeculation(
        const VdfEvaluationState &state,
        const VdfOutput &output,
        TfBits *executedNodes,
        TfBits *speculatedNodes);

    // Write the computed output back to the write-back executor.
    void _WriteBackComputedOutput(
        const VdfOutput &output,
        const VdfSchedule::OutputId &outputId,
        const VdfSchedule &schedule);

    // The parent executor this speculation engine is going to write back to.
    VdfExecutorInterface *_writeBackExecutor;

};

///////////////////////////////////////////////////////////////////////////////

template <typename DataManagerType>
bool
VdfSpeculationExecutorEngine<DataManagerType>::_PushBackOutputForSpeculation(
    std::vector< _OutputToExecute > *outputs,
    const VdfOutput &output,
    const VdfSchedule &schedule)
{
    VdfSchedule::OutputId outputId = schedule.GetOutputId(output);

    if (outputId.IsValid()) {
        outputs->push_back(_OutputToExecute(outputId));
        return true;
    }

    // The output to push is not actually scheduled, which guarantees
    // that is value will never be needed by any computations.  So
    // just skip it.
    return false;
}

template <typename DataManagerType>
template <typename F>
void
VdfSpeculationExecutorEngine<DataManagerType>::RunSchedule(
    const VdfSchedule &schedule,
    const VdfRequest &computeRequest,
    VdfExecutorErrorLogger *errorLogger,
    F &&callback)
{
    TRACE_FUNCTION();

    // Make sure the executor data manager is appropriately sized.
    Base::_GetDataManager()->Resize(*schedule.GetNetwork());

    const size_t numNodes = schedule.GetNetwork()->GetNodeCapacity();

    // Has a bit set for any node that has already been run.
    TfBits executedNodes(numNodes);

    // Has a bit set for any node, which had one ore more inputs speculated.
    TfBits speculatedNodes(numNodes);

    // The persistent evaluation state
    VdfEvaluationState state(Base::_GetExecutor(), schedule, errorLogger);

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
        if (Base::_GetExecutor().GetOutputValue(output, mask)) {
            callback(*maskedOutput, i);
            continue;
        }

        _ExecuteOutputForSpeculation(
            state, output, &executedNodes, &speculatedNodes);

        // If we've been interrupted, bail out.
        if (Base::_GetExecutor().HasBeenInterrupted()) {
            break;
        }

        // Invoke the callback once the output has been evaluated, but only
        // if the executor has not been interrupted.
        else {
            callback(*maskedOutput, i);
        }
    }
}

template <typename DataManagerType>
void 
VdfSpeculationExecutorEngine<DataManagerType>::_ExecuteOutputForSpeculation(
    const VdfEvaluationState &state,
    const VdfOutput &output,
    TfBits *executedNodes,
    TfBits *speculatedNodes)
{
#if _VDF_SEE_TRACE_ON
    std::cout << "{ SpeculationOutputExecuteBegin();" << std::endl; 
#endif

    // The current schedule
    const VdfSchedule &schedule = state.GetSchedule();

    // This is the stack of the outputs currently in the process of execution.
    std::vector< _OutputToExecute > outputsStack;

    // This is a stack used for the return values of outputs.  A return value
    // of true means that the output couldn't be evaluated due to speculation.
    // XXX:optimization
    // It's possible to get rid of this vector all together if outputs were
    // allowed to write directly into their caller's stack space.
    std::vector< bool > speculated;

    // Add the initial output to start executing.  This call will check for
    // already cached values.
    _PushBackOutputForSpeculation(&outputsStack, output, schedule);

    bool hasBeenInterrupted = Base::_GetExecutor().HasBeenInterrupted();

    while (!outputsStack.empty() && !hasBeenInterrupted) {

        const VdfSchedule::OutputId &outputId = outputsStack.back().outputId;
        const VdfNode &node = *schedule.GetNode(outputId);
        typename Base::_ExecutionStage stage = outputsStack.back().stage;
        const size_t outputIndex = outputsStack.size() - 1;

        bool affective = schedule.IsAffective(outputId);

        // Pop all the return values from our inputs and check to see 
        // if any of them were 'true' (meaning that they hit a speculation
        // path).
        bool previousStageSpeculated = false;
        while (outputsStack.back().numPushed) {
            outputsStack.back().numPushed--;
            previousStageSpeculated |= speculated.back();
            speculated.pop_back();
        }

        switch (stage) {

        case Base::ExecutionStageStart:

#if _VDF_SEE_TRACE_ON
            std::cout << "{ SpeculationBeginNode(" << &node << ", \"" 
                      << node.GetDebugName() << "\");" << std::endl;
#endif

            // If this is the node that started the speculation, we need to
            // skip it. Note that this means we encountered a true data
            // dependency cycle and have a bad result.  Additionally, we may
            // write back the bad result to any parent executors.  
            if (static_cast<const VdfSpeculationExecutorBase &>(
                    Base::_GetExecutor()).IsSpeculatingNode(&node)) {
                speculated.push_back(true);
                outputsStack.pop_back();
#if _VDF_SEE_TRACE_ON
                std::cout << "SpeculationEndNodeSpeculationNode(); (cycle) }" 
                          << std::endl; 
#endif
                continue;
            }

            // If this node has already been executed, do not run it a second
            // time. However, make sure to push the right value onto the
            // speculated stack, based on whether the node had inputs we
            // speculated about, the last time it was run.
            if (executedNodes->IsSet(VdfNode::GetIndexFromId(node.GetId()))) {
                speculated.push_back(speculatedNodes->IsSet(
                    VdfNode::GetIndexFromId(node.GetId())));
                outputsStack.pop_back();
#if _VDF_SEE_TRACE_ON
                std::cout << "SpeculationEndNodeRedundantCompute(); }" 
                          << std::endl; 
#endif

                continue;
            }

            // If we are already cached for this output (or if our parent
            // executor is), then we can provide a value, we can return early.
            if (Base::_GetExecutor().GetOutputValue(
                    *schedule.GetOutput(outputId),
                    schedule.GetRequestMask(outputId))) {
                speculated.push_back(false);
                outputsStack.pop_back();
#if _VDF_SEE_TRACE_ON
                std::cout << "SpeculationEndNodeFoundCache(); }" << std::endl; 
#endif
                continue;
            }

            TF_DEV_AXIOM(outputsStack.back().numPushed == 0);

            // The first stage of computation is to execute all the
            // prerequisites for current output.
            outputsStack.back().stage = Base::ExecutionStagePreRequisitesDone;

            // Push back all the prerequisites
            if (affective) {
                for (const VdfScheduleInput &input : schedule.GetInputs(node)) {
                    if (input.input->GetSpec().IsPrerequisite()) {
                        const bool pushed = _PushBackOutputForSpeculation(
                            &outputsStack, *input.source, schedule);
                        outputsStack[outputIndex].numPushed += pushed;
                    }
                }
            }

            // Little optimization to not go back to the top of the loop
            // for no reason.
            if (outputsStack[outputIndex].numPushed > 0) {
                break;
            } // else fall through to the next stage.


        case Base::ExecutionStagePreRequisitesDone:


            // Now that our prerequisites are done, unroll our return stack.


            // Update whether or not our previousStageSpeculated
            outputsStack.back().inputsSpeculate |= previousStageSpeculated;

            // The second stage of computation is to use the prerequisites 
            // to determine what other inputs need to run to satisfy the 
            // current output.

            // Mark that the next stage of computation
            outputsStack.back().stage = Base::ExecutionStageReadsDone;

            // Only need to run the reads of an output that will do something
            // and if our pre-requisites were computed without speculation.
            if (affective && !previousStageSpeculated) {

                // Get the list of required inputs based on the prerequisite
                // computations.
                VdfRequiredInputsPredicate inputsPredicate =
                    node.GetRequiredInputsPredicate(VdfContext(state, node));

                // Run the required reads first.
                // Here we try to run the "read" inputs before the "read/write" 
                // inputs so that we can maximize the chance of being able to 
                // re-use the buffer.
                if (inputsPredicate.HasRequiredReads()) {
                    for (const VdfScheduleInput &input :
                            schedule.GetInputs(node)) {
                        if (inputsPredicate.IsRequiredRead(*input.input)) {
                            const bool pushed = _PushBackOutputForSpeculation(
                                &outputsStack, *input.source, schedule);
                            outputsStack[outputIndex].numPushed += pushed;
                        }
                    }
                }
            }

            // Little optimization to not go back to the top of the loop
            // for no reason.
            if (outputsStack[outputIndex].numPushed > 0) {
                break;
            } // else fall through to the next stage.

        case Base::ExecutionStageReadsDone:

            // Mark that the next stage of computation
            outputsStack.back().stage = Base::ExecutionStageCompute;

            // Mark whether or not our read inputs depend on a speculation.
            outputsStack.back().inputsSpeculate |= previousStageSpeculated;

            // Now run the read/writes last.
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
                        const bool pushed = _PushBackOutputForSpeculation(
                            &outputsStack, *fromBufferOutput, schedule);
                        outputsStack[outputIndex].numPushed += pushed;
                        continue;
                    }
                }
                        
                // If the associated output is not scheduled, or it does not
                // have a pass-through scheduled, we need to consider all
                // connected source outputs!
                const bool pushed = _PushBackOutputForSpeculation(
                    &outputsStack, *input.source, schedule);
                outputsStack[outputIndex].numPushed += pushed;
            }

            // Little optimization to not go back to the top of the loop
            // for no reason.
            if (outputsStack[outputIndex].numPushed > 0) {
                break;
            } // else fall through to the next stage.


        default:

            // Mark whether or not our read/write inputs depend on
            // a speculation.
            outputsStack.back().inputsSpeculate |= previousStageSpeculated;
        
            // Set a bit indicating that this node has been executed.
            executedNodes->Set(VdfNode::GetIndexFromId(node.GetId()));

            // If any of our inputs speculated, there is nothing we can do.
            // Skip this node, but make sure to still touch its outputs.
            if (outputsStack.back().inputsSpeculate) {
#if _VDF_SEE_TRACE_ON
                std::cout << "SpeculationSkipNode (cycle) (\""
                          << node.GetDebugName() 
                          << "\"); }" << std::endl;
#endif

                // This node has speculated inputs
                speculatedNodes->Set(VdfNode::GetIndexFromId(node.GetId()));
                speculated.push_back(true);

            // Compute this node, if it is affective, or pass-through if any
            // of the reads speculated.
            } else if (affective) {
                // None of our inputs speculated, we can just compute as 
                // normal.
                Base::_ComputeNode(state, node);
                speculated.push_back(false);

#if _VDF_SEE_TRACE_ON
                std::cout << "SpeculationComputedNode(\""
                          << node.GetDebugName() 
                          << "\"); }" << std::endl; 
#endif

            // The node is not affective, and none of its reads or read/writes
            // did speculate.
            } else {
                // None of the outputs on this node contribute to the 
                // results in the request, so we will skip over this node
                // by passing through all the outputs with associated
                // inputs and use the fallback value for all the outputs
                // that don't.
#if _VDF_SEE_TRACE_ON
                std::cout << "SpeculationPassThrough(\""
                          << node.GetDebugName() 
                          << "\"); }" << std::endl; 
#endif
                Base::_PassThroughNode(schedule, node);
                speculated.push_back(false);

            }

            // Check interruption.
            hasBeenInterrupted = Base::_GetExecutor().HasBeenInterrupted();

            // Mark that we've visited these outputs in our parent
            // executor.  We need to tell the parent executor that
            // we've visited this node so that we receive invalidation
            // the next time it is required.  If we don't mark the
            // output as needing invalidation and the main executor
            // never needs to execute it, then it will never get
            // invalidated.
            // Also write back any computed or pass-through data to the
            // write back executor, so that the data can be picked up by
            // another executor. Note, that we do NOT want to write back
            // any data after interruption, because the buffers may
            // contain junk.
            VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(outputId, schedule, node) {
                const VdfOutput& output = *schedule.GetOutput(outputId);
                if (_writeBackExecutor &&
                    !hasBeenInterrupted &&
                    !outputsStack.back().inputsSpeculate) {
                    _WriteBackComputedOutput(output, outputId, schedule);
                }
                Base::_GetExecutor()._TouchOutput(output);
            }

            outputsStack.pop_back();
        }
    }

#if _VDF_SEE_TRACE_ON
    std::cout << "SpeculationOutputExecuteEnd(); }" << std::endl; 
#endif
}

template <typename DataManagerType>
void
VdfSpeculationExecutorEngine<DataManagerType>::_WriteBackComputedOutput(
    const VdfOutput &output,
    const VdfSchedule::OutputId &outputId,
    const VdfSchedule &schedule)
{
    // Retrieve the data handle.
    const typename Base::_DataHandle dataHandle =
        Base::_GetDataManager()->GetDataHandle(output.GetId());
    if (!Base::_GetDataManager()->IsValidDataHandle(dataHandle)) {
        return;
    }

    // Get the buffer data associated with the data handle.
    VdfExecutorBufferData *bufferData = 
        Base::_GetDataManager()->GetBufferData(dataHandle);

    // Get the output vector and computed output mask
    const VdfVector *value = bufferData->GetExecutorCache();

    // If the data is not available we are done.  This can happen with
    // node that manage their own buffers and choose to leave them empty.
    if (!value) {
        return;
    }

    const VdfMask &computedMask = bufferData->GetExecutorCacheMask();

    // If the computed output mask is empty, we can bail out early. This may
    // happen if, for example, the executor was interrupted and opted for not
    // writing a computed output mask for the current node.
    // Don't even bother writing back an all-zeros mask.
    if (computedMask.IsEmpty() || computedMask.IsAllZeros()) {
        return;
    }

    // If the output does not pass its data, we can write the full output value
    // back to the write executor.
    if (!output.GetAssociatedInput()) {
        _writeBackExecutor->SetOutputValue(output, *value, computedMask);

        // Reclaim locally, so that future cache lookups result in hits on
        // the parent executor, but not the local executor.
        //
        // XXX
        // This guards against client callbacks that mutate cached values (which
        // is something we have encountered in practice), causing output values
        // to change after the node has already run. By removing the buffer
        // locally, we ensure that the next time we access the buffer we get it
        // from the parent executor and modify it there. We would prefer to not
        // support this client behavior, but for now, we choose to keep this,
        // since it's not expensive, and safer.
        bufferData->ResetExecutorCache();
    } 

    // If the output passes its data, we may still be able to write back some
    // or all of it.
    else {
        // If this output is not scheduled to pass its data, we can simply copy
        // the entire executor cache. Alternatively, if the output is scheduled
        // to pass its data, we can at least copy anything that will be kept at
        // the output. Unless, however, invalidation entered somewhere between
        // this output, and the output we are going to pass the data to. If this
        // is the case, we want to write back the entire cache to the write back
        // executor, making this algorithm the equivalent of mung buffer locking
        // on the main executor!
        const VdfMask *writeBackMask = &computedMask;
        const VdfOutput *passToOutput = schedule.GetPassToOutput(outputId);
        if (passToOutput &&
            !_writeBackExecutor->HasInvalidationTimestampMismatch(
                output, *passToOutput)) {
            writeBackMask = &schedule.GetKeepMask(outputId);
        }

        if (!writeBackMask->IsEmpty()) {
            _writeBackExecutor->SetOutputValue(output, *value, *writeBackMask);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
