//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdPhysics/limitAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdPhysicsLimitAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdPhysicsLimitAPI::~UsdPhysicsLimitAPI()
{
}

/* static */
UsdPhysicsLimitAPI
UsdPhysicsLimitAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsLimitAPI();
    }
    TfToken name;
    if (!IsPhysicsLimitAPIPath(path, &name)) {
        TF_CODING_ERROR("Invalid limit path <%s>.", path.GetText());
        return UsdPhysicsLimitAPI();
    }
    return UsdPhysicsLimitAPI(stage->GetPrimAtPath(path.GetPrimPath()), name);
}

UsdPhysicsLimitAPI
UsdPhysicsLimitAPI::Get(const UsdPrim &prim, const TfToken &name)
{
    return UsdPhysicsLimitAPI(prim, name);
}

/* static */
std::vector<UsdPhysicsLimitAPI>
UsdPhysicsLimitAPI::GetAll(const UsdPrim &prim)
{
    std::vector<UsdPhysicsLimitAPI> schemas;
    
    for (const auto &schemaName :
         UsdAPISchemaBase::_GetMultipleApplyInstanceNames(prim, _GetStaticTfType())) {
        schemas.emplace_back(prim, schemaName);
    }

    return schemas;
}


/* static */
bool 
UsdPhysicsLimitAPI::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdPhysicsTokens->limit_MultipleApplyTemplate_PhysicsLow),
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdPhysicsTokens->limit_MultipleApplyTemplate_PhysicsHigh),
    };

    return find(attrsAndRels.begin(), attrsAndRels.end(), baseName)
            != attrsAndRels.end();
}

/* static */
bool
UsdPhysicsLimitAPI::IsPhysicsLimitAPIPath(
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
        && tokens[0] == UsdPhysicsTokens->limit) {
        *name = TfToken(propertyName.substr(
           UsdPhysicsTokens->limit.GetString().size() + 1));
        return true;
    }

    return false;
}

/* virtual */
UsdSchemaKind UsdPhysicsLimitAPI::_GetSchemaKind() const
{
    return UsdPhysicsLimitAPI::schemaKind;
}

/* static */
bool
UsdPhysicsLimitAPI::CanApply(
    const UsdPrim &prim, const TfToken &name, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdPhysicsLimitAPI>(name, whyNot);
}

/* static */
UsdPhysicsLimitAPI
UsdPhysicsLimitAPI::Apply(const UsdPrim &prim, const TfToken &name)
{
    if (prim.ApplyAPI<UsdPhysicsLimitAPI>(name)) {
        return UsdPhysicsLimitAPI(prim, name);
    }
    return UsdPhysicsLimitAPI();
}

/* static */
const TfType &
UsdPhysicsLimitAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdPhysicsLimitAPI>();
    return tfType;
}

/* static */
bool 
UsdPhysicsLimitAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdPhysicsLimitAPI::_GetTfType() const
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
UsdPhysicsLimitAPI::GetLowAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdPhysicsTokens->limit_MultipleApplyTemplate_PhysicsLow));
}

UsdAttribute
UsdPhysicsLimitAPI::CreateLowAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdPhysicsTokens->limit_MultipleApplyTemplate_PhysicsLow),
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsLimitAPI::GetHighAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdPhysicsTokens->limit_MultipleApplyTemplate_PhysicsHigh));
}

UsdAttribute
UsdPhysicsLimitAPI::CreateHighAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdPhysicsTokens->limit_MultipleApplyTemplate_PhysicsHigh),
                       SdfValueTypeNames->Float,
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
UsdPhysicsLimitAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdPhysicsTokens->limit_MultipleApplyTemplate_PhysicsLow,
        UsdPhysicsTokens->limit_MultipleApplyTemplate_PhysicsHigh,
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
UsdPhysicsLimitAPI::GetSchemaAttributeNames(
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
