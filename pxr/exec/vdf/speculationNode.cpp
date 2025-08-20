//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/speculationNode.h"

#include "pxr/exec/vdf/input.h"
#include "pxr/exec/vdf/inputSpec.h"
#include "pxr/exec/vdf/outputSpec.h"
#include "pxr/exec/vdf/request.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/speculationExecutor.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////

VdfSpeculationNode::VdfSpeculationNode(VdfNetwork *network,
                       const VdfInputSpecs &inputSpecs,
                       const VdfOutputSpecs &outputSpecs)
        : VdfNode(network, inputSpecs, outputSpecs)
{
    // Speculation nodes have exactly as many inputs as they have outputs.
    if (!TF_VERIFY(inputSpecs.GetSize() == outputSpecs.GetSize())) {
        return;
    }

    // Verify that our inputs and outputs match 1 to 1 on type and name
    for (size_t i=0; i< inputSpecs.GetSize(); ++i) {
        const VdfInputSpec *inputSpec   = inputSpecs.GetInputSpec(i);
        const VdfOutputSpec *outputSpec = outputSpecs.GetOutputSpec(i);        

        TF_VERIFY(inputSpec->GetType() == outputSpec->GetType());
        TF_VERIFY(inputSpec->GetName() == outputSpec->GetName());
    }
}

VdfSpeculationNode::~VdfSpeculationNode() 
{
}

static VdfMaskedOutput
_GetSourceOutput(const VdfNode &node, const TfToken &name)
{
    // Make sure the input exists.
    const VdfInput *input = node.GetInput(name);
    if (!TF_VERIFY(input)){
        return VdfMaskedOutput();
    }

    // Sanity check that the input has exactly one incoming connection.
    if (!TF_VERIFY(input->GetNumConnections() == 1,
        "input \"%s\" has %zu incoming connections instead of 1 on node %s",
        input->GetName().GetText(),
        input->GetNumConnections(),
        node.GetDebugName().c_str())) {
        return VdfMaskedOutput();
    }

    // Return the source output.
    const VdfConnection &connection = (*input)[0];
    return VdfMaskedOutput(
        &connection.GetNonConstSourceOutput(),
        connection.GetMask());
} 

void
VdfSpeculationNode::Compute(const VdfContext &context) const
{
    TfAutoMallocTag2 tag("Vdf", "VdfSpeculationNode::Compute");

    TRACE_FUNCTION();

    // Get the calling schedule from the context
    const VdfSchedule &callingSchedule = context._GetSchedule();

    // Get the input request. These are the outputs that the speculation node
    // consumes as inputs.
    const VdfRequest inputRequest = _GetInputRequest(callingSchedule);

    // Get a valid local schedule.
    const VdfSchedule *localSchedule = _GetSchedule(inputRequest);

    // Make an executor and set up the parent executor.
    const VdfExecutorInterface &contextExecutor = _GetContextExecutor(context);
    std::unique_ptr<VdfSpeculationExecutorBase> executor(
        contextExecutor.GetFactory().ManufactureSpeculationExecutor(
            this, &contextExecutor));

    // Inherit the executor invalidation timestamp from the parent executor
    // for use with mung buffer locking.
    executor->InheritExecutorInvalidationTimestamp(contextExecutor);

    // Retrieve the error logger from the context, if any.
    VdfExecutorErrorLogger *errorLogger = context._GetErrorLogger();

    // Run the speculation executor
    executor->Run(*localSchedule, errorLogger);

    // Bail if the executor has been interrupted, don't bother reading out
    // its values.
    if (executor->HasBeenInterrupted()) {
        return;
    }

    // Get the execution stats from the parent executor, if any.
    VdfExecutionStats *parentStats = contextExecutor.GetExecutionStats();
    
    // Iterate through inputs and outputs together, passing values
    // on each input to the corresponding output.  Inputs and outputs
    // on this node match one-to-one
    VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(outputId, callingSchedule, *this) {
        const VdfOutput *output = callingSchedule.GetOutput(outputId);

        // The source output connected to the corresponding input.
        const VdfMaskedOutput sourceOutput =
            _GetSourceOutput(*this, output->GetName());
        if (!sourceOutput) {
            continue;
        }

        // Retrieve the value from the source output and copy it to the
        // speculation node output. We expect the source value to always be
        // available, since we just executed the input request.
        const VdfVector *val =
            executor->GetOutputValue(
                *sourceOutput.GetOutput(), sourceOutput.GetMask());
        if (val) {
            VdfVector *result =
                contextExecutor._GetOutputValueForWriting(*output);
            if (TF_VERIFY(result)) {
                *result = *val;
            }
        }

        // If the source value does not exist, something went awry during the
        // input request execution. We may end up getting here after
        // encountering a true dependency cycle during the input evaluation.
        else {
            context.Warn(
                "Speculation computation failed. Requested data unavailable at "
                "output: %s", 
                sourceOutput.GetOutput()->GetDebugName().c_str());
        }

        // Mark the output of this VdfSpeculationNode as requested in the
        // stats belonging to the parent executor, where this node is
        // executing.
        if (parentStats) {
            parentStats->LogData(
                VdfExecutionStats::RequestedOutputInSpeculationsEvent,
                output->GetNode(),
                output->GetId());
        }
    }
}

const VdfSchedule &
VdfSpeculationNode::GetSchedule(const VdfSchedule *requestingSched) const
{
    return *_GetSchedule(_GetInputRequest(*requestingSched));
}

VdfRequest
VdfSpeculationNode::_GetInputRequest(const VdfSchedule &requestingSched) const
{
    VdfMaskedOutputVector maskedOutputs;

    // Build a request that pulls on all our inputs.
    VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(outputId, requestingSched, *this) {
        const VdfOutput *output = requestingSched.GetOutput(outputId);
        if (VdfMaskedOutput mo = _GetSourceOutput(*this, output->GetName())) {
            maskedOutputs.push_back(std::move(mo));
        }
    }

    return VdfRequest(std::move(maskedOutputs));
}

const VdfSchedule *
VdfSpeculationNode::_GetSchedule(const VdfRequest &request) const
{
    // XXX: Note that here we store schedules based on the request
    // and unless the node itself is destroyed we can potentially store as
    // many schedules as there are combinations of requests on its inputs.
    TfAutoMallocTag2 tag("Vdf", "VdfSpeculationNode::_GetSchedule");

    // Fast-path for finding an existing, and valid schedule. This will acquire
    // a reader lock only.
    {
        _ScheduleMap::const_accessor accessor;
        if (_scheduleMap.find(accessor, request)) {
            const VdfSchedule *schedule = accessor->second.get();
            if (schedule && schedule->IsValid()) {
                return schedule;
            }
        }
    }

    // Insert a new schedule, if one does not alraedy exist.
    _ScheduleMap::accessor accessor;
    _scheduleMap.insert(accessor, request);

    // Construct a new schedule, if the entry is a nullptr.
    VdfSchedule *schedule = accessor->second.get();
    if (!schedule) {
        accessor->second.reset(new VdfSchedule());
        schedule = accessor->second.get();
    }

    // If the schedule is currently not valid, re-schedule it. Newly
    // constructed schedules will not be valid.
    if (!schedule->IsValid()) {
        VdfScheduler::Schedule(
            request, schedule, false /* topologicallySort */);
    }

    // Return the valid schedule;
    return schedule;
}

VdfMask
VdfSpeculationNode::_ComputeOutputDependencyMask(
    const VdfConnection &inputConnection,
    const VdfMask       &inputDependencyMask,
    const VdfOutput     &output) const
{
    // Get the input targeted by inputConnection, and find the corresponding
    // output.
    const VdfOutput *correspondingOutput =
        GetOutput(inputConnection.GetTargetInput().GetName());

    // If we're talking about the corresponding output, the dependencies are
    // correlated.
    if (TF_VERIFY(correspondingOutput) && &output == correspondingOutput) {
        return inputDependencyMask & inputConnection.GetMask();
    }

    // Otherwise, there's no dependency, so return an empty mask.
    return VdfMask();
}

VdfMask::Bits
VdfSpeculationNode::_ComputeInputDependencyMask(
    const VdfMaskedOutput &maskedOutput,
    const VdfConnection &inputConnection) const
{
    // Get the input targeted by inputConnection, and find the corresponding
    // output.
    const VdfOutput *correspondingOutput =
        GetOutput(inputConnection.GetTargetInput().GetName());
    const VdfOutput *output = maskedOutput.GetOutput();

    // If we're talking about the corresponding output, the dependencies are
    // correlated.
    if (TF_VERIFY(correspondingOutput) &&
        TF_VERIFY(output) &&
        output == correspondingOutput) {
        return
            inputConnection.GetMask().GetBits() &
            maskedOutput.GetMask().GetBits();
    }

    // Otherwise, there's no dependency, so return an empty mask.
    return VdfMask::Bits();
}

PXR_NAMESPACE_CLOSE_SCOPE
