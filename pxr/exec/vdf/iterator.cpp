//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/iterator.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/error.h"
#include "pxr/exec/vdf/schedule.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

const VdfVector *
VdfIterator::_GetInputValue(
    const VdfContext &context,
    const VdfConnection &connection,
    const VdfMask &mask) const
{
    return _GetExecutor(context)._GetInputValue(connection, mask);
}

const VdfVector &
VdfIterator::_GetRequiredInputValue(
    const VdfContext &context,
    const VdfConnection &connection,
    const VdfMask &mask) const
{
    const VdfVector *value = _GetInputValue(context, connection, mask);
    if (!value) {
        VDF_FATAL_ERROR(connection.GetTargetInput().GetNode(),
                        "No input cache available for " 
                        + connection.GetDebugName() 
                        + ", requested with mask "
                        + mask.GetRLEString());
    }
    
    return *value;
}

const VdfOutput *
VdfIterator::_GetRequiredOutputForWriting(
    const VdfContext &context,
    const TfToken &name) const
{
    const VdfNode &node = _GetNode(context);

    // If no input name has been provided, find the only output on the
    // current node.
    const VdfOutput *output = nullptr;
    if (name.IsEmpty()) {
        output = node.GetOutput();
    }

    // Find the named input on the current node, and use its associated output.
    // If there is no associated output, move on to finding the named output
    // instead.
    else {
        const VdfInput *input = node.GetInput(name);
        output = input && input->GetAssociatedOutput()
            ? input->GetAssociatedOutput()
            : node.GetOptionalOutput(name);
    }

    // Issue a coding error if the output has not been found.
    if (!output) {
        TF_CODING_ERROR("No output available to write to.");
    }

    return output;
}

VdfVector *
VdfIterator::_GetOutputValueForWriting(
    const VdfContext &context,
    const VdfOutput &output) const
{
    return _GetExecutor(context)._GetOutputValueForWriting(output);
}

bool 
VdfIterator::_GetOutputMasks(
    const VdfContext &context,
    const VdfOutput &output,
    const VdfMask **requestMask,
    const VdfMask **affectsMask) const
{
    return context._GetOutputMasks(output, requestMask, affectsMask);
}

bool
VdfIterator::_IsRequiredInput(
    const VdfContext &context,
    const VdfConnection &connection) const
{
    return context._IsRequiredOutput(connection.GetSourceOutput());
}

const VdfMask *
VdfIterator::_GetRequestMask(
    const VdfContext &context,
    const VdfOutput &output) const
{
    return context._GetRequestMask(output);
}

void
VdfIterator::_ForEachScheduledOutput(
    const VdfContext &context,
    const VdfNode &node,
    const VdfScheduledOutputCallback &callback) const
{
    context._GetSchedule().ForEachScheduledOutput(node, callback);
}

PXR_NAMESPACE_CLOSE_SCOPE
