//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSemantics/labelsAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdSemanticsLabelsAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdSemanticsLabelsAPI::~UsdSemanticsLabelsAPI()
{
}

/* static */
UsdSemanticsLabelsAPI
UsdSemanticsLabelsAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdSemanticsLabelsAPI();
    }
    TfToken name;
    if (!IsSemanticsLabelsAPIPath(path, &name)) {
        TF_CODING_ERROR("Invalid semantics:labels path <%s>.", path.GetText());
        return UsdSemanticsLabelsAPI();
    }
    return UsdSemanticsLabelsAPI(stage->GetPrimAtPath(path.GetPrimPath()), name);
}

UsdSemanticsLabelsAPI
UsdSemanticsLabelsAPI::Get(const UsdPrim &prim, const TfToken &name)
{
    return UsdSemanticsLabelsAPI(prim, name);
}

/* static */
std::vector<UsdSemanticsLabelsAPI>
UsdSemanticsLabelsAPI::GetAll(const UsdPrim &prim)
{
    std::vector<UsdSemanticsLabelsAPI> schemas;
    
    for (const auto &schemaName :
         UsdAPISchemaBase::_GetMultipleApplyInstanceNames(prim, _GetStaticTfType())) {
        schemas.emplace_back(prim, schemaName);
    }

    return schemas;
}


/* static */
bool 
UsdSemanticsLabelsAPI::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdSemanticsTokens->semanticsLabels_MultipleApplyTemplate_),
    };

    return find(attrsAndRels.begin(), attrsAndRels.end(), baseName)
            != attrsAndRels.end();
}

/* static */
bool
UsdSemanticsLabelsAPI::IsSemanticsLabelsAPIPath(
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
        && tokens[0] == UsdSemanticsTokens->semanticsLabels) {
        *name = TfToken(propertyName.substr(
           UsdSemanticsTokens->semanticsLabels.GetString().size() + 1));
        return true;
    }

    return false;
}

/* virtual */
UsdSchemaKind UsdSemanticsLabelsAPI::_GetSchemaKind() const
{
    return UsdSemanticsLabelsAPI::schemaKind;
}

/* static */
bool
UsdSemanticsLabelsAPI::CanApply(
    const UsdPrim &prim, const TfToken &name, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdSemanticsLabelsAPI>(name, whyNot);
}

/* static */
UsdSemanticsLabelsAPI
UsdSemanticsLabelsAPI::Apply(const UsdPrim &prim, const TfToken &name)
{
    if (prim.ApplyAPI<UsdSemanticsLabelsAPI>(name)) {
        return UsdSemanticsLabelsAPI(prim, name);
    }
    return UsdSemanticsLabelsAPI();
}

/* static */
const TfType &
UsdSemanticsLabelsAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdSemanticsLabelsAPI>();
    return tfType;
}

/* static */
bool 
UsdSemanticsLabelsAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdSemanticsLabelsAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/// Returns the property name prefixed with the correct namespace prefix, which
/// is composed of the the API's propertyNamespacePrefix metadata and the
/// instance name of the API.
static inline
TfToken
_GetNamespacedPropertyName(const TfToken instanceName, const TfToken propName)
{
    return UsdSchemaRegistry::MakeMultipleApplyNameInstance(propName, instanceName);
}

UsdAttribute
UsdSemanticsLabelsAPI::GetLabelsAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdSemanticsTokens->semanticsLabels_MultipleApplyTemplate_));
}

UsdAttribute
UsdSemanticsLabelsAPI::CreateLabelsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdSemanticsTokens->semanticsLabels_MultipleApplyTemplate_),
                       SdfValueTypeNames->TokenArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
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
UsdSemanticsLabelsAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdSemanticsTokens->semanticsLabels_MultipleApplyTemplate_,
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
UsdSemanticsLabelsAPI::GetSchemaAttributeNames(
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

#include "pxr/usd/usd/common.h"

PXR_NAMESPACE_OPEN_SCOPE

std::vector<TfToken>
UsdSemanticsLabelsAPI::GetDirectTaxonomies(const UsdPrim& prim)
{
    if (prim.IsPseudoRoot()) {
        return {};
    }
    const auto allAppliedSchemas = UsdSemanticsLabelsAPI::GetAll(prim);
    std::vector<TfToken> result(allAppliedSchemas.size());
    std::transform(std::cbegin(allAppliedSchemas),
                   std::cend(allAppliedSchemas),
                   std::begin(result),
                   [](const auto& schema) { return schema.GetName(); });
    return result;
}

std::vector<TfToken>
UsdSemanticsLabelsAPI::ComputeInheritedTaxonomies(const UsdPrim& prim)
{
    std::unordered_set<TfToken, TfHash> unique;
    for (const auto& path : prim.GetPath().GetAncestorsRange()) {
        const auto allAppliedSchemas = UsdSemanticsLabelsAPI::GetAll(
            prim.GetStage()->GetPrimAtPath(path));
            std::transform(
                std::cbegin(allAppliedSchemas),
                std::cend(allAppliedSchemas),
                std::inserter(unique, std::end(unique)),
                [](const auto& schema) { return schema.GetName(); });
    }
    std::vector<TfToken> result{std::cbegin(unique), std::cend(unique)};
    std::sort(std::begin(result), std::end(result));
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
