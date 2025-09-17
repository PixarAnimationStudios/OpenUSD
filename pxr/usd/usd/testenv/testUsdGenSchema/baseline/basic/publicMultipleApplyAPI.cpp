//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/publicMultipleApplyAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedPublicMultipleApplyAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdContrivedPublicMultipleApplyAPI::~UsdContrivedPublicMultipleApplyAPI()
{
}

/* static */
UsdContrivedPublicMultipleApplyAPI
UsdContrivedPublicMultipleApplyAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedPublicMultipleApplyAPI();
    }
    TfToken name;
    if (!IsPublicMultipleApplyAPIPath(path, &name)) {
        TF_CODING_ERROR("Invalid testo path <%s>.", path.GetText());
        return UsdContrivedPublicMultipleApplyAPI();
    }
    return UsdContrivedPublicMultipleApplyAPI(stage->GetPrimAtPath(path.GetPrimPath()), name);
}

UsdContrivedPublicMultipleApplyAPI
UsdContrivedPublicMultipleApplyAPI::Get(const UsdPrim &prim, const TfToken &name)
{
    return UsdContrivedPublicMultipleApplyAPI(prim, name);
}

/* static */
std::vector<UsdContrivedPublicMultipleApplyAPI>
UsdContrivedPublicMultipleApplyAPI::GetAll(const UsdPrim &prim)
{
    std::vector<UsdContrivedPublicMultipleApplyAPI> schemas;
    
    for (const auto &schemaName :
         UsdAPISchemaBase::_GetMultipleApplyInstanceNames(prim, _GetStaticTfType())) {
        schemas.emplace_back(prim, schemaName);
    }

    return schemas;
}


/* static */
bool 
UsdContrivedPublicMultipleApplyAPI::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdContrivedTokens->testo_MultipleApplyTemplate_TestAttrOne),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdContrivedTokens->testo_MultipleApplyTemplate_TestAttrTwo),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdContrivedTokens->testo_MultipleApplyTemplate_),
    };

    return find(attrsAndRels.begin(), attrsAndRels.end(), baseName)
            != attrsAndRels.end();
}

/* static */
bool
UsdContrivedPublicMultipleApplyAPI::IsPublicMultipleApplyAPIPath(
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
        && tokens[0] == UsdContrivedTokens->testo) {
        *name = TfToken(propertyName.substr(
           UsdContrivedTokens->testo.GetString().size() + 1));
        return true;
    }

    return false;
}

/* virtual */
UsdSchemaKind UsdContrivedPublicMultipleApplyAPI::_GetSchemaKind() const
{
    return UsdContrivedPublicMultipleApplyAPI::schemaKind;
}

/* static */
bool
UsdContrivedPublicMultipleApplyAPI::CanApply(
    const UsdPrim &prim, const TfToken &name, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdContrivedPublicMultipleApplyAPI>(name, whyNot);
}

/* static */
UsdContrivedPublicMultipleApplyAPI
UsdContrivedPublicMultipleApplyAPI::Apply(const UsdPrim &prim, const TfToken &name)
{
    if (prim.ApplyAPI<UsdContrivedPublicMultipleApplyAPI>(name)) {
        return UsdContrivedPublicMultipleApplyAPI(prim, name);
    }
    return UsdContrivedPublicMultipleApplyAPI();
}

/* static */
const TfType &
UsdContrivedPublicMultipleApplyAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedPublicMultipleApplyAPI>();
    return tfType;
}

/* static */
bool 
UsdContrivedPublicMultipleApplyAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedPublicMultipleApplyAPI::_GetTfType() const
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
UsdContrivedPublicMultipleApplyAPI::GetTestAttrOneAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdContrivedTokens->testo_MultipleApplyTemplate_TestAttrOne));
}

UsdAttribute
UsdContrivedPublicMultipleApplyAPI::CreateTestAttrOneAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdContrivedTokens->testo_MultipleApplyTemplate_TestAttrOne),
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedPublicMultipleApplyAPI::GetTestAttrTwoAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdContrivedTokens->testo_MultipleApplyTemplate_TestAttrTwo));
}

UsdAttribute
UsdContrivedPublicMultipleApplyAPI::CreateTestAttrTwoAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdContrivedTokens->testo_MultipleApplyTemplate_TestAttrTwo),
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedPublicMultipleApplyAPI::GetPublicAPIAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdContrivedTokens->testo_MultipleApplyTemplate_));
}

UsdAttribute
UsdContrivedPublicMultipleApplyAPI::CreatePublicAPIAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdContrivedTokens->testo_MultipleApplyTemplate_),
                       SdfValueTypeNames->Opaque,
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
UsdContrivedPublicMultipleApplyAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdContrivedTokens->testo_MultipleApplyTemplate_TestAttrOne,
        UsdContrivedTokens->testo_MultipleApplyTemplate_TestAttrTwo,
        UsdContrivedTokens->testo_MultipleApplyTemplate_,
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
UsdContrivedPublicMultipleApplyAPI::GetSchemaAttributeNames(
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
