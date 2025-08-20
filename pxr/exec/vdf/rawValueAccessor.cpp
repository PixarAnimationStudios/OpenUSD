//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/rawValueAccessor.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/input.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/output.h"
#include "pxr/exec/vdf/vector.h"

#include "pxr/base/tf/mallocTag.h"

PXR_NAMESPACE_OPEN_SCOPE

const VdfVector *
VdfRawValueAccessor::GetInputVector(
    const VdfInput &input,
    VdfMask *mask) const
{
    if (input.GetNumConnections() == 0) {
        return NULL;
    }

    const VdfConnection &connection = input[0];

    if (mask) {
        *mask = connection.GetMask();
    }

    // Regardless of where we actually read the input value from, the request
    // mask contains what is requested at the output we are sourcing the
    // value from.
    const VdfMask *requestMask =
        _GetRequestMask(_context, connection.GetSourceOutput());

    // We should never fail this verify. In fact, if this ever fails, it is
    // most definitely an error condition to ask for the cached value of an
    // output, which has never been requested in the current schedule.
    if (!TF_VERIFY(requestMask)) {
        requestMask = &(connection.GetMask());
    }

    return _GetInputValue(_context, connection, *requestMask);
}

void
VdfRawValueAccessor::SetOutputVector(
    const VdfOutput &output,
    const VdfMask &mask,
    const VdfVector &value)
{
    _SetOutputVector(output, mask, value);
}

void
VdfRawValueAccessor::SetOutputVector(
    const VdfOutput &output,
    const VdfMask &mask,
    VdfVector &&value)
{
    _SetOutputVector(output, mask, std::move(value));
}

template <typename Vector>
inline void
VdfRawValueAccessor::_SetOutputVector(
    const VdfOutput &output,
    const VdfMask &mask,
    Vector &&value)
{
    TfAutoMallocTag2 tag("Vdf", "VdfRawValueAccessor::SetOutputVector");

    VdfVector *out = _GetOutputValueForWriting(_context, output);

    if (!TF_VERIFY(out)) {
        return;
    }

    // If the request mask is all ones, copy the vector, otherwise
    // do a sparse copy of only the requested elements.

    if (mask.IsAllOnes()) {
        *out = std::forward<Vector>(value);
    }
    else {
        out->Copy(value, mask);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
