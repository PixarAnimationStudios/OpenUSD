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
#ifndef USDSHADE_GENERATED_SHADER_H
#define USDSHADE_GENERATED_SHADER_H

/// \file usdShade/shader.h

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdShade/tokens.h"

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
// SHADER                                                                     //
// -------------------------------------------------------------------------- //

/// \class UsdShadeShader
///
/// Base class for all USD shaders. Shaders are the building blocks
/// of shading networks. While UsdShadeShader objects are not target specific,
/// each renderer or application target may derive its own renderer-specific 
/// shader object types from this base, if needed.
/// 
/// Objects of this class generally represent a single shading object, whether
/// it exists in the target renderer or not. For example, a texture, a fractal,
/// or a mix node.
/// 
/// The main property of this class is the info:id token, which uniquely 
/// identifies the type of this node. The id resolution into a renderable 
/// shader target is deferred to the consuming application.
/// 
/// The purpose of representing them in Usd is two-fold:
/// \li To represent, via "connections" the topology of the shading network
/// that must be reconstructed in the renderer. Facilities for authoring and 
/// manipulating connections are encapsulated in the Has-A schema 
/// UsdShadeConnectableAPI.
/// \li To present a (partial or full) interface of typed input parameters 
/// whose values can be set and overridden in Usd, to be provided later at 
/// render-time as parameter values to the actual render shader objects. Shader 
/// input parameters are encapsulated in the property schema UsdShadeInput.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdShadeTokens.
/// So to set an attribute to the value "rightHanded", use UsdShadeTokens->rightHanded
/// as the value.
///
class UsdShadeShader : public UsdTyped
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

    /// Construct a UsdShadeShader on UsdPrim \p prim .
    /// Equivalent to UsdShadeShader::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdShadeShader(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdShadeShader on the prim held by \p schemaObj .
    /// Should be preferred over UsdShadeShader(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdShadeShader(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDSHADE_API
    virtual ~UsdShadeShader();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDSHADE_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdShadeShader holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdShadeShader(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDSHADE_API
    static UsdShadeShader
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
    static UsdShadeShader
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
    // --------------------------------------------------------------------- //
    // ID 
    // --------------------------------------------------------------------- //
    /// The id is an identifier for the type or purpose of the 
    /// shader. E.g.: Texture or FractalFloat.
    /// The use of this id will depend on the render target: some will turn it
    /// into an actual shader path, some will use it to generate dynamically
    /// a shader source code.
    /// 
    ///
    /// \n  C++ Type: TfToken
    /// \n  Usd Type: SdfValueTypeNames->Token
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDSHADE_API
    UsdAttribute GetIdAttr() const;

    /// See GetIdAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSHADE_API
    UsdAttribute CreateIdAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// Allow UsdShadeShader to auto-convert to UsdShadeConnectableAPI, so 
    /// you can pass in a UsdShadeShader to any function that accepts 
    /// a UsdShadeConnectableAPI.
    USDSHADE_API
    operator UsdShadeConnectableAPI () const;

    /// Contructs and returns a UsdShadeConnectableAPI object with this shader.
    ///
    /// Note that most tasks can be accomplished without explicitly constructing 
    /// a UsdShadeConnectable API, since connection-related API such as
    /// UsdShadeConnectableAPI::ConnectToSource() are static methods, and 
    /// UsdShadeShader will auto-convert to a UsdShadeConnectableAPI when 
    /// passed to functions that want to act generically on a connectable
    /// UsdShadeConnectableAPI object.
    USDSHADE_API
    UsdShadeConnectableAPI ConnectableAPI() const;

    /// \name Outputs API
    ///
    /// Outputs represent a typed property on a shader or node-graph whose value 
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


    /// \name Inputs API
    ///
    /// Inputs are connectable properties with a typed value. 
    /// 
    /// On shaders, the shader parameters are encoded as inputs. On node-graphs,
    /// interface attributes are represented as inputs.
    /// 
    /// @{
        
    /// Create an input which can either have a value or can be connected.
    /// The attribute representing the input is created in the "inputs:" 
    /// namespace. Inputs on both shaders and node-graphs are connectable.
    /// 
    USDSHADE_API
    UsdShadeInput CreateInput(const TfToken& name,
                              const SdfValueTypeName& typeName);

    /// Return the requested input if it exists.
    /// 
    USDSHADE_API
    UsdShadeInput GetInput(const TfToken &name) const;

    /// Inputs are represented by attributes in the "inputs:" namespace.
    /// 
    USDSHADE_API
    std::vector<UsdShadeInput> GetInputs() const;

    /// @}

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
