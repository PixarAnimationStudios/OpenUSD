//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_NODE_H
#define PXR_EXEC_VDF_NODE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/connectorSpecs.h"
#include "pxr/exec/vdf/input.h"
#include "pxr/exec/vdf/inputAndOutputSpecs.h"
#include "pxr/exec/vdf/linearMap.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/output.h"
#include "pxr/exec/vdf/request.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/functionLite.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/token.h"

#include <functional>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfContext;
class VdfExecutorInterface;
class VdfMaskedOutput;
class VdfNetwork;
class VdfRequiredInputsPredicate;
class VdfVector;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfNode
///
/// This is the base class for all nodes in a VdfNetwork.
///
class VDF_API_TYPE VdfNode 
{
private:

    // Map of tokens to output connectors.
    //
    typedef VdfLinearMap<TfToken, VdfOutput *> _TokenOutputMap;

    // Map of tokens to input connectors.
    //
    typedef VdfLinearMap<TfToken, VdfInput *> _TokenInputMap;

public:

    TF_MALLOC_TAG_NEW("Vdf", "new VdfNode");

    // This little class allows clients to use TF_FOR_ALL to iterate
    // through the input connectors.
    //
    class InputMapIterator {
    public:
        typedef _TokenInputMap::const_iterator const_iterator;
        typedef _TokenInputMap::const_reverse_iterator const_reverse_iterator;

        InputMapIterator(const _TokenInputMap &map) : _map(map) {}
        const_iterator begin() const { return _map.begin(); }
        const_iterator end() const { return _map.end(); }
        const_reverse_iterator rbegin() const { return _map.rbegin(); }
        const_reverse_iterator rend() const { return _map.rend(); }

    private:
        const _TokenInputMap &_map;
    };

    // This little class allows clients to use TF_FOR_ALL to iterate
    // through the output connectors.
    //
    class OutputMapIterator {
    public:
        typedef _TokenOutputMap::const_iterator const_iterator;
        typedef _TokenOutputMap::const_reverse_iterator const_reverse_iterator;

        OutputMapIterator(const _TokenOutputMap &map) : _map(map) {}
        const_iterator begin() const { return _map.begin(); }
        const_iterator end() const { return _map.end(); }
        const_reverse_iterator rbegin() const { return _map.rbegin(); }
        const_reverse_iterator rend() const { return _map.rend(); }

    private:
        const _TokenOutputMap &_map;
    };


    /// Constructs a node in \p network with the inputs described by 
    /// \p inputSpecs and outputs described by \p outputSpecs.
    ///
    VDF_API
    VdfNode(VdfNetwork *network,
            const VdfInputSpecs &inputSpecs,
            const VdfOutputSpecs &outputSpecs);

    /// Returns the unique id of this node in its network. No current, or
    /// previously constructed node in the same network will share this id.
    ///
    VdfId GetId() const { return _id; }

    /// Get the node index from the node id. The index uniquely identifies
    /// the node in the current state of the network, but may alias an index
    /// which existed in a previous state of the network and has since been
    /// destructed.
    ///
    static VdfIndex GetIndexFromId(const VdfId id) {
        return static_cast<VdfIndex>(id);
    }

    /// Get the node version from the node id. Node ids with the same
    /// index are disambiguated by a version number. Given multiple node ids
    /// with the same index, only one version will match the current state
    /// of the network.
    ///
    static VdfVersion GetVersionFromId(const VdfId id) {
        return static_cast<VdfVersion>(id >> 32);
    }

    /// Returns the network to which this node belongs.
    ///
    const VdfNetwork &GetNetwork() const { return _network; }

    /// Returns the network to which this node belongs.
    ///
    VdfNetwork &GetNetwork() { return _network; }

    /// Returns true, if this node is of type \p TYPE.
    ///
    template<typename TYPE>
    bool IsA() const
    {
        return dynamic_cast<const TYPE *>(this);
    }

    /// \name Dataflow network management - Input API
    /// @{

    /// Returns the list of input specs.
    ///
    const VdfInputSpecs &GetInputSpecs() const {
        return _specs->GetInputSpecs();
    }

    /// Returns the connector named \p inputName, returns NULL if no 
    /// input of that name exists.
    ///
    const VdfInput *GetInput(const TfToken &inputName) const {
        return const_cast<VdfNode *>(this)->GetInput(inputName);
    }

    /// Returns the input named \p inputName, returns NULL if no 
    /// input of that name exists.
    ///
    VDF_API
    VdfInput *GetInput(const TfToken &inputName);

    /// Returns an iterator class that can be used with TF_FOR_ALL to
    /// iterate through the inputs.
    ///
    const InputMapIterator GetInputsIterator() const { 
        return InputMapIterator(_inputs);
    }

    /// Returns true if the node has input connections, false otherwise.
    ///
    bool HasInputConnections() const {
        TF_FOR_ALL(i, _inputs) {
            if (i->second->GetNumConnections())
                return true;
        }
        return false;
    }

    /// Returns true if the node has output connections, false otherwise.
    ///
    bool HasOutputConnections() const {
        TF_FOR_ALL(output, _outputs) {
            if (output->second->GetConnections().size())
                return true;
        }
        return false;
    }

    /// Returns a flat vector of all input connections.
    ///
    VDF_API
    VdfConnectionVector GetInputConnections() const;

    /// Returns a flat vector of all output connections.
    ///
    VDF_API
    VdfConnectionVector GetOutputConnections() const;

    /// @}


    /// \name Dataflow network management - Output API
    /// @{

    /// Returns the list of output specs.
    ///
    const VdfOutputSpecs &GetOutputSpecs() const {
        return _specs->GetOutputSpecs();
    }

    /// Returns the output object named \p name.
    ///
    /// Returns \c NULL and issues a coding error if no such output exists.
    ///
    const VdfOutput *GetOutput(const TfToken &name) const {
        return const_cast<VdfNode *>(this)->GetOutput(name);
    }

    /// Returns the output object named \p name.
    ///
    /// Returns \c NULL and issues a coding error if no such output exists.
    ///
    VDF_API
    VdfOutput *GetOutput(const TfToken &name);

    /// Returns the output object named \p name.
    ///
    /// Returns \c NULL if no such output exists, but issues no errors.
    ///
    VDF_API
    VdfOutput *GetOptionalOutput(const TfToken &name);

    /// Returns the output object named \p name.
    ///
    /// Returns \c NULL if no such output exists, but issues no errors.
    ///
    const VdfOutput *GetOptionalOutput(const TfToken &name) const {
        return const_cast<VdfNode *>(this)->GetOptionalOutput(name);
    }

    /// Returns the only output object that this node contains.
    ///
    /// It is only valid to call this method on nodes that contain 
    /// exactly one output.  A coding error will be issued otherwise.
    ///
    /// Returns \c NULL if the node doesn't have exactly one output.
    ///
    const VdfOutput *GetOutput() const {
        return const_cast<VdfNode *>(this)->GetOutput();
    }

    /// Returns the only output object that this node contains.
    ///
    /// It is only valid to call this method on nodes that contain 
    /// exactly one output.  A coding error will be issued otherwise.
    ///
    /// Returns \c NULL if the node doesn't have exactly one output.
    ///
    VDF_API
    VdfOutput *GetOutput();

    /// Returns an iterator class that can be used with TF_FOR_ALL to
    /// iterator through the output connectors.
    ///
    const OutputMapIterator GetOutputsIterator() const { 
        return OutputMapIterator(_outputs);
    }

    /// Returns the number of outputs that this node currently has.
    ///
    size_t GetNumOutputs() const {
        return _outputs.size();
    }

    /// Returns the number of inputs that this node currently has.
    ///
    size_t GetNumInputs() const {
        return _inputs.size();
    }

    /// @}


    /// \name Diagnostic API
    /// @{

    /// Sets the debug name for this node.
    ///
    /// By default, a node's debug name is the node type name; this function
    /// appends \p name to the node type name.
    ///
    VDF_API
    void SetDebugName(const std::string &name);

    /// Sets the debug name for this node with a lazily invoked callback.
    ///
    /// By default, a node's debug name is the node type name; this function
    /// appends the string returned by \p f to the node type name.
    ///
    template <class F>
    void SetDebugNameCallback(F &&f) {
        TfAutoMallocTag tag("Vdf", __ARCH_PRETTY_FUNCTION__);
        SetDebugNameCallback(VdfNodeDebugNameCallback(std::forward<F>(f)));
    }

    /// Sets the debug name for this node with a lazily invoked callback.
    ///
    /// By default, a node's debug name is the node type name; this function
    /// appends the string returned by \p callback to the node type name.
    ///
    /// If \p callback is empty, return without setting the debug name.
    ///
    VDF_API
    void SetDebugNameCallback(VdfNodeDebugNameCallback &&callback);

    /// Returns the debug name for this node, if one is registered. Otherwise,
    /// returns the node type name by default. 
    ///
    VDF_API
    const std::string GetDebugName() const;

    /// Returns the amount of memory used by the node in bytes.
    ///
    /// Does not account for data structures to keep track of connections
    /// etc.
    ///
    VDF_API
    virtual size_t GetMemoryUsage() const;

    /// @}


    /// \name VdfExecutor API
    /// @{
    
    /// This is the method called to perform computation.
    ///
    /// Derived classes must ensure that this method is thread safe, in the
    /// sense that Compute() may be called simultaneously on a single node.
    ///
    virtual void Compute(const VdfContext &context) const = 0;

    /// Returns \c true if this node performs speculation.
    ///
    /// The default implementation returns \c false.
    ///
    VDF_API
    virtual bool IsSpeculationNode() const;

    /// Returns a predicate, determining whether a given input and its
    /// connections are required in order to fulfill this node's input
    /// dependencies.
    ///
    /// This method is only allowed to read input values on the inputs that
    /// have been marked as prerequisites.
    ///
    VDF_API
    virtual VdfRequiredInputsPredicate GetRequiredInputsPredicate(
        const VdfContext &context) const;

    /// Returns a mask that indicates which elements of the data that flows
    /// along \p output depend on the elements indicated by
    /// \p inputDependencyMask that flow in via \p inputConnection.
    ///
    /// This is used for dependency queries in the input-to-output direction.
    ///
    /// - If \p output is an associated output, returns a mask that is computed
    ///   based on the semantics of associated input/output pairs.
    /// - Otherwise, calls the virtual method _ComputeOutputDependencyMask().
    ///
    /// \p inputConnection must be a connection that connects to an input
    /// on this node.  \p output must be owned by this node.
    ///
    VDF_API
    VdfMask ComputeOutputDependencyMask(
        const VdfConnection &inputConnection,
        const VdfMask &inputDependencyMask,
        const VdfOutput &output) const;

    /// Vectorized version of ComputeOutputDependencyMask.  For a given
    /// inputConnection and inputDependencyMask, returns the outputs and
    /// masks that depend on them in \p outputDependencies.
    ///
    VDF_API
    void ComputeOutputDependencyMasks(
        const VdfConnection &inputConnection,
        const VdfMask &inputDependencyMask,
        VdfMaskedOutputVector *outputDependencies) const;

    /// Returns a mask that indicates which elements of the data that flows
    /// along \p inputConnection are needed to compute the data flowing out
    /// as indicated by \p maskedOutput.
    ///
    /// This is used for dependency queries in the output-to-input direction.
    ///
    /// - If \p inputConnection is associated with the output in
    ///   \p maskedOutput, returns a mask that is computed based on the
    ///   semantics of associated input/output pairs.
    /// - Otherwise, calls the virtual method _ComputeInputDependencyMask().
    ///
    /// \p inputConnection must be a connection that connects to an input
    /// on this node.  The output in \p maskedOutput must be owned
    /// by this node.
    ///
    VDF_API
    VdfMask::Bits ComputeInputDependencyMask(
        const VdfMaskedOutput &maskedOutput,
        const VdfConnection &inputConnection) const;

    /// Vectorized version of ComputeInputDependencyMask.
    ///
    /// Additional parameter \p skipAssociatedInputs can control if associated
    /// inputs should be considered.
    ///
    VDF_API
    VdfConnectionAndMaskVector ComputeInputDependencyMasks(
        const VdfMaskedOutput &maskedOutput,
        bool skipAssociatedInputs) const;

    /// Vectorized version of ComputeInputDependencyMasks().  
    ///
    /// Computes all input dependencies for \p request in one go.
    ///
    VDF_API
    VdfConnectionAndMaskVector ComputeInputDependencyRequest(
        const VdfMaskedOutputVector &request) const;

    /// @}


    /// Returns true, if \p rhs and this node compute the same value(s). 
    ///
    VDF_API
    bool IsEqual(const VdfNode &rhs) const;

protected:

    /// Protected constructor.
    /// 
    /// Can be used by derived classes that want to manage their own
    /// storage for input/output specs.
    /// 
    /// The derived constructor should call _InitializeInputAndOutputSpecs
    /// because this version of the constructor won't do it.
    /// 
    VDF_API
    VdfNode(VdfNetwork *network);

    /// Builds inputs from the supplied input specs and appends them to
    /// the already-existing set of inputs, if any.
    ///
    /// Pointers to the newly-created VdfInputs are appended to resultingInputs,
    /// if resultingInputs is not NULL.
    VDF_API
    void _AppendInputs(
        const VdfInputSpecs &inputSpecs,
        std::vector<VdfInput*> *resultingInputs = NULL);

    /// Builds outputs from the supplied output specs and appends them to
    /// the already-existing set of outputs, if any.
    ///
    /// Pointers to the newly-created VdfOutputs are appended to
    /// resultingInputs, if resultingOutputs is not NULL.
    VDF_API
    void _AppendOutputs(
        const VdfOutputSpecs &outputSpecs,
        std::vector<VdfOutput*> *resultingOutputs = NULL);

    /// Can be overridden by derived classes to facilitate equality
    /// comparision.
    ///
    /// The default implementation will always return false.  Note that all
    /// connector specs, etc. are already taken care of via VdfNode::IsEqual().
    ///
    VDF_API
    virtual bool _IsDerivedEqual(const VdfNode &rhs) const;

    /// Notifies a node that one connection has been added.
    ///
    /// \p atIndex will either contain the `atIndex` parameter passed to
    /// VdfNetwork's Connect() method, which specifies at what index a new
    /// connection has been inserted, or VdfNetwork::AppendConnection if the new
    /// connection was appended.
    ///
    /// \note
    /// Note that this method will be called _after_ a connection has been made
    /// and is linked into the respective VdfInput and VdfOutput.
    ///
    VDF_API
    virtual void _DidAddInputConnection(const VdfConnection *c, int atIndex);

    /// Notifies a node that one connection will be removed.
    ///
    /// \note
    /// Note that this method will be called _before_ a connection is unlinked
    /// from the respective VdfInput and VdfOutput in case of deletion.
    ///
    VDF_API
    virtual void _WillRemoveInputConnection(const VdfConnection *c);

    // The network is the only entity allowed to delete a node and set a node's
    // id.
    friend class VdfNetwork;
    
    /// Protected Destructor.
    ///
    /// Only the network is allowed to destroy nodes. 
    ///
    VDF_API
    virtual ~VdfNode();

    /// Sets the node id. The node id is controlled by VdfNetwork.
    ///
    void _SetId(const VdfVersion version, const VdfIndex index) {
        _id = (static_cast<VdfId>(version) << 32) | static_cast<VdfId>(index);
    }

    /// \name VdfInputAndOutputSpecs management
    /// @{

    /// Gets an input/output specs pointer that the node can use.  This is
    /// virtual so that a derived class may specify exactly where the storage
    /// comes from.  This function is not called during performance-critical
    /// code-paths.
    /// 
    VDF_API
    virtual const VdfInputAndOutputSpecs *_AcquireInputAndOutputSpecsPointer(
        const VdfInputSpecs &inputSpecs,
        const VdfOutputSpecs &outputSpecs);

    /// Releases an input/output specs pointer that was acquired with a
    /// previous call to _AcquireInputAndOutputSpecsPointer().  This is virtual
    /// so that a derived class may specify exactly where the storage comes 
    /// from.  This function is not called during performance-critical
    /// code-paths.
    ///
    VDF_API
    virtual void _ReleaseInputAndOutputSpecsPointer(
        const VdfInputAndOutputSpecs *specs);

    /// Initializes the input/output specs pointer for this node.
    ///
    /// Only to be called from constructors of derived classes that want to
    /// manage their own storage for the input/output specs.
    ///
    VDF_API
    void _InitializeInputAndOutputSpecs(
        const VdfInputAndOutputSpecs *specs);

    /// Clears the input/output specs pointer.
    /// 
    /// This can be used in the destructor of derived classes to avoid having
    /// their own storage for the input/output specs go through the default 
    /// destruction pattern of the base class.
    ///
    VDF_API
    void _ClearInputAndOutputSpecsPointer();

    /// Replaces the node's input specs with \p inputSpecs and rebuilds all
    /// inputs.
    ///
    VDF_API
    void _ReplaceInputSpecs(const VdfInputSpecs &inputSpecs);

    /// @}

    /// Helper method for determining the amount of memory that a node
    /// uses.
    ///
    /// Using this function is much safer than trying to do the computation
    /// manually, since it knows how to avoid double counting of base class
    /// structures.
    ///
    template <typename BaseClassType, typename ClassType>
    static size_t _GetMemoryUsage(const ClassType &c, size_t dynamicSize)
    {
        return c.BaseClassType::GetMemoryUsage() + 
            (sizeof(ClassType) - sizeof(BaseClassType)) + 
            dynamicSize;
    }

    /// Returns a mask that indicates which elements of the data that flows
    /// along \p output depend on the elements indicated by
    /// \p inputDependencyMask that flow in via \p inputConnection.
    ///
    /// This is only called from ComputeOutputDependencyMask(), to handle
    /// non-associated inputs.
    ///
    /// The default implementation does the following:
    /// - If \p output has an affects mask, the affects mask is returned,
    ///   indicating that all affected elements depend on the input.
    /// - Otherwise, returns an all-ones mask, indicating that all elements
    ///   depend on the input.
    ///
    /// Derived classes can override this method in order to make the
    /// resulting dependencies more sparse.
    ///
    /// Derived classes must ensure that this method is thread safe, in the
    /// sense that _ComputeOutputDependencyMask() may be called simultaneously
    /// on a single node.
    ///
    VDF_API
    virtual VdfMask _ComputeOutputDependencyMask(
        const VdfConnection &inputConnection,
        const VdfMask &inputDependencyMask,
        const VdfOutput &output) const;

    /// Vectorized version of _ComputeOutputDependencyMask.
    ///
    /// Unlike _ComputeDependencyMask, non-trivial derived nodes do not have to
    /// implement this function.  If a derived node is capable of supporting
    /// this vectorized API, then it must return \c true.  The base class
    /// returns \p false, meaning this is not implemented.
    ///
    VDF_API
    virtual bool _ComputeOutputDependencyMasks(
        const VdfConnection &inputConnection,
        const VdfMask &inputDependencyMask,
        VdfMaskedOutputVector *outputDependencies) const;

    /// Vectorized version of _ComputeOutputDependencyMasks.
    ///
    /// Unlike _ComputeDependencyMasks, non-trivial derived nodes do not have to
    /// implement this function.  If a derived node is capable of supporting
    /// this vectorized API, then it can override this function.
    ///
    VDF_API
    virtual VdfConnectionAndMaskVector _ComputeInputDependencyRequest(
        const VdfMaskedOutputVector &request) const;

    /// Returns a mask that indicates which elements of the data that flows
    /// along \p inputConnection are needed to compute the data flowing out
    /// as indicated by \p maskedOutput.
    ///
    /// This is only called from ComputeInputDependencyMask(), to handle
    /// non-associated inputs.
    ///
    /// The default implementation returns the connection mask on
    /// \p inputConnection, indicating that all elements are required.
    /// Derived classes can override this method in order to make the
    /// resulting dependencies more sparse.
    ///
    /// This method should always return a mask that is a subset of the
    /// connection mask on \p inputConnection or an empty mask to indicate
    /// no dependencies.
    ///
    /// Derived classes must ensure that this method is thread safe, in the
    /// sense that _ComputeInputDependencyMask() may be called simultaneously
    /// on a single node.
    ///
    VDF_API
    virtual VdfMask::Bits _ComputeInputDependencyMask(
        const VdfMaskedOutput &maskedOutput,
        const VdfConnection &inputConnection) const;

    /// Vectorized version of _ComputeInputDependencyMask.
    ///
    /// Unlike _ComputeInputDependencyMask, non-trivial derived nodes do not
    /// have to implement this function.
    ///
    /// Additional parameter \p skipAssociatedInputs can control if associated
    /// inputs should be considered.
    ///
    VDF_API
    virtual VdfConnectionAndMaskVector _ComputeInputDependencyMasks(
        const VdfMaskedOutput &maskedOutput,
        bool skipAssociatedInputs) const;

private:

    // This is the network to which the node belongs.
    // XXX:memory
    // We may want to come up with a better scheme than adding a back pointer
    // per-node!
    VdfNetwork &_network;

    // This is the unique id of this node in its network.
    VdfId _id;

    // This object holds on to the specs of the input and output connectors.
    // This object is never owned by VdfNode.  In the base implementation,
    // this object is owned by Vdf_InputAndOutputSpecsRegistry and must be
    // acquired and released through this object.  Derived classes may provide
    // their own management of this pointer.
    const VdfInputAndOutputSpecs *_specs;

    // The list of inputs.  _inputs[i] gives you all the links into 
    // the i-th input of this node.
    _TokenInputMap _inputs;

    // The list of outputs.  _outputs[name] gives you all the links out of
    // the output connector of this node with the label name.
    //
    // We require _outputs.size() be constant time.
    _TokenOutputMap _outputs;
};

template <>
struct Tf_ShouldIterateOverCopy<VdfNode::InputMapIterator> : std::true_type {};
template <>
struct Tf_ShouldIterateOverCopy<VdfNode::OutputMapIterator> : std::true_type {};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
