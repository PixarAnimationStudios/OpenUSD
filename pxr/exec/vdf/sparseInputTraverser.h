//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SPARSE_INPUT_TRAVERSER_H
#define PXR_EXEC_VDF_SPARSE_INPUT_TRAVERSER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/object.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/maskedOutputVector.h"
#include "pxr/exec/vdf/poolChainIndex.h"

#include "pxr/base/tf/hashmap.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

class VdfConnection;
class VdfNode;

/// \class VdfSparseInputTraverser
///
/// \brief A class used for fast sparse traversals of VdfNetworks in the
///        output-to-input direction.
/// 
///        A sparse traversal takes affects masks into account and avoids
///        traversing nodes that don't have an affect on the outputs
///        requested for the traversal.  This is most often useful for
///        dependency traversals.
///
///        In contrast, VdfIsTopologicalSourceNode() does a full topological
///        traversal.
///
class VdfSparseInputTraverser
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
    /// masked outputs.
    ///
    /// A return value of false halts traversal locally but allows prior 
    /// branches of traversal to continue.
    ///
    using NodeCallback = std::function<bool (const VdfNode &)>;

    /// Traverses the network in the input direction, starting from the
    /// masked outputs in \p outputs.
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
    static void Traverse(
        const VdfMaskedOutputVector &outputs,
        const NodeCallback          &nodeCallback,
        CallbackMode                callbackMode=CallbackModeAllNodes);

    /// Callback used when traversing a network.
    ///
    /// Called for each connection and dependency mask that is visited while
    /// traversing nodes that affect values of the initial masked outputs.
    ///
    using ConnectionCallback = std::function<
        bool (const VdfConnection &, const VdfMask &)> ;

    /// Traverses the network in the input direction, starting from the
    /// masked outputs in \p outputs.  The traversal is identical to the one
    /// provided by Traverse(), except this method calls a connection callback
    /// instad of a node callback.
    ///
    /// Calls \p connectionCallback for each connection visited in the sparse
    /// traversal.
    ///
    /// If the callback returns \c false, then traversal along the supplied
    /// connection stops, and traversal along sibling connections continues.
    ///
    VDF_API
    static void TraverseWithConnectionCallback(
        const VdfMaskedOutputVector &outputs,
        const ConnectionCallback &connectionCallback);

    /// @}


    /// \name Traversal with Path Reporting
    /// @{

    /// Callback used when traversing a network with path information.
    ///
    /// Called for each node that is visited in the sparse traversal.
    ///
    /// The path to the visited node from the start is given by \p path and it
    /// only contains nodes that have an affect on the requested outputs.
    ///
    using NodePathCallback = std::function<
        bool (const VdfNode &node, const VdfObjectPtrVector &path)>;

    /// Callback used when traversing a network.
    ///
    /// Called for each connection and dependency mask that is visited that
    /// affects values of the initial masked outputs.  Note that the currently
    /// visited connection isn't appended to the path yet.
    ///
    using ConnectionPathCallback = std::function<
        bool (const VdfConnection &, const VdfMask &,
            const VdfObjectPtrVector &)>;

    /// Traverses the network in the input direction, starting from the
    /// masked outputs in \p outputs, providing the traversal path to each
    /// invocation of \p nodePathCallback.
    ///
    /// Calls \p nodePathCallback (if specified) for each node visited in the
    /// sparse traversal.  A sparse traversal only visits nodes that have an 
    /// affect on the requested outputs.
    ///
    /// Calls \p connectionCallback (if specified) for each node visited in the
    /// sparse traversal.  A sparse traversal only visits nodes that have an 
    /// affect on the requested outputs.
    ///
    /// If \p callbackMode is set to CallbackModeTerminalNodes, then
    /// the \p nodeCallback is only invoked on terminal nodes (i.e.
    /// nodes without input connections). If it is set to
    /// CallbackModeAllNodes (which is the default), then the
    /// callback is invoked on all nodes that are visited by the
    /// traverser.
    ///
    VDF_API
    static void TraverseWithPath(
        const VdfMaskedOutputVector  &outputs,
        const NodePathCallback       &nodePathCallback,
        const ConnectionPathCallback &connectionPathCallback,
        CallbackMode                 callbackMode=CallbackModeAllNodes);

    /// @}

private:

    // A type used to represent an input in a priority queue.
    class _PrioritizedOutput;

    // A map from pool chain index to prioritized output, used to ensure that we
    // process outputs in their order in the pool chain.
    //
    // Note that using a std::map<> gives us the _PrioritizedOutputs sorted by
    // the pool chain index.
    //
    typedef std::map<VdfPoolChainIndex, _PrioritizedOutput,
                     std::greater<VdfPoolChainIndex> > _PrioritizedOutputMap;

    // An individual stack frame in the traversal state.
    class _StackFrame;

    // Type used to identify the masks that have already been visited for
    // traversed connections.
    typedef TfHashMap<const VdfConnection *, VdfMask::Bits, TfHash>
        _VisitedConnections;
    
    // This struct embodies the total state of a sparse traversal.
    struct _TraversalState;

    // Helper to initialize a traversal.
    static void _InitTraversal(
        const VdfMaskedOutputVector &outputs,
        _TraversalState             *state,
        const CallbackMode          callbackMode=CallbackModeAllNodes);

    // Helper to traverse an output.
    static void _TraverseOutput(
        _TraversalState     *state,
        const _StackFrame    &frame,
        const CallbackMode  callbackMode);

    // Adapter that takes a NodeCallback and acts like a NodePathCallback by
    // ignoring the path.
    //
    // XXX:optimization
    // Using this means we do 2 std function calls for each call to the client
    // provided callback.  That's avoidable if we factor the code differently.
    //
    static bool _NodePathCallbackAdapter(
        const NodeCallback       &nodeCallback,
        const VdfNode            &node,
        const VdfObjectPtrVector &)
    {
        return nodeCallback(node);
    }

    // Adapter that takes a ConnectionCallback and acts like a ConnectionPathCallback
    // by ignoring the path.
    //
    static bool _ConnectionPathCallbackAdapter(
        const ConnectionCallback &connectionCallback,
        const VdfConnection      &connection,
        const VdfMask            &dependencyMask,
        const VdfObjectPtrVector &)
    {
        return connectionCallback(connection, dependencyMask);
    }

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
