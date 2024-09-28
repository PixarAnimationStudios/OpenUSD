//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdUI/accessibilityAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdUIAccessibilityAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdUIAccessibilityAPI::~UsdUIAccessibilityAPI()
{
}

/* static */
UsdUIAccessibilityAPI
UsdUIAccessibilityAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdUIAccessibilityAPI();
    }
    TfToken name;
    if (!IsAccessibilityAPIPath(path, &name)) {
        TF_CODING_ERROR("Invalid accessibility path <%s>.", path.GetText());
        return UsdUIAccessibilityAPI();
    }
    return UsdUIAccessibilityAPI(stage->GetPrimAtPath(path.GetPrimPath()), name);
}

UsdUIAccessibilityAPI
UsdUIAccessibilityAPI::Get(const UsdPrim &prim, const TfToken &name)
{
    return UsdUIAccessibilityAPI(prim, name);
}

/* static */
std::vector<UsdUIAccessibilityAPI>
UsdUIAccessibilityAPI::GetAll(const UsdPrim &prim)
{
    std::vector<UsdUIAccessibilityAPI> schemas;
    
    for (const auto &schemaName :
         UsdAPISchemaBase::_GetMultipleApplyInstanceNames(prim, _GetStaticTfType())) {
        schemas.emplace_back(prim, schemaName);
    }

    return schemas;
}


/* static */
bool 
UsdUIAccessibilityAPI::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdUITokens->accessibility_MultipleApplyTemplate_Label),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdUITokens->accessibility_MultipleApplyTemplate_Description),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdUITokens->accessibility_MultipleApplyTemplate_Priority),
    };

    return find(attrsAndRels.begin(), attrsAndRels.end(), baseName)
            != attrsAndRels.end();
}

/* static */
bool
UsdUIAccessibilityAPI::IsAccessibilityAPIPath(
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
        && tokens[0] == UsdUITokens->accessibility) {
        *name = TfToken(propertyName.substr(
           UsdUITokens->accessibility.GetString().size() + 1));
        return true;
    }

    return false;
}

/* virtual */
UsdSchemaKind UsdUIAccessibilityAPI::_GetSchemaKind() const
{
    return UsdUIAccessibilityAPI::schemaKind;
}

/* static */
bool
UsdUIAccessibilityAPI::CanApply(
    const UsdPrim &prim, const TfToken &name, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdUIAccessibilityAPI>(name, whyNot);
}

/* static */
UsdUIAccessibilityAPI
UsdUIAccessibilityAPI::Apply(const UsdPrim &prim, const TfToken &name)
{
    if (prim.ApplyAPI<UsdUIAccessibilityAPI>(name)) {
        return UsdUIAccessibilityAPI(prim, name);
    }
    return UsdUIAccessibilityAPI();
}

/* static */
const TfType &
UsdUIAccessibilityAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdUIAccessibilityAPI>();
    return tfType;
}

/* static */
bool 
UsdUIAccessibilityAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdUIAccessibilityAPI::_GetTfType() const
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
UsdUIAccessibilityAPI::GetLabelAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdUITokens->accessibility_MultipleApplyTemplate_Label));
}

UsdAttribute
UsdUIAccessibilityAPI::CreateLabelAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdUITokens->accessibility_MultipleApplyTemplate_Label),
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdUIAccessibilityAPI::GetDescriptionAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdUITokens->accessibility_MultipleApplyTemplate_Description));
}

UsdAttribute
UsdUIAccessibilityAPI::CreateDescriptionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdUITokens->accessibility_MultipleApplyTemplate_Description),
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdUIAccessibilityAPI::GetPriorityAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdUITokens->accessibility_MultipleApplyTemplate_Priority));
}

UsdAttribute
UsdUIAccessibilityAPI::CreatePriorityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdUITokens->accessibility_MultipleApplyTemplate_Priority),
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
UsdUIAccessibilityAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdUITokens->accessibility_MultipleApplyTemplate_Label,
        UsdUITokens->accessibility_MultipleApplyTemplate_Description,
        UsdUITokens->accessibility_MultipleApplyTemplate_Priority,
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
UsdUIAccessibilityAPI::GetSchemaAttributeNames(
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

/*static*/
UsdUIAccessibilityAPI UsdUIAccessibilityAPI::CreateDefaultAPI(const UsdPrim& prim) {
    return UsdUIAccessibilityAPI(prim, UsdUITokens->default_);
}

/*static*/
UsdUIAccessibilityAPI UsdUIAccessibilityAPI::CreateDefaultAPI(const UsdSchemaBase& schemaObj) {
    return UsdUIAccessibilityAPI(schemaObj, UsdUITokens->default_);
}

/*static*/
UsdUIAccessibilityAPI UsdUIAccessibilityAPI::ApplyDefaultAPI(const UsdPrim& prim) {
    return UsdUIAccessibilityAPI::Apply(prim, UsdUITokens->default_);
}

PXR_NAMESPACE_CLOSE_SCOPE
