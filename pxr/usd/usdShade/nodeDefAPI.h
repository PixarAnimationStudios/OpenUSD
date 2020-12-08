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
#ifndef USDSHADE_GENERATED_NODEDEFAPI_H
#define USDSHADE_GENERATED_NODEDEFAPI_H

/// \file usdShade/nodeDefAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdShade/tokens.h"

#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/sdr/shaderNode.h"
    

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// NODEDEFAPI                                                                 //
// -------------------------------------------------------------------------- //

/// \class UsdShadeNodeDefAPI
///
/// UsdShadeNodeDefAPI is an API schema that provides attributes
/// for a prim to select a corresponding Shader Node Definition ("Sdr Node"),
/// as well as to look up a runtime entry for that shader node in the
/// form of an SdrShaderNode.
/// 
/// UsdShadeNodeDefAPI is intended to be a pre-applied API schema for any
/// prim type that wants to refer to the SdrRegistry for further implementation
/// details about the behavior of that prim.  The primary use in UsdShade
/// itself is as UsdShadeShader, which is a basis for material shading networks
/// (UsdShadeMaterial), but this is intended to be used in other domains
/// that also use the Sdr node mechanism.
/// 
/// This schema provides properties that allow a prim to identify an external
/// node definition, either by a direct identifier key into the SdrRegistry
/// (info:id), an asset to be parsed by a suitable NdrParserPlugin
/// (info:sourceAsset), or an inline source code that must also be parsed
/// (info:sourceCode); as well as a selector attribute to determine which
/// specifier is active (info:implementationSource).
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdShadeTokens.
/// So to set an attribute to the value "rightHanded", use UsdShadeTokens->rightHanded
/// as the value.
///
class UsdShadeNodeDefAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// \deprecated
    /// Same as schemaKind, provided to maintain temporary backward 
    /// compatibility with older generated schemas.
    static const UsdSchemaKind schemaType = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdShadeNodeDefAPI on UsdPrim \p prim .
    /// Equivalent to UsdShadeNodeDefAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdShadeNodeDefAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdShadeNodeDefAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdShadeNodeDefAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdShadeNodeDefAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDSHADE_API
    virtual ~UsdShadeNodeDefAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDSHADE_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdShadeNodeDefAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdShadeNodeDefAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDSHADE_API
    static UsdShadeNodeDefAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "NodeDefAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdShadeNodeDefAPI object is returned upon success. 
    /// An invalid (or empty) UsdShadeNodeDefAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDSHADE_API
    static UsdShadeNodeDefAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDSHADE_API
    UsdSchemaKind _GetSchemaKind() const override;

    /// \deprecated
    /// Same as _GetSchemaKind, provided to maintain temporary backward 
    /// compatibility with older generated schemas.
    USDSHADE_API
    UsdSchemaKind _GetSchemaType() const override;

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
    // --------------------------------------------------------------------- //
    // IMPLEMENTATIONSOURCE 
    // --------------------------------------------------------------------- //
    /// Specifies the attribute that should be consulted to get the 
    /// shader's implementation or its source code.
    /// 
    /// * If set to "id", the "info:id" attribute's value is used to 
    /// determine the shader source from the shader registry.
    /// * If set to "sourceAsset", the resolved value of the "info:sourceAsset" 
    /// attribute corresponding to the desired implementation (or source-type)
    /// is used to locate the shader source.  A source asset file may also
    /// specify multiple shader definitions, so there is an optional attribute
    /// "info:sourceAsset:subIdentifier" whose value should be used to indicate
    /// a particular shader definition from a source asset file.
    /// * If set to "sourceCode", the value of "info:sourceCode" attribute 
    /// corresponding to the desired implementation (or source type) is used as 
    /// the shader source.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token info:implementationSource = "id"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdShadeTokens "Allowed Values" | id, sourceAsset, sourceCode |
    USDSHADE_API
    UsdAttribute GetImplementationSourceAttr() const;

    /// See GetImplementationSourceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSHADE_API
    UsdAttribute CreateImplementationSourceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ID 
    // --------------------------------------------------------------------- //
    /// The id is an identifier for the type or purpose of the 
    /// shader. E.g.: Texture or FractalFloat.
    /// The use of this id will depend on the render target: some will turn it
    /// into an actual shader path, some will use it to generate shader source 
    /// code dynamically.
    /// 
    /// \sa SetShaderId()
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token info:id` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
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


    // -------------------------------------------------------------------------
    /// \anchor UsdShadeNodeDefAPI_ImplementationSource
    /// \name Shader Source API
    /// 
    /// This section provides API for identifying the source of a shader's 
    /// implementation.
    /// 
    /// @{

    /// Reads the value of info:implementationSource attribute and returns a 
    /// token identifying the attribute that must be consulted to identify the 
    /// shader's source program.
    /// 
    /// This returns 
    /// * <b>id</b>, to indicate that the "info:id" attribute must be 
    /// consulted.
    /// * <b>sourceAsset</b> to indicate that the asset-valued 
    /// "info:{sourceType}:sourceAsset" attribute associated with the desired 
    /// <b>sourceType</b> should be consulted to locate the asset with the 
    /// shader's source.
    /// * <b>sourceCode</b> to indicate that the string-valued 
    /// "info:{sourceType}:sourceCode" attribute associated with the desired
    /// <b>sourceType</b> should be read to get shader's source.
    ///
    /// This issues a warning and returns <b>id</b> if the 
    /// <i>info:implementationSource</i> attribute has an invalid value.
    /// 
    /// <i>{sourceType}</i> above is a place holder for a token that identifies 
    /// the type of shader source or its implementation. For example: osl, 
    /// glslfx, riCpp etc. This allows a shader to specify different sourceAsset
    /// (or sourceCode) values for different sourceTypes. The sourceType tokens 
    /// usually correspond to the sourceType value of the NdrParserPlugin that's 
    /// used to parse the shader source (\ref NdrParserPlugin::SourceType).
    /// 
    /// When sourceType is empty, the corresponding sourceAsset or sourceCode is 
    /// considered to be "universal" (or fallback), which is represented by the 
    /// empty-valued token UsdShadeTokens->universalSourceType. When the 
    /// sourceAsset (or sourceCode) corresponding to a specific, requested 
    /// sourceType is unavailable, the universal sourceAsset (or sourceCode) is 
    /// returned by GetSourceAsset (and GetSourceCode} API, if present.
    /// 
    /// \sa GetShaderId()
    /// \sa GetSourceAsset()
    /// \sa GetSourceCode()
    USDSHADE_API
    TfToken GetImplementationSource() const; 
    
    /// Sets the shader's ID value. This also sets the 
    /// <i>info:implementationSource</i> attribute on the shader to
    /// <b>UsdShadeTokens->id</b>, if the existing value is different.
    USDSHADE_API
    bool SetShaderId(const TfToken &id) const; 

    /// Fetches the shader's ID value from the <i>info:id</i> attribute, if the 
    /// shader's <i>info:implementationSource</i> is <b>id</b>. 
    /// 
    /// Returns <b>true</b> if the shader's implementation source is <b>id</b> 
    /// and the value was fetched properly into \p id. Returns false otherwise.
    /// 
    /// \sa GetImplementationSource()
    USDSHADE_API
    bool GetShaderId(TfToken *id) const;

    /// Sets the shader's source-asset path value to \p sourceAsset for the 
    /// given source type, \p sourceType.
    /// 
    /// This also sets the <i>info:implementationSource</i> attribute on the 
    /// shader to <b>UsdShadeTokens->sourceAsset</b>.
    USDSHADE_API
    bool SetSourceAsset(
        const SdfAssetPath &sourceAsset,
        const TfToken &sourceType=UsdShadeTokens->universalSourceType) const;

    /// Fetches the shader's source asset value for the specified 
    /// \p sourceType value from the <b>info:<i>sourceType:</i>sourceAsset</b> 
    /// attribute, if the shader's <i>info:implementationSource</i> is 
    /// <b>sourceAsset</b>. 
    /// 
    /// If the <i>sourceAsset</i> attribute corresponding to the requested 
    /// <i>sourceType</i> isn't present on the shader, then the <i>universal</i> 
    /// <i>fallback</i> sourceAsset attribute, i.e. <i>info:sourceAsset</i> is 
    /// consulted, if present, to get the source asset path.
    /// 
    /// Returns <b>true</b> if the shader's implementation source is 
    /// <b>sourceAsset</b> and the source asset path value was fetched 
    /// successfully into \p sourceAsset. Returns false otherwise.
    /// 
    /// \sa GetImplementationSource()
    USDSHADE_API
    bool GetSourceAsset(
        SdfAssetPath *sourceAsset,
        const TfToken &sourceType=UsdShadeTokens->universalSourceType) const;

    /// Set a sub-identifier to be used with a source asset of the given source
    /// type.  This sets the <b>info:<i>sourceType:</i>sourceAsset:subIdentifier
    /// </b>.
    ///
    /// This also sets the <i>info:implementationSource</i> attribute on the
    /// shader to <b>UsdShadeTokens->sourceAsset</b>
    USDSHADE_API
    bool SetSourceAssetSubIdentifier(
        const TfToken &subIdentifier,
        const TfToken &sourceType=UsdShadeTokens->universalSourceType) const;

    /// Fetches the shader's sub-identifier for the source asset with the
    /// specified \p sourceType value from the <b>info:<i>sourceType:</i>
    /// sourceAsset:subIdentifier</b> attribute, if the shader's <i>info:
    /// implementationSource</i> is <b>sourceAsset</b>.
    ///
    /// If the <i>subIdentifier</i> attribute corresponding to the requested
    /// <i>sourceType</i> isn't present on the shader, then the <i>universal</i>
    /// <i>fallback</i> sub-identifier attribute, i.e. <i>info:sourceAsset:
    /// subIdentifier</i> is consulted, if present, to get the sub-identifier
    /// name.
    ///
    /// Returns <b>true</b> if the shader's implementation source is
    /// <b>sourceAsset</b> and the sub-identifier for the given source type was
    /// fetched successfully into \p subIdentifier. Returns false otherwise.
    USDSHADE_API
    bool GetSourceAssetSubIdentifier(
        TfToken *subIdentifier,
        const TfToken &sourceType=UsdShadeTokens->universalSourceType) const;

    /// Sets the shader's source-code value to \p sourceCode for the given 
    /// source type, \p sourceType.
    /// 
    /// This also sets the <i>info:implementationSource</i> attribute on the 
    /// shader to <b>UsdShadeTokens->sourceCode</b>.
    USDSHADE_API 
    bool SetSourceCode(
        const std::string &sourceCode, 
        const TfToken &sourceType=UsdShadeTokens->universalSourceType) const;

    /// Fetches the shader's source code for the specified \p sourceType value 
    /// by reading the <b>info:<i>sourceType:</i>sourceCode</b> attribute, if 
    /// the shader's <i>info:implementationSource</i> is <b>sourceCode</b>. 
    /// 
    /// If the <i>sourceCode</i> attribute corresponding to the 
    /// requested <i>sourceType</i> isn't present on the shader, then the 
    /// <i>universal</i> or <i>fallback</i> sourceCode attribute (i.e. 
    /// <i>info:sourceCode</i>) is consulted, if present, to get the source 
    /// code.
    /// 
    /// Returns <b>true</b> if the shader's implementation source is 
    /// <b>sourceCode</b> and the source code string was fetched successfully 
    /// into \p sourceCode. Returns false otherwise.    
    /// 
    /// \sa GetImplementationSource()
    USDSHADE_API
    bool GetSourceCode(
        std::string *sourceCode,
        const TfToken &sourceType=UsdShadeTokens->universalSourceType) const;

    /// @}
    // -------------------------------------------------------------------------

    /// This method attempts to ensure that there is a ShaderNode in the shader 
    /// registry (i.e. \ref SdrRegistry) representing this shader for the 
    /// given \p sourceType. It may return a null pointer if none could be 
    /// found or created.
    USDSHADE_API
    SdrShaderNodeConstPtr GetShaderNodeForSourceType(const TfToken &sourceType) 
        const; 

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
