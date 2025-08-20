//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/allocateBoxedValue.h"

#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/input.h"
#include "pxr/exec/vdf/iterator.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/output.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// An accessor class for retrieving the current node, request mask, and
// output value.
struct _ContextAccess final : public VdfIterator {
    // Returns the request mask for the given output.
    //
    const VdfMask *GetRequestMask(
        const VdfContext &context,
        const VdfOutput &output) {
        return _GetRequestMask(context, output);
    }

    // Returns the required output to write to.
    //
    const VdfOutput *GetRequiredOutputForWriting(
        const VdfContext &context,
        const TfToken &name) {
        return _GetRequiredOutputForWriting(context, name);
    }

    // Returns the output value to write into.
    //
    VdfVector *GetOutputValueForWriting(
        const VdfContext &context,
        const VdfOutput &output) {
        return _GetOutputValueForWriting(context, output);
    }
};

}

VdfVector *
Vdf_AllocateBoxedValueVector(const VdfContext &context, const TfToken &name)
{
    // Get the required output and issue a coding error if it is not available.
    // We expect the required output to always be available.
    const VdfOutput *output =
        _ContextAccess().GetRequiredOutputForWriting(context, name);
    if (!output) {
        return nullptr;
    }

    // Retrieve the request mask at the output. If the output is not requested,
    // we can bail out. This is not an error, but we do not need to allocate
    // anything in this case.
    const VdfMask *requestMask =
        _ContextAccess().GetRequestMask(context, *output);
    if (!requestMask || requestMask->IsAllZeros()) {
        return nullptr;
    }

    // We expect the request mask to be of size 1, meaning we expect the output
    // to always store a scalar or boxed value. We cannot allocate boxed value
    // storage for an output that carries a vectorized value.
    if (requestMask->GetSize() != 1) {
        TF_CODING_ERROR(
            "Output '%s' cannot hold a boxed value.",
            output->GetName().GetText());
        return nullptr;
    }

    // Return the vector to write into.
    return _ContextAccess().GetOutputValueForWriting(context, *output);
}

PXR_NAMESPACE_CLOSE_SCOPE
