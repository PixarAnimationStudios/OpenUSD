//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/poolChainIndexer.h"
#include "pxr/exec/vdf/input.h"
#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/output.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

VdfPoolChainIndexer::VdfPoolChainIndexer() = default;

VdfPoolChainIndexer::~VdfPoolChainIndexer() = default;

VdfPoolChainIndex
VdfPoolChainIndexer::GetIndex(const VdfOutput &output) const
{
    if (const int priority = _sorter.GetPriority(&output);
        priority != _PoolOutputSorter::InvalidPriority) {
        return VdfPoolChainIndex(priority, 0);
    }

    // GetIndex should only be called for a pool output
    if (!TF_VERIFY(Vdf_IsPoolOutput(output))) {
        return VdfPoolChainIndex(_PoolOutputSorter::InvalidPriority, 0);
    }

    const VdfIndex outputIndex = VdfOutput::GetIndexFromId(output.GetId());
    return VdfPoolChainIndex(_PoolOutputSorter::LastPriority, outputIndex);
}


/***************************************************************************
 *
 * Note that if there is a parallel mover, the pool outputs do not form a 
 * linear chain. Ignoring the (non-pool) inputs ".childvalues", the
 * pool outputs form a tree branching downstream:
 *
 *    --------------------
 *    |        A         |
 *    --------------------         
 *          /  |  \      \
 *         /   |   \      \ 
 *    -----  -----  -----  |
 *    | B |  | C |  | D |  |
 *    -----  -----  -----  |
 *      .      .      .    |
 *      .......+.......    |
 *             . +---------+
 *             . |
 *           -------
 *           |  E  |
 *           -------
 *
 *  This diagram shows E as a parallel mover and B, C, and D are child
 *  actions.  The connections to the non-pool input ".childvalues",
 *  represented by dots, of E are from B, C, and D.
 *
 *  In this case, the pool chain indexer will make no guarantees about the
 *  order of the indices of B, C and D. Only that they all appear after A
 *  and before E.
 *
 **************************************************************************/

namespace
{
    // A pair of outputs.  The source output is connected to an input
    // whose associated output is the target output.
    struct _PoolConnectedOutputs
    {
        _PoolConnectedOutputs()
            : source(NULL)
            , target(NULL)
        {}

        _PoolConnectedOutputs(
            const VdfOutput *source,
            const VdfOutput *target)
            : source(source)
            , target(target)
        {}

        bool IsValid() const
        {
            return source && target;
        }

        const VdfOutput *source;
        const VdfOutput *target;
    };
}

// Returns a pair of pool outputs if the connection connects to the pool at
// the source output side and the input's node also has a pool output.
static _PoolConnectedOutputs
_MakePoolConnectedOutput(const VdfConnection &connection)
{
    const VdfOutput &source = connection.GetSourceOutput();
    if (!Vdf_IsPoolOutput(source)) {
        return _PoolConnectedOutputs();
    }

    // This function gets called for every connection and disconnection, which
    // results in too much trace overhead.  However, we'd like to keep an eye
    // on the cost of finding the pool output.  The majority of calls to this
    // function, which are not on pool connections, will exit on one of the
    // conditions checked above.
    TRACE_FUNCTION_SCOPE("checking optional output");

    // If the target node has a pool output at all, consider this to
    // be a pool connection for the purpose of indexing.
    const VdfNode &targetNode = connection.GetTargetInput().GetNode();
    for (const auto &[name, output] : targetNode.GetOutputsIterator()) {
        if (Vdf_IsPoolOutput(*output)) {
            return _PoolConnectedOutputs(&source, output);
        }
    }

    return _PoolConnectedOutputs();
}

void
VdfPoolChainIndexer::Insert(const VdfConnection &connection)
{
    _PoolConnectedOutputs poolOutputs = _MakePoolConnectedOutput(connection);
    if (!poolOutputs.IsValid()) {
        // Ignore connections that don't create an order between two pool
        // outputs.
        return;
    }

    // Trace only after the pool-connected check because it used to live
    // outside of the indexer and we'd like to make Insert/Remove time
    // comparable to the old indexer's Compute times.
    TRACE_FUNCTION();

    // If we make connections between nodes with pool outputs in parallel, we
    // will quickly contend on this big lock. We should find a way to have more
    // fine-grained locking on adding/removing edges on the topological sorter.
    std::lock_guard<std::mutex> lock(_sorterMutex);
    _sorter.AddEdge(poolOutputs.source, poolOutputs.target);
}

void
VdfPoolChainIndexer::Remove(const VdfConnection &connection)
{
    _PoolConnectedOutputs poolOutputs = _MakePoolConnectedOutput(connection);
    if (!poolOutputs.IsValid()) {
        return;
    }

    TRACE_FUNCTION();

    std::lock_guard<std::mutex> lock(_sorterMutex);
    _sorter.RemoveEdge(poolOutputs.source, poolOutputs.target);
}

void
VdfPoolChainIndexer::Clear()
{
    TRACE_FUNCTION();

    _sorter.Clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
