//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/executionStats.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

VdfExecutionStats::ScopedEvent::ScopedEvent(
    VdfExecutionStats* stats,
    const VdfNode& node,
    EventType event) :
    _stats(NULL)
{
    if (stats) {
        _stats = stats;
        _node = &node;
        _event = event;
        _stats->LogBeginTimestamp(_event, *_node);
    }
}

VdfExecutionStats::ScopedEvent::~ScopedEvent()
{
    if (_stats) {
        _stats->LogEndTimestamp(_event, *_node);
    }
}

VdfExecutionStats::ScopedMallocEvent::ScopedMallocEvent(
    VdfExecutionStats* stats,
    const VdfNode& node,
    EventType eventType) : 
    VdfExecutionStats::ScopedEvent(stats, node, eventType)
{
    if (_stats && TfMallocTag::IsInitialized()) {
        const std::optional<VdfId> &invokingNodeId = _stats->_invokingNodeId;
        _tagName = VdfExecutionStats::GetMallocTagName(
            invokingNodeId ? &(*invokingNodeId) : nullptr, node);
        TfMallocTag::Push(_tagName);
    }
}

VdfExecutionStats::ScopedMallocEvent::~ScopedMallocEvent()
{
    if (_stats && TfMallocTag::IsInitialized()) {
        TfMallocTag::Pop();
    }
}

VdfExecutionStats::VdfExecutionStats(const VdfNetwork* network) : 
    _network(network)
{
    // Do nothing
}

VdfExecutionStats::VdfExecutionStats(const VdfNetwork* network, VdfId nodeId) :
    _network(network),
    _invokingNodeId(nodeId)
{
    // Do nothing
}

VdfExecutionStats::~VdfExecutionStats()
{
    // Delete sub stats
    VdfExecutionStats* stats;
    while (_subStats.try_pop(stats)) {
        delete stats;
    }
}

VdfExecutionStats*
VdfExecutionStats::AddSubStat(const VdfNetwork* network, const VdfNode* node)
{
    return _AddSubStat(network, node->GetId());
}

VdfExecutionStats*
VdfExecutionStats::_AddSubStat(const VdfNetwork* network, VdfId invokingNodeId) 
{
    VdfExecutionStats* child = new VdfExecutionStats(network, invokingNodeId);
    _subStats.push(child);
    return child;
}

std::string
VdfExecutionStats::GetMallocTagName(
    const VdfId *invokingNodeId,
    const VdfNode &node)
{
    return !invokingNodeId
        ? TfStringPrintf("%p n %lx", &node.GetNetwork(), node.GetId())
        : TfStringPrintf(
            "%p %lx %lx", &node.GetNetwork(), *invokingNodeId, node.GetId());
}

PXR_NAMESPACE_CLOSE_SCOPE
