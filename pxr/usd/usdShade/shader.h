//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDSHADE_GENERATED_SHADER_H
#define USDSHADE_GENERATED_SHADER_H

/// \file usdShade/shader.h

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/sdr/declare.h"
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
/// The UsdShadeNodeDefAPI provides attributes to uniquely identify the
/// type of this node.  The id resolution into a renderable shader target
/// type of this node.  The id resolution into a renderable shader target
/// is deferred to the consuming application.
/// 
/// The purpose of representing them in Usd is two-fold:
/// \li To represent, via "connections" the topology of the shading network
/// that must be reconstructed in the renderer. Facilities for authoring and 
/// manipulating connections are encapsulated in the API schema 
/// UsdShadeConnectableAPI.
/// \li To present a (partial or full) interface of typed input parameters 
/// whose values can be set and overridden in Usd, to be provided later at 
/// render-time as parameter values to the actual render shader objects. Shader 
/// input parameters are encapsulated in the property schema UsdShadeInput.
/// 
///
class UsdShadeShader : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

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

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDSHADE_API
    UsdSchemaKind _GetSchemaKind() const override;

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

    // -------------------------------------------------------------------------
    /// \name Conversion to and from UsdShadeConnectableAPI
    /// 
    /// @{

    /// Constructor that takes a ConnectableAPI object.
    /// Allow implicit (auto) conversion of UsdShadeConnectableAPI to 
    /// UsdShadeShader, so that a ConnectableAPI can be passed into any function
    /// that accepts a Shader.
    ///
    /// \note that the conversion may produce an invalid Shader object, because
    /// not all UsdShadeConnectableAPI%s are Shader%s
    USDSHADE_API
    UsdShadeShader(const UsdShadeConnectableAPI &connectable);

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
    USDSHADE_API
    UsdShadeOutput CreateOutput(const TfToken& name,
                                const SdfValueTypeName& typeName);

    /// Return the requested output if it exists.
    /// 
    USDSHADE_API
    UsdShadeOutput GetOutput(const TfToken &name) const;

    /// Outputs are represented by attributes in the "outputs:" namespace.
    /// If \p onlyAuthored is true (the default), then only return authored
    /// attributes; otherwise, this also returns un-authored builtins.
    /// 
    USDSHADE_API
    std::vector<UsdShadeOutput> GetOutputs(bool onlyAuthored=true) const;

    /// @}

    // ------------------------------------------------------------------------- 

    /// \name Inputs API
    ///
    /// Inputs are connectable attribute with a typed value. 
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
    /// If \p onlyAuthored is true (the default), then only return authored
    /// attributes; otherwise, this also returns un-authored builtins.
    /// 
    USDSHADE_API
    std::vector<UsdShadeInput> GetInputs(bool onlyAuthored=true) const;

    /// @}

    // -------------------------------------------------------------------------
    /// \name UsdShadeNodeDefAPI forwarding
    /// 
    /// @{

    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API
    UsdAttribute GetImplementationSourceAttr() const;

    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API
    UsdAttribute CreateImplementationSourceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API
    UsdAttribute GetIdAttr() const;

    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API
    UsdAttribute CreateIdAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API
    TfToken GetImplementationSource() const; 
    
    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API
    bool SetShaderId(const TfToken &id) const; 

    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API
    bool GetShaderId(TfToken *id) const;

    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API
    bool SetSourceAsset(
        const SdfAssetPath &sourceAsset,
        const TfToken &sourceType=UsdShadeTokens->universalSourceType) const;

    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API
    bool GetSourceAsset(
        SdfAssetPath *sourceAsset,
        const TfToken &sourceType=UsdShadeTokens->universalSourceType) const;

    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API
    bool SetSourceAssetSubIdentifier(
        const TfToken &subIdentifier,
        const TfToken &sourceType=UsdShadeTokens->universalSourceType) const;

    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API
    bool GetSourceAssetSubIdentifier(
        TfToken *subIdentifier,
        const TfToken &sourceType=UsdShadeTokens->universalSourceType) const;

    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API 
    bool SetSourceCode(
        const std::string &sourceCode, 
        const TfToken &sourceType=UsdShadeTokens->universalSourceType) const;

    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API
    bool GetSourceCode(
        std::string *sourceCode,
        const TfToken &sourceType=UsdShadeTokens->universalSourceType) const;
    
    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API
    std::vector<std::string> GetSourceTypes() const; 

    /// Forwards to UsdShadeNodeDefAPI(prim).
    USDSHADE_API
    SdrShaderNodeConstPtr GetShaderNodeForSourceType(const TfToken &sourceType) 
        const; 

    /// @}

    // -------------------------------------------------------------------------

    /// \anchor UsdShadeShader_SdrMetadata_API
    /// \name Shader Sdr Metadata API
    /// 
    /// This section provides API for authoring and querying shader registry
    /// metadata. When the shader's implementationSource is <b>sourceAsset</b> 
    /// or <b>sourceCode</b>, the authored "sdrMetadata" dictionary value 
    /// provides additional metadata needed to process the shader source
    /// correctly. It is used in combination with the sourceAsset or sourceCode
    /// value to fetch the appropriate node from the shader registry.
    /// 
    /// We expect the keys in sdrMetadata to correspond to the keys 
    /// in \ref SdrNodeMetadata. However, this is not strictly enforced in the 
    /// API. The only allowed value type in the "sdrMetadata" dictionary is a 
    /// std::string since it needs to be converted into a SdrTokenMap, which Sdr
    /// will parse using the utilities available in \ref SdrMetadataHelpers.
    /// 
    /// @{

    /// Returns this shader's composed "sdrMetadata" dictionary as a 
    /// SdrTokenMap.
    USDSHADE_API
    SdrTokenMap GetSdrMetadata() const;
    
    /// Returns the value corresponding to \p key in the composed 
    /// <b>sdrMetadata</b> dictionary.
    USDSHADE_API
    std::string GetSdrMetadataByKey(const TfToken &key) const;
        
    /// Authors the given \p sdrMetadata on this shader at the current 
    /// EditTarget.
    USDSHADE_API
    void SetSdrMetadata(const SdrTokenMap &sdrMetadata) const;

    /// Sets the value corresponding to \p key to the given string \p value, in 
    /// the shader's "sdrMetadata" dictionary at the current EditTarget.
    USDSHADE_API
    void SetSdrMetadataByKey(
        const TfToken &key, 
        const std::string &value) const;

    /// Returns true if the shader has a non-empty composed "sdrMetadata" 
    /// dictionary value.
    USDSHADE_API
    bool HasSdrMetadata() const;

    /// Returns true if there is a value corresponding to the given \p key in 
    /// the composed "sdrMetadata" dictionary.
    USDSHADE_API
    bool HasSdrMetadataByKey(const TfToken &key) const;

    /// Clears any "sdrMetadata" value authored on the shader in the current 
    /// EditTarget.
    USDSHADE_API
    void ClearSdrMetadata() const;

    /// Clears the entry corresponding to the given \p key in the 
    /// "sdrMetadata" dictionary authored in the current EditTarget.
    USDSHADE_API
    void ClearSdrMetadataByKey(const TfToken &key) const;

    /// @}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
