//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/schedule.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/debugCodes.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/rootNode.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/pxrTslRobinMap/robin_set.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"

#include <algorithm>
#include <tuple>

PXR_NAMESPACE_OPEN_SCOPE

constexpr int INVALID_NODE_INDEX = -1;

VdfSchedule::VdfSchedule() :
    _network(NULL),
    _isValid(false),
    _isSmallSchedule(false),
    _hasSMBL(false),
    _numUniqueInputDeps(0)
{
}

VdfSchedule::~VdfSchedule() 
{
    Clear();
}

void 
VdfSchedule::Clear()
{
    // Avoid debug output if nothing is scheduled.  This happens during 
    // initial scheduling.
    if (!_nodes.empty()) {

        TF_DEBUG(VDF_SCHEDULING).
            Msg("[Vdf] Clearing schedule %p with %zu nodes, "
                "_isSmallSchedule= %d\n",
                this, _nodes.size(), _isSmallSchedule);
    }

    _nodes.clear();
    _request = VdfRequest();
    _nodesToIndexMap.clear();
    _isSmallSchedule = false;
    _isValid = false;
    _hasSMBL = false;
    if (_network) {
        _network->_UnregisterSchedule(this);
    }
    _network = NULL;
    // Note that _scheduledNodes must only be cleared *after* it is
    // unregistered from the network.
    _scheduledNodes.Resize(0);
    _numUniqueInputDeps = 0;

    _computeTasks.clear();
    _inputsTasks.clear();
    _numKeepTasks = 0;
    _numPrepTasks = 0;
    _nodeInvocations.clear();
    _inputDeps.clear();
    _nodesToComputeTasks.clear();
    _nodesToKeepTasks.clear();
}

bool
VdfSchedule::IsScheduled(const VdfNode &node) const
{
    const VdfIndex index = VdfNode::GetIndexFromId(node.GetId());
    return index < _scheduledNodes.GetSize() && _scheduledNodes.IsSet(index);
}

VdfSchedule::OutputId
VdfSchedule::GetOutputId(const VdfOutput &output) const
{
    const int schedNodeIdx = _GetScheduleNodeIndex(output.GetNode());
    const int schedOutputIdx = schedNodeIdx < 0
        ? schedNodeIdx
        : _nodes[schedNodeIdx].GetOutputIndex(&output);
    return OutputId(schedNodeIdx, schedOutputIdx);
}

VdfSchedule::OutputId
VdfSchedule::GetOrCreateOutputId(const VdfOutput &output)
{
    const VdfNode &node = output.GetNode();

    TF_DEV_AXIOM(_nodesToIndexMap.size() == 
        node.GetNetwork().GetNodeCapacity());

    OutputId result = GetOutputId(output);

    // Note that the scheduled node bit set is always updated here.
    // The initialization that occurs inside the (result.IsValid())
    // block below isn't done when this is a small schedule.
    _scheduledNodes.Set(VdfNode::GetIndexFromId(node.GetId()));

    if (!result.IsValid()) {

        const int schedNodeIndex = _EnsureNodeInSchedule(node);
        VdfScheduleNode *scheduleNode = &_nodes[schedNodeIndex];
        TF_DEV_AXIOM(scheduleNode->node == &node);

        // Now make sure we have this scheduled output.
        scheduleNode->outputs.push_back(
            VdfScheduleOutput(&output, VdfMask(0)));

        result._scheduleNodeIndex = schedNodeIndex;
        result._secondaryIndex = scheduleNode->outputs.size() - 1;
    }

    return result;
}

void
VdfSchedule::AddInput(const VdfConnection &connection, const VdfMask &mask)
{
    // Make sure the node is scheduled.
    const VdfNode &node = connection.GetTargetNode();
    const int schedNodeIndex = _EnsureNodeInSchedule(node);

    // Get the source output and target input.
    const VdfOutput &source = connection.GetSourceOutput();
    const VdfInput &input = connection.GetTargetInput();

    std::vector<VdfScheduleInput> *scheduleInputs =
        &_nodes[schedNodeIndex].inputs;

    // Append the scheduled input.
    scheduleInputs->push_back({&source, mask, &input});
}

namespace
{
    // Equality & Hash functions that consider the source and input of a
    // VdfScheduleInput but not its mask.
    struct _SourceInputEq
    {
        bool operator()(
            const VdfScheduleInput *lhs, const VdfScheduleInput *rhs) const {
            return lhs->source == rhs->source && lhs->input == rhs->input;
        }
    };

    struct _SourceInputHash
    {
        size_t operator()(const VdfScheduleInput *schedInput) const {
            return TfHash::Combine(schedInput->source, schedInput->input);
        }
    };
}

// Consolidate masks for scheduled inputs that were added multiple times.
static void
_DeduplicateInputsForNode(VdfScheduleNode *node)
{
    std::vector<VdfScheduleInput> &inputs = node->inputs;
    if (inputs.size() <= 1) {
        return;
    }

    // For nodes with few scheduled inputs, building the hash set performs
    // worse than if we were to merge in AddInput using a linear search.
    // However, when there are thousands of inputs, e.g. on sharing nodes,
    // the savings yield a net improvement in scheduling performance.
    //
    // Production model profiling shows a small performance advantage during
    // evaluation when the order of inputs is preserved during deduplication.
    // We don't currently have a compelling theory for why this is the case.
    pxr_tsl::robin_set<
        VdfScheduleInput *, _SourceInputHash, _SourceInputEq>
        uniqueInputs(inputs.size());
    for (VdfScheduleInput &input : inputs) {
        const auto [it, isNew] = uniqueInputs.insert(&input);
        if (!isNew) {
            (*it)->mask |= input.mask;
            input.source = nullptr;
        }
    }

    inputs.erase(
        std::remove_if(
            inputs.begin(), inputs.end(),
            [](const VdfScheduleInput &input) {
                return !input.source;
            }),
        inputs.end());
}

void
VdfSchedule::DeduplicateInputs()
{
    TRACE_FUNCTION();

    WorkParallelForN(
        _nodes.size(),
        [nodes=_nodes.data()](size_t i, size_t n) {
            for (; i!=n; ++i) {
                _DeduplicateInputsForNode(&nodes[i]);
            }
        });
}

const VdfNode *
VdfSchedule::GetNode(const OutputId &outputId) const
{
    TF_DEV_AXIOM(outputId.IsValid());
    return _nodes[outputId._scheduleNodeIndex].node;
}

VdfSchedule::OutputId
VdfSchedule::GetOutputIdsBegin(const VdfNode &node) const
{
    const int schedNodeIdx = _GetScheduleNodeIndex(node);
    TF_DEV_AXIOM(schedNodeIdx >= 0 &&
        static_cast<size_t>(schedNodeIdx) < _nodes.size());
    return OutputId(schedNodeIdx, 0);
}

VdfSchedule::OutputId
VdfSchedule::GetOutputIdsEnd(const VdfNode &node) const
{
    const int schedNodeIdx = _GetScheduleNodeIndex(node);
    TF_DEV_AXIOM(schedNodeIdx >= 0 && 
        static_cast<size_t>(schedNodeIdx) < _nodes.size());
    return OutputId(schedNodeIdx, _nodes[schedNodeIdx].outputs.size());
}

VdfSchedule::InputsRange
VdfSchedule::GetInputs(const VdfNode &node) const
{
    const int schedNodeIdx = _GetScheduleNodeIndex(node);
    TF_DEV_AXIOM(schedNodeIdx >= 0 &&
        static_cast<size_t>(schedNodeIdx) < _nodes.size());
    const VdfScheduleNode &schedNode = _nodes[schedNodeIdx];
    return InputsRange(schedNode.inputs.cbegin(), schedNode.inputs.cend());
}

bool
VdfSchedule::IsAffective(const OutputId &outputId) const
{
    TF_DEV_AXIOM(outputId.IsValid());
    return _nodes[outputId._scheduleNodeIndex].affective;
}

const VdfOutput *
VdfSchedule::GetOutput(const OutputId &outputId) const
{
    TF_DEV_AXIOM(outputId.IsValid());
    return _nodes[outputId._scheduleNodeIndex]
        .outputs[outputId._secondaryIndex].output;
}

const VdfMask &
VdfSchedule::GetRequestMask(const OutputId &outputId) const
{
    TF_DEV_AXIOM(outputId.IsValid());
    return _nodes[outputId._scheduleNodeIndex]
        .outputs[outputId._secondaryIndex].requestMask;
}

void
VdfSchedule::GetRequestAndAffectsMask(
    const OutputId &outputId,
    const VdfMask **requestMask,
    const VdfMask **affectsMask) const
{
    TF_DEV_AXIOM(outputId.IsValid());

    // Return pointers to the masks by modifying the pointed-to output
    // pointer arguments.
    const VdfScheduleOutput &schedOutput =
        _nodes[outputId._scheduleNodeIndex].outputs[outputId._secondaryIndex];
    *requestMask = &schedOutput.requestMask;
    *affectsMask = &schedOutput.affectsMask;
}

const VdfMask &
VdfSchedule::GetAffectsMask(const OutputId &outputId) const
{
    TF_DEV_AXIOM(outputId.IsValid());
    return _nodes[outputId._scheduleNodeIndex]
        .outputs[outputId._secondaryIndex].affectsMask;
}

const VdfMask & 
VdfSchedule::GetKeepMask(const OutputId &outputId) const
{
    TF_DEV_AXIOM(outputId.IsValid());
    return _nodes[outputId._scheduleNodeIndex]
        .outputs[outputId._secondaryIndex].keepMask;
}

const VdfOutput * 
VdfSchedule::GetPassToOutput(const OutputId &outputId) const
{
    TF_DEV_AXIOM(outputId.IsValid());
    return _nodes[outputId._scheduleNodeIndex]
        .outputs[outputId._secondaryIndex].passToOutput;
}

const VdfOutput * 
VdfSchedule::GetFromBufferOutput(const OutputId &outputId) const
{
    TF_DEV_AXIOM(outputId.IsValid());
    return _nodes[outputId._scheduleNodeIndex]
        .outputs[outputId._secondaryIndex].fromBufferOutput;
}

const VdfOutput *
VdfSchedule::GetOutputToClear(const VdfNode &node) const
{
    const int schedNodeIdx = _GetScheduleNodeIndex(node);
    TF_DEV_AXIOM(schedNodeIdx >= 0 &&
        static_cast<size_t>(schedNodeIdx) < _nodes.size());
    return _nodes[schedNodeIdx].outputToClear;
}

VdfScheduleInputDependencyUniqueIndex
VdfSchedule::GetUniqueIndex(const OutputId outputId) const
{
    TF_DEV_AXIOM(outputId.IsValid());
    return _nodes[outputId._scheduleNodeIndex]
        .outputs[outputId._secondaryIndex].uniqueIndex;
}

void 
VdfSchedule::SetRequestMask(const OutputId &outputId, const VdfMask &mask)
{
    TF_DEV_AXIOM(outputId._scheduleNodeIndex >= 0 &&
        static_cast<size_t>(outputId._scheduleNodeIndex) < _nodes.size());

    _nodes[outputId._scheduleNodeIndex]
        .outputs[outputId._secondaryIndex].requestMask = mask;
}

void 
VdfSchedule::SetAffectsMask(const OutputId &outputId, const VdfMask &mask)
{
    TF_DEV_AXIOM(outputId._scheduleNodeIndex >= 0 &&
        static_cast<size_t>(outputId._scheduleNodeIndex) < _nodes.size());

    _nodes[outputId._scheduleNodeIndex]
        .outputs[outputId._secondaryIndex].affectsMask = mask;
}

void 
VdfSchedule::SetKeepMask(const OutputId &outputId, const VdfMask &mask)
{
    TF_DEV_AXIOM(outputId._scheduleNodeIndex >= 0 &&
        static_cast<size_t>(outputId._scheduleNodeIndex) < _nodes.size());

    _nodes[outputId._scheduleNodeIndex]
        .outputs[outputId._secondaryIndex].keepMask = mask;
}

void 
VdfSchedule::SetPassToOutput(const OutputId &outputId, const VdfOutput *output)
{
    TF_DEV_AXIOM(outputId._scheduleNodeIndex >= 0 &&
        static_cast<size_t>(outputId._scheduleNodeIndex) < _nodes.size());

    _nodes[outputId._scheduleNodeIndex]
        .outputs[outputId._secondaryIndex].passToOutput = output;
}

void 
VdfSchedule::SetFromBufferOutput(
    const OutputId &outputId, const VdfOutput *output)
{
    TF_DEV_AXIOM(outputId._scheduleNodeIndex >= 0 &&
        static_cast<size_t>(outputId._scheduleNodeIndex) < _nodes.size());

    _nodes[outputId._scheduleNodeIndex]
        .outputs[outputId._secondaryIndex].fromBufferOutput = output;
}

void
VdfSchedule::SetOutputToClear(const VdfNode &node,
    const VdfOutput* outputToClear)
{
    const int schedNodeIdx = _GetScheduleNodeIndex(node);
    TF_DEV_AXIOM(schedNodeIdx >= 0 &&
        static_cast<size_t>(schedNodeIdx) < _nodes.size());

    _nodes[schedNodeIdx].outputToClear = outputToClear;
}

void 
VdfSchedule::SetRequest(const VdfRequest &request)
{
    _request = request;
}

void
VdfSchedule::InitializeFromNetwork(const VdfNetwork &network)
{
    if (_nodesToIndexMap.empty()) {
        _nodesToIndexMap.resize(network.GetNodeCapacity(), INVALID_NODE_INDEX);
    }

    _scheduledNodes.Resize(network.GetNodeCapacity());
    _scheduledNodes.ClearAll();
    TF_VERIFY(network.GetNodeCapacity() == _nodesToIndexMap.size());
}

void 
VdfSchedule::_SetIsValidForNetwork(const VdfNetwork *network)
{
    _isValid = true;
    _network = network;

    // We can not have a network in the case that we're a valid empty 
    // schedule, so we won't receive any invalidation -- which is fine since
    // we can never be invalid
    if (_network) {
        _network->_RegisterSchedule(this);
    } else {
        // If we don't get a network, we better be empty.
        TF_AXIOM(_nodes.empty());
    }

    TF_DEBUG(VDF_SCHEDULING).
        Msg("[Vdf] Scheduled %p with %zu nodes, "
            "_isSmallSchedule= %d\n",
            this, _nodes.size(), _isSmallSchedule);
}

int
VdfSchedule::_GetScheduleNodeIndex(const VdfNode &node) const
{
    if (ARCH_UNLIKELY(_isSmallSchedule)) {
        for (size_t i=0; i<_nodes.size(); i++) {
            if (_nodes[i].node == &node) {
                return i;
            }
        }
        return INVALID_NODE_INDEX;
    }

    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(node.GetId());
    if (nodeIndex >= _nodesToIndexMap.size()) {
        // It is possible for nodes to be added to a network after scheduling,
        // so this method may be called with a node whose index is outside the
        // range of nodes the schedule knows about.  In this case it is correct
        // to say the node is not scheduled.
        return INVALID_NODE_INDEX;
    }

    return _nodesToIndexMap[nodeIndex];
}

int
VdfSchedule::_EnsureNodeInSchedule(const VdfNode &node)
{
    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(node.GetId());
    int schedNodeIndex = _nodesToIndexMap[nodeIndex];

    // Make sure we have a schedule node
    if (schedNodeIndex < 0) {

        schedNodeIndex = _nodes.size();
        _nodesToIndexMap[nodeIndex] = schedNodeIndex;

        _nodes.emplace_back(&node);
    }

    return schedNodeIndex;
}

void
VdfSchedule::ForEachScheduledOutput(
    const VdfNode &node,
    const VdfScheduledOutputCallback &callback) const
{
    VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(id, *this, node) {

        if (TF_VERIFY(id.IsValid()))
            callback(GetOutput(id), GetRequestMask(id));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
