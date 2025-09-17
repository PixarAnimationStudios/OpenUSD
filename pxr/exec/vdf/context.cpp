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
