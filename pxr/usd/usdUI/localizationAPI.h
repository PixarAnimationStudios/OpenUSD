//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDUI_GENERATED_LOCALIZATIONAPI_H
#define USDUI_GENERATED_LOCALIZATIONAPI_H

/// \file usdUI/localizationAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdUI/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdUI/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// LOCALIZATIONAPI                                                            //
// -------------------------------------------------------------------------- //

/// \class UsdUILocalizationAPI
///
/// This API describes em Language localization information for attributes.
/// 
/// It may be used to provide alternate language definitions for content like strings and asset paths that are
/// displayed to a user.
/// Runtimes may present the best language for a given users preference with this information.
/// 
/// OpenUSD leaves it up to the runtime that is consuming it to handle localized presentations.
/// As such, support for localization may vary across runtimes.
/// 
/// \important Lookup of localized attributes may be expensive, so are recommended to be used sparingly
/// It is recommended, but not enforced, to only use them for strings and asset paths.
/// Support for localization of different attributes may vary depending on the application runtime that 
/// the data is brought into.
/// 
/// 
/// \note Language identifiers must use the BCP-47 list of languages. However, since USD cannot currently use
/// hyphens in identifiers, any hyphens are replaced with underscores. This is similar in strategy to other
/// systems that adhere closely to the Unicode Identifier specification. e.g en-ca is en_CA .
/// Take care when converting language identifiers to a systems own formatting.
/// 
/// A default language is specifiable on a prim. This is the language that is assumed when attributes do
/// not include their own identifier for language. The default language is explicitly inherited by 
/// all prims under the current prims hierarchy.
/// 
/// \note Provide default localization information on the default prim of the layer, and any top level prims.
/// It is not recommended to keep declarations of the default localization to a minimum throughout the rest of
/// the hierarchy within a single layer.
/// 
/// Attributes are suffixed with \em :lang:<identifier> when expressing languages other than the default.
/// 
/// For example, "string text" would implicitly be in the default localization language, but you may have
/// "string text:lang:fr" for French.
/// 
/// Runtimes may provide their own logic for choosing which langauge to display, but following BCP-47,
/// a recommended logic set is:
/// 
/// \li If a preferred language is available within the set of declared languages, pick that language exactly.
/// e.g "en_CA" should not pick simply "en" if "en_CA" is available
/// 
/// \li If a preferred language isn't available, check for a more specific version of that language.
/// e.g "de_DE" may match to "de_DE_u_co_phonebk"
/// 
/// \li If a more specific language is not available, pick a less specific language.
/// e.g "en_US" may match to "en"
/// 
/// \li If a less specific language choice is not available, pick the attribute without language specification.
/// 
///
class UsdUILocalizationAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::MultipleApplyAPI;

    /// Construct a UsdUILocalizationAPI on UsdPrim \p prim with
    /// name \p name . Equivalent to
    /// UsdUILocalizationAPI::Get(
    ///    prim.GetStage(),
    ///    prim.GetPath().AppendProperty(
    ///        "localization:name"));
    ///
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdUILocalizationAPI(
        const UsdPrim& prim=UsdPrim(), const TfToken &name=UsdUITokens->default_)
        : UsdAPISchemaBase(prim, /*instanceName*/ name)
    { }

    /// Construct a UsdUILocalizationAPI on the prim held by \p schemaObj with
    /// name \p name.  Should be preferred over
    /// UsdUILocalizationAPI(schemaObj.GetPrim(), name), as it preserves
    /// SchemaBase state.
    explicit UsdUILocalizationAPI(
        const UsdSchemaBase& schemaObj, const TfToken &name=UsdUITokens->default_)
        : UsdAPISchemaBase(schemaObj, /*instanceName*/ name)
    { }

    /// Destructor.
    USDUI_API
    virtual ~UsdUILocalizationAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDUI_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes for a given instance name.  Does not
    /// include attributes that may be authored by custom/extended methods of
    /// the schemas involved. The names returned will have the proper namespace
    /// prefix.
    USDUI_API
    static TfTokenVector
    GetSchemaAttributeNames(bool includeInherited, const TfToken &instanceName);

    /// Returns the name of this multiple-apply schema instance
    TfToken GetName() const {
        return _GetInstanceName();
    }

    /// Return a UsdUILocalizationAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  \p path must be of the format
    /// <path>.localization:name .
    ///
    /// This is shorthand for the following:
    ///
    /// \code
    /// TfToken name = SdfPath::StripNamespace(path.GetToken());
    /// UsdUILocalizationAPI(
    ///     stage->GetPrimAtPath(path.GetPrimPath()), name);
    /// \endcode
    ///
    USDUI_API
    static UsdUILocalizationAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Return a UsdUILocalizationAPI with name \p name holding the
    /// prim \p prim. Shorthand for UsdUILocalizationAPI(prim, name);
    USDUI_API
    static UsdUILocalizationAPI
    Get(const UsdPrim &prim, const TfToken &name);

    /// Return a vector of all named instances of UsdUILocalizationAPI on the 
    /// given \p prim.
    USDUI_API
    static std::vector<UsdUILocalizationAPI>
    GetAll(const UsdPrim &prim);

    /// Checks if the given name \p baseName is the base name of a property
    /// of LocalizationAPI.
    USDUI_API
    static bool
    IsSchemaPropertyBaseName(const TfToken &baseName);

    /// Checks if the given path \p path is of an API schema of type
    /// LocalizationAPI. If so, it stores the instance name of
    /// the schema in \p name and returns true. Otherwise, it returns false.
    USDUI_API
    static bool
    IsLocalizationAPIPath(const SdfPath &path, TfToken *name);

    /// Returns true if this <b>multiple-apply</b> API schema can be applied,
    /// with the given instance name, \p name, to the given \p prim. If this 
    /// schema can not be a applied the prim, this returns false and, if 
    /// provided, populates \p whyNot with the reason it can not be applied.
    /// 
    /// Note that if CanApply returns false, that does not necessarily imply
    /// that calling Apply will fail. Callers are expected to call CanApply
    /// before calling Apply if they want to ensure that it is valid to 
    /// apply a schema.
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDUI_API
    static bool 
    CanApply(const UsdPrim &prim, const TfToken &name, 
             std::string *whyNot=nullptr);

    /// Applies this <b>multiple-apply</b> API schema to the given \p prim 
    /// along with the given instance name, \p name. 
    /// 
    /// This information is stored by adding "LocalizationAPI:<i>name</i>" 
    /// to the token-valued, listOp metadata \em apiSchemas on the prim.
    /// For example, if \p name is 'instance1', the token 
    /// 'LocalizationAPI:instance1' is added to 'apiSchemas'.
    /// 
    /// \return A valid UsdUILocalizationAPI object is returned upon success. 
    /// An invalid (or empty) UsdUILocalizationAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for 
    /// conditions resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDUI_API
    static UsdUILocalizationAPI 
    Apply(const UsdPrim &prim, const TfToken &name=UsdUITokens->default_);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDUI_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDUI_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDUI_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // LANGUAGE 
    // --------------------------------------------------------------------- //
    /// The default language for this prim hierarchy. This may only be created with the default instance name.
    /// 
    /// \note If no default localization language is provided, the runtime may optionally try and infer the
    /// language of the text.
    /// If the runtime does not infer the langauge, it should assume the language is in the users preferred language,
    /// which may be derived from the system or current user context.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform string language` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USDUI_API
    UsdAttribute GetLanguageAttr() const;

    /// See GetLanguageAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDUI_API
    UsdAttribute CreateLanguageAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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
    /// Returns the version of the property that has no localization specifics.
    /// If this cannot be found, a default constructed property is returned.
    /// If the input property doesn't specify a localization, it will be returned itself.
    USDUI_API
    static UsdProperty GetDefaultProperty(UsdProperty const &source);

    /// Returns a TfToken that represents the localization of the property
    /// If a localization is not found, an empty token is returned
    USDUI_API
    static TfToken GetPropertyLanguage(UsdProperty const &prop);

    /// Gets the name of a sibling property with a matching localization
    USDUI_API
    static TfToken
    GetLocalizedPropertyName(UsdProperty const &source, TfToken const &localization);

    /// Finds and returns a sibling proprety that has the specified localization.
    /// If it is not found, a default constructed property is returned.
    /// Only localizations that have been applied on the prim will be returned.
    USDUI_API
    static UsdProperty
    GetLocalizedProperty(UsdProperty const &source, TfToken const &localization);

    /// A convenience method for calling the static version of this method with the localization derived
    /// from the instance name
    USDUI_API
    UsdProperty GetLocalizedProperty(UsdProperty const &source) const;

    /// Creates an attribute with the given localization, or returns the attribute if one already exists
    /// \note It is up to the developer to apply the localization API for this locale to the prim
    USDUI_API
    static UsdAttribute
    CreateLocalizedAttribute(UsdAttribute const &source, TfToken const &localization, VtValue const &defaultValue,
                             bool writeSparsely=false);

    /// A convenience method for calling the static version of this method with the localization derived
    /// from the instance name
    USDUI_API
    UsdAttribute CreateLocalizedAttribute(UsdAttribute const &source, VtValue const &defaultValue,
                                          bool writeSparsely=false) const;

    /// Creates a relationship with the given localization, or returns the relationship if one already exists
    /// \note It is up to the developer to apply the localization API for this locale to the prim
    USDUI_API
    static UsdRelationship
    CreateLocalizedRelationship(UsdRelationship const &source, TfToken const &localization);

    /// A convenience method for calling the static version of this method with the localization derived
    /// from the instance name
    USDUI_API
    UsdRelationship CreateLocalizedRelationship(UsdRelationship const &source) const;


    /// Fills the map with all localized versions of the properties that have Applied schemas on the prim.
    /// The version of the property without a localization specifier will be returned directly from the method
    /// and not populated into the map.
    /// It is upto the developer to infer the localization using the rules as described in the schema
    /// In Python, the results are returned as a tuple of the property return and the dictionary.
    /// /sa GetAllPropertyLocalizations
    USDUI_API
    static UsdProperty
    GetAppliedPropertyLocalizations(UsdProperty const &source, std::map<TfToken, UsdProperty> &localizations);

    /// Fills the map with all localized versions of the property, regardless of whether they are applied.
    /// The version of the property without a localization specifier will be returned directly from the method
    /// and not populated into the map.
    /// It is upto the developer to infer the localization using the rules as described in the schema
    /// In Python, the results are returned as a tuple of the property return and the dictionary.
    /// /sa GetAppliedPropertyLocalizations
    USDUI_API
    static UsdProperty
    GetAllPropertyLocalizations(UsdProperty const &source, std::map<TfToken, UsdProperty> &localizations);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
