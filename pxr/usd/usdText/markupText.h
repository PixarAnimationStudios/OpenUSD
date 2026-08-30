//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDTEXT_GENERATED_MARKUPTEXT_H
#define USDTEXT_GENERATED_MARKUPTEXT_H

/// \file usdText/markupText.h

#include "pxr/pxr.h"
#include "pxr/usd/usdText/api.h"
#include "pxr/usd/usdGeom/gprim.h"
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
// MARKUPTEXT                                                                 //
// -------------------------------------------------------------------------- //

/// \class UsdTextMarkupText
///
/// This primitive represents a text object which can have one line or multiple lines, and it can 
/// have varied styles.
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdTextTokens.
/// So to set an attribute to the value "rightHanded", use UsdTextTokens->rightHanded
/// as the value.
///
class UsdTextMarkupText : public UsdGeomGprim
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdTextMarkupText on UsdPrim \p prim .
    /// Equivalent to UsdTextMarkupText::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdTextMarkupText(const UsdPrim& prim=UsdPrim())
        : UsdGeomGprim(prim)
    {
    }

    /// Construct a UsdTextMarkupText on the prim held by \p schemaObj .
    /// Should be preferred over UsdTextMarkupText(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdTextMarkupText(const UsdSchemaBase& schemaObj)
        : UsdGeomGprim(schemaObj)
    {
    }

    /// Destructor.
    USDTEXT_API
    virtual ~UsdTextMarkupText();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDTEXT_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdTextMarkupText holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdTextMarkupText(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDTEXT_API
    static UsdTextMarkupText
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
    static UsdTextMarkupText
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
    // MARKUP 
    // --------------------------------------------------------------------- //
    /// The markup string data. It can contain markups. The markup language is specified
    /// in the markupLanguage attribute.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string markup` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetMarkupAttr() const;

    /// See GetMarkupAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateMarkupAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MARKUPPLAIN 
    // --------------------------------------------------------------------- //
    /// The string data for the primitive when the implementation can not support the 
    /// markupLanguage.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string markup:plain = ""` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetMarkupPlainAttr() const;

    /// See GetMarkupPlainAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateMarkupPlainAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // MARKUPLANGUAGE 
    // --------------------------------------------------------------------- //
    /// The markup language.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token markupLanguage = "plain"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdTextTokens "Allowed Values" | plain, mtext |
    USDTEXT_API
    UsdAttribute GetMarkupLanguageAttr() const;

    /// See GetMarkupLanguageAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateMarkupLanguageAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // BACKGROUNDCOLOR 
    // --------------------------------------------------------------------- //
    /// Background color for the text.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform color3f primvars:backgroundColor` |
    /// | C++ Type | GfVec3f |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Color3f |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetBackgroundColorAttr() const;

    /// See GetBackgroundColorAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateBackgroundColorAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // BACKGROUNDOPACITY 
    // --------------------------------------------------------------------- //
    /// Background opacity for the text.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform float primvars:backgroundOpacity = 0` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDTEXT_API
    UsdAttribute GetBackgroundOpacityAttr() const;

    /// See GetBackgroundOpacityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateBackgroundOpacityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TEXTMETRICSUNIT 
    // --------------------------------------------------------------------- //
    /// The unit for the text related metrics, such as the unit of charHeight.
    /// If the value is 'pixel', the unit of text metrics will be the same as a pixel in the framebuffer. 
    /// If the value is 'publishingPoint', the unit will be the same as desktop publishing point, or 1/72
    /// of an inch on a screen's physical display. If textMetricsUnit is 'worldUnit'", the unit will be 
    /// the same as the unit of the world space. 
    /// If the text primitive has billboard, the textMetricsUnit can only be 'pixel' or 'publishingPoint'.
    /// Otherwise, the textMetricsUnit can only be 'worldUnit'.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token textMetricsUnit = "worldUnit"` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    /// | \ref UsdTextTokens "Allowed Values" | worldUnit, publishingPoint, pixel |
    USDTEXT_API
    UsdAttribute GetTextMetricsUnitAttr() const;

    /// See GetTextMetricsUnitAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDTEXT_API
    UsdAttribute CreateTextMetricsUnitAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
