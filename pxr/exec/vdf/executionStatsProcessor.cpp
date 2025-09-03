//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/executionStatsProcessor.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

VdfExecutionStatsProcessor::VdfExecutionStatsProcessor() :
    _network(nullptr)
{
    // Do nothing
}

VdfExecutionStatsProcessor::~VdfExecutionStatsProcessor()
{
    // Do nothing
}

void
VdfExecutionStatsProcessor::Process(const VdfExecutionStats* stats)
{
    if (!stats) {
        return;
    }

    // If processing multiple stats subsequently, we expect the network to be
    // the same.
    TF_VERIFY(!_network || _network == stats->_network);
    TF_VERIFY(!_invokingNodeId || 
        VdfNode::GetIndexFromId(*_invokingNodeId) == 
        VdfNode::GetIndexFromId(*stats->_invokingNodeId));

    _network = stats->_network;
    _invokingNodeId = stats->_invokingNodeId;

    _PreProcess();
    _ProcessEvents(stats);
    _ProcessSubStats(stats);
    _PostProcess();
}

const VdfNetwork* 
VdfExecutionStatsProcessor::GetNetwork() const
{
    return _network;
}

void
VdfExecutionStatsProcessor::_ProcessEvents(const VdfExecutionStats* stats)
{
    for (const VdfExecutionStats::_PerThreadEvents &thread : stats->_events) {
        for (const VdfExecutionStats::Event &event : thread.events) {
            _ProcessEvent(thread.threadId, event);
        }
    }
}

void
VdfExecutionStatsProcessor::_ProcessSubStats(
    const VdfExecutionStats* stats)
{
    tbb::concurrent_queue<VdfExecutionStats*>::const_iterator it =
        stats->_subStats.unsafe_begin();
    for (; it != stats->_subStats.unsafe_end(); ++it) {
        _ProcessSubStat(*it);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
