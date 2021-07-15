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
#include "pxr/usd/usdPhysics/driveAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdPhysicsDriveAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (PhysicsDriveAPI)
    (drive)
);

/* virtual */
UsdPhysicsDriveAPI::~UsdPhysicsDriveAPI()
{
}

/* static */
UsdPhysicsDriveAPI
UsdPhysicsDriveAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsDriveAPI();
    }
    TfToken name;
    if (!IsPhysicsDriveAPIPath(path, &name)) {
        TF_CODING_ERROR("Invalid drive path <%s>.", path.GetText());
        return UsdPhysicsDriveAPI();
    }
    return UsdPhysicsDriveAPI(stage->GetPrimAtPath(path.GetPrimPath()), name);
}

UsdPhysicsDriveAPI
UsdPhysicsDriveAPI::Get(const UsdPrim &prim, const TfToken &name)
{
    return UsdPhysicsDriveAPI(prim, name);
}


/* static */
bool 
UsdPhysicsDriveAPI::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
        UsdPhysicsTokens->physicsType,
        UsdPhysicsTokens->physicsMaxForce,
        UsdPhysicsTokens->physicsTargetPosition,
        UsdPhysicsTokens->physicsTargetVelocity,
        UsdPhysicsTokens->physicsDamping,
        UsdPhysicsTokens->physicsStiffness,
    };

    return find(attrsAndRels.begin(), attrsAndRels.end(), baseName)
            != attrsAndRels.end();
}

/* static */
bool
UsdPhysicsDriveAPI::IsPhysicsDriveAPIPath(
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
        && tokens[0] == _schemaTokens->drive) {
        *name = TfToken(propertyName.substr(
            _schemaTokens->drive.GetString().size() + 1));
        return true;
    }

    return false;
}

/* virtual */
UsdSchemaKind UsdPhysicsDriveAPI::_GetSchemaKind() const
{
    return UsdPhysicsDriveAPI::schemaKind;
}

/* static */
bool
UsdPhysicsDriveAPI::CanApply(
    const UsdPrim &prim, const TfToken &name, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdPhysicsDriveAPI>(name, whyNot);
}

/* static */
UsdPhysicsDriveAPI
UsdPhysicsDriveAPI::Apply(const UsdPrim &prim, const TfToken &name)
{
    // Ensure that the instance name is valid.
    TfTokenVector tokens = SdfPath::TokenizeIdentifierAsTokens(name);

    if (tokens.empty()) {
        TF_CODING_ERROR("Invalid PhysicsDriveAPI name '%s'.", 
                        name.GetText());
        return UsdPhysicsDriveAPI();
    }

    const TfToken &baseName = tokens.back();
    if (IsSchemaPropertyBaseName(baseName)) {
        TF_CODING_ERROR("Invalid PhysicsDriveAPI name '%s'. "
                        "The base-name '%s' is a schema property name.", 
                        name.GetText(), baseName.GetText());
        return UsdPhysicsDriveAPI();
    }

    if (prim.ApplyAPI<UsdPhysicsDriveAPI>(name)) {
        return UsdPhysicsDriveAPI(prim, name);
    }
    return UsdPhysicsDriveAPI();
}

/* static */
const TfType &
UsdPhysicsDriveAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdPhysicsDriveAPI>();
    return tfType;
}

/* static */
bool 
UsdPhysicsDriveAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdPhysicsDriveAPI::_GetTfType() const
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
        {_schemaTokens->drive, instanceName, propName};
    return TfToken(SdfPath::JoinIdentifier(identifiers));
}

UsdAttribute
UsdPhysicsDriveAPI::GetTypeAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdPhysicsTokens->physicsType));
}

UsdAttribute
UsdPhysicsDriveAPI::CreateTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdPhysicsTokens->physicsType),
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsDriveAPI::GetMaxForceAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdPhysicsTokens->physicsMaxForce));
}

UsdAttribute
UsdPhysicsDriveAPI::CreateMaxForceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdPhysicsTokens->physicsMaxForce),
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsDriveAPI::GetTargetPositionAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdPhysicsTokens->physicsTargetPosition));
}

UsdAttribute
UsdPhysicsDriveAPI::CreateTargetPositionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdPhysicsTokens->physicsTargetPosition),
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsDriveAPI::GetTargetVelocityAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdPhysicsTokens->physicsTargetVelocity));
}

UsdAttribute
UsdPhysicsDriveAPI::CreateTargetVelocityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdPhysicsTokens->physicsTargetVelocity),
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsDriveAPI::GetDampingAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdPhysicsTokens->physicsDamping));
}

UsdAttribute
UsdPhysicsDriveAPI::CreateDampingAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdPhysicsTokens->physicsDamping),
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsDriveAPI::GetStiffnessAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdPhysicsTokens->physicsStiffness));
}

UsdAttribute
UsdPhysicsDriveAPI::CreateStiffnessAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdPhysicsTokens->physicsStiffness),
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
UsdPhysicsDriveAPI::GetSchemaAttributeNames(
    bool includeInherited, const TfToken instanceName)
{
    static TfTokenVector localNames = {
        UsdPhysicsTokens->physicsType,
        UsdPhysicsTokens->physicsMaxForce,
        UsdPhysicsTokens->physicsTargetPosition,
        UsdPhysicsTokens->physicsTargetVelocity,
        UsdPhysicsTokens->physicsDamping,
        UsdPhysicsTokens->physicsStiffness,
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
