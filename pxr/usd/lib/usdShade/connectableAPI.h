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
#ifndef USDSHADE_GENERATED_CONNECTABLEAPI_H
#define USDSHADE_GENERATED_CONNECTABLEAPI_H

/// \file usdShade/connectableAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/nodeGraph.h"
    

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// CONNECTABLEAPI                                                             //
// -------------------------------------------------------------------------- //

/// \class UsdShadeConnectableAPI
///
/// UsdShadeConnectableAPI is an API schema that provides a common
/// interface for creating outputs and making connections between shading 
/// parameters and outputs. The interface is common to all UsdShade schemas
/// that support Inputs and Outputs, which currently includes UsdShadeShader,
/// UsdShadeNodeGraph, and UsdShadeMaterial .
/// 
/// One can construct a UsdShadeConnectableAPI directly from a UsdPrim, or
/// from objects of any of the schema classes listed above.  If it seems
/// onerous to need to construct a secondary schema object to interact with
/// Inputs and Outputs, keep in mind that any function whose purpose is either
/// to walk material/shader networks via their connections, or to create such
/// networks, can typically be written entirely in terms of 
/// UsdShadeConnectableAPI objects, without needing to care what the underlying
/// prim type is.
/// 
/// Additionally, the most common UsdShadeConnectableAPI behaviors
/// (creating Inputs and Outputs, and making connections) are wrapped as
/// convenience methods on the prim schema classes (creation) and 
/// UsdShadeInput and UsdShadeOutput.
/// 
///
class UsdShadeConnectableAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::NonAppliedAPI;

    /// Construct a UsdShadeConnectableAPI on UsdPrim \p prim .
    /// Equivalent to UsdShadeConnectableAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdShadeConnectableAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdShadeConnectableAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdShadeConnectableAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdShadeConnectableAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDSHADE_API
    virtual ~UsdShadeConnectableAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDSHADE_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdShadeConnectableAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdShadeConnectableAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDSHADE_API
    static UsdShadeConnectableAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    USDSHADE_API
    UsdSchemaType _GetSchemaType() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDSHADE_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDSHADE_API
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
    
protected:
    /// Returns true if the given prim is compatible with this API schema,
    /// i.e. if it is a valid shader or a node-graph.
    USDSHADE_API
    bool _IsCompatible() const override;
    
public:

    /// Constructor that takes a UsdShadeShader.
    /// Allow implicit (auto) conversion of UsdShadeShader to 
    /// UsdShadeConnectableAPI, so that a shader can be passed into any function
    /// that accepts a ConnectableAPI.
    UsdShadeConnectableAPI(const UsdShadeShader &shader):
        UsdShadeConnectableAPI(shader.GetPrim())
    { }

    /// Constructor that takes a UsdShadeNodeGraph.
    /// Allow implicit (auto) conversion of UsdShadeNodeGraph to 
    /// UsdShadeConnectableAPI, so that a nodegraph can be passed into any function
    /// that accepts a ConnectableAPI.
    UsdShadeConnectableAPI(const UsdShadeNodeGraph &nodeGraph):
        UsdShadeConnectableAPI(nodeGraph.GetPrim())
    { }

    /// Returns true if the prim is a shader.
    USDSHADE_API
    bool IsShader() const;

    /// Returns true if the prim is a node-graph.
    USDSHADE_API
    bool IsNodeGraph() const;

    /// \name Connections 
    /// 
    /// Inputs and outputs on shaders and node-graphs are connectable.
    /// This section provides API for authoring and managing these connections
    /// in a shading network.
    /// 
    /// @{

    /// Determines whether the given input can be connected to the given 
    /// source attribute, which can be an input or an output.
    /// 
    /// The result depends on the "connectability" of the input and the source 
    /// attributes. 
    /// 
    /// \sa UsdShadeInput::SetConnectability
    /// \sa UsdShadeInput::GetConnectability
    USDSHADE_API
    static bool CanConnect(const UsdShadeInput &input, 
                           const UsdAttribute &source);

    /// \overload
    USDSHADE_API
    static bool CanConnect(const UsdShadeInput &input, 
                           const UsdShadeInput &sourceInput) {
        return CanConnect(input, sourceInput.GetAttr());
    }

    /// \overload
    USDSHADE_API
    static bool CanConnect(const UsdShadeInput &input, 
                           const UsdShadeOutput &sourceOutput) {
        return CanConnect(input, sourceOutput.GetAttr());
    }

    /// Determines whether the given output can be connected to the given 
    /// source attribute, which can be an input or an output.
    /// 
    /// An output is considered to be connectable only if it belongs to a 
    /// node-graph. Shader outputs are not connectable.
    /// 
    /// \p source is an optional argument. If a valid UsdAttribute is supplied
    /// for it, this method will return true only if the source attribute is 
    /// owned by a descendant of the node-graph owning the output.
    ///
    USDSHADE_API
    static bool CanConnect(const UsdShadeOutput &output, 
                           const UsdAttribute &source=UsdAttribute());

    /// \overload
    USDSHADE_API
    static bool CanConnect(const UsdShadeOutput &output, 
                           const UsdShadeInput &sourceInput) {
        return CanConnect(output, sourceInput.GetAttr());
    }

    /// \overload
    USDSHADE_API
    static bool CanConnect(const UsdShadeOutput &output, 
                           const UsdShadeOutput &sourceOutput) {
        return CanConnect(output, sourceOutput.GetAttr());
    }

    /// Authors a connection for a given shading property \p shadingProp. 
    /// 
    /// \p shadingProp can represent a parameter, an interface attribute, an
    /// input or an output.
    /// \p sourceName is the name of the shading property that is the target
    /// of the connection. This excludes any namespace prefix that determines 
    /// the type of the source (eg, output or interface attribute).
    /// \p sourceType is used to indicate the type of the shading property 
    /// that is the target of the connection. The source type is used to 
    /// determine the namespace prefix that must be attached to \p sourceName
    /// to determine the source full property name.
    /// \p typeName if specified, is the typename of the attribute to create 
    /// on the source if it doesn't exist. It is also used to validate whether 
    /// the types of the source and consumer of the connection are compatible.
    /// \p source is the connectable prim that produces or contains a value 
    /// for the given shading property.
    /// 
    /// \return 
    /// \c true if a connection was created successfully. 
    /// \c false if \p shadingProp or \p source is invalid.
    /// 
    /// \note This method does not verify the connectability of the shading
    /// property to the source. Clients must invoke CanConnect() themselves
    /// to ensure compatibility.
    /// \note The source shading property is created if it doesn't exist 
    /// already.
    ///
    USDSHADE_API
    static bool ConnectToSource(
        UsdProperty const &shadingProp,
        UsdShadeConnectableAPI const &source, 
        TfToken const &sourceName, 
        UsdShadeAttributeType const sourceType=UsdShadeAttributeType::Output,
        SdfValueTypeName typeName=SdfValueTypeName());

    /// \overload
    USDSHADE_API
    static bool ConnectToSource(
        UsdShadeInput const &input,
        UsdShadeConnectableAPI const &source, 
        TfToken const &sourceName, 
        UsdShadeAttributeType const sourceType=UsdShadeAttributeType::Output,
        SdfValueTypeName typeName=SdfValueTypeName()) 
    {
        return ConnectToSource(input.GetAttr(), source, sourceName, sourceType, 
            typeName);
    }

    /// \overload
    USDSHADE_API
    static bool ConnectToSource(
        UsdShadeOutput const &output,
        UsdShadeConnectableAPI const &source, 
        TfToken const &sourceName, 
        UsdShadeAttributeType const sourceType=UsdShadeAttributeType::Output,
        SdfValueTypeName typeName=SdfValueTypeName()) 
    {
        return ConnectToSource(output.GetProperty(), source, sourceName, sourceType, 
            typeName);
    }

    /// \overload
    /// 
    /// Connect the given shading property to the source at path, \p sourcePath. 
    /// 
    /// \p sourcePath should be the fully namespaced property path. 
    /// 
    /// This overload is provided for convenience, for use in contexts where 
    /// the prim types are unknown or unavailable.
    /// 
    USDSHADE_API
    static bool ConnectToSource(UsdProperty const &shadingProp, 
                                SdfPath const &sourcePath);

    /// \overload
    USDSHADE_API
    static bool ConnectToSource(UsdShadeInput const &input, 
                                SdfPath const &sourcePath) {
        return ConnectToSource(input.GetAttr(), sourcePath);
    }

    /// \overload
    USDSHADE_API
    static bool ConnectToSource(UsdShadeOutput const &output, 
                                SdfPath const &sourcePath) {
        return ConnectToSource(output.GetProperty(), sourcePath);
    }

    /// \overload 
    /// 
    /// Connect the given shading property to the given source input. 
    /// 
    USDSHADE_API
    static bool ConnectToSource(UsdProperty const &shadingProp, 
                                UsdShadeInput const &sourceInput);

    /// \overload
    USDSHADE_API
    static bool ConnectToSource(UsdShadeInput const &input, 
                                UsdShadeInput const &sourceInput) {
        return ConnectToSource(input.GetAttr(), sourceInput);
    }

    /// \overload
    USDSHADE_API
    static bool ConnectToSource(UsdShadeOutput const &output, 
                                UsdShadeInput const &sourceInput) {
        return ConnectToSource(output.GetProperty(), sourceInput);                                
    }

    /// \overload 
    /// 
    /// Connect the given shading property to the given source output. 
    /// 
    USDSHADE_API
    static bool ConnectToSource(UsdProperty const &shadingProp, 
                                UsdShadeOutput const &sourceOutput);

    /// \overload 
    USDSHADE_API
    static bool ConnectToSource(UsdShadeInput const &input, 
                                UsdShadeOutput const &sourceOutput) {
        return ConnectToSource(input.GetAttr(), sourceOutput);                            
    }

    /// \overload 
    USDSHADE_API
    static bool ConnectToSource(UsdShadeOutput const &output, 
                                UsdShadeOutput const &sourceOutput) {
        return ConnectToSource(output.GetProperty(), sourceOutput);
    }

private:
    /// \deprecated 
    /// Provided for use by UsdRiLookAPI and UsdRiMaterialAPI to author 
    /// old-style interface attribute connections, which require the 
    /// \p renderTarget argument. 
    /// 
    static bool _ConnectToSource(
        UsdProperty const &shadingProp,
        UsdShadeConnectableAPI const &source, 
        TfToken const &sourceName, 
        TfToken const &renderTarget,
        UsdShadeAttributeType const sourceType=UsdShadeAttributeType::Output,
        SdfValueTypeName typeName=SdfValueTypeName());

protected:
    // Befriend UsdRiLookAPI and UsdRiMaterialAPI temporarily to assist in the
    // transition to the new shading encoding.
    friend class UsdRiLookAPI;
    friend class UsdRiMaterialAPI;
    
    /// \deprecated
    /// Connect the given shading property to the given source input. 
    /// 
    /// Provided for use by UsdRiLookAPI and UsdRiMaterialAPI to author 
    /// old-style interface attribute connections, which require the 
    /// \p renderTarget argument. 
    /// 
    USDSHADE_API
    static bool _ConnectToSource(UsdProperty const &shadingProp, 
                                UsdShadeInput const &sourceInput,
                                TfToken const &renderTarget);
    
public:

    /// Finds the source of a connection for the given shading property.
    /// 
    /// \p shadingProp is the input shading property which is typically an 
    /// attribute, but can be a relationship in the case of a terminal on a 
    /// material.
    /// \p source is an output parameter which will be set to the source 
    /// connectable prim.
    /// \p sourceName will be set to the name of the source shading property, 
    /// which could be the parameter name, output name or the interface 
    /// attribute name. This does not include the namespace prefix associated 
    /// with the source type. 
    /// \p sourceType will have the type of the source shading property.
    ///
    /// \return 
    /// \c true if the shading property is connected to a valid, defined source.
    /// \c false if the shading property is not connected to a single, valid 
    /// source. 
    /// 
    /// \note The python wrapping for this method returns a 
    /// (source, sourceName, sourceType) tuple if the parameter is connected, 
    /// else \c None
    ///
    USDSHADE_API
    static bool GetConnectedSource(UsdProperty const &shadingProp,
                                   UsdShadeConnectableAPI *source, 
                                   TfToken *sourceName,
                                   UsdShadeAttributeType *sourceType);

    /// \overload
    USDSHADE_API
    static bool GetConnectedSource(UsdShadeInput const &input,
                                   UsdShadeConnectableAPI *source, 
                                   TfToken *sourceName,
                                   UsdShadeAttributeType *sourceType) {
        return GetConnectedSource(input.GetAttr(), source, sourceName, 
                                  sourceType);
    }

    /// \overload
    USDSHADE_API
    static bool GetConnectedSource(UsdShadeOutput const &output,
                                   UsdShadeConnectableAPI *source, 
                                   TfToken *sourceName,
                                   UsdShadeAttributeType *sourceType) {
        return GetConnectedSource(output.GetProperty(), source, sourceName, 
                                  sourceType);
    }

    /// Returns the "raw" (authored) connected source paths for the given 
    /// shading property.
    /// 
    USDSHADE_API
    static bool GetRawConnectedSourcePaths(UsdProperty const &shadingProp, 
                                           SdfPathVector *sourcePaths);

    /// \overload
    USDSHADE_API
    static bool GetRawConnectedSourcePaths(UsdShadeInput const &input, 
                                           SdfPathVector *sourcePaths) {
        return GetRawConnectedSourcePaths(input.GetAttr(), sourcePaths);
    }

    /// \overload
    USDSHADE_API
    static bool GetRawConnectedSourcePaths(UsdShadeOutput const &output, 
                                           SdfPathVector *sourcePaths) {
        return GetRawConnectedSourcePaths(output.GetProperty(), sourcePaths);
    }

    /// Returns true if and only if the shading property is currently connected 
    /// to a valid (defined) source. 
    ///
    /// If you will be calling GetConnectedSource() afterwards anyways, 
    /// it will be \em much faster to instead guard like so:
    /// \code
    /// if (UsdShadeConnectableAPI::GetConnectedSource(property, &source, 
    ///         &sourceName, &sourceType)){
    ///      // process connected property
    /// } else {
    ///      // process unconnected property
    /// }
    /// \endcode
    USDSHADE_API
    static bool HasConnectedSource(const UsdProperty &shadingProp);

    /// \overload
    USDSHADE_API
    static bool HasConnectedSource(const UsdShadeInput &input) {
        return HasConnectedSource(input.GetAttr());
    }

    /// \overload
    USDSHADE_API
    static bool HasConnectedSource(const UsdShadeOutput &output) {
        return HasConnectedSource(output.GetProperty());
    }

    /// Returns true if the connection to the given shading property's source, 
    /// as returned by UsdShadeConnectableAPI::GetConnectedSource(), is authored
    /// across a specializes arc, which is used to denote a base material.
    /// 
    USDSHADE_API
    static bool IsSourceConnectionFromBaseMaterial(
        const UsdProperty &shadingProp);

    /// \overload
    USDSHADE_API
    static bool IsSourceConnectionFromBaseMaterial(const UsdShadeInput &input) {
        return IsSourceConnectionFromBaseMaterial(input.GetAttr());
    }

    /// \overload
    USDSHADE_API
    static bool IsSourceConnectionFromBaseMaterial(const UsdShadeOutput &output) 
    {
        return IsSourceConnectionFromBaseMaterial(output.GetProperty());
    }

    /// Disconnect source for this shading property.
    ///
    /// This may author more scene description than you might expect - we define
    /// the behavior of disconnect to be that, even if a shading property 
    /// becomes connected in a weaker layer than the current UsdEditTarget, the
    /// property will \em still be disconnected in the composition, therefore
    /// we must "block" it (see for e.g. UsdRelationship::BlockTargets()) in
    /// the current UsdEditTarget. 
    ///
    /// \sa ConnectToSource().
    USDSHADE_API
    static bool DisconnectSource(UsdProperty const &shadingProp);

    /// \overload
    USDSHADE_API
    static bool DisconnectSource(UsdShadeInput const &input) {
        return DisconnectSource(input.GetAttr());
    }

    /// \overload
    USDSHADE_API
    static bool DisconnectSource(UsdShadeOutput const &output) {
        return DisconnectSource(output.GetProperty());
    }

    /// Clears source for this shading property in the current UsdEditTarget.
    ///
    /// Most of the time, what you probably want is DisconnectSource()
    /// rather than this function.
    ///
    /// \sa DisconnectSource()
    USDSHADE_API
    static bool ClearSource(UsdProperty const &shadingProp);

    /// \overload 
    USDSHADE_API
    static bool ClearSource(UsdShadeInput const &input) {
        return ClearSource(input.GetAttr());
    }

    /// \overload 
    USDSHADE_API
    static bool ClearSource(UsdShadeOutput const &output) {
        return ClearSource(output.GetProperty());
    }

    /// \deprecated
    /// 
    /// Returns whether authoring of bidirectional connections for the old-style 
    /// interface attributes is enabled. When this returns true, interface 
    /// attribute connections are authored both ways (using both 
    /// interfaceRecipientOf: and connectedSourceFor: relationships)
    /// 
    /// \note This method exists only for testing equality of the old and new
    /// encoding of shading networks in USD. 
    USDSHADE_API
    static bool AreBidirectionalInterfaceConnectionsEnabled();

    /// @}


    /// \name Outputs 
    /// @{

    /// Create an output, which represents and externally computed, typed value.
    /// Outputs on node-graphs can be connected. 
    /// 
    /// The attribute representing an output is created in the "outputs:" 
    /// namespace.
    /// 
    USDSHADE_API
    UsdShadeOutput CreateOutput(const TfToken& name,
                                const SdfValueTypeName& typeName) const;

    /// Return the requested output if it exists.
    /// 
    /// \p name is the unnamespaced base name.
    ///
    USDSHADE_API
    UsdShadeOutput GetOutput(const TfToken &name) const;

    /// Returns all outputs on the connectable prim (i.e. shader or node-graph). 
    /// Outputs are represented by attributes in the "outputs:" namespace.
    /// 
    USDSHADE_API
    std::vector<UsdShadeOutput> GetOutputs() const;

    /// @}

    /// \name Inputs 
    /// @{
        
    /// Create an input which can both have a value and be connected.
    /// The attribute representing the input is created in the "inputs:" 
    /// namespace.
    /// 
    USDSHADE_API
    UsdShadeInput CreateInput(const TfToken& name,
                               const SdfValueTypeName& typeName) const;

    /// Return the requested input if it exists.
    /// 
    /// \p name is the unnamespaced base name.
    /// 
    USDSHADE_API
    UsdShadeInput GetInput(const TfToken &name) const;

    /// Returns all inputs on the connectable prim (i.e. shader or node-graph). 
    /// Inputs are represented by attributes in the "inputs:" namespace.
    /// 
    USDSHADE_API
    std::vector<UsdShadeInput> GetInputs() const;

    /// @}

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
