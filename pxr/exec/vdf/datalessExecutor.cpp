//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/datalessExecutor.h"

PXR_NAMESPACE_OPEN_SCOPE

VdfDatalessExecutor::VdfDatalessExecutor()
{

}

VdfDatalessExecutor::~VdfDatalessExecutor()
{

}

void
VdfDatalessExecutor::SetOutputValue(
    const VdfOutput &output,
    const VdfVector &value,
    const VdfMask &mask)
{
    TF_CODING_ERROR(
        "Attempt to call SetOutputValue() on a dataless executor.");
}

bool
VdfDatalessExecutor::TakeOutputValue(
    const VdfOutput &output,
    VdfVector *value,
    const VdfMask &mask)
{
    TF_CODING_ERROR(
        "Attempt to call TakeOutputValue() on a dataless executor.");
    return false;
}

void
VdfDatalessExecutor::_SetReferenceOutputValue(
    const VdfOutput &destOutput,
    const VdfOutput &sourceOutput,
    const VdfMask &sourceMask) const
{
    TF_CODING_ERROR(
        "Attempt to call _SetReferenceOutputValue() on a dataless executor.");
}

PXR_NAMESPACE_CLOSE_SCOPE
