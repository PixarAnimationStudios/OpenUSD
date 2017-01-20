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

#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/subgraph.h"
    

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

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
    virtual ~UsdShadeConnectableAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
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
    static UsdShadeConnectableAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    virtual const TfType &_GetTfType() const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to close the class declaration with }; and complete the
    // include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--
    
private:
    // Returns true if the given prim is compatible with this API schema,
    // i.e. if it is a shader or a subgraph.
    virtual bool _IsCompatible(const UsdPrim &prim) const;
    
public:

    /// Constructor that takes a UsdShadeShader.
    explicit UsdShadeConnectableAPI(const UsdShadeShader &shader):
        UsdShadeConnectableAPI(shader.GetPrim())
    {        
    }

    /// Constructor that takes a UsdShadeSubgraph.
    explicit UsdShadeConnectableAPI(const UsdShadeSubgraph &subgraph):
        UsdShadeConnectableAPI(subgraph.GetPrim())
    {        
    }

    /// Returns true if the prim is a shader.
    bool IsShader() const;

    /// Returns true if the prim is a subgraph.
    bool IsSubgraph() const;

    /// Allow UsdShadeConnectableAPI to auto-convert to UsdShadeSubgraph, so 
    /// you can pass in a UsdShadeConnectableAPI to any function that accepts 
    /// a UsdShadeSubgraph.
    operator UsdShadeSubgraph () {
        return UsdShadeSubgraph(GetPrim());
    }

    /// Allow UsdShadeConnectableAPI to auto-convert to UsdShadeShader, so 
    /// you can pass in a UsdShadeConnectableAPI to any function that accepts 
    /// a UsdShadeShader.
    operator UsdShadeShader () {
        return UsdShadeShader(GetPrim());
    }

    /// Authors a connection for a given shading property \p shadingProp. 
    /// 
    /// \p shadingProp can represent a parameter, an interface attribute or 
    /// an output.
    /// \p source is the connectable prim that produces or contains a value 
    /// for the given shading property.
    /// \p sourceName is the name of the shading property that is the target
    /// of the connection. This excludes any namespace prefix that determines 
    /// the type of the source (eg, output or interface attribute).
    /// \p typeName is used to validate whether the types of the source and 
    /// target of the connection are compatible.
    /// \p sourceType is used to indicate the type of the shading property 
    /// that is the target of the connection. The source type is used to 
    /// determine the namespace prefix that must be attached to \p sourceName
    /// to determine the source full property name.
    /// 
    /// \return 
    /// \c true if a connection was created successfully. 
    /// \c false if \p shadingProp or \p source is invalid.
    /// 
    static bool MakeConnection(
        UsdProperty const &shadingProp,
        UsdShadeConnectableAPI const &source, 
        TfToken const &sourceName, 
        SdfValueTypeName typeName,
        UsdShadeAttributeType const sourceType);

    /// Evaluates the source of a connection for the given shading property.
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
    static bool EvaluateConnection(
        UsdProperty const &shadingProp,
        UsdShadeConnectableAPI *source, 
        TfToken *sourceName,
        UsdShadeAttributeType *sourceType);

    /// Returns the relationship that encodes the connection to the given 
    /// shading property, which can be a parameter, an output or an 
    /// interface attribute.
    static UsdRelationship GetConnectionRel(
        const UsdProperty &shadingProp, 
        bool create);

    /// Create an output, which represents and externally computed, typed value.
    /// Outputs on subgraphs can be connected. 
    /// 
    /// The attribute representing an output is created in the "outputs:" 
    /// namespace.
    /// 
    UsdShadeOutput CreateOutput(const TfToken& name,
                                const SdfValueTypeName& typeName) const;

    /// Return the requested output if it exists.
    /// 
    UsdShadeOutput GetOutput(const TfToken &name) const;

    /// Returns all outputs on the connectable prim (i.e. shader or subgraph). 
    /// Outputs are represented by attributes in the "outputs" namespace.
    /// 
    std::vector<UsdShadeOutput> GetOutputs() const;

};

#endif
