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
#include "pxr/usd/usdContrived/derivedMultipleApplyAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

namespace foo {

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedDerivedMultipleApplyAPI,
        TfType::Bases< UsdContrivedMultipleApplyAPI > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (DerivedMultipleApplyAPI)
    (derived)
);

/* virtual */
UsdContrivedDerivedMultipleApplyAPI::~UsdContrivedDerivedMultipleApplyAPI()
{
}

/* static */
UsdContrivedDerivedMultipleApplyAPI
UsdContrivedDerivedMultipleApplyAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedDerivedMultipleApplyAPI();
    }
    TfToken name;
    if (!IsDerivedMultipleApplyAPIPath(path, &name)) {
        TF_CODING_ERROR("Invalid collection path <%s>.", path.GetText());
        return UsdCollectionAPI();
    }
    return UsdContrivedDerivedMultipleApplyAPI(stage->GetPrimAtPath(path.GetPrimPath()), name);
}

UsdContrivedDerivedMultipleApplyAPI
UsdContrivedDerivedMultipleApplyAPI::Get(const UsdPrim &prim, const TfToken &name)
{
    return UsdContrivedDerivedMultipleApplyAPI(prim, name);
}


/* static */
bool 
UsdContrivedDerivedMultipleApplyAPI::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
        UsdContrivedTokens->testAttrThree,
    };

    return find(attrsAndRels.begin(), attrsAndRels.end(), baseName)
            != attrsAndRels.end();
}

/* static */
bool
UsdContrivedDerivedMultipleApplyAPI::IsDerivedMultipleApplyAPIPath(
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
        && tokens[0] == UsdTokens->derived) {
        *name = TfToken(propertyName.substr(
            UsdTokens->derived.GetString().size() + 1));
        return true;
    }

    return false;
}
/*virtual*/
bool 
UsdContrivedDerivedMultipleApplyAPI::_IsAppliedAPISchema() const 
{
    return true;
}

/*virtual*/
bool 
UsdContrivedDerivedMultipleApplyAPI::_IsMultipleApplyAPISchema() const 
{
    return true;
}

/* static */
UsdContrivedDerivedMultipleApplyAPI
UsdContrivedDerivedMultipleApplyAPI::Apply(const UsdPrim &prim, const TfToken &name)
{
    return UsdAPISchemaBase::_MultipleApplyAPISchema<UsdContrivedDerivedMultipleApplyAPI>(
            prim, _schemaTokens->DerivedMultipleApplyAPI, name);
}

/* static */
const TfType &
UsdContrivedDerivedMultipleApplyAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedDerivedMultipleApplyAPI>();
    return tfType;
}

/* static */
bool 
UsdContrivedDerivedMultipleApplyAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedDerivedMultipleApplyAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/// Returns the property name prefixed with the correct namespace prefix, which
/// is composed of the the API's propertyNamespacePrefix metadata and the
/// instance name of the API.
inline
TfToken
_GetNamespacedPropertyName(const TfToken instanceName, const TfToken propName)
{
    TfTokenVector identifiers =
        {_schemaTokens->derived, instanceName, propName};
    return TfToken(SdfPath::JoinIdentifier(identifiers));
}

UsdAttribute
UsdContrivedDerivedMultipleApplyAPI::GetTestAttrThreeAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdContrivedTokens->testAttrThree));
}

UsdAttribute
UsdContrivedDerivedMultipleApplyAPI::CreateTestAttrThreeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdContrivedTokens->testAttrThree),
                       SdfValueTypeNames->Int,
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
UsdContrivedDerivedMultipleApplyAPI::GetSchemaAttributeNames(
    bool includeInherited, const TfToken instanceName)
{
    static TfTokenVector localNames = {
        UsdContrivedTokens->testAttrThree,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            instanceName,
            UsdContrivedMultipleApplyAPI::GetSchemaAttributeNames(true, instanceName),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

}

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'namespace foo {', '}'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
