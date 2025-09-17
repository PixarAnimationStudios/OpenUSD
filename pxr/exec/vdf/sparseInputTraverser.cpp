//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/sparseInputTraverser.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/poolChainIndexer.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/bits.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/trace/trace.h"

#include <functional>
#include <memory>
#include <vector>

#define _TRAVERSAL_TRACING 0
#if _TRAVERSAL_TRACING
#include <iostream>
#endif

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////

class VdfSparseInputTraverser::_PrioritizedOutput
{
public:
    _PrioritizedOutput(
        const VdfOutput *output,
        const VdfMask   &dependencyMask)
    :   _output(output),
        _dependencyBits(dependencyMask.GetBits())
    { }

    _PrioritizedOutput(
        const VdfOutput *output,
        const VdfMask   &dependencyMask,
        const VdfObjectPtrVector &basePath,
        const VdfConnection *pathElement)
    :   _output(output),
        _dependencyBits(dependencyMask.GetBits()),
        _path(std::make_shared<VdfObjectPtrVector>(basePath))
    {
        if (pathElement) {
            _path->push_back(pathElement);
        }
    }

    /// Returns the output.
    ///
    const VdfOutput *GetOutput() const {
        return _output;
    }

    /// Returns the accumulated dependency bits.
    ///
    const VdfMask::Bits &GetDependencyBits() const {
        return _dependencyBits;
    }

    /// Extends this prioritized output with \p dependencyMask and
    /// \p parentCacheIndex.
    ///
    void Extend(const VdfMask &dependencyMask) {
        _dependencyBits |= dependencyMask.GetBits();
    }

    /// Returns the path, or a reference to NULL if we have no path.
    const VdfObjectPtrVector &GetPath() const
    {
        static VdfObjectPtrVector empty;
        return _path ? *_path : empty;
    }

private:

    // The output.
    const VdfOutput *_output;

    // The (accumulated) dependency mask.
    VdfMask::Bits _dependencyBits;

    // The first path that leads to this pool output.
    // The _path is held via shared pointer because we use this call as
    // value type.
    std::shared_ptr<VdfObjectPtrVector> _path;
};

class VdfSparseInputTraverser::_StackFrame
{
public:

    /// Constructs a stack frame with no path info from a maskedOutput.
    ///
    _StackFrame(
        const VdfMaskedOutput &maskedOutput)
    :   _maskedOutput(maskedOutput) {}

    /// Constructs an initial stack frame with path information from
    /// a maskedOutput.
    ///
    _StackFrame(
        const VdfMaskedOutput    &maskedOutput,
        const VdfObjectPtrVector &basePath,
        const VdfConnection      *pathElement)
    :   _maskedOutput(maskedOutput),
        _path(std::make_shared<VdfObjectPtrVector>(basePath)) //XXX: Avoid copy? 
    {
        if (pathElement)
            _path->push_back(pathElement);
    }

    /// Returns the masked output.
    ///
    const VdfMaskedOutput &GetMaskedOutput() const
    {
        return _maskedOutput;
    }

    /// Returns the path, or a reference to NULL if we have no path.
    ///
    const VdfObjectPtrVector &GetPath() const
    {
        static VdfObjectPtrVector empty;
        return _path ? *_path : empty;
    }

private:

    VdfMaskedOutput _maskedOutput;

    // The _path is held via shared pointer because we use this call as 
    // value type.
    std::shared_ptr<VdfObjectPtrVector> _path;
};

struct VdfSparseInputTraverser::_TraversalState
{
    _TraversalState(
        const NodePathCallback       &nodePathCallback_,
        const ConnectionPathCallback &connectionPathCallback_,
        bool                         producePath_,
        size_t                       numNodes=0)
        :   nodePathCallback(nodePathCallback_),
            connectionPathCallback(connectionPathCallback_),
            producePath(producePath_),
            nodeCallbackInvocations(numNodes),
            nodeIsSkippable(numNodes)
    {}

    NodePathCallback            nodePathCallback;
    ConnectionPathCallback      connectionPathCallback;
    bool                        producePath;

    // When traversing without a path we use a global map of 
    // visited connections (because we are only interested in 
    // issuing nodeCallbacks for all nodes possible, not all potential
    // paths to all nodes possible).
    _VisitedConnections         visitedConnections;

    // A vector of traversal stack frames, used as the stack.
    std::vector<_StackFrame>    stack;

    // Map of pool outputs in priority order.
    _PrioritizedOutputMap       prioritizedOutputs;

    // One bit for each node in the network indicating whether or not the
    // node callback has been invoked for that node yet (to avoid redundant
    // node callback invocations).
    TfBits                     nodeCallbackInvocations;

    // One bit for each node in the network indicating that the last
    // nodeCallbackInvocation marked the node as skippable.
    TfBits                     nodeIsSkippable;
};

//
// Returns the number of nodes in the network.
//
static size_t
_GetNumNodesInNetwork(
    const VdfMaskedOutputVector &request)
{
    // Return the size of the network if the request isn't empty, otherwise
    // return 0.
    if (request.empty() || !TF_VERIFY(request.front().GetOutput())) {
        return 0;
    }

    return
        request.front().GetOutput()->GetNode().GetNetwork().GetNodeCapacity();
}

/*static */
void
VdfSparseInputTraverser::Traverse(
    const VdfMaskedOutputVector &request,
    const NodeCallback          &nodeCallback,
    CallbackMode                callbackMode /* = CallbackModeAllNodes*/)
{
    TRACE_FUNCTION();

    _TraversalState state(
        std::bind(_NodePathCallbackAdapter,
            nodeCallback, std::placeholders::_1, std::placeholders::_2),
        NULL /* connectionCallback */, false /* producePath */,
        nodeCallback ? _GetNumNodesInNetwork(request) : 0);

    _InitTraversal(request, &state, callbackMode);
}

/* static */
void
VdfSparseInputTraverser::TraverseWithConnectionCallback(
    const VdfMaskedOutputVector &request,
    const ConnectionCallback    &connectionCallback)
{
    TRACE_FUNCTION();

    _TraversalState state(
        NULL /* nodePathCallback */,
        std::bind(
            _ConnectionPathCallbackAdapter,
            connectionCallback,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3),
        false /* producePath */);

    _InitTraversal(request, &state);
}

/* static */
void
VdfSparseInputTraverser::TraverseWithPath(
    const VdfMaskedOutputVector  &request,
    const NodePathCallback       &nodePathCallback,
    const ConnectionPathCallback &connectionPathCallback,
    CallbackMode                 callbackMode)
{
    TRACE_FUNCTION();

    _TraversalState state(
        nodePathCallback, connectionPathCallback, true /* producePath */,
        nodePathCallback ? _GetNumNodesInNetwork(request) : 0);

    _InitTraversal(request, &state, callbackMode);
}

/* static */
void
VdfSparseInputTraverser::_InitTraversal(
    const VdfMaskedOutputVector &request,
    _TraversalState             *state,
    const CallbackMode          callbackMode/* = CallbackModeAllNodes */)
{
    TF_AXIOM(state);

    #if _TRAVERSAL_TRACING
        std::cout << "\n"
                  << "Starting sparse input traversal with "
                  << request.size()
                  << " outputs\n";
    #endif

    // Early bail-out for empty request.
    if (request.empty()) {
        return;
    }

    // Push the inital masked outputs onto the stack.
    state->stack.reserve( request.size() );

    TF_FOR_ALL(i, request) {

        if (state->producePath)
            state->stack.push_back(
                _StackFrame(*i, VdfObjectPtrVector(), NULL));
        else
            state->stack.push_back(_StackFrame(*i));
    }

    // Loop while we've got work to do.
    while (!state->stack.empty() || !state->prioritizedOutputs.empty()) {

        while (!state->stack.empty()) {
            // Get the next output to process.
            //
            // Since we're popping the frame off the stack, we have to be
            // careful to copy it by value, and not just get a reference.
            //
            _StackFrame frame = state->stack.back();
            state->stack.pop_back();

            // Process the output.
            _TraverseOutput(state, frame, callbackMode);
        }

        if (!state->prioritizedOutputs.empty()) {
            // Pull the top output from the priority queue. This works, because
            // _PrioritizedOutputMap is a std::map and hence sorted.
            _PrioritizedOutputMap::iterator topIter =
                state->prioritizedOutputs.begin();

            const _PrioritizedOutput &top = topIter->second;

            #if _TRAVERSAL_TRACING
                std::cout << "  Traversing pool output \""
                          << top.GetOutput()->GetDebugName()
                          << "\"" << std::endl;
            #endif

            // Process the output.

            VdfMask dependencyMask(top.GetDependencyBits());

            _StackFrame f(
                VdfMaskedOutput(
                    const_cast<VdfOutput *>(top.GetOutput()), dependencyMask),
                top.GetPath(), NULL);

            _TraverseOutput(state, f, callbackMode);

            // Remove prioritzed output.  Note that the call to
            // _TraverseOutput() above may have inserted more prioritized
            // outputs.  However, topIter is still valid in that case.
            // If we remove topIter before the call to _TraverseOutput(), we
            // would need to copy all data by value which we want to avoid.
            //
            // However, we still make sure that the dependencyMask hasn't been
            // modified in the meantime.

            TF_VERIFY(dependencyMask.GetBits() == top.GetDependencyBits());

            state->prioritizedOutputs.erase(topIter);

        }
    }
}

/* static */
void
VdfSparseInputTraverser::_TraverseOutput(
    _TraversalState     *state,
    const _StackFrame    &frame,
    const CallbackMode  callbackMode)
{
    const VdfMaskedOutput &maskedOutput = frame.GetMaskedOutput();
    const VdfOutput       *output       = maskedOutput.GetOutput();
    TF_DEV_AXIOM(output);

    #if _TRAVERSAL_TRACING
        std::cout << "  Traversing output "
                  << output->GetDebugName()
                  << " with mask = "
                  << maskedOutput.GetMask().GetRLEString()
                  << "\n";
    #endif

    const VdfNode &node = output->GetNode();

    // If we have a node callback, call it and see if we should stop the
    // traversal.  Be sure to only call it if this node affects the requested
    // outputs.

    // If callback mode is CallbackModeTerminalNodes, then only invoke the
    // node callback on terminal nodes (i.e. on nodes without input
    // connections).
    bool shouldInvokeCallback =
        callbackMode != CallbackModeTerminalNodes ||
        !node.HasInputConnections();

    if (shouldInvokeCallback && state->nodePathCallback) {

        const VdfIndex nodeIndex = VdfNode::GetIndexFromId(node.GetId());

        if (state->nodeIsSkippable.IsSet(nodeIndex)) {
            return;
        }

        // If the callback for this node has already been invoked, then skip.
        if (!state->nodeCallbackInvocations.IsSet(nodeIndex)) {
            bool execute = true;

            if (const VdfMask *affectsMask = output->GetAffectsMask())
                execute = affectsMask->Overlaps(maskedOutput.GetMask());

            if (execute) {
                state->nodeCallbackInvocations.Set(nodeIndex);

                if (!state->nodePathCallback(node, frame.GetPath()))
                {
                    state->nodeIsSkippable.Set(nodeIndex);
                    return;
                }
            }

        } else if (callbackMode == CallbackModeTerminalNodes) {
            // If the callback has already been invoked for this node, then
            // don't bother recursing over its inputs and input connections.
            // 
            // We can ONLY do this early-out when we are traversing in terminal
            // node callback mode, because we may potentially be missing new
            // paths if this node has already been marked as visited with a
            // different mask!!!
            return;
        }
    }

    // Ask the node for the dependencies.
    VdfConnectionAndMaskVector dependencies =
        node.ComputeInputDependencyMasks(
            maskedOutput, false /* skipAssociatedInputs */);

    TF_FOR_ALL(i, dependencies) {

        const VdfConnection *connection = i->first;
        TF_DEV_AXIOM(connection);

        // If we have a connection callback, call it and see if we should
        // stop the traversal for this branch.

        const VdfMask &dependencyMask = i->second;

        if (state->connectionPathCallback) {

            if (!state->connectionPathCallback(*connection, dependencyMask, frame.GetPath()))
                continue;
        }

        // See if we have already visited this connection.
        _VisitedConnections::iterator visitedIt =
            state->visitedConnections.find(connection);

        // Skip this connection if its accumulated traversal
        // mask contains the current dependency mask.
        if (visitedIt != state->visitedConnections.end() &&
            visitedIt->second.Contains(dependencyMask.GetBits())) {

            // At this point, we have detected another path leading
            // to this connection.  This maybe another path or a cycle.
            continue;
        }

        // Update the visited connections map.
        if (visitedIt != state->visitedConnections.end())
            visitedIt->second |= dependencyMask.GetBits();
        else
            state->visitedConnections.insert(
                std::make_pair(connection, dependencyMask.GetBits()) );

        VdfOutput &sourceOutput =
            connection->GetNonConstSourceOutput();

        // If we're not interested in producing paths and if this is a pool
        // output, accumulate the mask in the associated poolOutputs map,
        // and don't traverse the output until we're done with everything
        // on the stack.
        //
        // If we're producing paths, then we can't mess with the order
        // in which outputs are processed
        //
        // XXX:speculation
        // I think it would be faster if VdfSpeculationNodes were handled
        // specially here.  As it currently stands, I think we can end up
        // with inefficient traversals because speculation nodes take us
        // back up to a higher point in the pool.  It'd be better if we
        // finished all pool traversal before processing speculation nodes,
        // because that will better vectorize the resulting traversal.

        if (!state->producePath &&
            Vdf_IsPoolOutput(sourceOutput)) {
            
            // The input traverser processes nodes further down the pool chain
            // first so the priorities need to be the opposite of those given
            // by the pool chain indexer. Hence, we use std::greater for the
            // map.

            VdfPoolChainIndex poolIndex = 
                node.GetNetwork().GetPoolChainIndex(sourceOutput);

            _PrioritizedOutputMap::iterator iter =
                state->prioritizedOutputs.find(poolIndex);

            if (iter != state->prioritizedOutputs.end()) {

                // Make sure that poolIndex is computed consistently (ie.
                // there is an unique, consistent index for each output).
                TF_VERIFY(iter->second.GetOutput() == &sourceOutput);

                // Extend this prioritized output and make sure it referes
                // to the same output (since we use the pool chain index
                // as id).
                iter->second.Extend(dependencyMask);

            } else {

                // Insert this pool output into the priority queue.
                state->prioritizedOutputs.insert(
                    std::make_pair(
                        poolIndex,
                        state->producePath ?
                            _PrioritizedOutput(
                                &sourceOutput, dependencyMask,
                                frame.GetPath(), connection)
                        :   _PrioritizedOutput(
                                &sourceOutput, dependencyMask) ));
            }
        }

        // Otherwise, push the output onto the stack for immediate
        // processing.
        else {

            VdfMaskedOutput maskedOutput(&sourceOutput, dependencyMask);

            if (state->producePath) {
                state->stack.push_back(
                    _StackFrame(maskedOutput, frame.GetPath(), connection));
            } else {
                state->stack.push_back(_StackFrame(maskedOutput));
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
