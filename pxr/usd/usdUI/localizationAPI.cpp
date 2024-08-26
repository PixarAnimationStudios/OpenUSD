//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdUI/localizationAPI.h"

#include "tokens.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdUILocalizationAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdUILocalizationAPI::~UsdUILocalizationAPI()
{
}

/* static */
UsdUILocalizationAPI
UsdUILocalizationAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdUILocalizationAPI();
    }
    TfToken name;
    if (!IsLocalizationAPIPath(path, &name)) {
        TF_CODING_ERROR("Invalid localization path <%s>.", path.GetText());
        return UsdUILocalizationAPI();
    }
    return UsdUILocalizationAPI(stage->GetPrimAtPath(path.GetPrimPath()), name);
}

UsdUILocalizationAPI
UsdUILocalizationAPI::Get(const UsdPrim &prim, const TfToken &name)
{
    return UsdUILocalizationAPI(prim, name);
}

/* static */
std::vector<UsdUILocalizationAPI>
UsdUILocalizationAPI::GetAll(const UsdPrim &prim)
{
    std::vector<UsdUILocalizationAPI> schemas;
    
    for (const auto &schemaName :
         UsdAPISchemaBase::_GetMultipleApplyInstanceNames(prim, _GetStaticTfType())) {
        schemas.emplace_back(prim, schemaName);
    }

    return schemas;
}


/* static */
bool 
UsdUILocalizationAPI::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdUITokens->localization_MultipleApplyTemplate_Language),
    };

    return find(attrsAndRels.begin(), attrsAndRels.end(), baseName)
            != attrsAndRels.end();
}

/* static */
bool
UsdUILocalizationAPI::IsLocalizationAPIPath(
    const SdfPath &path, TfToken *name)
{
    if (!path.IsPropertyPath()) {
        return false;
    }

    std::string propertyName = path.GetName();
    TfTokenVector tokens = SdfPath::TokenizeIdentifierAsTokens(propertyName);

    // The baseName of the  path can't be one of the 
    // schema properties. We should validate this in the creation (or apply)
    // API.
    TfToken baseName = *tokens.rbegin();
    if (IsSchemaPropertyBaseName(baseName)) {
        return false;
    }

    if (tokens.size() >= 2
        && tokens[0] == UsdUITokens->localization) {
        *name = TfToken(propertyName.substr(
           UsdUITokens->localization.GetString().size() + 1));
        return true;
    }

    return false;
}

/* virtual */
UsdSchemaKind UsdUILocalizationAPI::_GetSchemaKind() const
{
    return UsdUILocalizationAPI::schemaKind;
}

/* static */
bool
UsdUILocalizationAPI::CanApply(
    const UsdPrim &prim, const TfToken &name, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdUILocalizationAPI>(name, whyNot);
}

/* static */
UsdUILocalizationAPI
UsdUILocalizationAPI::Apply(const UsdPrim &prim, const TfToken &name)
{
    if (prim.ApplyAPI<UsdUILocalizationAPI>(name)) {
        return UsdUILocalizationAPI(prim, name);
    }
    return UsdUILocalizationAPI();
}

/* static */
const TfType &
UsdUILocalizationAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdUILocalizationAPI>();
    return tfType;
}

/* static */
bool 
UsdUILocalizationAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdUILocalizationAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdUILocalizationAPI::GetLanguageAttr() const
{
    return GetPrim().GetAttribute(UsdUITokens->languageAttribute);
}

UsdAttribute
UsdUILocalizationAPI::CreateLanguageAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       UsdUITokens->languageAttribute,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdUILocalizationAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdUITokens->localization_MultipleApplyTemplate_Language,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

/*static*/
TfTokenVector
UsdUILocalizationAPI::GetSchemaAttributeNames(
    bool includeInherited, const TfToken &instanceName)
{
    const TfTokenVector &attrNames = GetSchemaAttributeNames(includeInherited);
    if (instanceName.IsEmpty()) {
        return attrNames;
    }
    TfTokenVector result;
    result.reserve(attrNames.size());
    for (const TfToken &attrName : attrNames) {
        result.push_back(
            UsdSchemaRegistry::MakeMultipleApplyNameInstance(attrName, instanceName));
    }
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

PXR_NAMESPACE_OPEN_SCOPE

/* static */
UsdProperty
UsdUILocalizationAPI::GetDefaultProperty(UsdProperty const &source) {
    const auto nameTokens = source.SplitName();

    if (nameTokens.size() < 3 || nameTokens[nameTokens.size() - 2] != UsdUITokens->lang) {
        // The most minimal name would be foo:lang:en_us
        // If we have less than 3 tokens, we assume it doesn't have a language specifier.
        // If the second last element isn't lang, we can assume the same as well
        return source;
    }

    const auto defaultName = TfStringJoin(nameTokens.begin(), nameTokens.end()-2, ":");

    const auto prim = source.GetPrim();
    if (!prim) {
        TF_CODING_ERROR("Cannot find parent prim");
        return {};
    }

    auto prop = prim.GetProperty(TfToken(defaultName));
    return prop;
}


TfToken UsdUILocalizationAPI::GetPropertyLanguage(UsdProperty const &prop) {
    auto nameTokens = prop.SplitName();
    if (nameTokens.size() < 3 || nameTokens[nameTokens.size() - 2] != UsdUITokens->lang) {
        return TfToken();
    }

    return TfToken(nameTokens[nameTokens.size() - 1]);
}

/* static */
TfToken
UsdUILocalizationAPI::GetLocalizedPropertyName(UsdProperty const &source, TfToken const &localization) {
    // It's fastest to just get the default attribute
    const auto defaultProp = GetDefaultProperty(source);
    if (!defaultProp) {
        TF_CODING_ERROR("Cannot find the default-localized attribute for this property");
        return {};
    }

    const auto sep = ":";
    const auto localizedName = (
        defaultProp.GetName().GetString() + sep
        + UsdUITokens->lang.GetString() + sep
        + localization.GetString()
    );

    return TfToken(localizedName);
}

/* static */
UsdProperty
UsdUILocalizationAPI::GetLocalizedProperty(UsdProperty const &source, TfToken const &localization) {
    const auto prim = source.GetPrim();
    if (!prim) {
        TF_CODING_ERROR("Cannot find attributes parent prim");
        return {};
    }

    const auto localizedAttrName = GetLocalizedPropertyName(source, localization);
    return prim.GetProperty(TfToken(localizedAttrName));
}

UsdProperty
UsdUILocalizationAPI::GetLocalizedProperty(UsdProperty const &source) const {
    return GetLocalizedProperty(source, GetName());
}

/* static */
UsdAttribute
UsdUILocalizationAPI::CreateLocalizedAttribute(UsdAttribute const &source, TfToken const &localization,
                                               VtValue const &defaultValue, bool writeSparsely) {
    const auto prim = source.GetPrim();
    if (!prim) {
        TF_CODING_ERROR("Cannot find attributes parent prim");
        return {};
    }

    if (writeSparsely) {
        // From UsdSchemaBase::_CreateAttr
        auto prop = GetLocalizedProperty(source, localization);
        if (prop) {
            UsdAttribute attr = prim.GetAttributeAtPath(prop.GetPath());
            if (!attr) {
                TF_CODING_ERROR("Could not construct attribute from property");
                return {};
            }

            VtValue fallback;
            if (defaultValue.IsEmpty() ||
                (!attr.HasAuthoredValue()
                 && attr.Get(&fallback)
                 && fallback == defaultValue)){
                return attr;
            }
        }
    }

    auto property = UsdUILocalizationAPI::GetDefaultProperty(source);
    if (!property) {
        TF_CODING_ERROR("Could not find default property");
        return {};
    }

    UsdAttribute defaultAttr = prim.GetAttributeAtPath(property.GetPath());
    if (!defaultAttr) {
        TF_CODING_ERROR("Could not construct attribute from property");
        return {};
    }

    UsdAttribute attr(prim.CreateAttribute(
        UsdUILocalizationAPI::GetLocalizedPropertyName(source, localization),
        defaultAttr.GetTypeName(),
        defaultAttr.IsCustom(),
        defaultAttr.GetVariability()
    ));

    if (attr && !defaultValue.IsEmpty()) {
        attr.Set(defaultValue);
    }

    return attr;
}

UsdAttribute
UsdUILocalizationAPI::CreateLocalizedAttribute(UsdAttribute const &source, VtValue const &defaultValue,
                                               bool writeSparsely) const {
    return CreateLocalizedAttribute(source, GetName(), defaultValue, writeSparsely);
}

UsdRelationship
UsdUILocalizationAPI::CreateLocalizedRelationship(UsdRelationship const &source, TfToken const &localization) {
    const auto prim = source.GetPrim();
    if (!prim) {
        TF_CODING_ERROR("Cannot find attributes parent prim");
        return {};
    }

    auto property = UsdUILocalizationAPI::GetDefaultProperty(source);
    if (!property) {
        TF_CODING_ERROR("Could not find default property");
        return {};
    }

    UsdRelationship defaultRel = prim.GetRelationshipAtPath(property.GetPath());
    if (!defaultRel) {
        TF_CODING_ERROR("Could not construct relationship from property");
        return {};
    }

    UsdRelationship rel(prim.CreateRelationship(
        UsdUILocalizationAPI::GetLocalizedPropertyName(source, localization),
        defaultRel.IsCustom()
    ));

    return rel;
}


UsdRelationship
UsdUILocalizationAPI::CreateLocalizedRelationship(UsdRelationship const &source) const{
    return UsdUILocalizationAPI::CreateLocalizedRelationship(source, GetName());
}

/* static */
UsdProperty
UsdUILocalizationAPI::GetAppliedPropertyLocalizations(UsdProperty const &source,
                                                       std::map<TfToken, UsdProperty> &localizations) {
    auto prim = source.GetPrim();
    if (!prim) {
        TF_CODING_ERROR("Cannot find parent prim");
        return {};
    }
    auto apis = UsdUILocalizationAPI::GetAll(source.GetPrim());
    auto defaultAttr = UsdUILocalizationAPI::GetDefaultProperty(source);
    auto defaultAttrName = defaultAttr.GetName().GetString();
    const auto sep = ":";
    for (const auto &api: apis) {
        auto attrName = defaultAttrName + sep + UsdUITokens->lang.GetString() + sep + api.GetName().GetString();
        auto attr = prim.GetAttribute(TfToken(attrName));
        if (attr) {
            localizations.insert({api.GetName(), attr});
        }
    }
    return defaultAttr;
}

/* static */
UsdProperty
UsdUILocalizationAPI::GetAllPropertyLocalizations(UsdProperty const &source,
                                                   std::map<TfToken, UsdProperty> &localizations) {
    auto prim = source.GetPrim();
    if (!prim) {
        TF_CODING_ERROR("Cannot find parent prim");
        return {};
    }
    auto defaultAttr = UsdUILocalizationAPI::GetDefaultProperty(source);

    const auto sep = ":";
    auto prefix = defaultAttr.GetName().GetString() + sep + UsdUITokens->lang.GetString() + sep;
    for (const auto &props: prim.GetProperties()) {
        auto attrName = props.GetName().GetString();
        if (!TfStringStartsWith(attrName, prefix)) {
            continue;
        }

        auto locale = UsdUILocalizationAPI::GetPropertyLanguage(props);
        if (!locale.IsEmpty()) {
            localizations.insert({locale, props});
        }
    }

    return defaultAttr;
}

PXR_NAMESPACE_CLOSE_SCOPE