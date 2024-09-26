//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDRENDER_GENERATED_VAR_H
#define USDRENDER_GENERATED_VAR_H

/// \file usdRender/var.h

#include "pxr/pxr.h"
#include "pxr/usd/usdRender/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdRender/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// RENDERVAR                                                                  //
// -------------------------------------------------------------------------- //

/// \class UsdRenderVar
///
/// A UsdRenderVar describes a custom data variable for
/// a render to produce.  The prim describes the source of the data, which
/// can be a shader output or an LPE (Light Path Expression), and also
/// allows encoding of (generally renderer-specific) parameters that
/// configure the renderer for computing the variable.
/// 
/// \note The name of the RenderVar prim drives the name of the data 
/// variable that the renderer will produce.
/// 
/// \note In the future, UsdRender may standardize RenderVar representation
/// for well-known variables under the sourceType `intrinsic`,
/// such as _r_, _g_, _b_, _a_, _z_, or _id_.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdRenderTokens.
/// So to set an attribute to the value "rightHanded", use UsdRenderTokens->rightHanded
/// as the value.
///
class UsdRenderVar : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdRenderVar on UsdPrim \p prim .
    /// Equivalent to UsdRenderVar::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdRenderVar(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdRenderVar on the prim held by \p schemaObj .
    /// Should be preferred over UsdRenderVar(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdRenderVar(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDRENDER_API
    virtual ~UsdRenderVar();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDRENDER_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdRenderVar holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdRenderVar(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDRENDER_API
    static UsdRenderVar
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
    USDRENDER_API
    static UsdRenderVar
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDRENDER_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDRENDER_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDRENDER_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // DATATYPE 
    // --------------------------------------------------------------------- //
    /// The type of this channel, as a USD attribute type.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token dataType = "color3f"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDRENDER_API
    UsdAttribute GetDataTypeAttr() const;

    /// See GetDataTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreateDataTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SOURCENAME 
    // --------------------------------------------------------------------- //
    /// The renderer should look for an output of this name
    /// as the computed value for the RenderVar.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string sourceName = ""` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDRENDER_API
    UsdAttribute GetSourceNameAttr() const;

    /// See GetSourceNameAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreateSourceNameAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SOURCETYPE 
    // --------------------------------------------------------------------- //
    /// 
    /// Indicates the type of the source.
    /// 
    /// - "raw": The name should be passed directly to the
    /// renderer.  This is the default behavior.
    /// - "primvar":  This source represents the name of a primvar.
    /// Some renderers may use this to ensure that the primvar
    /// is provided; other renderers may require that a suitable
    /// material network be provided, in which case this is simply
    /// an advisory setting.
    /// - "lpe":  Specifies a Light Path Expression in the
    /// [OSL Light Path Expressions language](https://github.com/imageworks/OpenShadingLanguage/wiki/OSL-Light-Path-Expressions) as the source for
    /// this RenderVar.  Some renderers may use extensions to
    /// that syntax, which will necessarily be non-portable.
    /// - "intrinsic":  This setting is currently unimplemented,
    /// but represents a future namespace for UsdRender to provide
    /// portable baseline RenderVars, such as camera depth, that
    /// may have varying implementations for each renderer.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token sourceType = "raw"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdRenderTokens "Allowed Values" | raw, primvar, lpe, intrinsic |
    USDRENDER_API
    UsdAttribute GetSourceTypeAttr() const;

    /// See GetSourceTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreateSourceTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
