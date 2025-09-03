//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/inputValueBlock.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/stl.h"
#include "pxr/exec/vdf/executorInterface.h"

PXR_NAMESPACE_OPEN_SCOPE

EfInputValueBlock::EfInputValueBlock(const EfInputValueBlock &rhs)
{
    _Append(rhs);
}

EfInputValueBlock::~EfInputValueBlock()
{
    _Clear();
}

EfInputValueBlock &
EfInputValueBlock::operator=(const EfInputValueBlock &rhs)
{
    if (this == &rhs) {
        return *this;
    }

    _Clear();
    _Append(rhs);
    return *this;
}

void EfInputValueBlock::Apply(
    VdfExecutorInterface *executor, 
    VdfMaskedOutputVector *invalidationRequest) const
{
    if (!TF_VERIFY(executor) || _values.empty()) {
        return;
    }

    TRACE_FUNCTION();

    // First, build the invalidation request. If an invalidationRequest output
    // parameter is set, use the instance of invalidationRequest to build the
    // request and return it to the caller.
    VdfMaskedOutputVector tmpRequest;
    VdfMaskedOutputVector *theRequest =
        invalidationRequest ? invalidationRequest : &tmpRequest;
    TF_VERIFY(theRequest->empty());
    TF_FOR_ALL (i, _values) {
        theRequest->push_back(i->first);
    }

    _Invalidate(executor, *theRequest);
    _SetValues(executor);
}

void
EfInputValueBlock::InvalidateAndApply(
    VdfExecutorInterface *executor,
    const VdfMaskedOutputVector &invalidationRequest) const
{
    if (!TF_VERIFY(executor) || _values.empty()) {
        return;
    }

    TRACE_FUNCTION();

    _Invalidate(executor, invalidationRequest);
    _SetValues(executor);
}

void
EfInputValueBlock::_Invalidate(
    VdfExecutorInterface *executor,
    const VdfMaskedOutputVector &invalidationRequest) const
{
    // If the data manager is empty, don't bother making an expensive call
    // to invalidate values which will only setup the correct masks for
    // invalidation -- that'll be done by the first compute anyway.
    if (!invalidationRequest.empty() && !executor->IsEmpty()) {
        executor->InvalidateValues(invalidationRequest);
    }
}

void
EfInputValueBlock::_SetValues(VdfExecutorInterface *executor) const
{
    // Set the outputs in block.  The idea here is that the executors
    // never compute the outputs in block.  This is because we can only poke
    // a single value into them via initialization and thus all executors
    // would compute the same value.
    //
    // In order for the executors see different values, we create their
    // output caches manually here, poke in the desired value and set the 
    // computed output mask.
    TF_FOR_ALL (i, _values) {
        executor->SetOutputValue(
            *(i->first.GetOutput()), *i->second, i->first.GetMask());
    }
}

void 
EfInputValueBlock::_Clear()
{
    TF_FOR_ALL(i, _values) {
        delete i->second;
    }
    TfReset(_values);
}

void
EfInputValueBlock::_Append(const EfInputValueBlock &rhs)
{
    TF_FOR_ALL(i, rhs._values) {
        _values.push_back(
            std::make_pair(i->first, new VdfVector(*(i->second))));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
