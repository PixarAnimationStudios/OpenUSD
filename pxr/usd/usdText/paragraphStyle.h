//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDTEXT_GENERATED_PARAGRAPHSTYLE_H
#define USDTEXT_GENERATED_PARAGRAPHSTYLE_H

/// \file usdText/paragraphStyle.h

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
// PARAGRAPHSTYLE                                                             //
// -------------------------------------------------------------------------- //

/// \class UsdTextParagraphStyle
///
/// Class for style of a paragraph.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdTextTokens.
/// So to set an attribute to the value "rightHanded", use UsdTextTokens->rightHanded
/// as the value.
///
class UsdTextParagraphStyle : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdTextParagraphStyle on UsdPrim \p prim .
    /// Equivalent to UsdTextParagraphStyle::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdTextParagraphStyle(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdTextParagraphStyle on the prim held by \p schemaObj .
    /// Should be preferred over UsdTextParagraphStyle(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdTextParagraphStyle(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDTEXT_API
    virtual ~UsdTextParagraphStyle();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDTEXT_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdTextParagraphStyle holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdTextParagraphStyle(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDTEXT_API
    static UsdTextParagraphStyle
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
    static UsdTextParagraphStyle
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
    // FIRSTLINEINDENT 
    // --------------------------------------------------------------------- //
    /// The left indent in the first line.
    /// A text primitive is commonly horizontally expanded. By default, it will fill the horizontal space of a block.
    /// If the line is the first line of a paragraph, there will be a space before the left start of the line. The space
    /// is the first line indent. Even if the direction of the text is from right to left, the firstLineIndent is added
    /// to the left of the line.
    /// If the block has margins, the firstLineIndent will be on the right of the left margin.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float firstLineIndent = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetFirstLineIndentAttr() const;

    /// See GetFirstLineIndentAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateFirstLineIndentAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LEFTINDENT 
    // --------------------------------------------------------------------- //
    /// The left indent in a normal line.
    /// A text primitive is commonly horizontally expanded. By default, it will fill the horizontal space of a block.
    /// If the line is not the first line of a paragraph, we can define there is space before the left start of the line.
    /// And the space is the left indent. Even if the direction of the text is from right to left, the leftIndent is
    /// added to the left of the line.
    /// If the block has margins, the leftIndent will be on the right of the left margin.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float leftIndent = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetLeftIndentAttr() const;

    /// See GetLeftIndentAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateLeftIndentAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // RIGHTINDENT 
    // --------------------------------------------------------------------- //
    /// The right indent in each line.
    /// A text primitive is commonly horizontally expanded. By default, it will fill the horizontal space of a block.
    /// But we can define there is space after the right end of the line. And the space is the right indent. Even if 
    /// the direction of the text is from right to left, the rightIndent is added to the right of the line.
    /// If the block has margins, the rightIndent will be on the left of the left margin.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float rightIndent = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetRightIndentAttr() const;

    /// See GetRightIndentAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateRightIndentAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PARAGRAPHSPACE 
    // --------------------------------------------------------------------- //
    /// The space after the paragraph.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float paragraphSpace = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetParagraphSpaceAttr() const;

    /// See GetParagraphSpaceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateParagraphSpaceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // PARAGRAPHALIGNMENT 
    // --------------------------------------------------------------------- //
    /// The text alignment in the paragraph.
    /// A text primitive is commonly horizontally expanded. By default, it will fill the horizontal space of a block.
    /// If the width of a text line is smaller than the horizontal space, this alignment will decide how the characters
    /// are positioned. You can see the examples below:
    /// 
    /// Alignment   |                                    text                                         |
    /// ----------- | ------------------------------------------------------------------------------- |
    /// left        | The quick brown fox jumps over the lazy dog.                                    |
    /// right       |                                    The quick brown fox jumps over the lazy dog. |
    /// center      |                  The quick brown fox jumps over the lazy dog.                   |
    /// justify     | The     quick      brown     fox     jumps     over      the     lazy      dog. |
    /// distributed | T h e  q u i c k  b r o w n  f o x  j u m p s  o v e r  t h e  l a z y  d o g . |
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token paragraphAlignment = "left"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdTextTokens "Allowed Values" | left, right, center, justify, distributed |
    USDTEXT_API
    UsdAttribute GetParagraphAlignmentAttr() const;

    /// See GetParagraphAlignmentAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateParagraphAlignmentAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TABSTOPPOSITIONS 
    // --------------------------------------------------------------------- //
    /// The positions for each tabstop.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float[] tabStopPositions` |
    /// | C++ Type | VtArray<float> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->FloatArray |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetTabStopPositionsAttr() const;

    /// See GetTabStopPositionsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateTabStopPositionsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TABSTOPTYPES 
    // --------------------------------------------------------------------- //
    /// The tabstop types.  You can see the examples below:
    /// 
    /// tabStop type | position ->    ^            ^           |
    /// ------------ | --------------------------------------- |
    /// leftTab      |            apple        34.52           |
    /// rightTab     |                apple        34.52       |
    /// centerTab    |              apple        34.52         |
    /// decimalTab   |            apple          34.52         |
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token[] tabStopTypes` |
    /// | C++ Type | VtArray<TfToken> |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->TokenArray |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdTextTokens "Allowed Values" | leftTab, rightTab, centerTab, decimalTab |
    USDTEXT_API
    UsdAttribute GetTabStopTypesAttr() const;

    /// See GetTabStopTypesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateTabStopTypesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LINESPACE 
    // --------------------------------------------------------------------- //
    /// The space between lines.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float lineSpace = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetLineSpaceAttr() const;

    /// See GetLineSpaceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateLineSpaceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LINESPACETYPE 
    // --------------------------------------------------------------------- //
    /// The type of the lineSpace value.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token lineSpaceType = "atLeast"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdTextTokens "Allowed Values" | exactly, atLeast, multiple |
    USDTEXT_API
    UsdAttribute GetLineSpaceTypeAttr() const;

    /// See GetLineSpaceTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateLineSpaceTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
