//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/maskedSubExecutor.h"

#include "pxr/base/work/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

const EfMaskedSubExecutor::_Factory EfMaskedSubExecutor::_factory;

EfMaskedSubExecutor::EfMaskedSubExecutor(
    const VdfExecutorInterface *parentExecutor)
{
    // A parent executor is required, since calls to GetOutputValue()
    // will be dispatched to the parent.
    if (TF_VERIFY(parentExecutor)) {
        SetParentExecutor(parentExecutor);

        // Inherit the interruption flag from the parent executor.
        SetInterruptionFlag(parentExecutor->GetInterruptionFlag());
    }
}

EfMaskedSubExecutor::~EfMaskedSubExecutor()
{
}

void
EfMaskedSubExecutor::DuplicateOutputData(
    const VdfOutput &sourceOutput,
    const VdfOutput &destOutput)
{
    const _InvalidOutputs::const_iterator it =
        _invalidOutputs.find(sourceOutput.GetId());
    if (it != _invalidOutputs.end()) {
        _invalidOutputs.insert(std::make_pair(destOutput.GetId(), it->second));
    }
}

void
EfMaskedSubExecutor::_Run(
    const VdfSchedule &schedule,
    const VdfRequest &computeRequest,
    VdfExecutorErrorLogger *errorLogger)
{
    // This executor does not allow for running a schedule, since it is
    // dataless. It does, however, support invalidation and dispatches calls
    // to GetOutputValue() to the parent executor, if appropriate.

    TF_CODING_ERROR("Attempt to call Run() on an EfMaskedSubExecutor.");
}

bool
EfMaskedSubExecutor::_IsOutputInvalid(
    const VdfId outputId,
    const VdfMask &invalidationMask) const
{
    if (invalidationMask.IsAllZeros()) {
        return true;
    }

    const _InvalidOutputs::const_iterator it = _invalidOutputs.find(outputId);
    if (it != _invalidOutputs.end()) {
        const VdfMask &invalidMask = it->second;
        return
            invalidMask.GetSize() == invalidationMask.GetSize() &&
            invalidMask.Contains(invalidationMask);
    }

    return false;
}

bool
EfMaskedSubExecutor::_InvalidateOutput(
    const VdfOutput &output,
    const VdfMask &invalidationMask)
{
    // No invalidation to do, if the invalidation mask is empty!
    if (invalidationMask.IsAllZeros()) {
        return false;
    }

    // If this output has already been invalidated before...
    const VdfId outputId = output.GetId();
    const _InvalidOutputs::iterator it = _invalidOutputs.find(outputId);
    if (it != _invalidOutputs.end()) {
        VdfMask &invalidMask = it->second;
        
        // Make sure that the invalid mask is still of the correct size, if
        // not, simply invalidate everything.
        if (invalidMask.GetSize() != invalidationMask.GetSize()) {
            invalidMask = invalidationMask;
            return true;
        }

        // If the data entries in the invalidation mask have already been
        // invalidated on this output, there is no need to further propagate
        // invalidation.
        if (invalidMask.Contains(invalidationMask)) {
            return false;
        }

        // Add the newly invalid entries to the existing invalid mask.
        invalidMask |= invalidationMask;
        return true;
    }

    // This output has never been invalidated, so add a new entry for it
    _invalidOutputs.insert(std::make_pair(outputId, invalidationMask));
    return true;
}

void
EfMaskedSubExecutor::_ClearData()
{
    // This executor does not store any temporary data caches, instead we want
    // to clear out the locally stored invalidation state.
    WorkSwapDestroyAsync(_invalidOutputs);
}

PXR_NAMESPACE_CLOSE_SCOPE
