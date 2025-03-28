//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/multipleApplyAPI_1.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedMultipleApplyAPI_1,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdContrivedMultipleApplyAPI_1::~UsdContrivedMultipleApplyAPI_1()
{
}

/* static */
UsdContrivedMultipleApplyAPI_1
UsdContrivedMultipleApplyAPI_1::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedMultipleApplyAPI_1();
    }
    TfToken name;
    if (!IsMultipleApplyAPI_1Path(path, &name)) {
        TF_CODING_ERROR("Invalid testNewVersion path <%s>.", path.GetText());
        return UsdContrivedMultipleApplyAPI_1();
    }
    return UsdContrivedMultipleApplyAPI_1(stage->GetPrimAtPath(path.GetPrimPath()), name);
}

UsdContrivedMultipleApplyAPI_1
UsdContrivedMultipleApplyAPI_1::Get(const UsdPrim &prim, const TfToken &name)
{
    return UsdContrivedMultipleApplyAPI_1(prim, name);
}

/* static */
std::vector<UsdContrivedMultipleApplyAPI_1>
UsdContrivedMultipleApplyAPI_1::GetAll(const UsdPrim &prim)
{
    std::vector<UsdContrivedMultipleApplyAPI_1> schemas;
    
    for (const auto &schemaName :
         UsdAPISchemaBase::_GetMultipleApplyInstanceNames(prim, _GetStaticTfType())) {
        schemas.emplace_back(prim, schemaName);
    }

    return schemas;
}


/* static */
bool 
UsdContrivedMultipleApplyAPI_1::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdContrivedTokens->testNewVersion_MultipleApplyTemplate_TestAttrOne),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdContrivedTokens->testNewVersion_MultipleApplyTemplate_TestAttrTwo),
    };

    return find(attrsAndRels.begin(), attrsAndRels.end(), baseName)
            != attrsAndRels.end();
}

/* static */
bool
UsdContrivedMultipleApplyAPI_1::IsMultipleApplyAPI_1Path(
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
        && tokens[0] == UsdContrivedTokens->testNewVersion) {
        *name = TfToken(propertyName.substr(
           UsdContrivedTokens->testNewVersion.GetString().size() + 1));
        return true;
    }

    return false;
}

/* virtual */
UsdSchemaKind UsdContrivedMultipleApplyAPI_1::_GetSchemaKind() const
{
    return UsdContrivedMultipleApplyAPI_1::schemaKind;
}

/* static */
bool
UsdContrivedMultipleApplyAPI_1::CanApply(
    const UsdPrim &prim, const TfToken &name, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdContrivedMultipleApplyAPI_1>(name, whyNot);
}

/* static */
UsdContrivedMultipleApplyAPI_1
UsdContrivedMultipleApplyAPI_1::Apply(const UsdPrim &prim, const TfToken &name)
{
    if (prim.ApplyAPI<UsdContrivedMultipleApplyAPI_1>(name)) {
        return UsdContrivedMultipleApplyAPI_1(prim, name);
    }
    return UsdContrivedMultipleApplyAPI_1();
}

/* static */
const TfType &
UsdContrivedMultipleApplyAPI_1::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedMultipleApplyAPI_1>();
    return tfType;
}

/* static */
bool 
UsdContrivedMultipleApplyAPI_1::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedMultipleApplyAPI_1::_GetTfType() const
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
UsdContrivedMultipleApplyAPI_1::GetTestAttrOneAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdContrivedTokens->testNewVersion_MultipleApplyTemplate_TestAttrOne));
}

UsdAttribute
UsdContrivedMultipleApplyAPI_1::CreateTestAttrOneAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdContrivedTokens->testNewVersion_MultipleApplyTemplate_TestAttrOne),
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedMultipleApplyAPI_1::GetTestAttrTwoAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdContrivedTokens->testNewVersion_MultipleApplyTemplate_TestAttrTwo));
}

UsdAttribute
UsdContrivedMultipleApplyAPI_1::CreateTestAttrTwoAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdContrivedTokens->testNewVersion_MultipleApplyTemplate_TestAttrTwo),
                       SdfValueTypeNames->Double,
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
UsdContrivedMultipleApplyAPI_1::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdContrivedTokens->testNewVersion_MultipleApplyTemplate_TestAttrOne,
        UsdContrivedTokens->testNewVersion_MultipleApplyTemplate_TestAttrTwo,
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
UsdContrivedMultipleApplyAPI_1::GetSchemaAttributeNames(
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
