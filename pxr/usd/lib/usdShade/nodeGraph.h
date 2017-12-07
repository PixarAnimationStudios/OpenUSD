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
#ifndef USDSHADE_GENERATED_NODEGRAPH_H
#define USDSHADE_GENERATED_NODEGRAPH_H

/// \file usdShade/nodeGraph.h

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <utility>
#include "pxr/usd/usd/editTarget.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"


#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// NODEGRAPH                                                                  //
// -------------------------------------------------------------------------- //

/// \class UsdShadeNodeGraph
///
/// A node-graph is a container for shading nodes, as well as other 
/// node-graphs. It has a public input interface and provides a list of public 
/// outputs.
/// 
/// <b>Node Graph Interfaces</b>
/// 
/// One of the most important functions of a node-graph is to host the "interface"
/// with which clients of already-built shading networks will interact.  Please
/// see \ref UsdShadeNodeGraph_Interfaces "Interface Inputs" for a detailed
/// explanation of what the interface provides, and how to construct and
/// use it, to effectively share/instance shader networks.
/// 
/// <b>Node Graph Outputs</b>
/// 
/// These behave like outputs on a shader and are typically connected to an 
/// output on a shader inside the node-graph.
/// 
///
class UsdShadeNodeGraph : public UsdTyped
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = true;

    /// Compile-time constant indicating whether or not this class inherits from
    /// UsdTyped. Types which inherit from UsdTyped can impart a typename on a
    /// UsdPrim.
    static const bool IsTyped = true;

    /// Construct a UsdShadeNodeGraph on UsdPrim \p prim .
    /// Equivalent to UsdShadeNodeGraph::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdShadeNodeGraph(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdShadeNodeGraph on the prim held by \p schemaObj .
    /// Should be preferred over UsdShadeNodeGraph(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdShadeNodeGraph(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDSHADE_API
    virtual ~UsdShadeNodeGraph();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDSHADE_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdShadeNodeGraph holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdShadeNodeGraph(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDSHADE_API
    static UsdShadeNodeGraph
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
    USDSHADE_API
    static UsdShadeNodeGraph
    Define(const UsdStagePtr &stage, const SdfPath &path);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDSHADE_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDSHADE_API
    virtual const TfType &_GetTfType() const;

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

    /// Allow UsdShadeNodeGraph to auto-convert to UsdShadeConnectableAPI, so 
    /// you can pass in a UsdShadeNodeGraph to any function that accepts 
    /// a UsdShadeConnectableAPI.
    USDSHADE_API
    operator UsdShadeConnectableAPI () const;

    /// Contructs and returns a UsdShadeConnectableAPI object with this 
    /// node-graph.
    /// 
    /// Note that most tasks can be accomplished without explicitly constructing 
    /// a UsdShadeConnectable API, since connection-related API such as
    /// UsdShadeConnectableAPI::ConnectToSource() are static methods, and 
    /// UsdShadeNodeGraph will auto-convert to a UsdShadeConnectableAPI when 
    /// passed to functions that want to act generically on a connectable
    /// UsdShadeConnectableAPI object.
    USDSHADE_API
    UsdShadeConnectableAPI ConnectableAPI() const;

    /// \anchor UsdShadeNodeGraph_Output
    /// \name Outputs of a node-graph. These typically connect to outputs of 
    /// shaders or nested node-graphs within the node-graph.
    /// 
    /// @{

    /// Create an output which can either have a value or can be connected.
    /// The attribute representing the output is created in the "outputs:" 
    /// namespace.
    /// 
    USDSHADE_API
    UsdShadeOutput CreateOutput(const TfToken& name,
                                const SdfValueTypeName& typeName);

    /// Return the requested output if it exists.
    /// 
    USDSHADE_API
    UsdShadeOutput GetOutput(const TfToken &name) const;

    /// Outputs are represented by attributes in the "outputs:" namespace.
    /// 
    USDSHADE_API
    std::vector<UsdShadeOutput> GetOutputs() const;
    
    /// @}

    /// \anchor UsdShadeNodeGraph_Interfaces
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
    /// \li for each such attribute, a list of UsdShaderInput targets
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
    /// attributes, we use the attribute schema UsdShadeInput, which also
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
    USDSHADE_API
    UsdShadeInput CreateInput(const TfToken& name,
                              const SdfValueTypeName& typeName);

    /// Return the requested input if it exists.
    /// 
    USDSHADE_API
    UsdShadeInput GetInput(const TfToken &name) const;

    /// Returns all inputs present on the node-graph. These are represented by
    /// attributes in the "inputs:" namespace.
    /// 
    USDSHADE_API
    std::vector<UsdShadeInput> GetInputs() const;
    
    /// @}

    // Provide custom hash and equality comparison function objects for 
    // UsdShadeNodeGraph until bug 143077 is resolved.

    /// Hash functor for UsdShadeNodeGraph objects.
    struct NodeGraphHasher {
        inline size_t operator()(const UsdShadeNodeGraph &nodeGraph) const {
            return hash_value(nodeGraph.GetPrim());
        }
    };
    /// Equality comparator for UsdShadeNodeGraph objects.
    struct NodeGraphEqualFn
    {
        inline bool operator() (UsdShadeNodeGraph const& s1, 
                                UsdShadeNodeGraph const& s2) const
        {
            return s1.GetPrim() == s2.GetPrim();
        }
    };

    // ---------------------------------------------------------------------- //
    /// \anchor UsdShadeNodeGraph_InterfaceInputs
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
    USDSHADE_API
    std::vector<UsdShadeInput> GetInterfaceInputs() const;

    /// Map of interface inputs to corresponding vectors of inputs that 
    /// consume their values.
    typedef std::unordered_map<UsdShadeInput, std::vector<UsdShadeInput>, 
        UsdShadeInput::Hash> InterfaceInputConsumersMap;

    /// Map of node-graphs to their associated input-consumers map.
    typedef std::unordered_map<UsdShadeNodeGraph,
                               InterfaceInputConsumersMap, 
                               NodeGraphHasher,
                               NodeGraphEqualFn> 
            NodeGraphInputConsumersMap;

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
    /// \p renderTarget exists for backwards compatibility to allow retrieving 
    /// the input-consumers map when the shading network has old-style interface 
    /// attributes.
    /// 
    /// This API is provided for use by DCC's that want to present node-graph
    /// interface / shader connections in the opposite direction than they are 
    /// encoded in USD.
    /// 
    USDSHADE_API
    InterfaceInputConsumersMap ComputeInterfaceInputConsumersMap(
        bool computeTransitiveConsumers=false) const;

protected:
    
    // Befriend UsdRiLookAPI and UsdRiMaterialAPI temporarily to assist in the
    // transition to the new shading encoding.
    friend class UsdRiLookAPI;
    friend class UsdRiMaterialAPI;

    /// \deprecated
    /// This is similar to ComputeInterfaceInputConsumersMap(), but takes an 
    /// additonal "renderTarget" argument which needs to be considered when 
    /// collecting old-style interface-input connections. 
    /// 
    /// Provided for use by UsdRiLookAPI and UsdRiMaterialAPI. 
    /// 
    USDSHADE_API
    InterfaceInputConsumersMap _ComputeInterfaceInputConsumersMap(
        bool computeTransitiveConsumers, 
        const TfToken &renderTarget) const;
    

    
    /// \deprecated
    /// This is similar to GetInterfaceInputs(), but takes an additonal 
    /// "renderTarget" argument which needs to be considered when collecting 
    /// old-style interface-input connections. 
    /// 
    /// Provided for use by UsdRiLookAPI and UsdRiMaterialAPI. 
    /// 
    USDSHADE_API
    std::vector<UsdShadeInput> _GetInterfaceInputs(const TfToken &renderTarget) 
        const;

    /// @}

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
