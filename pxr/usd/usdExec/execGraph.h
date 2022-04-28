//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef USDEXEC_GENERATED_EXECGRAPH_H
#define USDEXEC_GENERATED_EXECGRAPH_H

/// \file usdExec/execGraph.h

#include "pxr/pxr.h"
#include "pxr/usd/usdExec/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <utility>
#include "pxr/usd/usd/editTarget.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usdExec/execInput.h"
#include "pxr/usd/usdExec/execOutput.h"
#include "pxr/usd/usdExec/execNode.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// EXECGRAPH                                                                  //
// -------------------------------------------------------------------------- //

/// \class UsdExecGraph
///
/// A exec-graph is a container for exec-nodes, as well as other 
/// exec-graphs. It has a public input interface and provides a list of public 
/// outputs.
/// 
/// <b>Exec Graph Interfaces</b>
/// 
/// One of the most important functions of a exec-graph is to host the "interface"
/// with which clients of already-built execution networks will interact.  Please
/// see \ref ExecGraph_Interfaces "Interface Inputs" for a detailed
/// explanation of what the interface provides, and how to construct and
/// use it, to effectively share/instance execution networks.
/// 
/// <b>Exec Graph Outputs</b>
/// 
/// These behave like outputs on a exec-node and are typically connected to an 
/// output on a exec-node inside the exec-graph.
/// 
///
class UsdExecGraph : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdExecGraph on UsdPrim \p prim .
    /// Equivalent to UsdExecGraph::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdExecGraph(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdExecGraph on the prim held by \p schemaObj .
    /// Should be preferred over UsdExecGraph(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdExecGraph(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDEXEC_API
    virtual ~UsdExecGraph();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDEXEC_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdExecGraph holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdExecGraph(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDEXEC_API
    static UsdExecGraph
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Attempt to ensure a \a UsdPrim adhering to this schema at \p path
    /// is defined (according to UsdPrim::IsDefined()) on this stage.
    ///
    /// If a prim adhering to this schema at \p path is already defined on this
    /// stage, return that prim.  Otherwise author an \a SdfPrimSpec with
    /// \a specifier == \a SdfSpecifierDef and this schema's prim type name for
    /// the prim at \p path at the current EditTarget.  Author \a SdfPrimSpec s
    /// with \p specifier == \a SdfSpecifierDef and empty typeName at the
    /// current EditTarget for any nonexistent, or existing but not \a Defined
    /// ancestors.
    ///
    /// The given \a path must be an absolute prim path that does not contain
    /// any variant selections.
    ///
    /// If it is impossible to author any of the necessary PrimSpecs, (for
    /// example, in case \a path cannot map to the current UsdEditTarget's
    /// namespace) issue an error and return an invalid \a UsdPrim.
    ///
    /// Note that this method may return a defined prim whose typeName does not
    /// specify this schema class, in case a stronger typeName opinion overrides
    /// the opinion at the current EditTarget.
    ///
    USDEXEC_API
    static UsdExecGraph
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDEXEC_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDEXEC_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDEXEC_API
    const TfType &_GetTfType() const override;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with PXR_NAMESPACE_CLOSE_SCOPE
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

    /// Constructor that takes a ConnectableAPI object.
    /// Allow implicit (auto) conversion of UsdExecGraph to 
    /// UsdExecConnectableAPI, so that a NodeGraph can be passed into any 
    /// function that accepts a ConnectableAPI.
    USDEXEC_API
    UsdExecGraph(const UsdExecConnectableAPI &connectable);

    /// Contructs and returns a UsdExecConnectableAPI object with this 
    /// node-graph.
    /// 
    /// Note that most tasks can be accomplished without explicitly constructing 
    /// a UsdExecConnectable API, since connection-related API such as
    /// UsdExecConnectableAPI::ConnectToSource() are static methods, and 
    /// UsdExecGraph will auto-convert to a UsdExecConnectableAPI when 
    /// passed to functions that want to act generically on a connectable
    /// UsdExecConnectableAPI object.
    USDEXEC_API
    UsdExecConnectableAPI ConnectableAPI() const;

    /// \anchor UsdExecGraph_Output
    /// \name Outputs of a node-graph. These typically connect to outputs of 
    /// shaders or nested node-graphs within the node-graph.
    /// 
    /// @{

    /// Create an output which can either have a value or can be connected.
    /// The attribute representing the output is created in the "outputs:" 
    /// namespace.
    /// 
    USDEXEC_API
    UsdExecOutput CreateOutput(const TfToken& name,
                                const SdfValueTypeName& typeName) const;

    /// Return the requested output if it exists.
    /// 
    USDEXEC_API
    UsdExecOutput GetOutput(const TfToken &name) const;

    /// Outputs are represented by attributes in the "outputs:" namespace.
    /// If \p onlyAuthored is true (the default), then only return authored
    /// attributes; otherwise, this also returns un-authored builtins.
    ///
    USDEXEC_API
    std::vector<UsdExecOutput> GetOutputs(bool onlyAuthored=true) const;

    /// \deprecated in favor of GetValueProducingAttributes on UsdExecOutput
    /// Resolves the connection source of the requested output, identified by
    /// \p outputName to a shader output.
    /// 
    /// \p sourceName is an output parameter that is set to the name of the 
    /// resolved output, if the node-graph output is connected to a valid 
    /// shader source.
    ///
    /// \p sourceType is an output parameter that is set to the type of the 
    /// resolved output, if the node-graph output is connected to a valid 
    /// shader source.
    /// 
    /// \return Returns a valid shader object if the specified output exists and 
    /// is connected to one. Return an empty shader object otherwise.
    /// The python version of this method returns a tuple containing three 
    /// elements (the source shader, sourceName, sourceType).
    USDEXEC_API
    UsdExecNode ComputeOutputSource(
        const TfToken &outputName, 
        TfToken *sourceName, 
        UsdExecAttributeType *sourceType) const;

    /// @}

    /// \anchor UsdExecGraph_Interfaces
    /// \name Interface inputs of a node-graph. 
    ///
    /// In addition to serving as the "head" for all of the shading networks
    /// that describe each render target's particular node-graph, the node-graph
    /// prim provides a unified "interface" that allows node-graphs to share 
    /// shading networks while retaining the ability for each to specify its own
    /// set of unique values for the interface inputs that users may need to 
    /// modify.
    ///
    /// A "Node-graph Interface" is a combination of:
    /// \li a flat collection of attributes, of arbitrary names
    /// \li for each such attribute, a list of UsdExecrInput targets
    /// whose attributes on Shader prims should be driven by the interface
    /// input.
    ///
    /// A single interface input can drive multiple shader inputs and be 
    /// consumed by multiple render targets. The set of interface inputs itself 
    /// is intentionally flat, to encourage sharing of the interface between 
    /// render targets.  Clients are always free to create interface inputs with 
    /// namespacing to segregate "private" attributes exclusive to the render 
    /// target, but we hope this will be an exception.
    ///
    /// To facilitate connecting, qualifying, and interrogating interface
    /// attributes, we use the attribute schema UsdExecInput, which also
    /// serves as an abstraction for shader inputs.
    ///
    /// <b>Scoped Interfaces</b>
    ///
    /// \todo describe scoped interfaces and fix bug/108940 to account for them.
    ///
    /// @{

    /// Create an Input which can either have a value or can be connected.
    /// The attribute representing the input is created in the "inputs:" 
    /// namespace.
    /// 
    /// \todo clarify error behavior if typeName does not match existing,
    /// defined attribute - should match UsdPrim::CreateAttribute - bug/108970
    ///
    USDEXEC_API
    UsdExecInput CreateInput(const TfToken& name,
                              const SdfValueTypeName& typeName) const;

    /// Return the requested input if it exists.
    /// 
    USDEXEC_API
    UsdExecInput GetInput(const TfToken &name) const;

    /// Returns all inputs present on the node-graph. These are represented by
    /// attributes in the "inputs:" namespace.
    /// If \p onlyAuthored is true (the default), then only return authored
    /// attributes; otherwise, this also returns un-authored builtins.
    ///
    USDEXEC_API
    std::vector<UsdExecInput> GetInputs(bool onlyAuthored=true) const;
    
    /// @}

    // Provide custom hash and equality comparison function objects for 
    // UsdExecGraph until bug 143077 is resolved.

    /// Hash functor for UsdExecGraph objects.
    struct ExecGraphHasher {
        inline size_t operator()(const UsdExecGraph &nodeGraph) const {
            return hash_value(nodeGraph.GetPrim());
        }
    };
    /// Equality comparator for UsdExecGraph objects.
    struct ExecGraphEqualFn
    {
        inline bool operator() (UsdExecGraph const& s1, 
                                UsdExecGraph const& s2) const
        {
            return s1.GetPrim() == s2.GetPrim();
        }
    };

    // ---------------------------------------------------------------------- //
    /// \anchor UsdExecGraph_InterfaceInputs
    /// \name Interface Inputs
    /// 
    /// API to query the inputs that form the interface of the node-graph and 
    /// their connections.
    /// 
    /// @{
        
    /// Returns all the "Interface Inputs" of the node-graph. This is the same 
    /// as GetInputs(), but is provided  as a convenience, to allow clients to
    /// distinguish between inputs on shaders vs. interface-inputs on 
    /// node-graphs.
    USDEXEC_API
    std::vector<UsdExecInput> GetInterfaceInputs() const;

    /// Map of interface inputs to corresponding vectors of inputs that 
    /// consume their values.
    typedef std::unordered_map<UsdExecInput, std::vector<UsdExecInput>, 
        UsdExecInput::Hash> ExecInterfaceInputConsumersMap;

    /// Map of node-graphs to their associated input-consumers map.
    typedef std::unordered_map<UsdExecGraph,
                               ExecInterfaceInputConsumersMap, 
                               ExecGraphHasher,
                               ExecGraphEqualFn> 
            ExecGraphInputConsumersMap;

    /// Walks the namespace subtree below the node-graph and computes a map 
    /// containing the list of all inputs on the node-graph and the associated 
    /// vector of consumers of their values. The consumers can be inputs on 
    /// shaders within the node-graph or on nested node-graphs).
    /// 
    /// If \p computeTransitiveConsumers is true, then value consumers
    /// belonging to <b>node-graphs</b> are resolved transitively to compute the 
    /// transitive mapping from inputs on the node-graph to inputs on shaders 
    /// inside the material. Note that inputs on node-graphs that don't have 
    /// value consumers will continue to be included in the result.
    /// 
    /// This API is provided for use by DCC's that want to present node-graph
    /// interface / shader connections in the opposite direction than they are 
    /// encoded in USD.
    /// 
    USDEXEC_API
    ExecInterfaceInputConsumersMap ComputeExecInterfaceInputConsumersMap(
        bool computeTransitiveConsumers=false) const;

    /// @}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
