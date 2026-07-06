//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdProfiles/claimsAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdProfilesClaimsAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdProfilesClaimsAPI::~UsdProfilesClaimsAPI()
{
}

/* static */
UsdProfilesClaimsAPI
UsdProfilesClaimsAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdProfilesClaimsAPI();
    }
    return UsdProfilesClaimsAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdProfilesClaimsAPI::_GetSchemaKind() const
{
    return UsdProfilesClaimsAPI::schemaKind;
}

/* static */
bool
UsdProfilesClaimsAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdProfilesClaimsAPI>(whyNot);
}

/* static */
UsdProfilesClaimsAPI
UsdProfilesClaimsAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdProfilesClaimsAPI>()) {
        return UsdProfilesClaimsAPI(prim);
    }
    return UsdProfilesClaimsAPI();
}

/* static */
const TfType &
UsdProfilesClaimsAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdProfilesClaimsAPI>();
    return tfType;
}

/* static */
bool 
UsdProfilesClaimsAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdProfilesClaimsAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdProfilesClaimsAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

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

#include "pxr/usd/usdProfiles/profileRegistry.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"

PXR_NAMESPACE_OPEN_SCOPE

// Static key paths for the two top-level sub-dictionaries.
// Dynamic leaf paths (e.g. per-capability) are built by _ExtendKey at runtime.
TF_DEFINE_PRIVATE_TOKENS(
    _profilesTokens,
    (profilesInfo)
    (capabilityUsages)
    (profileCompatibility)
    ((capabilityUsagesKey,        "profilesInfo:capabilityUsages"))
    ((profileCompatibilityKey,    "profilesInfo:profileCompatibility"))
);

// Append a dynamic leaf token to a static prefix.
// e.g. _ExtendKey(capabilityUsagesKey, "usd.geom.mesh")
//      -> "profilesInfo:capabilityUsages:usd.geom.mesh"
static TfToken
_ExtendKey(const TfToken &prefix, const TfToken &leaf)
{
    return TfToken(prefix.GetString() + ":" + leaf.GetString());
}

// ---------------------------------------------------------------------- //
// _GetProfilesInfoByKey
// ---------------------------------------------------------------------- //

template <typename T>
bool
UsdProfilesClaimsAPI::_GetProfilesInfoByKey(const TfToken &keyPath, T *val) const
{
    VtValue v;
    if (!GetPrim().GetMetadataByDictKey(
            TfToken("customData"), keyPath, &v)) {
        return false;
    }
    if (!v.IsHolding<T>()) {
        return false;
    }
    *val = v.UncheckedGet<T>();
    return true;
}

// ---------------------------------------------------------------------- //
// Raw dictionary access
// ---------------------------------------------------------------------- //

VtDictionary
UsdProfilesClaimsAPI::GetProfilesInfo() const
{
    VtDictionary result;
    GetPrim().GetMetadataByDictKey(
        TfToken("customData"),
        _profilesTokens->profilesInfo,
        &result);
    return result;
}

void
UsdProfilesClaimsAPI::SetProfilesInfo(const VtDictionary &info) const
{
    GetPrim().SetMetadataByDictKey(
        TfToken("customData"),
        _profilesTokens->profilesInfo,
        info);
}

// ---------------------------------------------------------------------- //
// Capability Usage
// ---------------------------------------------------------------------- //

VtDictionary
UsdProfilesClaimsAPI::GetCapabilityUsages() const
{
    VtDictionary result;
    _GetProfilesInfoByKey(_profilesTokens->capabilityUsagesKey, &result);
    return result;
}

void
UsdProfilesClaimsAPI::SetCapabilityUsages(const VtDictionary &usages) const
{
    GetPrim().SetMetadataByDictKey(
        TfToken("customData"),
        _profilesTokens->capabilityUsagesKey,
        usages);
}

void
UsdProfilesClaimsAPI::SetCapabilityUsage(const TfToken &capability,
                                   const TfToken &degradationClass) const
{
    GetPrim().SetMetadataByDictKey(
        TfToken("customData"),
        _ExtendKey(_profilesTokens->capabilityUsagesKey, capability),
        VtValue(degradationClass));
}

TfToken
UsdProfilesClaimsAPI::GetCapabilityUsage(const TfToken &capability) const
{
    TfToken result;
    _GetProfilesInfoByKey(
        _ExtendKey(_profilesTokens->capabilityUsagesKey, capability), &result);
    return result;
}

// ---------------------------------------------------------------------- //
// Profile Compatibility
// ---------------------------------------------------------------------- //

VtArray<TfToken>
UsdProfilesClaimsAPI::GetCompatibleProfiles() const
{
    VtDictionary compat;
    if (!_GetProfilesInfoByKey(
            _profilesTokens->profileCompatibilityKey, &compat)) {
        return {};
    }
    VtArray<TfToken> profiles;
    profiles.reserve(compat.size());
    for (const auto &entry : compat) {
        profiles.push_back(TfToken(entry.first));
    }
    return profiles;
}

void
UsdProfilesClaimsAPI::SetProfileCompatible(const TfToken &profile) const
{
    GetPrim().SetMetadataByDictKey(
        TfToken("customData"),
        _ExtendKey(_profilesTokens->profileCompatibilityKey, profile),
        VtValue(VtArray<TfToken>()));
}

void
UsdProfilesClaimsAPI::SetProfileCompatibleWithExceptions(
    const TfToken &profile,
    const VtArray<TfToken> &exceptions) const
{
    GetPrim().SetMetadataByDictKey(
        TfToken("customData"),
        _ExtendKey(_profilesTokens->profileCompatibilityKey, profile),
        VtValue(exceptions));
}

VtArray<TfToken>
UsdProfilesClaimsAPI::GetProfileExceptions(const TfToken &profile) const
{
    VtArray<TfToken> result;
    _GetProfilesInfoByKey(
        _ExtendKey(_profilesTokens->profileCompatibilityKey, profile), &result);
    return result;
}

void
UsdProfilesClaimsAPI::ClearProfileCompatibility(const TfToken &profile) const
{
    GetPrim().ClearMetadataByDictKey(
        TfToken("customData"),
        _ExtendKey(_profilesTokens->profileCompatibilityKey, profile));
}

// ---------------------------------------------------------------------- //
// IsCompatibleWith
// ---------------------------------------------------------------------- //

UsdProfileRegistry::QueryStatus
UsdProfilesClaimsAPI::IsCompatibleWith(
    const TfToken &profile,
    std::vector<UsdProfileRegistry::CapabilityResult> *results) const
{
    // A compatibility entry must exist for this profile.
    VtDictionary compat;
    if (!_GetProfilesInfoByKey(
            _profilesTokens->profileCompatibilityKey, &compat)
        || compat.find(profile.GetString()) == compat.end()) {
        return UsdProfileRegistry::QueryStatus::NoPath;
    }

    // Collect required capabilities from the usage map.
    VtDictionary usages = GetCapabilityUsages();
    if (usages.empty()) {
        return UsdProfileRegistry::QueryStatus::NoPath;
    }
    std::vector<TfToken> required;
    required.reserve(usages.size());
    for (const auto &entry : usages) {
        required.push_back(TfToken(entry.first));
    }

    // Pass the stored exception set for this profile.
    VtArray<TfToken> exceptions = GetProfileExceptions(profile);
    std::vector<TfToken> excepted(exceptions.begin(), exceptions.end());

    return UsdProfileRegistry::CoversCapabilities(
        profile, required, excepted, results);
}

PXR_NAMESPACE_CLOSE_SCOPE
