//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDTEXT_GENERATED_COLUMNSTYLE_H
#define USDTEXT_GENERATED_COLUMNSTYLE_H

/// \file usdText/columnStyle.h

#include "pxr/pxr.h"
#include "pxr/usd/usdText/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdText/tokens.h"

#include <utility>
        

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// COLUMNSTYLE                                                                //
// -------------------------------------------------------------------------- //

/// \class UsdTextColumnStyle
///
/// Class for style of a column.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdTextTokens.
/// So to set an attribute to the value "rightHanded", use UsdTextTokens->rightHanded
/// as the value.
///
class UsdTextColumnStyle : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdTextColumnStyle on UsdPrim \p prim .
    /// Equivalent to UsdTextColumnStyle::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdTextColumnStyle(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdTextColumnStyle on the prim held by \p schemaObj .
    /// Should be preferred over UsdTextColumnStyle(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdTextColumnStyle(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDTEXT_API
    virtual ~UsdTextColumnStyle();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDTEXT_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdTextColumnStyle holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdTextColumnStyle(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDTEXT_API
    static UsdTextColumnStyle
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
    USDTEXT_API
    static UsdTextColumnStyle
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDTEXT_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDTEXT_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDTEXT_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // COLUMNWIDTH 
    // --------------------------------------------------------------------- //
    /// The width of the column.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float columnWidth = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetColumnWidthAttr() const;

    /// See GetColumnWidthAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateColumnWidthAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLUMNHEIGHT 
    // --------------------------------------------------------------------- //
    /// The height of the column.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float columnHeight = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetColumnHeightAttr() const;

    /// See GetColumnHeightAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateColumnHeightAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLUMNOFFSET 
    // --------------------------------------------------------------------- //
    /// The 2D offset from the position of the text primitive to the top-left corner of this column.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float2 columnOffset = (0, 0)` |
    /// | C++ Type | GfVec2f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float2 |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetColumnOffsetAttr() const;

    /// See GetColumnOffsetAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateColumnOffsetAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MARGINS 
    // --------------------------------------------------------------------- //
    /// The margins at the four direction. The order is left margin, right margin, top margin and 
    /// bottom margin.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float4 margins = (0, 0, 0, 0)` |
    /// | C++ Type | GfVec4f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float4 |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetMarginsAttr() const;

    /// See GetMarginsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateMarginsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // COLUMNALIGNMENT 
    // --------------------------------------------------------------------- //
    /// The position of the whole text primitive in the block. 
    /// A text primitive is commonly horizontally expanded. It will fill the horizontal space of the block 
    /// except the left margin and the right margin. So there is no horizontal alignment for a column. If the 
    /// height of the primitive is smaller than the height of the column, the column alignment will decide
    /// the position of the text primitive is at the top, at the center or at the bottom of the column.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token columnAlignment = "top"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdTextTokens "Allowed Values" | top, center, bottom |
    USDTEXT_API
    UsdAttribute GetColumnAlignmentAttr() const;

    /// See GetColumnAlignmentAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateColumnAlignmentAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
