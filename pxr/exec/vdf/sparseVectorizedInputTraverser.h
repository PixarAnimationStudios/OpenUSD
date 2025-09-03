//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SPARSE_VECTORIZED_INPUT_TRAVERSER_H
#define PXR_EXEC_VDF_SPARSE_VECTORIZED_INPUT_TRAVERSER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/maskedOutputVector.h"
#include "pxr/exec/vdf/object.h"
#include "pxr/exec/vdf/poolChainIndex.h"
#include "pxr/exec/vdf/sparseOutputTraverser.h"

#include "pxr/base/tf/denseHashMap.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/stl.h"

#include <functional>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNode;

/// \class VdfSparseVectorizedInputTraverser
///
/// \brief A class used for fast sparse traversals of VdfNetworks in the
///        output-to-input direction in a vectorized manner.
/// 
///        A sparse traversal takes affects masks into account and avoids
///        traversing nodes that don't have an affect on the outputs
///        requested for the traversal.  This is most often useful for
///        dependency traversals.
///
///        In contrast, VdfIsTopologicalSourceNode() does a full topological
///        traversal.
///
class VdfSparseVectorizedInputTraverser
{
public:

    /// Callback mode for the node callback.
    enum CallbackMode
    {
        /// Invoke the node callback on all inputs. This is the default.
        CallbackModeAllNodes,

        /// Invoke the node callback only on terminal nodes.
        CallbackModeTerminalNodes
    };

    /// \name Basic Traversal
    /// @{

    /// Callback used when traversing a network.
    ///
    /// Called for each node that is visited that affects values of the initial
    /// requests.  The TfBits parameter is used to identify which requests
    /// caused the callback to be called.
    ///
    /// A return value of false halts traversal locally but allows prior 
    /// branches of traversal to continue.
    ///
    using NodeCallback = std::function<
        bool (const VdfNode &, const TfBits &)>;

    /// Traverses the network in the input direction, starting from the
    /// masked outputs in \p sharedMaskedOutputs.
    ///
    /// Calls \p nodeCallback for each node visited in the sparse
    /// traversal.
    ///
    /// If \p callbackMode is set to CallbackModeTerminalNodes, then the
    /// \p nodeCallback is only invoked on terminal nodes (i.e. nodes without
    /// input connections). If it is set to CallbackModeAllNodes (which is the
    /// default), then the callback is invoked on all nodes that are visited
    /// by the traverser.
    ///
    /// If the callback returns \c false, then traversal halts locally
    /// but prior branches of traversal continue.
    ///
    VDF_API
    void Traverse(
        const VdfMaskedOutputVector &sharedMaskedOutputs,
        const NodeCallback          &nodeCallback,
        CallbackMode                callbackMode);

    /// Callback used when traversing a network.
    ///
    /// Called for each connection that is visited that affects values of the
    /// initial requests.  The TfBits parameter is used to identify which
    /// requests caused the callback to be called.
    ///
    /// A return value of false halts traversal locally but allows prior 
    /// branches of traversal to continue.
    ///
    using ConnectionCallback = std::function<
        bool (const VdfConnection &, const TfBits &)>;

    /// Traverses the network in the input direction, starting from the
    /// masked outputs in \p sharedMaskedOutputs.
    ///
    /// Calls \p connectionCallback for each connection visited in the sparse
    /// traversal.
    ///
    /// If the callback returns \c false, then traversal halts locally
    /// but prior branches of traversal continue.
    ///
    VDF_API
    void TraverseWithConnectionCallback(
        const VdfMaskedOutputVector &sharedMaskedOutputs,
        const ConnectionCallback    &connectionCallback);

    /// @}

private:

    // Helper class that holds a set of unique masks along with their request
    // bits.
    class _MasksToRequestsMap
    {
        // Map of unique masks to request indices using them.
        typedef
            TfDenseHashMap<VdfMask, TfBits, VdfMask::HashFunctor>
            _MaskToRequestBitsMap;

    public:
    
        // Ctor to initialize an empty object.
        _MasksToRequestsMap(size_t numRequests = 0)
        :   _numRequests(numRequests) {}
        
        // Ctor to initialize /w a single \p mask and \p requestBits.
        _MasksToRequestsMap(const VdfMask &mask, const TfBits &requestBits)
        :   _numRequests(requestBits.GetSize()) {
            _maskToRequestBitsMap[mask] = requestBits;
        }
        
        // Adds \p mask @ \p requestIndex.
        void AddMask(const VdfMask &mask, size_t requestIndex) {

            static TfBits empty;    
    
            std::pair<_MaskToRequestBitsMap::iterator, bool> res =
                _maskToRequestBitsMap.insert(std::make_pair(mask, empty));
                
            if (res.second) {
                res.first->second.Resize(_numRequests);
                res.first->second.ClearAll();
            }
            
            TF_VERIFY(!res.first->second.IsSet(requestIndex));
            res.first->second.Set(requestIndex);
        }
        
        // Adds \p mask with \p requestBits.
        void AddMask(const VdfMask &mask, const TfBits &requestBits) {

            std::pair<_MaskToRequestBitsMap::iterator, bool> res =
                _maskToRequestBitsMap.insert(std::make_pair(mask, requestBits));
                
            // If we didn't succeed to insert mask as a new entry, we must merge
            // in our new requestBits.
            
            if (!res.second)
                res.first->second |= requestBits;
        }
        
        // Iteration support.
        typedef _MaskToRequestBitsMap::const_iterator const_iterator;
        
        const_iterator begin() const {
            return _maskToRequestBitsMap.begin();
        }
        
        const_iterator end() const {
            return _maskToRequestBitsMap.end();
        }
        
        // Returns the request bits for \p mask.  Note that mask doesn't need
        // to be an exact match.
        const TfBits *GetRequestBits(const VdfMask &mask) const;
    
    private:
    
        size_t _numRequests;
    
        _MaskToRequestBitsMap _maskToRequestBitsMap;
    };

    // Helper to kick off the traversal.
    void _Traverse(const VdfMaskedOutputVector &sharedMaskedOutputs);

    // Helper to traverse an output.
    void _TraverseOutput(
        const VdfOutput           *output,
        const _MasksToRequestsMap &masks);

private:

    // The callback to use.
    NodeCallback _nodeCallback;
    ConnectionCallback _connectionCallback;
    
    // The current callback mode.
    CallbackMode _callbackMode;
    
    // Type used to identify the masks/request-bits that have already been
    // visited for traversed connections.  Note that we can't bunch together
    // all seen dependency bits along all seen request bits, because we could
    // have say two cycles through a single connection.  The first cycle would
    // manage to set all dependency bits there are and when the second cycle
    // for different request bits visits the connection the second time (since
    // there are two cycles) we would believe we would have seen that second
    // request with the second dependency mask already.

    typedef
        TfHashMap<const VdfConnection *, _MasksToRequestsMap, TfHash>
        _VisitedConnections;

    _VisitedConnections _visitedConnections;
    
    // The traversal stack frames, used as the stack. We are using an
    // unordered_map, because begin() will be called frequently and entries
    // will be erased from the front of the map.
    typedef
        std::unordered_map<const VdfOutput *, _MasksToRequestsMap, TfHash>
        _Stack;

    _Stack _stack;
    
    // A type used to represent an input in a priority queue.
    typedef std::pair<const VdfOutput *, _MasksToRequestsMap> _PrioritizedOutput;

    // A map from pool chain index to prioritized output, used to ensure that we
    // process outputs in their order in the pool chain.
    //
    // Note that using a std::map<> gives us the _PrioritizedOutputs sorted by
    // the pool chain index (the int key).
    //
    typedef std::map<VdfPoolChainIndex, _PrioritizedOutput,
                     std::greater<VdfPoolChainIndex> > _PrioritizedOutputMap;

    _PrioritizedOutputMap _prioritizedOutputs;
    
    // Initialized, empty _MasksToRequestsMap.
    _MasksToRequestsMap _emptyRequestToMaskMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
