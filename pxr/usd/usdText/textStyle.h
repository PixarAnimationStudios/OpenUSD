//
// Copyright 2023 Pixar
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
#ifndef USDTEXT_GENERATED_TEXTSTYLE_H
#define USDTEXT_GENERATED_TEXTSTYLE_H

/// \file usdText/textStyle.h

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
// TEXTSTYLE                                                                  //
// -------------------------------------------------------------------------- //

/// \class UsdTextTextStyle
///
/// Class for single line single style text.
///
class UsdTextTextStyle : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdTextTextStyle on UsdPrim \p prim .
    /// Equivalent to UsdTextTextStyle::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdTextTextStyle(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdTextTextStyle on the prim held by \p schemaObj .
    /// Should be preferred over UsdTextTextStyle(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdTextTextStyle(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDTEXT_API
    virtual ~UsdTextTextStyle();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDTEXT_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdTextTextStyle holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdTextTextStyle(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDTEXT_API
    static UsdTextTextStyle
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
    static UsdTextTextStyle
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
    // TYPEFACE 
    // --------------------------------------------------------------------- //
    /// The typeface.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string typeface` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetTypefaceAttr() const;

    /// See GetTypefaceAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateTypefaceAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // BOLD 
    // --------------------------------------------------------------------- //
    /// Bold.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform bool bold = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetBoldAttr() const;

    /// See GetBoldAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateBoldAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ITALIC 
    // --------------------------------------------------------------------- //
    /// Italic.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform bool italic = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetItalicAttr() const;

    /// See GetItalicAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateItalicAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // WEIGHT 
    // --------------------------------------------------------------------- //
    /// Weight.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform int weight = 0` |
    /// | C++ Type | int |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetWeightAttr() const;

    /// See GetWeightAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateWeightAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TEXTHEIGHT 
    // --------------------------------------------------------------------- //
    /// Size.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform int textHeight` |
    /// | C++ Type | int |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetTextHeightAttr() const;

    /// See GetTextHeightAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateTextHeightAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TEXTWIDTHFACTOR 
    // --------------------------------------------------------------------- //
    /// WidthFactor.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float textWidthFactor = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetTextWidthFactorAttr() const;

    /// See GetTextWidthFactorAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateTextWidthFactorAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // OBLIQUEANGLE 
    // --------------------------------------------------------------------- //
    /// The oblique.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float obliqueAngle = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetObliqueAngleAttr() const;

    /// See GetObliqueAngleAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateObliqueAngleAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CHARSPACING 
    // --------------------------------------------------------------------- //
    /// The charspacing.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float charSpacing = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetCharSpacingAttr() const;

    /// See GetCharSpacingAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateCharSpacingAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // UNDERLINETYPE 
    // --------------------------------------------------------------------- //
    /// The UnderlineType.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string underlineType = "none"` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdTextTokens "Allowed Values" | none, normal |
    USDTEXT_API
    UsdAttribute GetUnderlineTypeAttr() const;

    /// See GetUnderlineTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateUnderlineTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // OVERLINETYPE 
    // --------------------------------------------------------------------- //
    /// The OverlineType.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string overlineType = "none"` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdTextTokens "Allowed Values" | none, normal |
    USDTEXT_API
    UsdAttribute GetOverlineTypeAttr() const;

    /// See GetOverlineTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateOverlineTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // STRIKETHROUGHTYPE 
    // --------------------------------------------------------------------- //
    /// The StrikethroughType.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string strikethroughType = "none"` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdTextTokens "Allowed Values" | none, normal, doubleLines |
    USDTEXT_API
    UsdAttribute GetStrikethroughTypeAttr() const;

    /// See GetStrikethroughTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateStrikethroughTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
