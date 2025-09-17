//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/networkStats.h"

#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"

PXR_NAMESPACE_OPEN_SCOPE

// Record a maximum for a value, recording the corresponding node, 
// if there's a unique name.
template <typename T>
static void
_RecordMax(T stat, T *max, const VdfNode *node, const VdfNode **maxNode)
{
    TF_AXIOM(max);
    TF_AXIOM(node);
    TF_AXIOM(maxNode);

    if (stat == *max) {
        // If we encounter the same stat more than once, don't report
        // a max node, unless the debug name is the same.
        if (!*maxNode ||
            ( node->GetDebugName() != (*maxNode)->GetDebugName() ) ) {
            *maxNode = NULL;
        }
    }
    else if (stat > *max) {
        *max = stat;
        *maxNode = node;
    }
}


VdfNetworkStats::VdfNetworkStats(const VdfNetwork &network)
{
    // Determine the maxium type name length.
    _maxTypeNameLength = 0;

    // Determine the maximum fan in and fan out.
    _maxFanIn = 0;
    _maxFanOut = 0;

    const VdfNode *maxFanInNode = NULL;
    const VdfNode *maxFanOutNode = NULL;

    // Count the number of instances of each node type in the network.
    size_t numNodes = network.GetNodeCapacity();
    for (size_t i = 0; i < numNodes; ++i) {
        const VdfNode *node = network.GetNode(i);
        if (!node)
            continue;

        std::string typeName = ArchGetDemangled(typeid(*node));
        _TypeStatsMap::iterator iter = _statsMap.find(typeName);
        if (iter != _statsMap.end()) {
            iter->second.count++;
            iter->second.memUsage += node->GetMemoryUsage();
        } else {
            _statsMap[typeName].count = 1;
            _statsMap[typeName].memUsage = node->GetMemoryUsage();
        }

        if (typeName.length() > _maxTypeNameLength) {
            _maxTypeNameLength = typeName.length();
        }

        TF_FOR_ALL( i, node->GetInputsIterator() ) {
            _RecordMax(i->second->GetNumConnections(), &_maxFanIn,
                       node, &maxFanInNode);
        }

        TF_FOR_ALL( i, node->GetOutputsIterator() ) {
            _RecordMax<size_t>(i->second->GetConnections().size(), &_maxFanOut,
                       node, &maxFanOutNode);
        }
    }

    _maxFanInNodeName = maxFanInNode ? maxFanInNode->GetDebugName() : "NULL";
    _maxFanOutNodeName = maxFanOutNode ? maxFanOutNode->GetDebugName() : "NULL";

}

PXR_NAMESPACE_CLOSE_SCOPE
