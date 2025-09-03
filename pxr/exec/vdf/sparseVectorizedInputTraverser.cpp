//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/sparseVectorizedInputTraverser.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/poolChainIndexer.h"

#include "pxr/base/tf/bits.h"
#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/hash.h"

PXR_NAMESPACE_OPEN_SCOPE

#define _TRAVERSAL_TRACING 0

const TfBits *
VdfSparseVectorizedInputTraverser::_MasksToRequestsMap::GetRequestBits(
    const VdfMask &mask) const
{
    // Note that it may look strange that we iterate over the map instead
    // of doing a lookup.  But remember, this is a TfDenseHashMap which would
    // iterate anyways for a lookup.
    
    TF_FOR_ALL(i, _maskToRequestBitsMap)
        if (i->first.Contains(mask))
            return &(i->second);
        
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////

void
VdfSparseVectorizedInputTraverser::Traverse(
    const VdfMaskedOutputVector &maskedOutputs,
    const NodeCallback          &nodeCallback,
    CallbackMode                callbackMode)
{
    // Set the node and connection callbacks.
    _nodeCallback = nodeCallback;
    _connectionCallback = nullptr;

    // Set the node callback mode.
    _callbackMode = callbackMode;

    // Start the traversal.
    _Traverse(maskedOutputs);
}

void
VdfSparseVectorizedInputTraverser::TraverseWithConnectionCallback(
    const VdfMaskedOutputVector &maskedOutputs,
    const ConnectionCallback    &connectionCallback)
{
    // Set the node and connection callbacks.
    _nodeCallback = nullptr;
    _connectionCallback = connectionCallback;

    // Set the node callback mode.
    _callbackMode = CallbackModeAllNodes;

    // Start the traversal.
    _Traverse(maskedOutputs);
}

void
VdfSparseVectorizedInputTraverser::_Traverse(
    const VdfMaskedOutputVector &maskedOutputs)
{
    TRACE_FUNCTION();

    // Early bail-out for empty request.
    if (maskedOutputs.empty()) 
        return;

    #if _TRAVERSAL_TRACING
        printf("> Traverse() starting with %zu maskedOutputs\n", maskedOutputs.size());
        TF_FOR_ALL(i, maskedOutputs)
            printf("  %p %s %s\n",
                i->GetOutput(),
                i->GetOutput()->GetDebugName().c_str(),
                i->GetMask().GetRLEString().c_str());
    #endif

    TfReset(_visitedConnections);
    TfReset(_stack);
    TfReset(_prioritizedOutputs);

    _emptyRequestToMaskMap = _MasksToRequestsMap(maskedOutputs.size());

    // Push the initial outputs with their _MasksToRequestsMap on the stack.
    for(size_t i=0; i<maskedOutputs.size(); i++) {

        const VdfMaskedOutput &maskedOutput = maskedOutputs[i];

        std::pair<_Stack::iterator, bool> res = 
            _stack.insert(
                std::make_pair(maskedOutput.GetOutput(), _emptyRequestToMaskMap));
            
        res.first->second.AddMask(maskedOutput.GetMask(), i);
    }

    // Loop while we've got work to do.
    while (!_stack.empty() || !_prioritizedOutputs.empty()) {

        while (!_stack.empty()) {

            // Get the next output to process.
            _Stack::iterator i = _stack.begin();

            // Process the output with its masks.
            _TraverseOutput(i->first, i->second);
            
            // Erase i after the call to _TraverseOutput so that we don't need
            // to copy masks.
            _stack.erase(i);
        }

        if (!_prioritizedOutputs.empty()) {
        
            // Pull the top output from the priority queue. This works, because
            // _PrioritizedOutputMap is a std::map and hence sorted.

            _PrioritizedOutputMap::iterator i = _prioritizedOutputs.begin();

            // Process the output.
            _TraverseOutput(i->second.first, i->second.second);

            // Erase i after the call to _TraverseOutput so that we don't need
            // to copy masks.
            _prioritizedOutputs.erase(i);
        }
    }

    // Make sure we don't hog memory if someone uses this traverser persistently.
    TF_VERIFY(_stack.empty() && _prioritizedOutputs.empty());
    TfReset(_visitedConnections);
}

void
VdfSparseVectorizedInputTraverser::_TraverseOutput(
    const VdfOutput           *output,
    const _MasksToRequestsMap &masks)
{
    #if _TRAVERSAL_TRACING
        size_t unique = 0;
        printf("\n> _TraverseOutput: %s, masks.GetNumUniqueMasks() = %zu\n",
            output->GetDebugName().c_str(), masks.GetNumUniqueMasks());
    #endif

    const VdfNode &node = output->GetNode();

    // If we have a node callback, call it and see if we should stop the
    // traversal.  Be sure to only call it if this node affects the requested
    // outputs.

    // If callback mode is CallbackModeTerminalNodes, then only invoke the
    // node callback on terminal nodes (i.e. on nodes without input
    // connections).

    bool invokeNodeCallback =
        _nodeCallback &&
        (_callbackMode != CallbackModeTerminalNodes ||
         !node.HasInputConnections());

    // Loop over the # of unique masks.
    TF_FOR_ALL(i, masks) {

        const VdfMask &mask        = i->first;
        const TfBits &requestBits = i->second;

        #if _TRAVERSAL_TRACING
            printf(" -processing unique %zu\n", unique++);
            printf("  mask = %s\n", mask.GetRLEString().c_str());
            printf("  requestBits = %s\n", requestBits.GetAsStringLeftToRight().c_str());
        #endif

        if (invokeNodeCallback) {
    
            bool execute = true;
    
            if (const VdfMask *affectsMask = output->GetAffectsMask())
                execute = affectsMask->Overlaps(mask);
    
            if (execute) {
    
                // Note that we can't stop the overall traversal here, but 
                // we can do it for the current unique mask.
                if (!_nodeCallback(node, requestBits))
                    continue;
            }
        }

        VdfMaskedOutput maskedOutput(const_cast<VdfOutput *>(output), mask);
    
        // Ask the node for the dependencies.
        VdfConnectionAndMaskVector dependencies =
            node.ComputeInputDependencyMasks(
                maskedOutput, false /* skipAssociatedInputs */);
    
        #if _TRAVERSAL_TRACING
            printf("  got %zu dependencies\n", dependencies.size());
        #endif
    
        TF_FOR_ALL(i, dependencies) {
    
            const VdfConnection *connection     = i->first;
            const VdfMask       &dependencyMask = i->second;
    
            #if _TRAVERSAL_TRACING
                printf("  looking at connection %s with dependencyMask %s\n",
                    connection->GetDebugName().c_str(),
                    dependencyMask.GetRLEString().c_str());
            #endif

            // If we have a connection callback, call it and see if we should
            // stop the traversal for this branch.
            if (_connectionCallback) {
                if (!_connectionCallback(*connection, requestBits))
                    continue;
            }
    
            // See if we have already visited this connection.
            _VisitedConnections::iterator vc =
                _visitedConnections.find(connection);
    
            // Skip this connection if its accumulated traversal mask for
            // 'connection' contains the current dependency mask.
            if (vc != _visitedConnections.end()) {

                // At this point, we have detected another path leading
                // to this connection.  This maybe another path or a cycle.

                _MasksToRequestsMap &seenMasks = vc->second;
                
                // We have visited this connection already already with the
                // given dependency mask for the given requestsBits we can skip.

                if (const TfBits *seenRequests =
                    seenMasks.GetRequestBits(dependencyMask)) {
                
                    if (seenRequests->Contains(requestBits))
                        continue;
                        
                    // Nope, either new dependency bits found or new reques
                    // bits. Re-traverse.
                    seenMasks.AddMask(dependencyMask, requestBits);
                }

            } else {

                // Update the visited connections map.
                _visitedConnections.insert(
                    std::make_pair(
                        connection,
                        _MasksToRequestsMap(dependencyMask, requestBits)));
            }
    
            // If this is a pool output, accumulate the mask in the associated
            // poolOutputs map, and don't traverse the output until we're done with
            // everything on the stack.
            //
            // XXX:speculation
            // I think it would be faster if VdfSpeculationNodes were handled
            // specially here.  As it currently stands, I think we can end up
            // with inefficient traversals because speculation nodes take us
            // back up to a higher point in the pool.  It'd be better if we
            // finished all pool traversal before processing speculation nodes,
            // because that will better vectorize the resulting traversal.
    
            const VdfOutput *sourceOutput = &connection->GetNonConstSourceOutput();

            // We can't add to sourceOutput because when this method is 
            // finished we will discard this entry, so modifying it won't
            // produce results.
            
            if (Vdf_IsPoolOutput(*sourceOutput)) {
                
                // The input traverser processes nodes further down the pool chain
                // first so the priorities need to be the opposite of those given
                // by the pool chain indexer. Hence, we use a std::greater for
                // the map.
    
                VdfPoolChainIndex poolIndex =
                    node.GetNetwork().GetPoolChainIndex(*sourceOutput);
    
                _PrioritizedOutputMap::iterator iter =
                    _prioritizedOutputs.find(poolIndex);
    
                #if _TRAVERSAL_TRACING
                    printf(
                        "  inserted dependencyMask %s, requestBits %s into pri "
                        "queue with pri %d = %s\n",
                        dependencyMask.GetRLEString().c_str(),
                        requestBits.GetAsStringLeftToRight().c_str(),
                        poolIndex,
                        sourceOutput->GetDebugName().c_str());
                #endif

                if (iter != _prioritizedOutputs.end()) {
    
                    // Make sure that poolIndex is computed consistently (ie.
                    // there is an unique, consistent index for each output).
                    TF_VERIFY(iter->second.first == sourceOutput);
    
                    // Extend this prioritized output.
                    iter->second.second.AddMask(dependencyMask, requestBits);
    
                } else {
    
                    // Insert this pool output into the priority queue along
                    // with the new vectorized dependencies.

                    _prioritizedOutputs.insert(std::make_pair(
                        poolIndex,
                        _PrioritizedOutput(
                            sourceOutput,
                            _MasksToRequestsMap(dependencyMask, requestBits))));
                }
    
            } else {
    
                // Otherwise, push the output onto the stack for immediate
                // processing.
    
                #if _TRAVERSAL_TRACING
                    printf(
                        "  inserted dependencyMask %s, requestBits %s into "
                        "normal queue %s\n",
                        dependencyMask.GetRLEString().c_str(),
                        requestBits.GetAsStringLeftToRight().c_str(),
                        sourceOutput->GetDebugName().c_str());
                #endif

                // Here we will merge with any pending traversal.
                std::pair<_Stack::iterator, bool> res = 
                    _stack.insert(
                        std::make_pair(sourceOutput, _emptyRequestToMaskMap));
    
                // here we are adding the new dependencyMask to either a new (or pending)
                // element.  if it is already pending we need to merge the masks
                    
                res.first->second.AddMask(dependencyMask, requestBits);
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
