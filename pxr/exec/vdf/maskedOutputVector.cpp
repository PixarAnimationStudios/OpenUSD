//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "maskedOutputVector.h"

#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/output.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/work/sort.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Sorting predicate for VdfMaskedOutputVectors by output indices.
//
// Note that dereferecing the output pointer in order to get at the output index
// is expensive. It would be cheaper to sort by pointer address only. However,
// most of the evaluation data structures are indexed by output indicies, so we
// do gain speed from better cache locality later on. We can reexamine this
// tradeoff in the future.
struct VdfMaskedOutputVector_SortPredicate
{
    bool operator()(
        const VdfMaskedOutput &lhs, 
        const VdfMaskedOutput &rhs) const
    {
        return
            VdfOutput::GetIndexFromId(lhs.GetOutput()->GetId()) <
            VdfOutput::GetIndexFromId(rhs.GetOutput()->GetId());
    }
};

} // anonymous namespace

void
VdfSortAndUniqueMaskedOutputVector(VdfMaskedOutputVector* vector)
{
    if (vector->size() <= 1) {
        return;
    }

    TRACE_FUNCTION();
         
    WorkParallelSort(vector, VdfMaskedOutputVector_SortPredicate());
    vector->erase(
        std::unique(vector->begin(), vector->end()),
        vector->end());
}

const VdfNetwork*
VdfGetMaskedOutputVectorNetwork(const VdfMaskedOutputVector& vector)
{
    return !vector.empty() ? 
        &vector.front().GetOutput()->GetNode().GetNetwork() :
        nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
