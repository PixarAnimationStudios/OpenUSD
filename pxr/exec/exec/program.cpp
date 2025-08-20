//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/program.h"

#include "pxr/exec/exec/authoredValueInvalidationResult.h"
#include "pxr/exec/exec/disconnectedInputsInvalidationResult.h"
#include "pxr/exec/exec/parallelForRange.h"
#include "pxr/exec/exec/timeChangeInvalidationResult.h"

#include "pxr/base/tf/span.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/withScopedParallelism.h"
#include "pxr/exec/ef/leafNode.h"
#include "pxr/exec/ef/time.h"
#include "pxr/exec/ef/timeInputNode.h"
#include "pxr/exec/ef/timeInterval.h"
#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/grapher.h"
#include "pxr/exec/vdf/input.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/maskedOutputVector.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/typedVector.h"
#include "pxr/exec/vdf/types.h"
#include "pxr/usd/sdf/path.h"

#include <tbb/enumerable_thread_specific.h>

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

static EfTimeInterval
_ComputeInvalidInterval(
    const std::optional<TsSpline> &,
    const std::optional<TsSpline> &);

static TfBits
_FilterTimeDependentOutputs(
    const VdfMaskedOutputVector &, const EfTime &, const EfTime &);

class Exec_Program::_EditMonitor final : public VdfNetwork::EditMonitor {
public:
    explicit _EditMonitor(Exec_Program *const program)
        : _program(program)
    {}

    void WillClear() override {
        _program->_leafNodeCache.Clear();
    }

    void DidConnect(const VdfConnection *connection) override {
        _program->_leafNodeCache.DidConnect(*connection);

        if (&connection->GetSourceNode() == _program->_timeInputNode) {
            _program->_ChangedTimeConnections(connection->GetTargetNode());
        }
    }

    void DidAddNode(const VdfNode *node) override {}

    void WillDelete(const VdfConnection *connection) override {
        _program->_leafNodeCache.WillDeleteConnection(*connection);

        if (&connection->GetSourceNode() == _program->_timeInputNode) {
            _program->_ChangedTimeConnections(connection->GetTargetNode());
        }
    }

    void WillDelete(const VdfNode *node) override {
        // TODO: When we implement parallel node deletion, this needs to be made
        // thread-safe.
        _program->_compiledOutputCache.EraseByNodeId(node->GetId());

        // Update the CompiledLeafNodeCache if the deleted node is a leaf node.
        if (const EfLeafNode *const leafNode = EfLeafNode::AsALeafNode(node)) {
            _program->_compiledLeafNodeCache.WillDeleteNode(leafNode);
        }

        // Unregister this node if it is an attribute input node.
        // 
        // The edit monitor captures both node deletion through
        // DisconnectAndDeleteNode() as well as isolated sub-network deletion.
        if (const Exec_AttributeInputNode *const inputNode = 
                dynamic_cast<const Exec_AttributeInputNode *const>(node)) {
            _program->_UnregisterInputNode(inputNode);
        }

        _program->_nodeRecompilationInfoTable.WillDeleteNode(node);
    }

private:
    Exec_Program *const _program;
};

Exec_Program::Exec_Program()
    : _timeInputNode(new EfTimeInputNode(&_network))
    , _timeDependentOutputsValid(true)
    , _editMonitor(std::make_unique<_EditMonitor>(this))
{
    _network.RegisterEditMonitor(_editMonitor.get());
}

Exec_Program::~Exec_Program()
{
    _network.UnregisterEditMonitor(_editMonitor.get());
}

void
Exec_Program::Connect(
    const EsfJournal &journal,
    const TfSpan<const VdfMaskedOutput> outputs,
    VdfNode *inputNode,
    const TfToken &inputName)
{
    for (const VdfMaskedOutput &output : outputs) {
        // XXX
        // Note that it's possible for the SourceOutputs contains null outputs.
        // This can happen if the input depends on output keys that could not
        // be compiled (e.g. requesting a computation on a prim which does not
        // have a registered computation of that name). This can be re-visited
        // if output keys contain Exec_ComputationDefinition pointers, as
        // that requires we find a matching computation in order to form that
        // output key.
        if (output) {
            _network.Connect(output, inputNode, inputName);
        }
    }
    _uncompilationTable.AddRulesForInput(
        inputNode->GetId(), inputName, journal);
}

Exec_DisconnectedInputsInvalidationResult
Exec_Program::InvalidateDisconnectedInputs()
{
    TRACE_FUNCTION();

    std::vector<const VdfNode *> disconnectedLeafNodes;
    VdfMaskedOutputVector invalidationRequest;
    invalidationRequest.reserve(_inputsRequiringRecompilation.size());

    for (VdfInput *const input : _inputsRequiringRecompilation) {
        VdfNode &node = input->GetNode();

        // Accumulate all disconnected leaf nodes. These nodes are no longer
        // reachable via the leaf node traversal below, and thus must be
        // recorded separately.
        if (EfLeafNode::IsALeafNode(node)) {
            disconnectedLeafNodes.push_back(&node);
        }

        // On speculation nodes, find the output corresponding to the input and
        // record it for the traversal.
        // 
        // TODO: We should add VdfNode::ComputeDependencyOnInput API to solve
        // this more generically.
        else if (node.IsSpeculationNode()) {
            VdfOutput *const correspondingOutput =
                node.GetOutput(input->GetName());
            if (TF_VERIFY(correspondingOutput)) {
                invalidationRequest.emplace_back(
                    correspondingOutput,
                    VdfMask::AllOnes(correspondingOutput->GetNumDataEntries()));
            }
        }

        // For all other types of nodes, collect all outputs for the traversal.
        else {
            for (const auto &[name, output] : node.GetOutputsIterator()) {
                invalidationRequest.emplace_back(
                    output, VdfMask::AllOnes(output->GetNumDataEntries()));
            }
        }
    }

    // Find all the leaf nodes reachable from the disconnected inputs.
    // We won't ask the leaf node cache to incur the cost of performing
    // incremental updates on the resulting cached traversal, because it is not
    // guaranteed that we will repeatedly see the exact same authored value
    // invalidation across rounds of structural change processing (in contrast
    // to time invalidation).
    const std::vector<const VdfNode *> &leafNodes = _leafNodeCache.FindNodes(
        invalidationRequest, /* updateIncrementally = */ false);

    return Exec_DisconnectedInputsInvalidationResult{
        std::move(invalidationRequest),
        leafNodes,
        std::move(disconnectedLeafNodes)};
}

Exec_AuthoredValueInvalidationResult
Exec_Program::InvalidateAuthoredValues(TfSpan<const SdfPath> invalidProperties)
{
    TRACE_FUNCTION();

    const size_t numInvalidProperties = invalidProperties.size();

    VdfMaskedOutputVector leafInvalidationRequest;
    leafInvalidationRequest.reserve(numInvalidProperties);
    TfBits compiledProperties(numInvalidProperties);
    _uninitializedInputNodes.reserve(
        _uninitializedInputNodes.size() + numInvalidProperties);
    EfTimeInterval totalInvalidInterval;
    bool isTimeDependencyChange = false;

    for (size_t i = 0; i < numInvalidProperties; ++i) {
        const SdfPath &path = invalidProperties[i];
        const auto it = _inputNodes.find(path);

        // Not every invalid property is also an input to the exec network.
        // If any of these properties have been included in an exec request,
        // clients still expect to receive invalidation notices, though.
        // However, we can skip including this property in the search for
        // dependent leaf nodes in that case.
        const bool isCompiled = it != _inputNodes.end();
        if (!isCompiled) {
            continue;
        }

        // Indicate this property was compiled.
        compiledProperties.Set(i);

        // Get the input node from the network.
        _InputNodeEntry &entry = it->second;
        Exec_AttributeInputNode *const node = entry.node;

        // Make sure that the input node's internal value resolution state is
        // updated after scene changes that could affect where resolved values
        // are sourced from.
        node->UpdateValueResolutionState();

        // Figure out if the input node's time dependence has changed based on
        // the authored value change.
        if (node->UpdateTimeDependence()) {
            _InvalidateTimeDependentOutputs();
            isTimeDependencyChange = true;
        }

        // If this is an input node to the exec network, we need to make sure
        // that it is re-initialized before the next round of evaluation.
        _uninitializedInputNodes.push_back(node->GetId());

        // Queue the input node's output(s) for leaf node invalidation.
        leafInvalidationRequest.emplace_back(
            node->GetOutput(), VdfMask::AllOnes(1));

        // Accumulate the invalid time interval, but only if the interval
        // accumulated so far isn't already the full interval.
        std::optional<TsSpline> newSpline = node->GetSpline();
        if (!totalInvalidInterval.IsFullInterval()) {
            totalInvalidInterval |= _ComputeInvalidInterval(
                entry.oldSpline, newSpline);
        }

        // Retain the new spline so we can compare it against future authored
        // value changes.
        entry.oldSpline = std::move(newSpline);
    }

    // Find all the leaf nodes reachable from the input nodes.
    // We won't ask the leaf node cache to incur the cost of performing
    // incremental updates on the resulting cached traversal, because it is not
    // guaranteed that we will repeatedly see the exact same authored value
    // invalidation across rounds of structural change processing (in contrast
    // to time invalidation).
    const std::vector<const VdfNode *> &leafNodes = _leafNodeCache.FindNodes(
        leafInvalidationRequest, /* updateIncrementally = */ false);

    return Exec_AuthoredValueInvalidationResult{
        invalidProperties,
        std::move(compiledProperties),
        std::move(leafInvalidationRequest),
        leafNodes,
        totalInvalidInterval,
        isTimeDependencyChange};
}

Exec_TimeChangeInvalidationResult
Exec_Program::InvalidateTime(const EfTime &oldTime, const EfTime &newTime)
{
    TRACE_FUNCTION();

    // Gather up the set of outputs that are currently time-dependent.
    const VdfMaskedOutputVector &timeDependentOutputs =
        _CollectTimeDependentOutputs();
    
    // Construct a bit set that filters the array of time dependent outputs down
    // to the ones that actually changed going from oldTime to newTime.
    const TfBits filter = _FilterTimeDependentOutputs(
        timeDependentOutputs, oldTime, newTime);

    // Compute the executor invalidation request, and gather leaf nodes for
    // exec request notification.
    VdfMaskedOutputVector invalidationRequest;
    const std::vector<const VdfNode *> *leafNodes = nullptr;
    WorkWithScopedDispatcher(
        [&filter, &timeDependentOutputs, &invalidationRequest,
            &leafNodes, &leafNodeCache = _leafNodeCache]
        (WorkDispatcher &dispatcher){
        // Turn the invalid time-dependent outputs into a request.
        dispatcher.Run([&](){
            invalidationRequest.reserve(filter.GetNumSet());
            for (size_t i : filter.GetAllSetView()) {
                invalidationRequest.push_back(timeDependentOutputs[i]);
            }
        });

        // Find the leaf nodes that are dependent on the values that are
        // changing from oldTime to newTime.
        dispatcher.Run([&](){
            leafNodes = &(leafNodeCache.FindNodes(
                timeDependentOutputs, filter));
        });
    });

    TF_VERIFY(leafNodes);
    return Exec_TimeChangeInvalidationResult{
        std::move(invalidationRequest),
        *leafNodes,
        oldTime,
        newTime};
}

VdfMaskedOutputVector
Exec_Program::ResetUninitializedInputNodes()
{
    if (_uninitializedInputNodes.empty()) {
        return {};
    }

    TRACE_FUNCTION();

    // Collect the invalid outputs for all invalid input nodes accumulated
    // through previous rounds of authored value invalidation.
    VdfMaskedOutputVector invalidationRequest;
    invalidationRequest.reserve(_uninitializedInputNodes.size());
    for (const VdfId nodeId : _uninitializedInputNodes) {
        VdfNode *const node = _network.GetNodeById(nodeId);

        // Some nodes may have been uncompiled since they were marked as being
        // uninitialized. It's okay to simply skip these nodes.
        if (!node) {
            continue;
        }

        invalidationRequest.emplace_back(
            node->GetOutput(), VdfMask::AllOnes(1));
    }

    _uninitializedInputNodes.clear();

    return invalidationRequest;
}

void
Exec_Program::DisconnectAndDeleteNode(VdfNode *const node)
{
    TRACE_FUNCTION();

    // Track a set of connections to be deleted at the end of this function,
    // because it is not safe to remove connections while iterating over them.
    VdfConnectionVector connections;

    // Upstream nodes are potentially isolated.
    for (const auto &[name, input] : node->GetInputsIterator()) {
        TF_UNUSED(name);
        for (VdfConnection *const connection : input->GetConnections()) {
            _potentiallyIsolatedNodes.insert(&connection->GetSourceNode());
            connections.push_back(connection);
        }
    }

    // Downstream inputs require recompilation.
    for (const auto &[name, output] : node->GetOutputsIterator()) {
        TF_UNUSED(name);
        for (VdfConnection *const connection : output->GetConnections()) {
            _inputsRequiringRecompilation.insert(&connection->GetTargetInput());

            // TODO: We currently disconnect other connections incoming on the
            // target input, and we mark the nodes upstream of those connections
            // as potentially isolated. We do this because recompilation of
            // inputs expects those inputs to be fully disconnected. However,
            // a future change can add support to recompile inputs with existing
            // connections.
            for (VdfConnection *const targetInputConnection :
                connection->GetTargetInput().GetConnections()) {
                
                _potentiallyIsolatedNodes.insert(
                    &targetInputConnection->GetSourceNode());
                connections.push_back(targetInputConnection);
            }
        }
    }

    // This node cannot be isolated, and its inputs do not require
    // recompilation, because they are all about to be deleted.
    _potentiallyIsolatedNodes.erase(node);
    for (const auto &[name, input] : node->GetInputsIterator()) {
        TF_UNUSED(name);
        _inputsRequiringRecompilation.erase(input);
    }

    // Finally, delete the affected connections and the node.
    for (VdfConnection *const connection : connections) {
        _network.Disconnect(connection);
    }
    _network.Delete(node);
}

void
Exec_Program::DisconnectInput(VdfInput *const input)
{
    TRACE_FUNCTION();

    _inputsRequiringRecompilation.insert(input);

    // All source nodes of the input's connections are now potentially isolated.
    // Iterate over a copy of the connections, because the original vector will
    // be modified by VdfNetwork::Disconnect.
    const VdfConnectionVector connections = input->GetConnections();
    for (VdfConnection *const connection : connections) {
        _potentiallyIsolatedNodes.insert(&connection->GetSourceNode());
        _network.Disconnect(connection);
    }
}

std::unique_ptr<VdfIsolatedSubnetwork> 
Exec_Program::CreateIsolatedSubnetwork()
{
    TRACE_FUNCTION();

    const auto editFilter =
        [timeInputNode=_timeInputNode](const VdfNode *const node) {
            return node != timeInputNode;
        };

    std::unique_ptr<VdfIsolatedSubnetwork> subnetwork =
        VdfIsolatedSubnetwork::New(&_network);

    for (VdfNode *const node : _potentiallyIsolatedNodes) {
        subnetwork->AddIsolatedBranch(node, editFilter);
    }

    _potentiallyIsolatedNodes.clear();

    return subnetwork;
}

void
Exec_Program::GraphNetwork(
    const char *const filename,
    const VdfGrapherOptions &grapherOptions) const
{
    VdfGrapher::GraphToFile(_network, filename, grapherOptions);
}

void
Exec_Program::_AddNode(const EsfJournal &journal, const VdfNode *node)
{
    _uncompilationTable.AddRulesForNode(node->GetId(), journal);
}

void
Exec_Program::_RegisterInputNode(Exec_AttributeInputNode *const inputNode)
{
    const auto [it, emplaced] = _inputNodes.emplace(
        inputNode->GetAttributePath(), 
        _InputNodeEntry{inputNode});
    TF_VERIFY(emplaced);
}

void
Exec_Program::_UnregisterInputNode(
    const Exec_AttributeInputNode *const inputNode)
{
    const SdfPath attributePath = inputNode->GetAttributePath();
    const auto it = _inputNodes.find(attributePath);
    if (!TF_VERIFY(it != _inputNodes.end())) {
        return;
    }
    
    _inputNodes.unsafe_erase(attributePath);
}

void
Exec_Program::_ChangedTimeConnections(const VdfNode &targetNode)
{
    // If there was a chance to a connection on a time-varying input node, or a
    // connection on a node that isn't an input node, we need to invalidate the
    // cached subset of time varying outputs.
    const Exec_AttributeInputNode *const inputNode = 
        dynamic_cast<const Exec_AttributeInputNode *const>(&targetNode);
    if (!inputNode || inputNode->IsTimeDependent()) {
        _InvalidateTimeDependentOutputs();
    }
}

void
Exec_Program::_InvalidateTimeDependentOutputs()
{
    // We set an atomic flag here instead of fiddling with the
    // _timeDependentOutputs array directly, so that we don't have to  worry
    // about making the latter a concurrent data structure.
    _timeDependentOutputsValid.store(false, std::memory_order_relaxed);
}

const VdfMaskedOutputVector &
Exec_Program::_CollectTimeDependentOutputs()
{
    // If the cached array of time-dependent outputs is still valid, return it.
    if (_timeDependentOutputsValid.load(std::memory_order_relaxed)) {
        return _timeDependentOutputs;
    }

    TRACE_FUNCTION();

    // Get all outgoing connections on the time input node.
    const VdfConnectionVector &timeConnections =
        _timeInputNode->GetOutput()->GetConnections();

    // To allow us to rebuild the array of time-dependent outputs in parallel,
    // pessimally size it to accommodate all outputs, and keep track of how many
    // entries have really been populated. We will shrink the array later.
    std::atomic<size_t> num(0);
    _timeDependentOutputs.resize(timeConnections.size());

    // Iterate over all the connections and select the target nodes that are
    // currently time dependent. This will include time-dependent input nodes,
    // as well as nodes that establish a direct input dependency on the
    // computeTime builtin.
    WorkParallelForN(timeConnections.size(),
        [&timeConnections, &num, &result = _timeDependentOutputs]
        (size_t b, size_t e){
        VdfMaskedOutputVector outputs;

        for (size_t i = b; i != e; ++i) {
            const VdfConnection *const connection = timeConnections[i];
            const VdfNode &targetNode = connection->GetTargetNode();
            if (TF_VERIFY(targetNode.GetNumOutputs() <= 1)) {
                targetNode.ComputeOutputDependencyMasks(
                    *connection, VdfMask::AllOnes(1), &outputs);
            }
        }

        if (!outputs.empty()) {
            const size_t i = num.fetch_add(outputs.size());
            std::copy(outputs.begin(), outputs.end(), result.begin() + i);
        }
    });

    // Shrink the array to only contain the entries we just populated.
    _timeDependentOutputs.resize(num.load());

    // The array of time-dependent outputs is valid again. Return it.
    _timeDependentOutputsValid.store(true, std::memory_order_relaxed);
    return _timeDependentOutputs;
}

static EfTimeInterval
_ComputeInvalidInterval(
    const std::optional<TsSpline> &oldSpline,
    const std::optional<TsSpline> &newSpline)
{
    // If either the new- or old value (or both) resolve to anything but a 
    // spline (fallback, default, or time samples) we invalidate the full
    // interval: Both fallback and default values apply over all time, and time
    // samples typically encode such dense data that we do not want to incur
    // the cost of detailed analysis of that data.
    const bool hasOldSpline = oldSpline.has_value();
    const bool hasNewSpline = newSpline.has_value();
    if (!hasOldSpline || !hasNewSpline) {
        return EfTimeInterval::GetFullInterval();
    }

    TRACE_FUNCTION();

    // If we are going from an empty spline to a non-empty spline or vice-versa,
    // invalidate the full interval.
    if (oldSpline->IsEmpty() != newSpline->IsEmpty()) {
        return EfTimeInterval::GetFullInterval();
    }

    // If loop parameters changed, we invalidate the full interval.
    if (oldSpline->HasLoops() != newSpline->HasLoops()) {
        return EfTimeInterval::GetFullInterval();
    }

    // If both splines are empty, nothing is invalid.
    if (oldSpline->IsEmpty() && newSpline->IsEmpty()) {
        return EfTimeInterval();
    }

    // TODO: Compute the change interval between oldSpline and newSpline. For
    // the time-being, let's over-invalidate the time range.
    return EfTimeInterval::GetFullInterval();
}

static TfBits
_FilterTimeDependentOutputs(
    const VdfMaskedOutputVector &timeDependentInputNodeOutputs,
    const EfTime &oldTime,
    const EfTime &newTime)
{
    TRACE_FUNCTION();

    // One bitset per thread.
    const size_t numInputs = timeDependentInputNodeOutputs.size();
    tbb::enumerable_thread_specific<TfBits> threadBits([numInputs](){
        return TfBits(numInputs);
    });

    // For each time-dependent output, figure out if the computed value actually
    // changes between oldTime and newTime. If so, set the corresponding bit
    // in the bit set.
    WorkWithScopedParallelism(
        [&numInputs, &timeDependentInputNodeOutputs, &oldTime, &newTime,
            &threadBits](){
        WorkParallelForN(numInputs,[&](size_t b, size_t e){
            TfBits *const bits = &threadBits.local();
            for (size_t i = b; i != e; ++i) {
                const VdfNode &node =
                    timeDependentInputNodeOutputs[i].GetOutput()->GetNode();
                const Exec_AttributeInputNode *const inputNode =
                        dynamic_cast<const Exec_AttributeInputNode*>(&node);
                if (!inputNode || inputNode->IsTimeVarying(oldTime, newTime)) {
                    bits->Set(i);
                }
            }
        });
    });

    // Combine the thread-local bit sets into a single bit set and return it.
    TfBits result(numInputs);
    threadBits.combine_each([&result](const TfBits &bits){
        result |= bits;
    });
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
