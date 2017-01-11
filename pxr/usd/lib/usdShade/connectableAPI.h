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

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/subgraph.h"

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

    /// Authors a connection, given the associated output relationship, \p rel, 
    /// the source prim to which to connect to and the name of the output to 
    /// connect to. 
    /// \p typeName is used to validate whether the types of the source and 
    /// target of the connection are compatible.
    /// \p outputIsParameter is used to indicate that the source of the 
    /// connection is a UsdShadeParameter and not a UsdShadeOutput.
    static bool MakeConnection(
        UsdRelationship const &rel,
        UsdShadeConnectableAPI const &sourceShader, 
        TfToken const &outputName, 
        SdfValueTypeName typeName,
        bool outputIsParameter);

    /// Evaluates the source of a connection.
    /// XXX: Maybe this should return a UsdShadeParameter.
    /// (or some common base class of UsdShadeParameter and UsdShadeOutput)
    static bool EvaluateConnection(
        UsdRelationship const &connection,
        UsdShadeConnectableAPI *source, 
        TfToken *outputName);

    /// Create an output which can either have a value or can be connected.
    /// The attribute representing the output is created in the "outputs:" 
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
