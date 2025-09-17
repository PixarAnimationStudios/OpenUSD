//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/nodeProcessInvalidationInterface.h"
#include "pxr/exec/vdf/node.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

VdfNodeProcessInvalidationInterface::~VdfNodeProcessInvalidationInterface()
{
}

void
VdfNodeProcessInvalidationInterface::ProcessInvalidation(
    const VdfExecutorInterface *executor,
    const VdfNodeToInputPtrVectorMap &inputs)
{
    if (inputs.empty()) {
        return;
    }

    TRACE_FUNCTION();

    // If there is only one node to process invalidation for, do it serially.
    if (inputs.size() == 1) {
        _ProcessInvalidationForEntry(executor, *inputs.begin());
    }

    // If there are multiple nodes to process invalidation for, we can process
    // each node in a separate task for that work to happen in parallel.
    else {
        WorkParallelForEach(
            inputs.begin(), inputs.end(),
            [executor] (const auto &entry) {
                _ProcessInvalidationForEntry(executor, entry);
            });
    }
}

void
VdfNodeProcessInvalidationInterface::ProcessInvalidation(
    const VdfExecutorInterface *executor,
    const VdfNodeProcessInvalidationInterface &node,
    const VdfInputPtrVector &inputs)
{
    TRACE_FUNCTION();

    node._ProcessInvalidation(&inputs, executor);
}

void
VdfNodeProcessInvalidationInterface::_ProcessInvalidationForEntry(
    const VdfExecutorInterface *executor,
    const VdfNodeToInputPtrVectorMap::value_type &entry)
{
    const VdfNodeProcessInvalidationInterface *node =
        dynamic_cast<const VdfNodeProcessInvalidationInterface *>(entry.first);
    if (TF_VERIFY(node)) {
        ProcessInvalidation(executor, *node, entry.second);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
