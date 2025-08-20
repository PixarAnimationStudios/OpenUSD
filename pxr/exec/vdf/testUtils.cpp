//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace VdfTestUtils 
{

////////////////////////////////////////////////////////////////////////////////
NodeType::~NodeType()
{
}

////////////////////////////////////////////////////////////////////////////////

Node &
Node::operator>>(const _NodeInput &rhs)
{
    _Connect(rhs, _vdfNode->GetOutput());
    // XXX: I think this is wrong, it should return NodeInput's node.
    return *this;
}

Node &
Node::_NodeOutput::operator>>(const _NodeInput &rhs)
{
    owner->_Connect(rhs, owner->_vdfNode->GetOutput(outputName));
    // XXX: I think this is wrong, it should return NodeInput's node.
    return *owner;
}


Node::_NodeInput 
Node::In(const TfToken &inputName, const VdfMask &inputMask)
{
    _NodeInput ret;
    ret.inputName = inputName;
    ret.inputMask = inputMask;
    ret.inputNode = _vdfNode;
    return ret;
}

Node::_NodeOutput 
Node::Output(const TfToken &outputName)
{
    _NodeOutput ret;
    ret.outputName = outputName;
    ret.owner = this;
    return ret;
}

void 
Node::_Connect(const _NodeInput &rhs, VdfOutput *output)
{
    VdfConnection *connection =
        _network->Connect(output, rhs.inputNode, rhs.inputName, rhs.inputMask);
    TF_AXIOM(connection);
}

////////////////////////////////////////////////////////////////////////////////

VdfTestUtils::Network::_EditMonitor::~_EditMonitor() {}

void
VdfTestUtils::Network::_EditMonitor::WillDelete(const VdfNode *node)
{
    // Technically, we don't have to loop through all the nodes here. In the 
    // Add functions, the node's DebugName is set to the same string as the key
    // for _network->_nodes. However, we don't want to rely on DebugName as it 
    // not the same. Unless this is prohibitively slow, we will simply loop 
    // through all nodes to be sure to find the correct one.
    _StringToNodeMap::iterator it = _network->_nodes.begin();
    for (; it != _network->_nodes.end(); ++it) {
        if(node == it->second._vdfNode) {
            _network->_nodes.erase(it);
            break;
        }
    }
}

void
VdfTestUtils::Network::_EditMonitor::WillClear()
{
    _network->_nodes.clear();
}

////////////////////////////////////////////////////////////////////////////////

void
Network::Add(const std::string &nodeName, const NodeType &nodeType)
{
    Node node;

    node._network = &_network;
    node._vdfNode = nodeType.NewNode(&_network);
    node._vdfNode->SetDebugName(nodeName);

    // Keep in mind this internal mapping of nodes uses the explicitly supplied
    // string "nodeName", but the node debug name registered with the network
    // is prefixed with the node type name. 
    //
    // This means _nodes[nodeName]._vdfNode->GetDebugName() != nodeName
    //
    _nodes[nodeName] = node;
}

void 
Network::Add(const std::string &nodeName, VdfNode *customNode)
{
    Node node;

    node._network = &_network;
    node._vdfNode = customNode;
    node._vdfNode->SetDebugName(nodeName);

    _nodes[nodeName] = node;
}

Node &
Network::operator[](const std::string &nodeName)
{
    _StringToNodeMap::iterator i = _nodes.find(nodeName);
    if (i != _nodes.end()) {
        return i->second;
    } else {
        TF_FATAL_ERROR("Node '" + nodeName + "' not found.");
        return _nodes[nodeName];
    }
}

const Node &
Network::operator[](const std::string &nodeName) const
{
    return const_cast<Network *>(this)->operator[](nodeName);
}

// To stay consistent with the node name that is associated with a 
// VdfTestUtils::Node, we loop through all _network->_nodes to be sure to find
// the correct key.
//
// If this is prohibitively slow, we can consider adding a map from VdfId -> 
// VdfTestUtils::Node.
//
const std::string
Network::GetNodeName(const VdfId nodeId)
{
    for (const auto &n : _nodes) {
        if (n.second._vdfNode->GetId() == nodeId) {
            return n.first;
        }
    }
    return NULL;
}

VdfConnection *
Network::GetConnection(const std::string &connectionName)
{
    std::vector<std::string> components =
        TfStringTokenize(connectionName, " ");

    if (components.size() == 3) {

        std::vector<std::string> src = TfStringTokenize(components[0], ":");
        std::vector<std::string> tgt = TfStringTokenize(components[2], ":");

        if (src.size() < 1 || tgt.size() < 1)
            return NULL;

        // If the source or target have been deleted, this connection is no
        // longer valid and is treated as though it doesn't exist.
        if (_nodes.find(src[0]) == _nodes.end() || 
            _nodes.find(tgt[0]) == _nodes.end())
            return NULL;

        VdfNode *srcNode = (*this)[src[0].c_str()];
        VdfNode *tgtNode = (*this)[tgt[0].c_str()];

        if (!srcNode || !tgtNode)
            return NULL;

        // If there is no connector name specified, use first available one.
        if (src.size() == 1) {
            const VdfOutputSpecs &specs = srcNode->GetOutputSpecs();

            if (specs.GetSize() >= 1)
                src.push_back(specs.GetOutputSpec(0)->GetName().GetString());
        }

        if (src.size() != 2)
            return NULL;

        // If there is no connector name specified, use first available one.
        if (tgt.size() == 1) {
            const VdfInputSpecs &specs = srcNode->GetInputSpecs();

            if (specs.GetSize() >= 1)
                tgt.push_back(specs.GetInputSpec(0)->GetName().GetString());
        }

        if (tgt.size() != 2)
            return NULL;

        if (VdfInput *tgtInput = tgtNode->GetInput(TfToken(tgt[1]))) {

            for(size_t i=0; i<tgtInput->GetNumConnections(); i++) {

                VdfConnection &c = tgtInput->GetNonConstConnection(i);

                if (&c.GetSourceNode() == srcNode)
                    if (c.GetSourceOutput().GetName() == src[1])
                        return &c;
            }
        }
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////

ExecutionStatsProcessor::~ExecutionStatsProcessor()
{
    for (size_t i = 0; i < subStats.size(); ++i) {
        delete subStats[i];
    }

    subStats.clear();
}

void
ExecutionStatsProcessor::_ProcessEvent(
    VdfExecutionStatsProcessor::ThreadId threadId,
    const VdfExecutionStats::Event& event)
{
    events[threadId].push_back(event);
}

void
ExecutionStatsProcessor::_ProcessSubStat(const VdfExecutionStats* stats) 
{
    ExecutionStatsProcessor* p = new ExecutionStatsProcessor();
    p->Process(stats);
    subStats.push_back(p);
}

////////////////////////////////////////////////////////////////////////////////

void
ExecutionStats::Log(
    VdfExecutionStats::EventType event,
    VdfId nodeId, 
    uint64_t data)
{
    _stats->Log(event, nodeId, data);
}

void
ExecutionStats::LogBegin(
    VdfExecutionStats::EventType event,
    VdfId nodeId,
    uint64_t data)
{
    _stats->LogBegin(event, nodeId, data);
}

void ExecutionStats::LogEnd(
    VdfExecutionStats::EventType event,
    VdfId nodeId, 
    uint64_t data)
{
    _stats->LogEnd(event, nodeId, data);
}

void
ExecutionStats::GetProcessedStats(VdfExecutionStatsProcessor* processor) const
{
    processor->Process(_stats.get());
}

void
ExecutionStats::AddSubStat(VdfId nodeId)
{
    _stats->AddSubStat(nodeId);
}

void
ExecutionStats::_ExecutionStats::Log(
    VdfExecutionStats::EventType event,
    VdfId nodeId, 
    uint64_t data)
{
    _Log(event, nodeId, data);
}

void 
ExecutionStats::_ExecutionStats::LogBegin(
    VdfExecutionStats::EventType event,
    VdfId nodeId,
    uint64_t data)
{
    Log(_TagBegin(event), nodeId, data);
}

void
ExecutionStats::_ExecutionStats::LogEnd(
    VdfExecutionStats::EventType event,
    VdfId nodeId,
    uint64_t data)
{
    Log(_TagEnd(event), nodeId, data);
}

void
ExecutionStats::_ExecutionStats::AddSubStat(VdfId nodeId)
{
    _AddSubStat(&_network, nodeId);
}
} // namespace VdfTestUtils

PXR_NAMESPACE_CLOSE_SCOPE
