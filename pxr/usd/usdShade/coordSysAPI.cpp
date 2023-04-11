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
#include "pxr/usd/usdShade/coordSysAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdShadeCoordSysAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (coordSys)
);

/* virtual */
UsdShadeCoordSysAPI::~UsdShadeCoordSysAPI()
{
}

/* static */
UsdShadeCoordSysAPI
UsdShadeCoordSysAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeCoordSysAPI();
    }
    TfToken name;
    if (!IsCoordSysAPIPath(path, &name)) {
        TF_CODING_ERROR("Invalid coordSys path <%s>.", path.GetText());
        return UsdShadeCoordSysAPI();
    }
    return UsdShadeCoordSysAPI(stage->GetPrimAtPath(path.GetPrimPath()), name);
}

UsdShadeCoordSysAPI
UsdShadeCoordSysAPI::Get(const UsdPrim &prim, const TfToken &name)
{
    return UsdShadeCoordSysAPI(prim, name);
}

/* static */
std::vector<UsdShadeCoordSysAPI>
UsdShadeCoordSysAPI::GetAll(const UsdPrim &prim)
{
    std::vector<UsdShadeCoordSysAPI> schemas;
    
    for (const auto &schemaName :
         UsdAPISchemaBase::_GetMultipleApplyInstanceNames(prim, _GetStaticTfType())) {
        schemas.emplace_back(prim, schemaName);
    }

    return schemas;
}


/* static */
bool 
UsdShadeCoordSysAPI::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
        UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
            UsdShadeTokens->coordSys_MultipleApplyTemplate_Binding),
    };

    return find(attrsAndRels.begin(), attrsAndRels.end(), baseName)
            != attrsAndRels.end();
}

/* static */
bool
UsdShadeCoordSysAPI::IsCoordSysAPIPath(
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
        && tokens[0] == _schemaTokens->coordSys) {
        *name = TfToken(propertyName.substr(
            _schemaTokens->coordSys.GetString().size() + 1));
        return true;
    }

    return false;
}

/* virtual */
UsdSchemaKind UsdShadeCoordSysAPI::_GetSchemaKind() const
{
    return UsdShadeCoordSysAPI::schemaKind;
}

/* static */
bool
UsdShadeCoordSysAPI::CanApply(
    const UsdPrim &prim, const TfToken &name, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdShadeCoordSysAPI>(name, whyNot);
}

/* static */
UsdShadeCoordSysAPI
UsdShadeCoordSysAPI::Apply(const UsdPrim &prim, const TfToken &name)
{
    if (prim.ApplyAPI<UsdShadeCoordSysAPI>(name)) {
        return UsdShadeCoordSysAPI(prim, name);
    }
    return UsdShadeCoordSysAPI();
}

/* static */
const TfType &
UsdShadeCoordSysAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdShadeCoordSysAPI>();
    return tfType;
}

/* static */
bool 
UsdShadeCoordSysAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdShadeCoordSysAPI::_GetTfType() const
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

UsdRelationship
UsdShadeCoordSysAPI::GetBindingRel() const
{
    return GetPrim().GetRelationship(
        _GetNamespacedPropertyName(
            GetName(),
            UsdShadeTokens->coordSys_MultipleApplyTemplate_Binding));
}

UsdRelationship
UsdShadeCoordSysAPI::CreateBindingRel() const
{
    return GetPrim().CreateRelationship(
                       _GetNamespacedPropertyName(
                           GetName(),
                           UsdShadeTokens->coordSys_MultipleApplyTemplate_Binding),
                       /* custom = */ false);
}

/*static*/
const TfTokenVector&
UsdShadeCoordSysAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

/*static*/
TfTokenVector
UsdShadeCoordSysAPI::GetSchemaAttributeNames(
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

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(USD_SHADE_COORD_SYS_IS_MULTI_APPLY, "Warn",
        "Environment variable to phase in conversion of UsdShadeCoordSysAPI to "
        "a multi-apply API. The default is being set to Warn, which will "
        "appropriately warn about using UsdShadeCoordSysAPI APIs which operate "
        "for non-applied mode. We expect to turn this environment varialbe to "
        "True in subsequent releases. Additionally clients can ignore the "
        "warnings by setting the environment variable to False");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    // token values for USD_SHADE_COORD_SYS_IS_MULTI_APPLY
    (Warn)
    (True)
    (False)
);

// Return appropriate value depending on the 
// USD_SHADE_COORD_SYS_IS_MULTI_APPLY environment variable.
// 0: False (Old deprecated behavior)
// 1: True (New UsdShadeCoordSysAPI multiapply)
// 2: Warn (Warn, use new if nothing fallback to old).
// 1: Anything else
int
_UsdShadeCoordSysAPIMultiApplyChecker()
{
    const std::string usdShadeCoordSysMultApply = 
        TfGetEnvSetting(USD_SHADE_COORD_SYS_IS_MULTI_APPLY);

    static const int checker = 
        (usdShadeCoordSysMultApply == _tokens->False.GetString()) ? 0 :
        (usdShadeCoordSysMultApply == _tokens->True.GetString()) ? 1 :
        (usdShadeCoordSysMultApply == _tokens->Warn.GetString()) ? 2 : 1;

    return checker;
}

void
_WarnOnUseOfDeprecatedNonAppliedAPI(const char* deprecatedAPI)
{
    TF_WARN("Using deprecated method (%s) from non-applied "
            "UsdShadeCoordSysAPI. UsdShadeCoordSysAPI schema has been updated "
            "to be a multi-apply API.", deprecatedAPI);
}

void
_WarnOnDeprecatedAsset(const UsdPrim &prim)
{
    TF_WARN("Prim at path (%s) is using old style non-applied "
            "UsdShadeCoordSysAPI coordSys bindings. UsdShadeCoordSysAPI schema "
            "has been updated to be a multi-apply API.", 
            prim.GetPath().GetText());
}

/* deprecated */
std::vector<UsdShadeCoordSysAPI::Binding> 
UsdShadeCoordSysAPI::GetLocalBindings() const
{
    TRACE_FUNCTION();

    std::vector<Binding> result;
    SdfPathVector targets;
    static const int checker = _UsdShadeCoordSysAPIMultiApplyChecker();

    // First try new multi-applied UsdShadeCoordSysAPI
    // USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to True.
    if (checker == 1) {
        return UsdShadeCoordSysAPI::GetLocalBindingsForPrim(GetPrim());
    }

    // USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to Warn, try to get
    // multi-api compliant bindings.
    if (checker == 2) {
        result = UsdShadeCoordSysAPI::GetLocalBindingsForPrim(GetPrim());
        if (!result.empty()) {
            return result;
        }
    }

    for (const UsdProperty &prop:
         GetPrim().GetAuthoredPropertiesInNamespace(_schemaTokens->coordSys)) {
        if (UsdRelationship rel = prop.As<UsdRelationship>()) {
            targets.clear();
            if (rel.GetForwardedTargets(&targets) && !targets.empty()) {
                Binding b = {rel.GetBaseName(), rel.GetPath(), targets.front()};
                result.push_back(b);
            }
        }
    }

    // If result is not empty old style coordSysBindings are found
    if (!result.empty() && checker == 2) {
        _WarnOnDeprecatedAsset(GetPrim());
    }

    return result;
}

/* static */
void
UsdShadeCoordSysAPI::_GetBindingsForPrim(const UsdPrim &prim,
        std::vector<Binding> &result, bool checkExistingBindings)
{
    if (!prim.HasAPI<UsdShadeCoordSysAPI>()) {
        return;
    }

    SdfPathVector targets;
    // only get binding relationship with specific instances applied on the
    // prim.
    for (const TfToken &schemaName : 
            UsdAPISchemaBase::_GetMultipleApplyInstanceNames(prim, 
                _GetStaticTfType())) {
        const TfToken &relName = _GetNamespacedPropertyName(schemaName,
                UsdShadeTokens->coordSys_MultipleApplyTemplate_Binding);

        if (UsdRelationship rel = prim.GetRelationship(relName)) {
            bool nameIsAlreadyBound = false;
            if (checkExistingBindings) {
                for (Binding const& existing : result) {
                    if (existing.name == 
                            UsdShadeCoordSysAPI::GetBindingBaseName(
                                rel.GetName())) {
                        nameIsAlreadyBound = true;
                        break;
                    }
                }
            }
            if (!checkExistingBindings || !nameIsAlreadyBound) {
                targets.clear();
                if (rel.GetForwardedTargets(&targets) && !targets.empty()) {
                    Binding b = {UsdShadeCoordSysAPI::GetBindingBaseName(
                            rel.GetName()), rel.GetPath(),
                        targets.front()};
                    result.push_back(b);
                }
            }
        }
    }
}

/* static */
std::vector<UsdShadeCoordSysAPI::Binding>
UsdShadeCoordSysAPI::GetLocalBindingsForPrim(const UsdPrim &prim)
{
    std::vector<Binding> result;

    _GetBindingsForPrim(prim, result, false);

    return result;
}

UsdShadeCoordSysAPI::Binding
UsdShadeCoordSysAPI::GetLocalBinding() const
{
    Binding result;
    SdfPathVector targets;
    const UsdRelationship &rel = GetBindingRel();
    if (rel) {
        if (rel.GetForwardedTargets(&targets) && !targets.empty()) {
            result = {UsdShadeCoordSysAPI::GetBindingBaseName(rel.GetName()), 
                rel.GetPath(), targets.front()};
        }
    }
    return result;
}

/* deprecated */
std::vector<UsdShadeCoordSysAPI::Binding> 
UsdShadeCoordSysAPI::FindBindingsWithInheritance() const
{
    TRACE_FUNCTION();

    std::vector<Binding> result;
    SdfPathVector targets;
    static const int checker = _UsdShadeCoordSysAPIMultiApplyChecker();

    // First try new multi-applied UsdShadeCoordSysAPI
    // USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to True.
    if (checker == 1) {
        return 
            UsdShadeCoordSysAPI::FindBindingsWithInheritanceForPrim(GetPrim());
    }

    // USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to Warn, try to get
    // multi-api compliant bindings.
    if (checker == 2) {
        result = 
            UsdShadeCoordSysAPI::FindBindingsWithInheritanceForPrim(GetPrim());
        if (!result.empty()) {
            return result;
        }
    }

    for (UsdPrim prim = GetPrim(); prim; prim = prim.GetParent()) {
        SdfPathVector targets;
        for (UsdProperty prop : prim.GetAuthoredPropertiesInNamespace(
                    _schemaTokens->coordSys)) {
            if (UsdRelationship rel = prop.As<UsdRelationship>()) {
                // Check if name is already bound; skip if bound.
                bool nameIsAlreadyBound = false;
                for (Binding const& existing: result) {
                    if (existing.name == rel.GetBaseName()) {
                        nameIsAlreadyBound = true;
                        break;
                    }
                }
                if (!nameIsAlreadyBound) {
                    targets.clear();
                    if (rel.GetForwardedTargets(&targets) && !targets.empty()) {
                        Binding b = {rel.GetBaseName(), rel.GetPath(),
                            targets.front()};
                        result.push_back(b);
                    }
                }
            }
        }
    }

    // If result is not empty old style coordSysBindings are found
    if (!result.empty() && checker == 2) {
        _WarnOnDeprecatedAsset(GetPrim());
    }

    return result;
}

/* static */
std::vector<UsdShadeCoordSysAPI::Binding> 
UsdShadeCoordSysAPI::FindBindingsWithInheritanceForPrim(const UsdPrim &prim)
{
    std::vector<Binding> result;
    
    for (UsdPrim p = prim; p; p = p.GetParent()) {
        _GetBindingsForPrim(p, result, true);
    }
    
    return result;
}

UsdShadeCoordSysAPI::Binding
UsdShadeCoordSysAPI::FindBindingWithInheritance() const
{
    SdfPathVector targets;
    UsdShadeCoordSysAPI::Binding binding;
    const TfToken &relName = _GetNamespacedPropertyName(GetName(),
            UsdShadeTokens->coordSys_MultipleApplyTemplate_Binding);

    for (UsdPrim p = GetPrim(); p; p = p.GetParent()) {
        if (!p.HasAPI<UsdShadeCoordSysAPI>(GetName())) {
            continue;
        }
        
        if (UsdRelationship rel = p.GetRelationship(relName)) {
            if (rel.GetForwardedTargets(&targets) && !targets.empty()) {
                binding = {UsdShadeCoordSysAPI::GetBindingBaseName(
                        rel.GetName()), rel.GetPath(),
                    targets.front()};
                break;
            }
        }
    }

    return binding;
}

/* deprecated */
bool
UsdShadeCoordSysAPI::HasLocalBindings() const
{
    TRACE_FUNCTION();
    
    static const int checker = _UsdShadeCoordSysAPIMultiApplyChecker();

    // First try new multi-applied UsdShadeCoordSysAPI
    // USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to True.
    if (checker == 1) {
        return UsdShadeCoordSysAPI::HasLocalBindingsForPrim(GetPrim());
    }

    // USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to Warn, check if
    // multi-api compliant bindings are present.
    if (checker == 2) {
        bool result = UsdShadeCoordSysAPI::HasLocalBindingsForPrim(GetPrim());
        if (result) {
            return true;
        }
    }

    for (const UsdProperty &prop:
         GetPrim().GetAuthoredPropertiesInNamespace(_schemaTokens->coordSys)) {
        if (UsdRelationship rel = prop.As<UsdRelationship>()) {
            if (rel) {
                if (checker == 2) {
                    _WarnOnDeprecatedAsset(GetPrim());
                }
                return true;
            }
        }
    }
    return false;
}

/* static */
bool
UsdShadeCoordSysAPI::HasLocalBindingsForPrim(const UsdPrim &prim)
{
    return prim.HasAPI<UsdShadeCoordSysAPI>();
}

/* deprecated */
bool 
UsdShadeCoordSysAPI::Bind(const TfToken &name, const SdfPath &path) const
{
    TRACE_FUNCTION();

    bool bound = false;
    static const int checker = _UsdShadeCoordSysAPIMultiApplyChecker();
    
    // First try new multi-applied UsdShadeCoordSysAPI
    // USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to True.
    if (checker == 1) {
        return UsdShadeCoordSysAPI::Apply(GetPrim(), name).Bind(path);
    }

    // USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to Warn, Try to create 
    // binding also for multi-apply compliant relationship. 
    if (checker == 2) {
        if (GetPrim().HasAPI<UsdShadeCoordSysAPI>(name)) {
            bound |= UsdShadeCoordSysAPI::Apply(GetPrim(), name).Bind(path);
        }
    }

    TfToken relName = GetCoordSysRelationshipName(name);
    if (UsdRelationship rel = GetPrim().CreateRelationship(relName)) {
        if (checker == 2) {
            _WarnOnUseOfDeprecatedNonAppliedAPI("UsdShadeCoordSysAPI::Bind");
        }
        bound |= rel.SetTargets(SdfPathVector(1, path));
    }
    return bound;
}

/* deprecated */
bool
UsdShadeCoordSysAPI::ApplyAndBind(
        const TfToken &name, const SdfPath &path) const
{
    UsdShadeCoordSysAPI coordSysAPI = 
        UsdShadeCoordSysAPI::Apply(GetPrim(), name);
    return coordSysAPI.Bind(name, path);
}

bool
UsdShadeCoordSysAPI::Bind(const SdfPath &path) const
{
    UsdRelationship rel = CreateBindingRel();
    if (rel) {
        return rel.SetTargets(SdfPathVector(1, path));
    }
    return false;
}

/* deprecated */
bool 
UsdShadeCoordSysAPI::ClearBinding(const TfToken &name, bool removeSpec) const
{
    TRACE_FUNCTION();

    bool cleared = false;
    static const int checker = _UsdShadeCoordSysAPIMultiApplyChecker();

    // First try new multi-applied UsdShadeCoordSysAPI
    // USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to True
    if (checker == 1) {
        return UsdShadeCoordSysAPI::Apply(GetPrim(), name).
            ClearBinding(removeSpec);
    }

    // USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to Warn, also try to clear
    // binding for mult-apply compliant relationship.
    if (checker == 2) {
        if (GetPrim().HasAPI<UsdShadeCoordSysAPI>(name)) {
            cleared |= UsdShadeCoordSysAPI::Apply(GetPrim(), name).
                ClearBinding(removeSpec);
        }
    }

    TfToken relName = GetCoordSysRelationshipName(name);
    if (UsdRelationship rel = GetPrim().GetRelationship(relName)) {
        if (checker == 2) {
            _WarnOnUseOfDeprecatedNonAppliedAPI(
                    "UsdShadeCoordSysAPI::ClearBinding");
        }
        cleared |= rel.ClearTargets(removeSpec);
    }
    return cleared;
}

bool
UsdShadeCoordSysAPI::ClearBinding(bool removeSpec) const
{
    UsdRelationship rel = GetBindingRel();
    if (rel) {
        return rel.ClearTargets(removeSpec);
    }
    return false;
}

/* deprecated */
bool 
UsdShadeCoordSysAPI::BlockBinding(const TfToken &name) const
{
    bool blocked = false;

    static const int checker = _UsdShadeCoordSysAPIMultiApplyChecker();

    TRACE_FUNCTION()

    // First try new multi-applied UsdShadeCoordSysAPI
    // USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to True
    if (checker == 1) {
        return UsdShadeCoordSysAPI::Apply(GetPrim(), name).BlockBinding();
    }

    // USD_SHADE_COORD_SYS_IS_MULTI_APPLY is set to Warn, also try to block
    // binding for mult-apply compliant relationship.
    if (checker == 2) {
        if (GetPrim().HasAPI<UsdShadeCoordSysAPI>(name)) {
            blocked |= UsdShadeCoordSysAPI::Apply(GetPrim(), name).
                BlockBinding();
        }
    }

    TfToken relName = GetCoordSysRelationshipName(name);
    if (UsdRelationship rel = GetPrim().CreateRelationship(relName)) {
        if (checker == 2) {
            _WarnOnUseOfDeprecatedNonAppliedAPI(
                    "UsdShadeCoordSysAPI::BlockBinding");
        }
        blocked |= rel.SetTargets({});
    }
    return blocked;
}

bool
UsdShadeCoordSysAPI::BlockBinding() const
{
    UsdRelationship rel = CreateBindingRel();
    if (rel) {
        return rel.SetTargets({});
    }
    return false;
}

/* deprecated */
TfToken
UsdShadeCoordSysAPI::GetCoordSysRelationshipName(const std::string &name)
{
    return TfToken(_schemaTokens->coordSys.GetString() + ":" + name);
}

/* static */
bool
UsdShadeCoordSysAPI::CanContainPropertyName(const TfToken &name)
{
    return TfStringStartsWith(name, UsdShadeTokens->coordSys);
}

/* static */
TfToken
UsdShadeCoordSysAPI::GetBindingBaseName(const TfToken &bindingName)
{
    return TfToken(
            SdfPath::StripPrefixNamespace(
                bindingName, UsdShadeTokens->coordSys).first);
}

TfToken
UsdShadeCoordSysAPI::GetBindingBaseName() const
{
    const TfToken &relName = _GetNamespacedPropertyName(GetName(),
            UsdShadeTokens->coordSys_MultipleApplyTemplate_Binding);
    return UsdShadeCoordSysAPI::GetBindingBaseName(relName);
}

PXR_NAMESPACE_CLOSE_SCOPE
