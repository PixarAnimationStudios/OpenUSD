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
#ifndef USDEXEC_GENERATED_EXECNODE_H
#define USDEXEC_GENERATED_EXECNODE_H

/// \file usdExec/execNode.h

#include "pxr/pxr.h"
#include "pxr/usd/usdExec/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usdExec/execInput.h"
#include "pxr/usd/usdExec/execOutput.h"
#include "pxr/usd/usdExec/tokens.h"
#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/exec/execNode.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// EXECNODE                                                                   //
// -------------------------------------------------------------------------- //

/// \class UsdExecNode
///
/// Base class for all USD execution nodes. Exec-nodes are the building blocks
/// of procedural networks.
/// 
/// The purpose of representing them in Usd is two-fold:
/// \li To represent, via "connections" the topology of the procedural network
/// that must be reconstructed in the engine. Facilities for authoring and 
/// manipulating connections are encapsulated in the API schema 
/// ExecConnectableAPI.
/// \li To present a (partial or full) interface of typed input parameters 
/// whose values can be set and overridden in Usd, to be provided later at 
/// run-time as parameter values to the actual procedural objects. Node 
/// input parameters are encapsulated in the property schema ExecInput.
/// 
///
class UsdExecNode : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdExecNode on UsdPrim \p prim .
    /// Equivalent to UsdExecNode::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdExecNode(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdExecNode on the prim held by \p schemaObj .
    /// Should be preferred over UsdExecNode(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdExecNode(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDEXEC_API
    virtual ~UsdExecNode();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDEXEC_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdExecNode holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdExecNode(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDEXEC_API
    static UsdExecNode
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
    static UsdExecNode
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

     // -------------------------------------------------------------------------
    /// \name Conversion to and from UsdExecConnectableAPI
    /// 
    /// @{

    /// Constructor that takes a ConnectableAPI object.
    /// Allow implicit (auto) conversion of UsdExecNode to 
    /// UsdExecConnectableAPI, so that a ExecNode can be passed into any function
    /// that accepts a ConnectableAPI.
    USDEXEC_API
    UsdExecNode(const UsdExecConnectableAPI &connectable);

    /// Contructs and returns a UsdExecConnectableAPI object with this shader.
    ///
    /// Note that most tasks can be accomplished without explicitly constructing 
    /// a UsdExecConnectable API, since connection-related API such as
    /// UsdExecConnectableAPI::ConnectToSource() are static methods, and 
    /// UsdExecNode will auto-convert to a UsdExecConnectableAPI when 
    /// passed to functions that want to act generically on a connectable
    /// UsdExecConnectableAPI object.
    USDEXEC_API
    UsdExecConnectableAPI ConnectableAPI() const;

    /// @}

    // -------------------------------------------------------------------------
    /// \name Outputs API
    ///
    /// Outputs represent a typed attribute on a shader or node-graph whose value 
    /// is computed externally. 
    /// 
    /// When they exist on a node-graph, they are connectable and are typically 
    /// connected to the output of a shader within the node-graph.
    /// 
    /// @{
        
    /// Create an output which can either have a value or can be connected.
    /// The attribute representing the output is created in the "outputs:" 
    /// namespace. Outputs on a shader cannot be connected, as their 
    /// value is assumed to be computed externally.
    /// 
    USDEXEC_API
    UsdExecOutput CreateOutput(const TfToken& name,
                                const SdfValueTypeName& typeName);

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

    /// @}

    // ------------------------------------------------------------------------- 

    /// \name Inputs API
    ///
    /// Inputs are connectable attribute with a typed value. 
    /// 
    /// On Exec Node, the parameters are encoded as inputs. On Exec Graph,
    /// interface attributes are represented as inputs.
    /// 
    /// @{
        
    /// Create an input which can either have a value or can be connected.
    /// The attribute representing the input is created in the "inputs:" 
    /// namespace. Inputs on both ExecNode and ExecGraph are connectable.
    /// 
    USDEXEC_API
    UsdExecInput CreateInput(const TfToken& name,
                              const SdfValueTypeName& typeName);

    /// Return the requested input if it exists.
    /// 
    USDEXEC_API
    UsdExecInput GetInput(const TfToken &name) const;

    /// Inputs are represented by attributes in the "inputs:" namespace.
    /// If \p onlyAuthored is true (the default), then only return authored
    /// attributes; otherwise, this also returns un-authored builtins.
    /// 
    USDEXEC_API
    std::vector<UsdExecInput> GetInputs(bool onlyAuthored=true) const;

    /// @}

    // -------------------------------------------------------------------------

    /// \anchor UsdExecNode_ExecMetadata_API
    /// \name Exec Node Metadata API
    /// 
    /// This section provides API for authoring and querying node registry
    /// metadata. When the node's implementationSource is <b>sourceAsset</b> 
    /// or <b>sourceCode</b>, the authored "execMetadata" dictionary value 
    /// provides additional metadata needed to process the node source
    /// correctly. It is used in combination with the sourceAsset or sourceCode
    /// value to fetch the appropriate node from the node registry.
    /// 
    /// We expect the keys in execMetadata to correspond to the keys 
    /// in \ref SdrNodeMetadata. However, this is not strictly enforced in the 
    /// API. The only allowed value type in the "execMetadata" dictionary is a 
    /// std::string since it needs to be converted into a NdrTokenMap, which Sdr
    /// will parse using the utilities available in \ref ExecMetadataHelpers.
    /// 
    /// @{

    /// Returns this node's composed "execMetadata" dictionary as a 
    /// NdrTokenMap.
    USDEXEC_API
    NdrTokenMap GetExecMetadata() const;
    
    /// Returns the value corresponding to \p key in the composed 
    /// <b>execMetadata</b> dictionary.
    USDEXEC_API
    std::string GetExecMetadataByKey(const TfToken &key) const;
        
    /// Authors the given \p execMetadata on this node at the current 
    /// EditTarget.
    USDEXEC_API
    void SetExecMetadata(const NdrTokenMap &execMetadata) const;

    /// Sets the value corresponding to \p key to the given string \p value, in 
    /// the node's "execMetadata" dictionary at the current EditTarget.
    USDEXEC_API
    void SetExecMetadataByKey(
        const TfToken &key, 
        const std::string &value) const;

    /// Returns true if the node has a non-empty composed "execMetadata" 
    /// dictionary value.
    USDEXEC_API
    bool HasExecMetadata() const;

    /// Returns true if there is a value corresponding to the given \p key in 
    /// the composed "execMetadata" dictionary.
    USDEXEC_API
    bool HasExecMetadataByKey(const TfToken &key) const;

    /// Clears any "execMetadata" value authored on the node in the current 
    /// EditTarget.
    USDEXEC_API
    void ClearExecMetadata() const;

    /// Clears the entry corresponding to the given \p key in the 
    /// "execMetadata" dictionary authored in the current EditTarget.
    USDEXEC_API
    void ClearExecMetadataByKey(const TfToken &key) const;

    /// @}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
