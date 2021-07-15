//
// Copyright 2016 Pixar
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
#include "pxr/usd/usdPhysics/limitAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdPhysicsLimitAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (PhysicsLimitAPI)
    (limit)
);

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
bool 
UsdPhysicsLimitAPI::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
        UsdPhysicsTokens->physicsLow,
        UsdPhysicsTokens->physicsHigh,
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
        && tokens[0] == _schemaTokens->limit) {
        *name = TfToken(propertyName.substr(
            _schemaTokens->limit.GetString().size() + 1));
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
    // Ensure that the instance name is valid.
    TfTokenVector tokens = SdfPath::TokenizeIdentifierAsTokens(name);

    if (tokens.empty()) {
        TF_CODING_ERROR("Invalid PhysicsLimitAPI name '%s'.", 
                        name.GetText());
        return UsdPhysicsLimitAPI();
    }

    const TfToken &baseName = tokens.back();
    if (IsSchemaPropertyBaseName(baseName)) {
        TF_CODING_ERROR("Invalid PhysicsLimitAPI name '%s'. "
                        "The base-name '%s' is a schema property name.", 
                        name.GetText(), baseName.GetText());
        return UsdPhysicsLimitAPI();
    }

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
    TfTokenVector identifiers =
        {_schemaTokens->limit, instanceName, propName};
    return TfToken(SdfPath::JoinIdentifier(identifiers));
}

UsdAttribute
UsdPhysicsLimitAPI::GetLowAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdPhysicsTokens->physicsLow));
}

UsdAttribute
UsdPhysicsLimitAPI::CreateLowAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdPhysicsTokens->physicsLow),
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
            UsdPhysicsTokens->physicsHigh));
}

UsdAttribute
UsdPhysicsLimitAPI::CreateHighAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdPhysicsTokens->physicsHigh),
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(
    const TfToken instanceName,
    const TfTokenVector& left,
    const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());

    for (const TfToken attrName : right) {
        result.push_back(
            _GetNamespacedPropertyName(instanceName, attrName));
    }
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdPhysicsLimitAPI::GetSchemaAttributeNames(
    bool includeInherited, const TfToken instanceName)
{
    static TfTokenVector localNames = {
        UsdPhysicsTokens->physicsLow,
        UsdPhysicsTokens->physicsHigh,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            instanceName,
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
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
