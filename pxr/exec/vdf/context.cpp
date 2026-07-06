//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/context.h"

#include "pxr/exec/vdf/grapher.h"
#include "pxr/exec/vdf/schedule.h"

#include "pxr/base/tf/stringUtils.h"

#include <cstdarg>

PXR_NAMESPACE_OPEN_SCOPE

void
VdfContext::SetEmptyOutput() const
{
    // GetOutput emits an error if it returns null. Note that there is no need
    // to check _IsRequiredOutput: By virtue of the owning node being scheduled,
    // we can conclude that its only output is therefore scheduled.
    const VdfOutput *const output = _node.GetOutput();
    if (!output) {
        return;
    }

    if (!TF_VERIFY(!output->GetAssociatedInput(),
                   "Attempt to set an empty value on associated output '%s'",
                   output->GetDebugName().c_str())) {
        return;
    }

    VdfVector *const vector = _GetExecutor()._GetOutputValueForWriting(*output);
    if (!vector) {
        VDF_FATAL_ERROR(_GetNode(), "Couldn't get output vector.");
    }

    vector->Clear();
}

void
VdfContext::SetEmptyOutput(const TfToken &outputName) const
{
    // GetOutput emits an error if it returns null.
    const VdfOutput *const output = _node.GetOutput(outputName);
    if (!(output && _IsRequiredOutput(*output))) {
        return;
    }

    if (!TF_VERIFY(!output->GetAssociatedInput(),
                   "Attempt to set an empty value on associated output '%s'",
                   output->GetDebugName().c_str())) {
        return;
    }

    VdfVector *const vector = _GetExecutor()._GetOutputValueForWriting(*output);
    if (!vector) {
        VDF_FATAL_ERROR(_GetNode(), "Couldn't get output vector.");
    }

    vector->Clear();
}

void
VdfContext::SetOutputToReferenceInput(const TfToken &inputName) const
{
    TfAutoMallocTag2 tag("Vdf", "VdfContext::SetOutputToReferenceInput");

    const VdfInput *input = _node.GetInput(inputName);
    const VdfOutput *output = _node.GetOutput();

    if (!input) {
        TF_CODING_ERROR("Invalid input name '%s' specified.",
                        inputName.GetText());
        return;
    }

    if (input->GetNumConnections() != 1) {
        TF_CODING_ERROR("Invalid number of inputs on '%s'.",
                        inputName.GetText());
        return;
    }

    if (!output) {
        TF_CODING_ERROR("Invalid output for node '%s'.",
                        _node.GetDebugName().c_str());
        return;
    }

    // See if we can apply the reference optimization.  We can't do this if the 
    // output is connected to an r/w input. 
    bool needCopy = false;
    for (const VdfConnection *wc : output->GetConnections()) {
        if (wc->GetTargetInput().GetAssociatedOutput()) {
            needCopy = true;
            break;
        }
    }
    
    const VdfConnection *c = input->GetConnections()[0];
    const VdfMask &sourceMask = c->GetMask();

    if (needCopy) {
        // Our output feeds into a r/w, need to make a copy.
        const VdfVector *rd = _GetExecutor()._GetInputValue(*c, sourceMask);
        if (VdfVector *wr = _GetExecutor()._GetOutputValueForWriting(*output)) {
            *wr = *rd;
        }
        return;
    }
    
    // XXX: Need to type check?
    _GetExecutor()._SetReferenceOutputValue(
        *output, c->GetSourceOutput(), sourceMask);
}


void
VdfContext::Warn(const char *fmt, ...) const
{
    va_list vl;
    va_start(vl, fmt);
    _state.LogWarning(_node, TfVStringPrintf(fmt, vl));
    va_end(vl);
}

std::string
VdfContext::GetNodeDebugName() const
{
    return _node.GetDebugName();
}

void 
VdfContext::CodingError(const char *fmt, ...) const
{
    VdfGrapher::GraphNodeNeighborhood(_node, 5, 5);
    va_list vl;
    va_start(vl, fmt);
    TF_CODING_ERROR(TfVStringPrintf(fmt, vl));
    va_end(vl);
}

bool 
VdfContext::_GetOutputMasks(const VdfOutput &output,
                             const VdfMask **requestMask,
                             const VdfMask **affectsMask) const
{
    TF_DEV_AXIOM(&output.GetNode() == &_GetNode());

    const VdfSchedule &schedule = _GetSchedule();
    if (!VdfScheduleTaskIsInvalid(_invocation)) {
        schedule.GetRequestAndAffectsMask(
            _invocation, requestMask, affectsMask);
        return true;
    }

    const VdfSchedule::OutputId outputId = schedule.GetOutputId(output);
    if (outputId.IsValid()) {
        schedule.GetRequestAndAffectsMask(outputId, requestMask, affectsMask);
        return true;
    }

    // Note that this is not an error and can readily happen when a 
    // node with multiple outputs gets executed and sets all its outputs
    // at once.  Some of these outputs are not necessarily scheduled.
    // The caller then is responsible for checking the return value,
    // and skipping outputs that are not scheduled.
    return false;
}

bool
VdfContext::HasInputValue(const TfToken &name) const
{
    // The implementation here is similar to what _GetFirstInputValue does, and
    // to what VdfReadIterator does.
    const VdfInput *const input = _GetNode().GetInput(name);
    if (!input) {
        return false;
    }

    for (const VdfConnection *const connection : input->GetConnections()) {
        const VdfMask &mask = connection->GetMask();
        if (mask.IsAllZeros()) {
            continue;
        }

        // The connection has a mask on it, return true if there's a value.
        if (const VdfVector *const vector =
            _GetExecutor()._GetInputValue(*connection, mask)) {
            if (vector->GetNumStoredElements() > 0) {
                return true;
            }
        }
    }

    // No values on any of the connections.
    return false;
}

bool
VdfContext::IsOutputRequested(const TfToken &outputName) const
{
    // Look up the output for outputName and use the private method to return
    // its requested-ness.
    const VdfOutput *output = _node.GetOutput(outputName);
    return output && _IsRequiredOutput(*output);
}

bool
VdfContext::_IsRequiredOutput(const VdfOutput &output) const
{
    VdfSchedule::OutputId outputId = _GetSchedule().GetOutputId(output);
    return outputId.IsValid();
}

const VdfMask *
VdfContext::_GetRequestMask(const VdfOutput &output) const
{
    VdfSchedule::OutputId outputId = _GetSchedule().GetOutputId(output);
    // If the output is not even scheduled, there is no request mask.
    if (outputId.IsValid()) {
        return &_GetSchedule().GetRequestMask(outputId);
    }

    return NULL;
}

PXR_NAMESPACE_CLOSE_SCOPE
