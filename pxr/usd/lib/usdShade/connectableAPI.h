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
#include "pxr/usd/usd/schemaBase.h"
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
/// parameters and outputs.
/// 
///
class UsdShadeConnectableAPI : public UsdSchemaBase
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Construct a UsdShadeConnectableAPI on UsdPrim \p prim .
    /// Equivalent to UsdShadeConnectableAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdShadeConnectableAPI(const UsdPrim& prim=UsdPrim())
        : UsdSchemaBase(prim)
    {
    }

    /// Construct a UsdShadeConnectableAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdShadeConnectableAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdShadeConnectableAPI(const UsdSchemaBase& schemaObj)
        : UsdSchemaBase(schemaObj)
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
    
private:
    // Returns true if the given prim is compatible with this API schema,
    // i.e. if it is a shader or a node-graph.
    USDSHADE_API
    virtual bool _IsCompatible(const UsdPrim &prim) const;
    
public:

    /// Constructor that takes a UsdShadeShader.
    explicit UsdShadeConnectableAPI(const UsdShadeShader &shader):
        UsdShadeConnectableAPI(shader.GetPrim())
    {        
    }

    /// Constructor that takes a UsdShadeNodeGraph.
    explicit UsdShadeConnectableAPI(const UsdShadeNodeGraph &nodeGraph):
        UsdShadeConnectableAPI(nodeGraph.GetPrim())
    {        
    }

    /// Returns true if the prim is a shader.
    USDSHADE_API
    bool IsShader() const;

    /// Returns true if the prim is a node-graph.
    USDSHADE_API
    bool IsNodeGraph() const;

    /// Allow UsdShadeConnectableAPI to auto-convert to UsdShadeNodeGraph, so 
    /// you can pass in a UsdShadeConnectableAPI to any function that accepts 
    /// a UsdShadeNodeGraph.
    operator UsdShadeNodeGraph () {
        return UsdShadeNodeGraph(GetPrim());
    }

    /// Allow UsdShadeConnectableAPI to auto-convert to UsdShadeShader, so 
    /// you can pass in a UsdShadeConnectableAPI to any function that accepts 
    /// a UsdShadeShader.
    operator UsdShadeShader () {
        return UsdShadeShader(GetPrim());
    }

    /// \name Connections 
    /// 
    /// Inputs and outputs on shaders and node-graphs are connectable.
    /// This section provides API for authoring and managing these connections
    /// in a shading network.
    /// 
    /// @{

    /// Authors a connection for a given shading property \p shadingProp. 
    /// 
    /// \p shadingProp can represent a parameter, an interface attribute or 
    /// an output.
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
    /// Connect the given shading property to the given source input. 
    USDSHADE_API
    static bool ConnectToSource(UsdProperty const &shadingProp, 
                                UsdShadeInput const &sourceInput);

    /// \overload 
    /// Connect the given shading property to the given source output. 
    USDSHADE_API
    static bool ConnectToSource(UsdProperty const &shadingProp, 
                                UsdShadeOutput const &sourceOutput);

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
    static bool GetConnectedSource(
        UsdProperty const &shadingProp,
        UsdShadeConnectableAPI *source, 
        TfToken *sourceName,
        UsdShadeAttributeType *sourceType);

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

    /// Clears source for this shading property in the current UsdEditTarget.
    ///
    /// Most of the time, what you probably want is DisconnectSource()
    /// rather than this function.
    ///
    /// \sa DisconnectSource()
    USDSHADE_API
    static bool ClearSource(UsdProperty const &shadingProp);

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
