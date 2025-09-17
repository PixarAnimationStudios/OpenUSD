//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_TEST_UTILS_H
#define PXR_EXEC_VDF_TEST_UTILS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/dataManagerVector.h"
#include "pxr/exec/vdf/executionStats.h"
#include "pxr/exec/vdf/executionStatsProcessor.h"
#include "pxr/exec/vdf/inputVector.h"
#include "pxr/exec/vdf/iterator.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/parallelDataManagerVector.h"
#include "pxr/exec/vdf/parallelSpeculationExecutorEngine.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/speculationExecutor.h"
#include "pxr/exec/vdf/speculationExecutorEngine.h"
#include "pxr/exec/vdf/simpleExecutor.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"

#include <functional>
#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class VdfMask;
class VdfNode;
class VdfSpeculationNode;

////////////////////////////////////////////////////////////////////////////////
///
/// This file contains classes to facilitate network creation in unit tests.
///
/// Simple example of how to use these classes:
///
/// \code
///     VdfTestUtils::Network graph;   // The container for the nodes.
///     VdfTestUtils::CallbackNodeType outType; // The consumer node type.
///
///     // Consumer nodes read and write ints.
///     outType.ReadWrite<int>(_tokens->moves, _tokens->moves); 
///
///     graph.AddInputVector<int>("input", 3);  // Add an input node.
///     graph.Add("consume", outType);          // Add a consumer node.
///
///     graph["input"]                          // Add a few input values to
///         .SetValue(0, 11)                    // the input node.
///         .SetValue(1, 22)
///         .SetValue(2, 33)
///         ;
///
///     // Finally, connect node "input"'s default output to "consume's" moves
///     // input with a mask.
///
///     graph["input"] >>                    
///         graph["consume"].In(_tokens->moves, VdfMask::AllOnes(3));
/// 
/// \endcode
/// 
///

namespace VdfTestUtils 
{

////////////////////////////////////////////////////////////////////////////////
/// \class CallbackNode
///
/// \brief A helper class that implements a simple callback node.
///
class VDF_API_TYPE CallbackNode : public VdfNode
{
public:

    using ValueFunction = void(const VdfContext &context);

    CallbackNode(
        VdfNetwork *network,
        const VdfInputSpecs &inputSpecs,
        const VdfOutputSpecs &outputSpecs,
        ValueFunction *const cb)
    :   VdfNode(network, inputSpecs, outputSpecs),
        _cb(cb) {}

    virtual void Compute(const VdfContext &context) const override {
        (*_cb)(context);
    }

protected:

    virtual ~CallbackNode() {}

    virtual bool _IsDerivedEqual(const VdfNode &rhs) const override {
        return false;
    }

private:

    ValueFunction *const _cb;
};

////////////////////////////////////////////////////////////////////////////////
/// \class OutputAccessor
///
/// \brief A helper class which enables access to a VdfOutput from a VdfContext.
///
class OutputAccessor : public VdfIterator
{
public:
    OutputAccessor(const VdfContext &context) : _context(context) {}

    const VdfOutput *GetOutput() const {
        return _GetNode(_context).GetOutput();
    }

private:
    const VdfContext &_context;

};

////////////////////////////////////////////////////////////////////////////////
/// \class DependencyCallbackNode
///
/// \brief A CallbackNode which allows for passing in a custom input / output
/// dependency callback.
///
class VDF_API_TYPE DependencyCallbackNode : public CallbackNode
{
public:
    using InputDependencyFunction = std::function<
        VdfMask::Bits (const VdfMaskedOutput &maskedOutput,
              const VdfConnection &inputConnection)>;

    using OutputDependencyFunction = std::function<
        VdfMask (const VdfConnection &inputConnection,
              const VdfMask &inputDependencyMask,
              const VdfOutput &output)>;

    DependencyCallbackNode(
        VdfNetwork                      *network,
        const VdfInputSpecs             &inputSpecs,
        const VdfOutputSpecs            &outputSpecs,
        ValueFunction                   *function,
        const InputDependencyFunction   &inputDependencyFunction,
        const OutputDependencyFunction  &outputDependencyFunction) :
        CallbackNode(network, inputSpecs, outputSpecs, function),
        _inputDependencyFunction(inputDependencyFunction),
        _outputDependencyFunction(outputDependencyFunction) {
    }

protected:
    virtual VdfMask::Bits _ComputeInputDependencyMask(
        const VdfMaskedOutput &maskedOutput,
        const VdfConnection &inputConnection) const {
        if (_inputDependencyFunction) {
            return _inputDependencyFunction(
                maskedOutput, inputConnection); 
        } else {
            return VdfNode::_ComputeInputDependencyMask(
                maskedOutput, inputConnection);
        }
    }

    virtual VdfMask _ComputeOutputDependencyMask(
        const VdfConnection &inputConnection,
        const VdfMask &inputDependencyMask,
        const VdfOutput &output) const {
        if (_outputDependencyFunction) {
            return _outputDependencyFunction(
                inputConnection, inputDependencyMask, output);
        } else {
            return VdfNode::_ComputeOutputDependencyMask(
                inputConnection, inputDependencyMask, output);
        }
    }

private:
    InputDependencyFunction _inputDependencyFunction;
    OutputDependencyFunction _outputDependencyFunction;

};

////////////////////////////////////////////////////////////////////////////////
/// \class NodeType
///
/// \brief Base class for various kinds of nodes that can be created.
///
class VDF_API_TYPE NodeType 
{
public:
    VDF_API
    virtual ~NodeType();
    virtual VdfNode *NewNode(VdfNetwork *net) const = 0;
};


////////////////////////////////////////////////////////////////////////////////
/// \class InputNodeType
///
/// \brief This class specifies a VdfInputVector of type T
///
template<typename T>
class InputNodeType  : public NodeType
{
public:
    InputNodeType(size_t size) : _size(size) {}

    /// Creates a VdfInputVector<T>.
    ///
    virtual VdfNode *NewNode(VdfNetwork *net) const
    {
        return new VdfInputVector<T>(net, _size);
    }

private:

    const size_t _size;

};

////////////////////////////////////////////////////////////////////////////////
/// \class CallbackNodeType
///
/// \brief This class specifies a CallbackNode with a given callback function.
///
class CallbackNodeType : public NodeType
{
public:

    /// Creates a callback node type with callback function \p function.
    ///
    CallbackNodeType(CallbackNode::ValueFunction *function) :
        _function(function) {
    }

    /// Creates a CallbackNode from this node type.
    ///
    virtual VdfNode *NewNode(VdfNetwork *net) const
    {
        return new DependencyCallbackNode(
            net, _inputSpecs, _outputSpecs, 
            _function, _inputDependencyFunction, _outputDependencyFunction);
    }

    /// Adds a ReadConnector to this node type.
    ///
    template <typename T>
    CallbackNodeType &Read(const TfToken &name) {
        _inputSpecs.ReadConnector<T>(name);
        return *this;
    }

    /// Adds a ReadWrite input and an associated Output to this node type.
    ///
    template<typename T>
    CallbackNodeType &ReadWrite(const TfToken &name, const TfToken &outName) {
        _inputSpecs.ReadWriteConnector<T>(name, outName);
        _outputSpecs.Connector<T>(outName);
        return *this;
    }

    /// Adds an output to this node type.
    ///
    template<typename T>
    CallbackNodeType &Out(const TfToken &name) {
        _outputSpecs.Connector<T>(name);
        return *this;
    }

    /// Sets an input dependency mask computation callback for this
    /// node type.
    ///
    CallbackNodeType &ComputeInputDependencyMaskCallback(
        const DependencyCallbackNode::InputDependencyFunction &function) {
        _inputDependencyFunction = function;
        return *this;
    }

    /// Sets an output dependency mask computation callback for this
    /// node type.
    ///
    CallbackNodeType &ComputeOutputDependencyMaskCallback(
        const DependencyCallbackNode::OutputDependencyFunction &function) {
        _outputDependencyFunction = function;
        return *this;
    }

private:

    VdfInputSpecs _inputSpecs;
    VdfOutputSpecs _outputSpecs;

    CallbackNode::ValueFunction *_function;
    DependencyCallbackNode::InputDependencyFunction _inputDependencyFunction;
    DependencyCallbackNode::OutputDependencyFunction _outputDependencyFunction;
};


////////////////////////////////////////////////////////////////////////////////
/// \class Node
///
/// \brief This class is a wrapper around a VdfNode.
///

class Node 
{
private:

    // These are private helper classes to support connection creation.

    // Represents a node's input.
    class _NodeInput
    {
    public:
        VdfNode *inputNode;
        TfToken  inputName;
        VdfMask  inputMask;
    };

    // Represents a node's output.
    class _NodeOutput
    {
    public:

        VDF_API
        Node &operator>>(const _NodeInput &rhs);

        Node    *owner;
        TfToken  outputName;
    };


public:

    /// Operator used to connect the default output of this node to 
    /// the input described by \p rhs.
    ///
    VDF_API
    Node &operator>>(const _NodeInput &rhs);

    /// Returns an input to this node that can be connected to an output.
    ///
    VDF_API
    _NodeInput In(const TfToken &inputName, const VdfMask &inputMask);

    /// Returns an output to this node that can be connected to an input.
    ///
    VDF_API
    _NodeOutput Output(const TfToken &outputName);

    /// Set a value on this node.  Assumes it is an input node.
    ///
    /// Note: you'll get a crash if this node isn't an input vector.
    ///
    template<typename T>
    Node &SetValue(int index, const T &val) {
        dynamic_cast<VdfInputVector<T> *>(_vdfNode)->SetValue(index, val);
        return *this;
    }

    /// Returns a pointer to the underlying VdfNode.
    ///
    operator VdfNode *() { return GetVdfNode(); }

    VdfNode *GetVdfNode() { return _vdfNode; }

    const VdfNode *GetVdfNode() const { return _vdfNode; }

    VdfOutput *GetOutput() const { return _vdfNode->GetOutput(); }

private:

    // Helper method to actually do the connection.
    //
    void _Connect(const _NodeInput &rhs, VdfOutput *output);

    friend class Network;
    friend class _NodeOutput;

    // The network for connection purposes.
    VdfNetwork *_network;

    // The underlying VdfNode that we represent.
    VdfNode *_vdfNode;
};


////////////////////////////////////////////////////////////////////////////////
/// \class Network
///
/// \brief This is a container class used to hold on to all the nodes and
/// to facilitate their management.
///
class Network
{

private:
    
    ////////////////////////////////////////////////////////////////////////////////
    ///
    /// \class _EditMonitor
    ///
    /// An EditMonitor used to track node actions
    ///
    class VDF_API_TYPE _EditMonitor : public VdfNetwork::EditMonitor
    {    
    public:

        /// Constructs an _EditMonitor for \p network
        ///
        _EditMonitor(Network *network) : _network(network) {}
        
        VDF_API
        ~_EditMonitor();
    
        /// \name Implemented Edit Monitor Events
        /// @{
    
        /// Ensures that the VdfTestUtils::Node corresponding to the deleted
        /// VdfNode \p node is also deleted from the VdfTestUtils::Network
        ///
        VDF_API
        void WillDelete(const VdfNode *node) override;

        /// Ensures that all VdfTestUtils::Nodes are deleted from the
        /// VdfTestUtils::Network
        ///
        VDF_API
        void WillClear() override;
        /// @}

        /// \name Unimplemented Edit Monitor Events
        /// @{
        void DidConnect(const VdfConnection *connection) override {}
        void DidAddNode(const VdfNode *node) override {}
        void WillDelete(const VdfConnection *connection) override {}
        /// @}


    private:
        Network *_network;
    };


public:

    Network() : _editMonitor(this) {
        _network.RegisterEditMonitor(&_editMonitor);
    }

    ~Network() {
        _network.UnregisterEditMonitor(&_editMonitor);
    }

    /// Creates a node named \p nodeName of type \p nodeType.
    ///
    /// Note that \p nodeName will be the debug name of the created node.
    /// 
    /// Note also that there is no error checking of whether or not this 
    /// nodeName has already been used, and the new node will simply overwrite
    /// the old one.
    ///
    VDF_API
    void Add(const std::string &nodeName, const NodeType &nodeType);

    /// Takes ownership of a \p customNode that was created externally.
    ///
    /// It must have been created with this network's VdfNetwork though.
    ///
    VDF_API
    void Add(const std::string &nodeName, VdfNode *customNode);

    /// Creates an input vector of type T named \p nodeName.
    ///
    template <typename T>
    void AddInputVector(const std::string &nodeName, size_t size = 1) {
        Add(nodeName, InputNodeType<T>(size));
    }

    /// Returns a reference to a node named \p nodeName.
    ///
    VDF_API
    Node &operator[](const std::string &nodeName);

    /// Returns a const reference to a node named \p nodeName.
    ///
    VDF_API
    const Node &operator[](const std::string &nodeName) const;

    /// Returns the node name for the VdfTestUtils::Node corresponding to
    /// a VdfNode with VdfId \p nodeId
    ///
    VDF_API
    const std::string GetNodeName(const VdfId nodeId);

    /// Returns a pointer to a connection named \p connectionName. The syntax
    /// for connectionName is:
    ///
    /// srcNode:connector -> tgtNode:connector
    ///
    /// If there is exactly one input or output connector only, you can also
    /// write:
    ///
    /// srcNode -> tgtNode:connector
    ///
    VDF_API
    VdfConnection *GetConnection(const std::string &connectionName);

    /// Returns a reference to the underlying VdfNetwork.
    ///
    VdfNetwork &GetNetwork() { return _network; }

    /// Returns a const reference to the underlying VdfNetwork.
    ///
    const VdfNetwork &GetNetwork() const { return _network; }

private:

    // Node class needs to publish _connections;
    friend class Node;

    // Nodes that have been created, indexed by their name.
    typedef TfHashMap<std::string, Node, TfHash> _StringToNodeMap;
    _StringToNodeMap _nodes;

    // The network that will contain the VdfNodes we create.
    VdfNetwork _network;

    // An EditMonitor that allows us to keep the Network in sync when Nodes
    // are deleted
    _EditMonitor _editMonitor;
};


////////////////////////////////////////////////////////////////////////////////
/// \class ExecutionStatsProcessor
///
/// \brief Simple processor that processor ExecutionStats into a vector of 
///        vector of events and a vector of substats that mirrors the internal
///        structure of ExecutionStats.
///


class VDF_API_TYPE ExecutionStatsProcessor : public VdfExecutionStatsProcessor {
public:

    /// Constructor.
    ///
    ExecutionStatsProcessor() : VdfExecutionStatsProcessor() {}

    /// Destructor.
    ///
    VDF_API
    ~ExecutionStatsProcessor();

    typedef
        std::unordered_map<
            ExecutionStatsProcessor::ThreadId,
            std::vector<VdfExecutionStats::Event>>
        ThreadToEvents;

    ThreadToEvents events;
    std::vector<ExecutionStatsProcessor*> subStats;

protected:
    /// Virtual method implementing process event for processing.
    ///
    VDF_API
    void _ProcessEvent(
        VdfExecutionStatsProcessor::ThreadId threadId, 
        const VdfExecutionStats::Event& event) override;

    /// Virtual method implementing process sub stat for processing.
    ///
    VDF_API
    void _ProcessSubStat(const VdfExecutionStats* stats) override;
};

////////////////////////////////////////////////////////////////////////////////
/// \class ExecutionStats
///
/// \brief Simple wrapper around ExecutionStats that allows for logging 
///        arbitrary data for testing.
///

class ExecutionStats {
public:
    
    /// Constructor.
    ///
    ExecutionStats() : _stats(std::make_unique<_ExecutionStats>()) {}

    //// Destructor.
    ///
    ~ExecutionStats() {}

    /// Public log function.
    ///
    VDF_API
    void Log(VdfExecutionStats::EventType event, VdfId nodeId, uint64_t data);

    /// Public log begin function.
    ///
    VDF_API
    void LogBegin(
        VdfExecutionStats::EventType event,
        VdfId nodeId,
        uint64_t data);

    /// Public log end function.
    ///
    VDF_API
    void LogEnd(
        VdfExecutionStats::EventType event,
        VdfId nodeId,
        uint64_t data);

    /// Processes the processor using the internally held stats.
    ///
    VDF_API
    void GetProcessedStats(VdfExecutionStatsProcessor* processor) const;

    /// Adds a sub stat to the internally held ExecutionStats.
    ///
    VDF_API
    void AddSubStat(VdfId nodeId);

private:
    /// Sub-classed ExecutionStats that calls directly to log to bypass
    /// needing a node.
    ///
    class _ExecutionStats : public VdfExecutionStats {
    public:
        _ExecutionStats() : VdfExecutionStats(&_network) {}
        ~_ExecutionStats() {}

        void Log(
            VdfExecutionStats::EventType event, 
            VdfId nodeId, 
            uint64_t data);

        void LogBegin(
            VdfExecutionStats::EventType event,
            VdfId nodeId,
            uint64_t data);

        void LogEnd(
            VdfExecutionStats::EventType event,
            VdfId nodeId,
            uint64_t data);

        void AddSubStat(VdfId nodeId);

    private:
        VdfNetwork _network;
    };

private:
    std::unique_ptr<_ExecutionStats> _stats;
};

////////////////////////////////////////////////////////////////////////////////
///
/// Create a new test speculation executor.
///
inline std::unique_ptr<VdfSpeculationExecutorBase>
CreateSpeculationExecutor(
    const VdfSpeculationNode *speculationNode,
    const VdfExecutorInterface *parentExecutor) {
    // Multi-threaded executor.
    if (VdfIsParallelEvaluationEnabled()) {
        return std::unique_ptr<VdfSpeculationExecutorBase>(
            new VdfSpeculationExecutor<
                VdfParallelSpeculationExecutorEngine,
                VdfParallelDataManagerVector>(
                    speculationNode, parentExecutor));
    } 

    // Single-threaded executor.
    return std::unique_ptr<VdfSpeculationExecutorBase>(
        new VdfSpeculationExecutor<
            VdfSpeculationExecutorEngine,
            VdfDataManagerVector<VdfDataManagerDeallocationMode::Background> >(
                speculationNode, parentExecutor));
}

////////////////////////////////////////////////////////////////////////////////

} // namespace VdfTestUtils

PXR_NAMESPACE_CLOSE_SCOPE

#endif
