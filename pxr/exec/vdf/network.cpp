//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/network.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/execNodeDebugName.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"
#include "pxr/exec/vdf/input.h"
#include "pxr/exec/vdf/inputAndOutputSpecsRegistry.h"
#include "pxr/exec/vdf/networkStats.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/object.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduleInvalidator.h"
#include "pxr/exec/vdf/poolChainIndex.h"
#include "pxr/exec/vdf/poolChainIndexer.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/trace/trace.h"

#include <algorithm>
#include <memory>
#include <ostream>
#include <sstream>
#include <typeinfo>

PXR_NAMESPACE_OPEN_SCOPE

/* virtual */
VdfNetwork::EditMonitor::~EditMonitor()
{
}

VdfNetwork::VdfNetwork() :
    _initialNodeVersion(0),
    _outputCapacity(0),
    _scheduleInvalidator(std::make_unique<Vdf_ScheduleInvalidator>()),
    _poolChainIndexer(std::make_unique<VdfPoolChainIndexer>()),
    _specsRegistry(std::make_unique<Vdf_InputAndOutputSpecsRegistry>()),
    _version(0)
{
    // We do this to force registration of all execution types in Vdf
    // as soon as the first network is constructed. Doing so populates the
    // various type info tables in Vdf, such as the output spec type info table,
    // which will be accessed every time a VdfOutputSpecis created.
    // 
    // Note, we should change this so that random type info tables are not
    // spread throughout Vdf. Instead, tables should be owned by the registry
    // such that first access of the table through the registry will take
    // care of the population.
    VdfExecutionTypeRegistry::GetInstance();
}

VdfNetwork::~VdfNetwork()
{
    Clear();
}

const std::string 
VdfNetwork::GetNodeDebugName(const VdfNode *node) const
{
    const VdfIndex index = VdfNode::GetIndexFromId(node->GetId());
    const _NodeDebugNamesMap::const_iterator it = _nodeDebugNames.find(index);

    // Even if we find an entry for this node, we need to make sure to check
    // that the pointer is valid, in case this is a tombstoned (previously
    // unregistered) entry for a node with the same index.
    if (it != _nodeDebugNames.end() && it->second) {
        return it->second->_GetDebugName();
    }

    return ArchGetDemangled(typeid(*node));
}

void 
VdfNetwork::_RegisterNodeDebugName(
    const VdfNode &node, 
    VdfNodeDebugNameCallback &&callback)
{
    if (TF_VERIFY(callback, "Null callback for node: %s", 
                  ArchGetDemangled(typeid(node)).c_str())) {
        const VdfIndex index = VdfNode::GetIndexFromId(node.GetId());
        _nodeDebugNames[index] =
            std::make_unique<Vdf_ExecNodeDebugName>(node, std::move(callback));
    }
}

void
VdfNetwork::_UnregisterNodeDebugName(const VdfNode &node)
{
    // Erasure from a concurrent map is not thread safe, so let's reset
    // the pointer to the node debug name struct instead.
    // Note that while it's safe to unregister multiple node debug names
    // concurrently, it is not safe to unregister the debug name for the
    // *same* node from multiple threads. Thus, we do not need to synchronize
    // on the call to reset().
    const VdfIndex index = VdfNode::GetIndexFromId(node.GetId());
    const _NodeDebugNamesMap::iterator it = _nodeDebugNames.find(index);
    if (it != _nodeDebugNames.end()) {
        it->second.reset();
    }
}

const VdfNode *
VdfNetwork::GetNodeById(const VdfId nodeId) const
{
    const VdfNode *node = GetNode(VdfNode::GetIndexFromId(nodeId));
    return node && node->GetId() == nodeId ? node : nullptr;
}

VdfNode *
VdfNetwork::GetNodeById(const VdfId nodeId)
{
    VdfNode *node = GetNode(VdfNode::GetIndexFromId(nodeId));
    return node && node->GetId() == nodeId ? node : nullptr;
}

void
VdfNetwork::Clear()
{
    TRACE_FUNCTION();

    // Update the edit version.
    _IncrementVersion();

    // Notify all monitors.
    for (EditMonitor * const monitor : _monitors) {
        monitor->WillClear();
    }

    // Delete all nodes in the network.
    // Find the maximum node version, increment it, and use it as the initial
    // node version for all new nodes.
    VdfVersion maxVersion = 0;
    for (VdfNode *node : _nodes) {
        if (node) {
            maxVersion = std::max(
                maxVersion, VdfNode::GetVersionFromId(node->GetId()));
            delete node;
        }
    }
    _nodes.clear();
    _freeNodeIds.clear();
    _initialNodeVersion = maxVersion + 1;
    
    // Debug names will have been unregistered along with node deletion, so
    // we can now clear the debug names map.
    _nodeDebugNames.clear();

    _scheduleInvalidator->InvalidateAll();
    _poolChainIndexer->Clear();
}

void
VdfNetwork::_AddNode(VdfNode *node)
{
    // Re-use an existing node index if we find one on the free list. If the
    // free list is empty, generate a new index by pushing the node to the end
    // of the array.
    VdfId freeId;
    if (_freeNodeIds.try_pop(freeId)) {
        const VdfVersion version = VdfNode::GetVersionFromId(freeId) + 1;
        const VdfIndex index = VdfNode::GetIndexFromId(freeId);

        TF_VERIFY(!_nodes[index]);
        _nodes[index] = node;
  
        node->_SetId(version, index);
    }

    else {
        const auto it = _nodes.push_back(node);

        node->_SetId(_initialNodeVersion, std::distance(_nodes.begin(), it));
    }

    // Update the edit version.
    _IncrementVersion();

    // Notify all monitors.
    for (EditMonitor * const monitor : _monitors) {
        monitor->DidAddNode(node);
    }

    // Note: We don't need to clear any schedules since the new node can't
    //       be referenced by any schedule.
}

VdfConnection *
VdfNetwork::Connect(
    VdfOutput     *output,
    VdfNode       *inputNode,
    const TfToken &inputName,
    const VdfMask &mask,
    int            atIndex)
{
    if (!TF_VERIFY(output) || !TF_VERIFY(inputNode)) {
        return nullptr;
    }

    // Make sure we don't connect a node's output directly to a node's input.
    if (!TF_VERIFY(
        inputNode->IsSpeculationNode() || &output->GetNode() != inputNode,
        "Can't connect '%s' to node '%s [%s]': creates cycle.",
        output->GetDebugName().c_str(),
        inputNode->GetDebugName().c_str(),
        inputName.GetText())) {
        return nullptr;
    }

    // When connecting to a read/write output, make sure that the data flowing
    // into the associated input contains the data flowing accross the new
    // connection, as determined by the specified mask.
    //
    // Note, that if the code below is causing performance issues, we can wrap
    // this section to only execute in dev builds.
    if (const VdfInput *associated = output->GetAssociatedInput()) {
        const VdfConnection *connection = associated->GetNumConnections() == 1
            ? &((*associated)[0])
            : nullptr;
        if (connection && connection->GetMask().GetSize() == mask.GetSize()) {
            TF_VERIFY(connection->GetMask().Contains(mask));
        }
    }

    VdfConnection *connection =
        output->_Connect(inputNode, inputName, mask, atIndex);

    // Only handle a new connection if we successfully made the connection
    // (a coding error will be issued otherwise).
    if (connection) {

        // Update the pool chain indexer if the new connection
        // involves a pool output.
        _poolChainIndexer->Insert(*connection);

        // Notify the node that an input connection changed.
        inputNode->_DidAddInputConnection(connection, atIndex);

        // Update the edit version.
        _IncrementVersion();

        // Notify all edit monitors that a connection was made.
        for (EditMonitor * const monitor : _monitors) {
            monitor->DidConnect(connection);
        }

        // Invalidate all schedules that contain the target node.
        _scheduleInvalidator->UpdateForConnectionChange(connection);
    }

    return connection;
}

VdfConnection *
VdfNetwork::Connect(
    const VdfMaskedOutput &maskedOutput,
    VdfNode               *inputNode,
    const TfToken         &inputName,
    int                    atIndex)
{
    return Connect(
        maskedOutput.GetOutput(),
        inputNode,
        inputName,
        maskedOutput.GetMask(),
        atIndex);
}

bool
VdfNetwork::DisconnectAndDelete(VdfNode *node)
{
    VdfConnectionVector connections;

    // Disconnect inputs and outputs. Need to be careful to make a copy of
    // the connections vector, since operating directly on the input/outputs
    // may crash since Disconnect(); will modify them.

    for (const auto &[name, output] : node->GetOutputsIterator()) {
        for (const auto &connection : output->GetConnections()) {
            connections.push_back(connection);
        }
    }

    for (const auto &[name, input] : node->GetInputsIterator()) {
        for (const auto &connection : input->GetConnections()) {
            connections.push_back(connection);
        }
    }

    for (VdfConnection *connection : connections) {
        Disconnect(connection);
    }

    return Delete(node);

}

void
VdfNetwork::_DeleteNode(VdfNode *node)
{
    if (!TF_VERIFY(
            node && 
            !node->HasInputConnections() && 
            !node->HasOutputConnections()))
        return;

    delete node;
}

bool
VdfNetwork::Delete(VdfNode *node)
{
    if (node->HasInputConnections()) {

        TF_CODING_ERROR(
            "Attempt to delete a VdfNode that has input connections: %s",
            node->GetDebugName().c_str());
        return false;
    }

    if (node->HasOutputConnections()) {

        TF_CODING_ERROR(
            "Attempt to delete a VdfNode that has output connections: %s",
            node->GetDebugName().c_str());
        return false;
    }

    // Remove the node from the network.
    _RemoveNode(node);

    // Delete the node from the network.
    _DeleteNode(node);

    return true;
}

void
VdfNetwork::_RemoveNode(VdfNode *node)
{
    TF_VERIFY(node);

    // Get the old index.
    const VdfId nodeId = node->GetId();
    const VdfIndex index = VdfNode::GetIndexFromId(nodeId);

    TF_VERIFY(index < _nodes.size());
    TF_VERIFY(_nodes[index] == node);

    // Clear schedules that contain this node, update others.
    _scheduleInvalidator->InvalidateContainingNode(node);

    // Update the edit version.
    _IncrementVersion();

    // Notify any monitor. Note that we only notify for nodes that actually
    // have been inserted in the network before. 
    for (EditMonitor * const m : _monitors) {
        m->WillDelete(node);
    }

    // Mark this node as already removed from the network.
    _nodes[index] = NULL;

    // Add this node's id to the list of ids that we can now re-use.
    _freeNodeIds.push(nodeId);
}

void
VdfNetwork::_DidChangeAffectsMask(VdfOutput &output)
{
    _scheduleInvalidator->UpdateForAffectsMaskChange(&output);
}

void
VdfNetwork::Disconnect(
    VdfConnection *connection)
{
    // Remove the connection from the network, sending out any notifications,
    // as well as invalidating dependent state, then delete.
    _RemoveConnection(connection);
    _DeleteConnection(connection);
}

void
VdfNetwork::_RemoveConnection(VdfConnection *connection)
{
    // Update the edit version.
    _IncrementVersion();

    // Notify monitors.
    for (EditMonitor * const m : _monitors) {
        m->WillDelete(connection);
    }

    // Update pool chain indexer if the disconnected connection
    // involves a pool output.
    _poolChainIndexer->Remove(*connection);

    // We only notify the schedule invalidator on the initial disconnect.
    _scheduleInvalidator->UpdateForConnectionChange(connection);

    // Notify the target node that an input connection changed before it is
    // removed from any input/output.
    connection->GetTargetNode()._WillRemoveInputConnection(connection);

    // Remove the connection from the endpoints.
    connection->GetTargetInput()._RemoveConnection(connection);
    connection->GetSourceOutput()._RemoveConnection(connection);
}

void
VdfNetwork::_DeleteConnection(VdfConnection *connection)
{
    delete connection;
}

void
VdfNetwork::ReorderInputConnections(
    VdfInput *const input,
    const TfSpan<const VdfConnectionVector::size_type> &newToOldIndices)
{
    TRACE_FUNCTION();
    input->_ReorderInputConnections(newToOldIndices);
}

void
VdfNetwork::RegisterEditMonitor(EditMonitor *monitor)
{
    if (std::find(_monitors.begin(), _monitors.end(), monitor) !=
        _monitors.end()) {
        TF_CODING_ERROR("EditMonitor %p registered multiple times.", &monitor);
        return;
    }

    _monitors.push_back(monitor);
}

void
VdfNetwork::UnregisterEditMonitor(EditMonitor *monitor)
{
    _EditMonitorVector::iterator iter =
        std::find(_monitors.begin(), _monitors.end(), monitor);

    if (iter == _monitors.end()) {
        TF_CODING_ERROR("EditMonitor %p not registered.", &monitor);
        return;
    }

    _monitors.erase(iter);
}

size_t
VdfNetwork::_AcquireOutputId()
{
    // If there are no output ids on the free list, return the value of the
    // capacity before incrementing it.
    VdfId freeId;
    if (!_freeOutputIds.try_pop(freeId)) {
        return _outputCapacity++;
    }

    // We were able to grab an output id from the free list.
    // Extract the version and index from the id, and make sure to increment
    // the version. This will prevent aliasing outputs with the same index.
    const VdfVersion version = VdfOutput::GetVersionFromId(freeId) + 1;
    const VdfIndex index = VdfOutput::GetIndexFromId(freeId);

    // Construct the id from the newly incremented version, and the index.
    return (static_cast<VdfId>(version) << 32) | static_cast<VdfId>(index);
}

void
VdfNetwork::_ReleaseOutputId(const VdfId id)
{
    _freeOutputIds.push(id);
}

void
VdfNetwork::_IncrementVersion()
{
    ++_version;
}

void
VdfNetwork::_RegisterSchedule(VdfSchedule *schedule) const
{
    _scheduleInvalidator->Register(schedule);
}

void
VdfNetwork::_UnregisterSchedule(VdfSchedule *schedule) const
{
    _scheduleInvalidator->Unregister(schedule);
}

// Print a labeled value, using fancy justification.
template <typename T>
static void
_PrintLabeledValue(std::ostream &os, 
                   const std::string &label, T value, int width)
{
    int extraWidth = width - label.length();
    std::ostringstream tmp;
    tmp << label << ":  ";
    tmp.width(extraWidth + 10);
    tmp.setf(std::ios::right);
    tmp << value;
    os << tmp.str() << std::endl;
}

static void
_PrintLabeledValue(std::ostream &os,
                   const std::string &inLabel,
                   VdfNetworkStats::NodeTypeStats value, int width)
{
    std::string label = inLabel;
    int extraWidth = width - label.length();
    if (extraWidth < 0) {
        label.resize(width-3);
        label += "...";
        extraWidth = 0;
    }
    std::ostringstream tmp;
    tmp << label << ":  ";
    tmp.width(extraWidth + 10);
    tmp.setf(std::ios::right);
    tmp << value.count;
    tmp << " (" << (value.memUsage >> 10) << " kb)";
    os << tmp.str() << std::endl;

}

size_t
VdfNetwork::DumpStats(std::ostream &os) const
{
    TRACE_FUNCTION();

    // Count the number of instances of each node type in the network.
    VdfNetworkStats stats(*this);

    size_t longestType = TfMin(stats.GetMaxTypeNameLength(), size_t(50));

    size_t numOutputs = 0;
    for (const auto &node : _nodes) {
        if (node) {
            numOutputs += node->GetOutputSpecs().GetSize();
        }
    }

    os << "Network containing " 
       << GetNumOwnedNodes() << " nodes (with room for "
       << _freeNodeIds.unsafe_size() << " more), " 
       << numOutputs << " outputs and " 
       << std::endl;

    os << "----------------------------------------" << std::endl;

    for (const auto &[typeName, values] : stats.GetStatsMap()) {
        _PrintLabeledValue(os, typeName, values, longestType);
    }

    os << "----------------------------------------" << std::endl;

    _PrintLabeledValue(os, "Maximum Fan In", stats.GetMaxFanIn(), longestType);
    _PrintLabeledValue(os, "Maximum Fan In Node",
                       stats.GetMaxFanInNodeName(), longestType);
    _PrintLabeledValue(os, "Maximum Fan Out", stats.GetMaxFanOut(),
                       longestType);
    _PrintLabeledValue(os, "Maximum Fan Out Node",
                       stats.GetMaxFanOutNodeName(), longestType);

    os << "----------------------------------------" << std::endl << std::endl;

    // We choose to return the number of owned nodes, we could have also
    // chosen to return the node capacity.  Currently this value is only
    // used for tests.  We chose the number of owned nodes to have as little
    // disruption on the tests as possible.
    return GetNumOwnedNodes();
}

VdfPoolChainIndex
VdfNetwork::GetPoolChainIndex(const VdfOutput &output) const {
    return _poolChainIndexer->GetIndex(output);
}

PXR_NAMESPACE_CLOSE_SCOPE
