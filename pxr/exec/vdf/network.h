//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_NETWORK_H
#define PXR_EXEC_VDF_NETWORK_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashset.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/span.h"

#include "tbb/concurrent_queue.h"
#include "tbb/concurrent_unordered_map.h"
#include "tbb/concurrent_vector.h"

#include <atomic>
#include <cstdint>
#include <vector>
#include <iosfwd>
#include <memory>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;

class VdfConnection;
class VdfInput;
class VdfObjectPtr;
class VdfMask;
class VdfMaskedOutput;
class VdfNode;
class VdfOutput;
class VdfSchedule;
class VdfPoolChainIndex;
class VdfPoolChainIndexer;
class Vdf_ExecNodeDebugName;
class Vdf_InputAndOutputSpecsRegistry;
class Vdf_ScheduleInvalidator;

typedef TfHashSet<const VdfOutput *, TfHash> VdfOutputPtrSet;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfNetwork
///
/// A VdfNetwork is a collection of VdfNodes and their connections.
///
class VdfNetwork
{
public:

    /// Constant for connection API to indicate to place the connection at
    /// the end.
    ///
    static const int AppendConnection = -1;

    /// Constructs an empty network.
    ///
    VDF_API
    VdfNetwork();

    /// Destructs the network and the nodes managed by it.
    ///
    VDF_API
    ~VdfNetwork();

    /// Retrieves a debug name for a given node
    ///
    VDF_API
    const std::string GetNodeDebugName(const VdfNode *node) const;

    /// Returns the number of indices currently available for outputs. Note
    /// that this is a high water mark, and that the number of outputs
    /// currently in the network may be less than the value returned.
    ///
    uint32_t GetOutputCapacity() const { 
        return _outputCapacity.load(std::memory_order_acquire);
    }

    /// Returns the number of entries currently available for nodes and 
    /// for which it is valid to call GetNode().  Note that some entries
    /// may be NULL.
    ///
    size_t GetNodeCapacity() const { return _nodes.size(); }

    /// Returns the number of nodes that are currently owned by the network.
    ///
    size_t GetNumOwnedNodes() const { 
        return _nodes.size() - _freeNodeIds.unsafe_size();
    }

    /// Returns the node at index \p i.
    ///
    const VdfNode *GetNode(size_t i) const {
        return i < _nodes.size() ? _nodes[i] : nullptr;
    }

    /// Returns the non-const node at index \p i.
    ///
    VdfNode *GetNode(size_t i) {
        return i < _nodes.size() ? _nodes[i] : nullptr;
    }

    /// Returns the node with id \p nodeId, if it exists.
    ///
    VDF_API
    const VdfNode *GetNodeById(const VdfId nodeId) const;

    /// Returns the non-const node with id \p nodeId, if it exists
    ///
    VDF_API
    VdfNode *GetNodeById(const VdfId nodeId);

    /// \name Edit
    ///
    /// Used by clients to edit networks, e.g., to perform incremental changes.
    ///
    /// @{

    /// \class EditMonitor
    ///
    /// An abstract class to monitor network edit operations.
    ///
    /// Note that "will" notification is sent before any edits are made.  "Did"
    /// notification is sent after the edit operation is completed.
    ///
    class VDF_API_TYPE EditMonitor
    {
    public:
        VDF_API
        virtual ~EditMonitor();

        /// Will be called before a network is to be cleared out.
        ///
        /// When clearing out a network all nodes and connections will be 
        /// deleted.  Note that we don't sent notices for them during the
        /// clear operation.
        ///
        virtual void WillClear() = 0;

        /// Will be called after a connection has been made.
        ///
        virtual void DidConnect(const VdfConnection *connection) = 0;

        /// Will be called after a node has been added to the network.
        ///
        virtual void DidAddNode(const VdfNode *node) = 0;

        /// Will be called before \p node is deleted.
        ///
        virtual void WillDelete(const VdfNode *node) = 0;

        /// Will be called before a connection is deleted.
        ///
        virtual void WillDelete(const VdfConnection *connection) = 0; 
    };

    /// Clears all nodes from the network.
    ///
    VDF_API
    void Clear();

    /// Connects the \p output to the given \p inputNode's input \p inputName 
    /// with \p mask.  If \p atIndex is >= 0 the connection will be placed at
    /// index \p atIndex on the target input.  Otherwise it will be appened
    /// at the end.
    ///
    VDF_API
    VdfConnection *Connect(
        VdfOutput     *output, 
        VdfNode       *inputNode,
        const TfToken &inputName,
        const VdfMask &mask,
        int            atIndex = AppendConnection);

    /// Connects the \p maskedOutput to the given \p inputNode's input
    /// \p inputName.  If \p atIndex is >= 0 the connection will be placed at
    /// index \p atIndex on the target input.  Otherwise it will be appened
    /// at the end.
    ///
    VDF_API
    VdfConnection *Connect(
        const VdfMaskedOutput &maskedOutput, 
        VdfNode               *inputNode,
        const TfToken         &inputName,
        int                    atIndex = AppendConnection);

    /// Deletes \p node from the network. 
    ///
    /// The node must have no inputs and no outputs connected.
    ///
    /// Note: Calling Delete() may change the index VdfNetwork assigns to each
    ///       VdfNode.
    ///
    /// Returns true, if the node has been deleted.
    ///
    VDF_API
    bool Delete(VdfNode *node);

    /// Deletes \p connection from the network. 
    ///
    VDF_API
    void Disconnect(VdfConnection *connection);

    /// Disconnects and deletes \p node. 
    ///
    /// Returns true, if the node has been deleted.
    ///
    VDF_API
    bool DisconnectAndDelete(VdfNode *node);

    /// Reorders all input connections for \p input according to the mapping
    /// defined by \p newToOldIndices.
    ///
    /// \param input
    /// The input with the input connections to be reordered.
    ///
    /// \param newToOldIndices
    /// For each index `i`, `newToOldIndices[i]` is the old connection index and
    /// `i` is the desired new connection index. The number of indices given
    /// must be the same as the number of input connections, each index must
    /// be a valid connection index, and the indices must be unique.
    ///
    VDF_API
    void ReorderInputConnections(
        VdfInput *input,
        const TfSpan<const VdfConnectionVector::size_type> &newToOldIndices);

    /// Registers an edit monitor for this network. The edit monitor needs to
    /// be persistent as long as the network is alive and will receive
    /// notifications for network edits.
    ///
    VDF_API
    void RegisterEditMonitor(EditMonitor *monitor);

    /// Unregisters an edit monitor for this network.
    ///
    VDF_API
    void UnregisterEditMonitor(EditMonitor *monitor);

    /// Returns the current edit version of the network. The version will
    /// be updated every time the network topology changes.
    ///
    /// Note, no assumptions shall be made about the absolute value returned
    /// from this function. It is merely guaranteed that if two version of the
    /// same VdfNetwork instance equal, no topological edits have been made!
    ///
    size_t GetVersion() const { 
        return _version.load(std::memory_order_acquire);
    }

    /// @}


    /// \name Statistics
    /// @{

    /// Prints useful statistics about the network to \p os.
    ///
    /// Returns the numer of nodes owned by this network.
    ///
    VDF_API
    size_t DumpStats(std::ostream &os) const;

    /// @}

    /// \name Pool Chain Index
    /// @{

    /// Returns the pool chain index from the pool chain indexer.
    ///
    VDF_API
    VdfPoolChainIndex GetPoolChainIndex(const VdfOutput &output) const;

    /// @}

private:

    friend class VdfNode;
    friend class VdfIsolatedSubnetwork;
    friend class VdfOutput;

    // Registers a name for a given node. 
    // 
    // Only VdfNode should call this function.
    //
    void _RegisterNodeDebugName(
        const VdfNode &node, VdfNodeDebugNameCallback &&callback);

    // Unregisters a name for a given node.  
    //
    // Should only be called by VdfNode's destructor.
    //
    void _UnregisterNodeDebugName(const VdfNode &node);

    // Adds a node to this network.
    //
    // Returns the index of node that was added.  Once a node is added to the
    // network, the network takes ownership.  Only VdfNode should call
    // this function.
    //
    void _AddNode(VdfNode *node);

    // Delete a node helper.
    //
    void _DeleteNode(VdfNode *node);

    // Removes the connection from the network, but does not delete it. Sends
    // out the deletion notification, as well as invalidates any structures
    // dependent on the connection state.
    //
    void _RemoveConnection(VdfConnection *connection);

    // Delete a connection helper function. Note that this method won't 
    // send out deletion notification.
    //
    void _DeleteConnection(VdfConnection *connection);

    // Removes \p node from the network, but doesn't delete it.  Uses \p monitor
    // to notify about any reindexing.
    //
    void _RemoveNode(VdfNode *node);

    // Informs the network that the affects mask for \p output has changed.
    //
    void _DidChangeAffectsMask(VdfOutput &output);
    
    // Returns an output id for a newly created output to use.
    //
    size_t _AcquireOutputId();

    // Releases an output id for use by a future output.
    //
    void _ReleaseOutputId(const VdfId id);

    // Increment the network edit version.
    //
    void _IncrementVersion();

    // \name Schedule Management
    //
    // These are methods that can only be called by VdfSchedule to register
    // an unregister itself from a network.  
    //
    // These methods are thread-safe and can safely be called from 
    // within execution.  This is needed for clients that schedule during
    // execution (e.g. speculation).
    //
    // @{

    friend class VdfSchedule;
    
    // Sets that a schedule is referencing this network.
    // 
    void _RegisterSchedule(VdfSchedule *schedule) const;

    // Clears the schedule from the list of schedules referencing this network.
    //
    void _UnregisterSchedule(VdfSchedule *schedule) const;

    // Returns a pointer to the schedule invalidator.
    //
    Vdf_ScheduleInvalidator* _GetScheduleInvalidator() const {
        return _scheduleInvalidator.get();
    }

    // @}

    // Returns the input/output specs registry for this network.
    //
    Vdf_InputAndOutputSpecsRegistry &_GetInputOutputSpecsRegistry() {
        return *_specsRegistry;
    }

    // The complete list of nodes managed by this network.
    tbb::concurrent_vector<VdfNode *> _nodes;

    // A queue of ids that are free to be assigned to new nodes.
    tbb::concurrent_queue<VdfId> _freeNodeIds;

    // The version with which to initialize new nodes
    VdfVersion _initialNodeVersion;

    // The number of output indices in this network.
    std::atomic<uint32_t> _outputCapacity;

    // The free output indices in the network.
    tbb::concurrent_queue<VdfId> _freeOutputIds;

    // The list of static edit monitors registered with this network.
    using _EditMonitorVector = TfSmallVector<EditMonitor *, 1>;
    _EditMonitorVector _monitors;

    // Helper class for invalidating schedules after topological changes.
    std::unique_ptr<Vdf_ScheduleInvalidator> _scheduleInvalidator;

    // Pool Chain Indexer
    std::unique_ptr<VdfPoolChainIndexer> _poolChainIndexer;

    // Input and output specs registry.
    std::unique_ptr<Vdf_InputAndOutputSpecsRegistry> _specsRegistry;

    // Edit version.
    std::atomic<size_t> _version;

    // Map from node to debug name.
    using _NodeDebugNamesMap = tbb::concurrent_unordered_map<
        VdfIndex,
        std::unique_ptr<Vdf_ExecNodeDebugName>, 
        TfHash>;

    _NodeDebugNamesMap _nodeDebugNames;
};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
