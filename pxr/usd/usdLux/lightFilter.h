//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDLUX_GENERATED_LIGHTFILTER_H
#define USDLUX_GENERATED_LIGHTFILTER_H

/// \file usdLux/lightFilter.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLux/tokens.h"

#include "pxr/usd/usd/collectionAPI.h"
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
// LIGHTFILTER                                                                //
// -------------------------------------------------------------------------- //

/// \class UsdLuxLightFilter
///
/// A light filter modifies the effect of a light.
/// Lights refer to filters via relationships so that filters may be
/// shared.
/// 
/// <b>Linking</b>
/// 
/// Filters can be linked to geometry.  Linking controls which geometry
/// a light-filter affects, when considering the light filters attached
/// to a light illuminating the geometry.
/// 
/// Linking is specified as a collection (UsdCollectionAPI) which can
/// be accessed via GetFilterLinkCollection().
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdLuxTokens.
/// So to set an attribute to the value "rightHanded", use UsdLuxTokens->rightHanded
/// as the value.
///
class UsdLuxLightFilter : public UsdGeomXformable
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdLuxLightFilter on UsdPrim \p prim .
    /// Equivalent to UsdLuxLightFilter::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLuxLightFilter(const UsdPrim& prim=UsdPrim())
        : UsdGeomXformable(prim)
    {
    }

    /// Construct a UsdLuxLightFilter on the prim held by \p schemaObj .
    /// Should be preferred over UsdLuxLightFilter(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLuxLightFilter(const UsdSchemaBase& schemaObj)
        : UsdGeomXformable(schemaObj)
    {
    }

    /// Destructor.
    USDLUX_API
    virtual ~UsdLuxLightFilter();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLUX_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLuxLightFilter holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLuxLightFilter(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLUX_API
    static UsdLuxLightFilter
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
    USDLUX_API
    static UsdLuxLightFilter
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDLUX_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDLUX_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDLUX_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // SHADERID 
    // --------------------------------------------------------------------- //
    /// Default ID for the light filter's shader. 
    /// This defines the shader ID for this light filter when a render context 
    /// specific shader ID is not available. 
    /// 
    /// \see GetShaderId
    /// \see GetShaderIdAttrForRenderContext
    /// \see SdrRegistry::GetShaderNodeByIdentifier
    /// \see SdrRegistry::GetShaderNodeByIdentifierAndType
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token lightFilter:shaderId = ""` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDLUX_API
    UsdAttribute GetShaderIdAttr() const;

    /// See GetShaderIdAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShaderIdAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
    /// Allow implicit conversion of UsdShadeConnectableAPI to
    /// UsdLuxLightFilter.
    USDLUX_API
    UsdLuxLightFilter(const UsdShadeConnectableAPI &connectable);

    /// Contructs and returns a UsdShadeConnectableAPI object with this light
    /// filter.
    ///
    /// Note that most tasks can be accomplished without explicitly constructing 
    /// a UsdShadeConnectable API, since connection-related API such as
    /// UsdShadeConnectableAPI::ConnectToSource() are static methods, and 
    /// UsdLuxLightFilter will auto-convert to a UsdShadeConnectableAPI when 
    /// passed to functions that want to act generically on a connectable
    /// UsdShadeConnectableAPI object.
    USDLUX_API
    UsdShadeConnectableAPI ConnectableAPI() const;

    /// @}

    // -------------------------------------------------------------------------
    /// \name Outputs API
    ///
    /// Outputs represent a typed attribute on a light filter whose value is 
    /// computed externally. 
    /// 
    /// @{

    /// Create an output which can either have a value or can be connected.
    /// The attribute representing the output is created in the "outputs:" 
    /// namespace. Outputs on a light filter cannot be connected, as their 
    /// value is assumed to be computed externally.
    /// 
    USDLUX_API
    UsdShadeOutput CreateOutput(const TfToken& name,
                                const SdfValueTypeName& typeName);

    /// Return the requested output if it exists.
    /// 
    USDLUX_API
    UsdShadeOutput GetOutput(const TfToken &name) const;

    /// Outputs are represented by attributes in the "outputs:" namespace.
    /// If \p onlyAuthored is true (the default), then only return authored
    /// attributes; otherwise, this also returns un-authored builtins.
    /// 
    USDLUX_API
    std::vector<UsdShadeOutput> GetOutputs(bool onlyAuthored=true) const;

    /// @}

    // ------------------------------------------------------------------------- 

    /// \name Inputs API
    ///
    /// Inputs are connectable attribute with a typed value. 
    /// 
    /// Light filter parameters are encoded as inputs. 
    /// 
    /// @{

    /// Create an input which can either have a value or can be connected.
    /// The attribute representing the input is created in the "inputs:" 
    /// namespace. Inputs on light filters are connectable.
    /// 
    USDLUX_API
    UsdShadeInput CreateInput(const TfToken& name,
                              const SdfValueTypeName& typeName);

    /// Return the requested input if it exists.
    /// 
    USDLUX_API
    UsdShadeInput GetInput(const TfToken &name) const;

    /// Inputs are represented by attributes in the "inputs:" namespace.
    /// If \p onlyAuthored is true (the default), then only return authored
    /// attributes; otherwise, this also returns un-authored builtins.
    /// 
    USDLUX_API
    std::vector<UsdShadeInput> GetInputs(bool onlyAuthored=true) const;

    /// @}

    /// Return the UsdCollectionAPI interface used for examining and
    /// modifying the filter-linking of this light filter.  Linking
    /// controls which geometry this light filter affects.
    USDLUX_API
    UsdCollectionAPI GetFilterLinkCollectionAPI() const;

    /// Returns the shader ID attribute for the given \p renderContext.
    ///
    /// If \p renderContext is non-empty, this will try to return an attribute
    /// named _lightFilter:shaderId_ with the namespace prefix \p renderContext.
    /// For example, if the passed in render context is "ri" then the attribute 
    /// returned by this function would have the following signature:
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token ri:lightFilter:shaderId` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// 
    /// If the render context is empty, this will return the default shader ID 
    /// attribute as returned by GetShaderIdAttr().
    USDLUX_API
    UsdAttribute GetShaderIdAttrForRenderContext(
        const TfToken &renderContext) const;

    /// Creates the shader ID attribute for the given \p renderContext.
    ///
    /// See GetShaderIdAttrForRenderContext(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateShaderIdAttrForRenderContext(
        const TfToken &renderContext,
        VtValue const &defaultValue = VtValue(), 
        bool writeSparsely=false) const;

    /// Return the light filter's shader ID for the given list of available 
    /// \p renderContexts.
    ///
    /// The shader ID returned by this function is the identifier to use when 
    /// looking up the shader definition for this light filter in the 
    /// \ref SdrRegistry "shader registry".
    /// 
    /// The render contexts are expected to be listed in priority order, so
    /// for each render context provided, this will try to find the shader ID 
    /// attribute specific to that render context (see 
    /// GetShaderIdAttrForRenderContext()) and will return the 
    /// value of the first one found that has a non-empty value. If no shader ID
    /// value can be found for any of the given render contexts or 
    /// \p renderContexts is empty, then this will return the value of the 
    /// default shader ID attribute (see GetShaderIdAttr()).
    USDLUX_API
    TfToken GetShaderId(const TfTokenVector &renderContexts) const;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
