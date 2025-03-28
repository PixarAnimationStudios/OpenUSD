//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDRENDER_GENERATED_PRODUCT_H
#define USDRENDER_GENERATED_PRODUCT_H

/// \file usdRender/product.h

#include "pxr/pxr.h"
#include "pxr/usd/usdRender/api.h"
#include "pxr/usd/usdRender/settingsBase.h"
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
// RENDERPRODUCT                                                              //
// -------------------------------------------------------------------------- //

/// \class UsdRenderProduct
///
/// A UsdRenderProduct describes an image or other
/// file-like artifact produced by a render. A RenderProduct
/// combines one or more RenderVars into a file or interactive
/// buffer.  It also provides all the controls established in
/// UsdRenderSettingsBase as optional overrides to whatever the
/// owning UsdRenderSettings prim dictates.
/// 
/// Specific renderers may support additional settings, such
/// as a way to configure compression settings, filetype metadata,
/// and so forth.  Such settings can be encoded using
/// renderer-specific API schemas applied to the product prim.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdRenderTokens.
/// So to set an attribute to the value "rightHanded", use UsdRenderTokens->rightHanded
/// as the value.
///
class UsdRenderProduct : public UsdRenderSettingsBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdRenderProduct on UsdPrim \p prim .
    /// Equivalent to UsdRenderProduct::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdRenderProduct(const UsdPrim& prim=UsdPrim())
        : UsdRenderSettingsBase(prim)
    {
    }

    /// Construct a UsdRenderProduct on the prim held by \p schemaObj .
    /// Should be preferred over UsdRenderProduct(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdRenderProduct(const UsdSchemaBase& schemaObj)
        : UsdRenderSettingsBase(schemaObj)
    {
    }

    /// Destructor.
    USDRENDER_API
    virtual ~UsdRenderProduct();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDRENDER_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdRenderProduct holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdRenderProduct(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDRENDER_API
    static UsdRenderProduct
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
    static UsdRenderProduct
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
    // PRODUCTTYPE 
    // --------------------------------------------------------------------- //
    /// 
    /// The type of output to produce. Allowed values are ones most 
    /// renderers should be able to support.
    /// Renderers that support custom output types are encouraged to supply an 
    /// applied API schema that will add an `token myRenderContext:productType`
    /// attribute (e.g. `token ri:productType`), which will override this
    /// attribute's value for that renderer. 
    /// 
    /// - "raster": This is the default type and indicates a 2D raster image of
    /// pixels.
    /// - "deepRaster": Indicates a deep image that contains multiple samples
    /// per pixel at varying depths.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token productType = "raster"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdRenderTokens "Allowed Values" | raster, deepRaster |
    USDRENDER_API
    UsdAttribute GetProductTypeAttr() const;

    /// See GetProductTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreateProductTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PRODUCTNAME 
    // --------------------------------------------------------------------- //
    /// Specifies the name that the output/display driver
    /// should give the product.  This is provided as-authored to the
    /// driver, whose responsibility it is to situate the product on a
    /// filesystem or other storage, in the desired location.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token productName = ""` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    USDRENDER_API
    UsdAttribute GetProductNameAttr() const;

    /// See GetProductNameAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDRENDER_API
    UsdAttribute CreateProductNameAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ORDEREDVARS 
    // --------------------------------------------------------------------- //
    /// Specifies the RenderVars that should be consumed and
    /// combined into the final product.  If ordering is relevant to the
    /// output driver, then the ordering of targets in this relationship
    /// provides the order to use.
    ///
    USDRENDER_API
    UsdRelationship GetOrderedVarsRel() const;

    /// See GetOrderedVarsRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDRENDER_API
    UsdRelationship CreateOrderedVarsRel() const;

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
