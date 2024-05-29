//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/editTarget.h"
#include "pxr/usd/usd/inherits.h"
#include "pxr/usd/usd/instanceCache.h"
#include "pxr/usd/usd/payloads.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/references.h"
#include "pxr/usd/usd/resolver.h"
#include "pxr/usd/usd/resolveTarget.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/specializes.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/usd/variantSets.h"

#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/plug/registry.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/singularTask.h"
#include "pxr/base/work/utils.h"
#include "pxr/base/work/withScopedParallelism.h"

#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pxrTslRobinMap/robin_set.h"
#include "pxr/base/tf/hash.h"

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/parallel_sort.h>

#include <algorithm>
#include <functional>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

SdfPath
UsdPrim::_ProtoToInstancePathMap
::MapProtoToInstance(SdfPath const &protoPath) const
{
    SdfPath ret = protoPath;
    if (_map.empty()) {
        return ret;
    }

    auto it = SdfPathFindLongestPrefix(
        _map.begin(), _map.end(), ret,
        [](auto const &p) { return p.first; });

    if (it != _map.end()) {
        ret = ret.ReplacePrefix(it->first, it->second);
    }
    
    return ret;
}

UsdPrim
UsdPrim::GetChild(const TfToken &name) const
{
    return GetStage()->GetPrimAtPath(GetPath().AppendChild(name));
}

bool
UsdPrim::_IsA(const UsdSchemaRegistry::SchemaInfo *schemaInfo) const
{
    if (!schemaInfo) {
        return false;
    }

    // Check that the actual schema type of the prim (accounts for fallback
    // types for types with no schema) is or derives from the passed in type.
    return GetPrimTypeInfo().GetSchemaType().IsA(schemaInfo->type);
}

bool
UsdPrim::IsA(const TfType& schemaType) const
{
    return _IsA(UsdSchemaRegistry::FindSchemaInfo(schemaType));
}

bool
UsdPrim::IsA(const TfToken& schemaIdentifier) const
{
    return _IsA(UsdSchemaRegistry::FindSchemaInfo(schemaIdentifier));
}

bool
UsdPrim::IsA(const TfToken& schemaFamily,
             UsdSchemaVersion schemaVersion) const
{
    return _IsA(UsdSchemaRegistry::FindSchemaInfo(schemaFamily, schemaVersion));
}

// Helper implementations for wrapping UsdSchemaRegistry's 
// FindSchemaInfosInFamily with all possible inputs to the 
// IsAny/HasAnyAPI#VersionInFamily functions.
static
const std::vector<const UsdSchemaRegistry::SchemaInfo *> &
_FindSchemaInfosInFamily(const TfToken &schemaFamily)
{
    return UsdSchemaRegistry::FindSchemaInfosInFamily(schemaFamily);
}

static 
std::vector<const UsdSchemaRegistry::SchemaInfo *>
_FindSchemaInfosInFamily(
    const TfToken &schemaFamily, 
    UsdSchemaVersion schemaVersion, 
    UsdSchemaRegistry::VersionPolicy versionPolicy)
{
    return UsdSchemaRegistry::FindSchemaInfosInFamily(
        schemaFamily, schemaVersion, versionPolicy);
}

static 
std::vector<const UsdSchemaRegistry::SchemaInfo *>
_FindSchemaInfosInFamily(
    const TfType& schemaType,
    UsdSchemaRegistry::VersionPolicy versionPolicy)
{
    // Use the family and version of the type's schema to find schemas the 
    // schemas in the family.
    const UsdSchemaRegistry::SchemaInfo *schemaInfo =
        UsdSchemaRegistry::FindSchemaInfo(schemaType);
    if (!schemaInfo) {
        return {};
    }
    return _FindSchemaInfosInFamily(
        schemaInfo->family, schemaInfo->version, versionPolicy);
}

static 
std::vector<const UsdSchemaRegistry::SchemaInfo *>
_FindSchemaInfosInFamily(
    const TfToken& schemaIdentifier,
    UsdSchemaRegistry::VersionPolicy versionPolicy)
{
    // First try to use the family and version of the identifiers's schema to 
    // find schemas the schemas in the family. This will typically be faster
    // than parsing the identifier itself.
    const UsdSchemaRegistry::SchemaInfo *schemaInfo =
        UsdSchemaRegistry::FindSchemaInfo(schemaIdentifier);
    if (schemaInfo) {
        return _FindSchemaInfosInFamily(
            schemaInfo->family, schemaInfo->version, versionPolicy);
    }

    // If we didn't find a registered schema for the identifier, then we parse 
    // it into its family and version to find the schemas.
    const auto familyAndVersion = 
        UsdSchemaRegistry::ParseSchemaFamilyAndVersionFromIdentifier(
            schemaIdentifier);
    return _FindSchemaInfosInFamily(
        familyAndVersion.first, familyAndVersion.second, versionPolicy);
}

// The implementation of IsInFamily for all input combinations.
// Finds all matching schemas in a schema family using the findInFamily 
// arguments and returns the first schema info that prim.IsA would return true
// for.
template <typename... FindInFamilyArgs>
static
const UsdSchemaRegistry::SchemaInfo *
_GetFirstSchemaInFamilyPrimIsA(
    const UsdPrim &prim, const FindInFamilyArgs &... findInFamilyArgs) 
{
    const TfType &primSchemaType = prim.GetPrimTypeInfo().GetSchemaType();
    for (const auto &schemaInfo : 
            _FindSchemaInfosInFamily(findInFamilyArgs...)) {
        if (primSchemaType.IsA(schemaInfo->type)) {
            return schemaInfo;
        }
    }
    return nullptr;
}

bool 
UsdPrim::IsInFamily(const TfToken& schemaFamily) const
{
    return _GetFirstSchemaInFamilyPrimIsA(*this, schemaFamily);
}

bool
UsdPrim::IsInFamily(
    const TfToken& schemaFamily,
    UsdSchemaVersion schemaVersion,
    UsdSchemaRegistry::VersionPolicy versionPolicy) const
{
    return _GetFirstSchemaInFamilyPrimIsA(
        *this, schemaFamily, schemaVersion, versionPolicy);
}

bool
UsdPrim::IsInFamily(
    const TfType& schemaType, 
    UsdSchemaRegistry::VersionPolicy versionPolicy) const
{
    return _GetFirstSchemaInFamilyPrimIsA(
        *this, schemaType, versionPolicy);
}

bool
UsdPrim::IsInFamily(
    const TfToken& schemaIdentifier,
    UsdSchemaRegistry::VersionPolicy versionPolicy) const
{
    return _GetFirstSchemaInFamilyPrimIsA(
        *this, schemaIdentifier, versionPolicy);
}             

bool 
UsdPrim::GetVersionIfIsInFamily(
    const TfToken& schemaFamily,
    UsdSchemaVersion *schemaVersion) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            _GetFirstSchemaInFamilyPrimIsA(*this, schemaFamily)) {
        *schemaVersion = schemaInfo->version;
        return true;
    }
    return false;
}

static bool
_IsSchemaInAppliedSchemas(
    const TfTokenVector &appliedSchemas,
    const UsdSchemaRegistry::SchemaInfo &schemaInfo)
{
    // If the schema is a multiple apply schema, we're looking for any instance
    // of the schema in the list. So we look for an applied schema that starts
    // with schema's identifier.
    if (schemaInfo.kind == UsdSchemaKind::MultipleApplyAPI) {
        return std::any_of(appliedSchemas.begin(), appliedSchemas.end(),
            [&schemaInfo](const TfToken &appliedSchema) {
                // The multiple apply schema name must always be followed by
                // the namespace delimiter ':' so check for that first.
                static const char delim = UsdObject::GetNamespaceDelimiter();
                const size_t prefixLen = schemaInfo.identifier.size();
                if (appliedSchema.size() <= prefixLen ||
                    appliedSchema.GetString()[prefixLen] != delim) {
                        return false;
                }
                return TfStringStartsWith(appliedSchema, schemaInfo.identifier);
            });
    }

    // For a single apply API we're just looking for schema identifier being
    // in the list.
    if (schemaInfo.kind == UsdSchemaKind::SingleApplyAPI) {
        return std::find(appliedSchemas.begin(), appliedSchemas.end(), 
            schemaInfo.identifier) != appliedSchemas.end();
    }

    // Not an applied API schema.
    return false;
}

static bool
_IsSchemaInstanceInAppliedSchemas(
    const TfTokenVector &appliedSchemas,
    const UsdSchemaRegistry::SchemaInfo &schemaInfo,
    const TfToken &instanceName)
{
    // Only multiple apply schemas are applied with instance names.
    if (schemaInfo.kind != UsdSchemaKind::MultipleApplyAPI) {
        return false;
    }

    // We're looking for exact match of "<schemaIdentifier>:<instanceName>"
    const TfToken apiSchemaName(
        SdfPath::JoinIdentifier(schemaInfo.identifier, instanceName));
    return std::find(appliedSchemas.begin(), appliedSchemas.end(), 
        apiSchemaName) != appliedSchemas.end();
}

bool 
UsdPrim::_HasAPI(const UsdSchemaRegistry::SchemaInfo *schemaInfo) const
{
    if (!schemaInfo) {
        return false;
    }

    const TfTokenVector appliedSchemas = GetAppliedSchemas();
    if (appliedSchemas.empty()) {
        return false;
    }

    return _IsSchemaInAppliedSchemas(appliedSchemas, *schemaInfo);
}

bool 
UsdPrim::_HasAPIInstance(
    const UsdSchemaRegistry::SchemaInfo *schemaInfo,
    const TfToken &instanceName) const
{
    if (instanceName.IsEmpty()) {
        TF_CODING_ERROR("Instance name must be non-empty");
        return false;
    }

    if (!schemaInfo) {
        return false;
    }

    const TfTokenVector appliedSchemas = GetAppliedSchemas();
    if (appliedSchemas.empty()) {
        return false;
    }

    return _IsSchemaInstanceInAppliedSchemas(
        appliedSchemas, *schemaInfo, instanceName);
}

bool
UsdPrim::HasAPI(const TfType& schemaType) const
{
    return _HasAPI(UsdSchemaRegistry::FindSchemaInfo(schemaType));
}

bool
UsdPrim::HasAPI(const TfType& schemaType, const TfToken& instanceName) const
{
    return _HasAPIInstance(
        UsdSchemaRegistry::FindSchemaInfo(schemaType), instanceName);
}

bool
UsdPrim::HasAPI(const TfToken& schemaIdentifier) const
{
    return _HasAPI(UsdSchemaRegistry::FindSchemaInfo(schemaIdentifier));
}

bool
UsdPrim::HasAPI(const TfToken& schemaIdentifier, 
                const TfToken& instanceName) const
{
    return _HasAPIInstance(
        UsdSchemaRegistry::FindSchemaInfo(schemaIdentifier), instanceName);
}

bool
UsdPrim::HasAPI(const TfToken& schemaFamily,
                UsdSchemaVersion schemaVersion) const
{
    return _HasAPI(
        UsdSchemaRegistry::FindSchemaInfo(schemaFamily, schemaVersion));
}

bool
UsdPrim::HasAPI(const TfToken& schemaFamily,
                UsdSchemaVersion schemaVersion, 
                const TfToken& instanceName) const
{
    return _HasAPIInstance(
        UsdSchemaRegistry::FindSchemaInfo(schemaFamily, schemaVersion),
        instanceName);
}

// The implementation of HasAPIInFamily (without an instance name) for
// all input combinations. Finds all matching schemas in a schema family using
// the findInFamily arguments and returns the first schema info that prim.HasAPI
// would return true for.
template <typename... FindInFamilyArgs>
static
const UsdSchemaRegistry::SchemaInfo *
_GetFirstSchemaInFamilyPrimHasAPI(
    const UsdPrim &prim, const FindInFamilyArgs &... findInFamilyArgs) 
{
    const TfTokenVector appliedSchemas = prim.GetAppliedSchemas();
    if (appliedSchemas.empty()) {
        return nullptr;
    }

    for (const auto &schemaInfo : 
            _FindSchemaInfosInFamily(findInFamilyArgs...)) {
        if (_IsSchemaInAppliedSchemas(appliedSchemas, *schemaInfo)) {
            return schemaInfo;
        }
    }
    return nullptr;
}

// The implementation of HasAPIInFamily (with an instance name) for
// all input combinations. Finds all matching schemas in a schema family using
// the findInFamily arguments and returns the first schema info that 
// prim.HasAPI(instanceName) would return true for.
template <typename... FindInFamilyArgs>
static
const UsdSchemaRegistry::SchemaInfo *
_GetFirstSchemaInFamilyPrimHasAPIInstance(
    const UsdPrim &prim, 
    const TfToken& instanceName, 
    const FindInFamilyArgs &... findInFamilyArgs) 
{
    if (instanceName.IsEmpty()) {
        TF_CODING_ERROR("Instance name must be non-empty");
        return nullptr;
    }

    const TfTokenVector appliedSchemas = prim.GetAppliedSchemas();
    if (appliedSchemas.empty()) {
        return nullptr;
    }

    for (const auto &schemaInfo : 
            _FindSchemaInfosInFamily(findInFamilyArgs...)) {
        if (_IsSchemaInstanceInAppliedSchemas(
                appliedSchemas, *schemaInfo, instanceName)) {
            return schemaInfo;
        }
    }
    return nullptr;
}

bool 
UsdPrim::HasAPIInFamily(const TfToken& schemaFamily) const
{
    return _GetFirstSchemaInFamilyPrimHasAPI(*this, schemaFamily);
}

USD_API
bool 
UsdPrim::HasAPIInFamily(
    const TfToken& schemaFamily, const TfToken& instanceName) const
{
    return _GetFirstSchemaInFamilyPrimHasAPIInstance(
        *this, instanceName, schemaFamily);
}

bool 
UsdPrim::HasAPIInFamily(
    const TfToken& schemaFamily,
    UsdSchemaVersion schemaVersion,
    UsdSchemaRegistry::VersionPolicy versionPolicy) const
{
    return _GetFirstSchemaInFamilyPrimHasAPI(
        *this, schemaFamily, schemaVersion, versionPolicy);
}

USD_API
bool 
UsdPrim::HasAPIInFamily(
    const TfToken& schemaFamily,
    UsdSchemaVersion schemaVersion,
    UsdSchemaRegistry::VersionPolicy versionPolicy,
    const TfToken& instanceName) const
{
    return _GetFirstSchemaInFamilyPrimHasAPIInstance(
        *this, instanceName, schemaFamily, schemaVersion, versionPolicy);
}

bool
UsdPrim::HasAPIInFamily(
    const TfType& schemaType,
    UsdSchemaRegistry::VersionPolicy versionPolicy) const
{
    return _GetFirstSchemaInFamilyPrimHasAPI(*this, schemaType, versionPolicy);
}

bool 
UsdPrim::HasAPIInFamily(
    const TfType& schemaType,
    UsdSchemaRegistry::VersionPolicy versionPolicy,
    const TfToken& instanceName) const
{
    return _GetFirstSchemaInFamilyPrimHasAPIInstance(
        *this, instanceName, schemaType, versionPolicy);
}

bool 
UsdPrim::HasAPIInFamily(
    const TfToken& schemaIdentifier,
    UsdSchemaRegistry::VersionPolicy versionPolicy) const
{
    return _GetFirstSchemaInFamilyPrimHasAPI(
        *this, schemaIdentifier, versionPolicy);
}

bool 
UsdPrim::HasAPIInFamily(
    const TfToken& schemaIdentifier,
    UsdSchemaRegistry::VersionPolicy versionPolicy,
    const TfToken& instanceName) const
{
    return _GetFirstSchemaInFamilyPrimHasAPIInstance(
        *this, instanceName, schemaIdentifier, versionPolicy);
}

bool
UsdPrim::GetVersionIfHasAPIInFamily(
    const TfToken &schemaFamily,
    UsdSchemaVersion *schemaVersion) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo = 
            _GetFirstSchemaInFamilyPrimHasAPI(*this, schemaFamily)) {
        *schemaVersion = schemaInfo->version;
        return true;
    }
    return false;
}

bool
UsdPrim::GetVersionIfHasAPIInFamily(
    const TfToken &schemaFamily,
    const TfToken &instanceName,
    UsdSchemaVersion *schemaVersion) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo = 
            _GetFirstSchemaInFamilyPrimHasAPIInstance(
                *this, instanceName, schemaFamily)) {
        *schemaVersion = schemaInfo->version;
        return true;
    }
    return false;
}                             

// Helpers for formatting and reporting error messages for each of the 
// supported schema function input types when they do not produce a valid schema.
static
void _ReportInvalidSchemaError(
    const char *funcName, 
    const TfType &schemaType,
    std::string *reason = nullptr) 
{
    std::string errorMsg = TfStringPrintf(
        "Cannot find a valid schema for the provided schema type '%s'", 
        schemaType.GetTypeName().c_str());
    TF_CODING_ERROR("%s: %s", funcName, errorMsg.c_str());
    if (reason) {
        *reason = std::move(errorMsg);
    }
}

static 
void _ReportInvalidSchemaError(
    const char *funcName, 
    const TfToken &schemaIdentifier,
    std::string *reason = nullptr) 
{
    std::string errorMsg = TfStringPrintf(
        "Cannot find a valid schema for the provided schema identifier '%s'", 
        schemaIdentifier.GetText());
    TF_CODING_ERROR("%s: %s", funcName, errorMsg.c_str());
    if (reason) {
        *reason = std::move(errorMsg);
    }
}

static 
void _ReportInvalidSchemaError(
    const char *funcName,
    const TfToken &schemaFamily,
    UsdSchemaVersion schemaVersion,
    std::string *reason = nullptr) 
{
    std::string errorMsg =  TfStringPrintf(
        "Cannot find a valid schema for the provided schema family '%s' and "
        "version '%u", schemaFamily.GetText(), schemaVersion);
    TF_CODING_ERROR("%s: %s", funcName, errorMsg.c_str());
    if (reason) {
        *reason = std::move(errorMsg);
    }
}

// Helpers for validating the expected schema kind for a schema and 
// reporting errors for schemas that are not the expected kind.
static bool 
_ValidateIsSingleApplyAPI(
    const char *funcName, 
    const UsdSchemaRegistry::SchemaInfo &schemaInfo,
    std::string *reason = nullptr) 
{
    if (schemaInfo.kind != UsdSchemaKind::SingleApplyAPI) {
        std::string errorMessage = TfStringPrintf(
            "Provided schema type %s is not a single-apply API schema.", 
            schemaInfo.type.GetTypeName().c_str());
        TF_CODING_ERROR("%s: %s", funcName, errorMessage.c_str());
        if (reason) {
            *reason = std::move(errorMessage);
        }
        return false;
    }
    return true;
}

static bool 
_ValidateIsMultipleApplyAPI(
    const char *funcName, 
    const UsdSchemaRegistry::SchemaInfo &schemaInfo,
    std::string *reason = nullptr) 
{
    if (schemaInfo.kind != UsdSchemaKind::MultipleApplyAPI) {
        std::string errorMessage = TfStringPrintf(
            "Provided schema type %s is not a multiple-apply API schema.", 
            schemaInfo.type.GetTypeName().c_str());
        TF_CODING_ERROR("%s: %s", funcName, errorMessage.c_str());
        if (reason) {
            *reason = std::move(errorMessage);
        }
        return false;
    }
    return true;
}

// Determines whether the given prim type can have the given API schema applied 
// to it based on a list of types the API "can only be applied to". If the 
// list is empty, this always returns true, otherwise the prim type must be in 
// the list or derived from a type in the list.
static bool 
_IsPrimTypeValidApplyToTarget(const TfType &primType, 
                              const TfToken &apiSchemaTypeName,
                              const TfToken &instanceName,
                              std::string *whyNot)
{
    // Get the list of prim types this API "can only apply to" if any.
    const TfTokenVector &canOnlyApplyToTypes =
        UsdSchemaRegistry::GetAPISchemaCanOnlyApplyToTypeNames(
            apiSchemaTypeName, instanceName);

    // If no "can only apply to" types are found, the schema can be 
    // applied to any prim type (including empty or invalid prims types)
    if (canOnlyApplyToTypes.empty()) {
        return true;
    }

    // If the prim type or any of its ancestor types are in the list, then it's
    // valid!
    if (!primType.IsUnknown()) {
        for (const TfToken &allowedPrimTypeName : canOnlyApplyToTypes) {
            const TfType allowedPrimType = 
                UsdSchemaRegistry::GetTypeFromSchemaTypeName(
                    allowedPrimTypeName);
            if (primType.IsA(allowedPrimType)) {
                return true;
            }
        }
    }

    // Otherwise, it wasn't in the list and can't be applied to.
    if (whyNot) {
        *whyNot = TfStringPrintf(
            "API schema '%s' can only be applied to prims of the following "
            "types: %s.", 
            SdfPath::JoinIdentifier(apiSchemaTypeName, instanceName).c_str(), 
            TfStringJoin(canOnlyApplyToTypes.begin(),
                         canOnlyApplyToTypes.end(), ", ").c_str());
    }
    return false;
}

bool
UsdPrim::_CanApplySingleApplyAPI(
    const UsdSchemaRegistry::SchemaInfo &schemaInfo,
    std::string *whyNot) const
{
    if (!_ValidateIsSingleApplyAPI("CanApplyAPI", schemaInfo, whyNot)) {
        return false;
    }

    // Can't apply API schemas to an invalid prim
    if (!IsValid()) {
        if (whyNot) {
            *whyNot = "Prim is not valid.";
        }
        return false;
    }

    // Return whether this prim's type is a valid target for applying the given
    // API schema.
    return _IsPrimTypeValidApplyToTarget(
        GetPrimTypeInfo().GetSchemaType(), 
        schemaInfo.identifier,
        /*instanceName=*/ TfToken(),
        whyNot);
}

bool 
UsdPrim::_CanApplyMultipleApplyAPI(
    const UsdSchemaRegistry::SchemaInfo &schemaInfo,
    const TfToken& instanceName, 
    std::string *whyNot) const
{
    if (!_ValidateIsMultipleApplyAPI("CanApplyAPI", schemaInfo, whyNot)) {
        return false;
    }

    // All API schema functions treat an empty instance for a multiple apply 
    // schema as a coding error.
    if (instanceName.IsEmpty()) {
        TF_CODING_ERROR("CanApplyAPI: for multiple apply API schema %s, a "
                        "non-empty instance name must be provided.",
            schemaInfo.identifier.GetText());
        return false;
    }

    // Can't apply API schemas to an invalid prim
    if (!IsValid()) {
        if (whyNot) {
            *whyNot = "Prim is not valid.";
        }
        return false;
    }

    // Multiple apply API schemas may have limitations on what instance names
    // are allowed to be used. Check if the requested instance name is valid.
    if (!UsdSchemaRegistry::IsAllowedAPISchemaInstanceName(
            schemaInfo.identifier, instanceName)) {
        if (whyNot) {
            *whyNot = TfStringPrintf(
                "'%s' is not an allowed instance name for multiple apply API "
                "schema '%s'.", 
                instanceName.GetText(), schemaInfo.identifier.GetText());
        }
        return false;
    }

    // Return whether this prim's type is a valid target for applying the given
    // API schema and instance name.
    return _IsPrimTypeValidApplyToTarget(
        GetPrimTypeInfo().GetSchemaType(), 
        schemaInfo.identifier,
        instanceName,
        whyNot);
}

bool 
UsdPrim::CanApplyAPI(const TfType& schemaType, 
                     std::string *whyNot) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaType)) {
        return _CanApplySingleApplyAPI(*schemaInfo, whyNot);
    } 
    
    _ReportInvalidSchemaError("CanApplyAPI", schemaType, whyNot);
    return false;
}

bool 
UsdPrim::CanApplyAPI(const TfType& schemaType, 
                     const TfToken& instanceName,
                     std::string *whyNot) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaType)) {
        return _CanApplyMultipleApplyAPI(*schemaInfo, instanceName, whyNot);
    }

    _ReportInvalidSchemaError("CanApplyAPI", schemaType, whyNot);
    return false;
}

bool
UsdPrim::CanApplyAPI(const TfToken& schemaIdentifier,
                     std::string *whyNot) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaIdentifier)) {
        return _CanApplySingleApplyAPI(*schemaInfo, whyNot);
    }

    _ReportInvalidSchemaError("CanApplyAPI", schemaIdentifier, whyNot);
    return false;
}                     

bool
UsdPrim::CanApplyAPI(const TfToken& schemaIdentifier,
                     const TfToken& instanceName,
                     std::string *whyNot) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaIdentifier)) {
        return _CanApplyMultipleApplyAPI(*schemaInfo, instanceName, whyNot);
    }

    _ReportInvalidSchemaError("CanApplyAPI", schemaIdentifier, whyNot);
    return false;
}                     

bool 
UsdPrim::CanApplyAPI(const TfToken& schemaFamily,
                     UsdSchemaVersion schemaVersion,
                     std::string *whyNot) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaFamily, schemaVersion)) {
        return _CanApplySingleApplyAPI(*schemaInfo, whyNot);
    }

    _ReportInvalidSchemaError("CanApplyAPI", schemaFamily, schemaVersion, whyNot);
    return false;
}                     

bool 
UsdPrim::CanApplyAPI(const TfToken& schemaFamily,
                     UsdSchemaVersion schemaVersion,
                     const TfToken& instanceName,
                     std::string *whyNot) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaFamily, schemaVersion)) {
        return _CanApplyMultipleApplyAPI(*schemaInfo, instanceName, whyNot);
    }

    _ReportInvalidSchemaError("CanApplyAPI", schemaFamily, schemaVersion, whyNot);
    return false;
}                     

bool 
UsdPrim::_ApplySingleApplyAPI(
    const UsdSchemaRegistry::SchemaInfo &schemaInfo) const
{
    if (!_ValidateIsSingleApplyAPI("ApplyAPI", schemaInfo)) {
        return false;
    }

    // Validate the prim to protect against crashes in the schema generated 
    // SchemaClass::Apply(const UsdPrim &prim) functions when called with a null
    // prim as these generated functions call prim.ApplyAPI<SchemaClass>
    //
    // While we don't typically validate "this" prim for public UsdPrim C++ API,
    // for performance reasons, we opt to do so here since ApplyAPI isn't 
    // performance critical. If ApplyAPI becomes performance critical in the
    // future, we may have to move this validation elsewhere if this validation
    // is problematic.
    if (!IsValid()) {
        TF_CODING_ERROR("Invalid prim '%s'", GetDescription().c_str());
        return false;
    }

    return AddAppliedSchema(schemaInfo.identifier);
}

bool 
UsdPrim::_ApplyMultipleApplyAPI(
    const UsdSchemaRegistry::SchemaInfo &schemaInfo,
    const TfToken &instanceName) const
{
    if (!_ValidateIsMultipleApplyAPI("ApplyAPI", schemaInfo)) {
        return false;
    }

    // All API schema functions treat an empty instance for a multiple apply 
    // schema as a coding error.
    if (instanceName.IsEmpty()) {
        TF_CODING_ERROR("ApplyAPI: for mutiple apply API schema %s, a "
                        "non-empty instance name must be provided.",
            schemaInfo.identifier.GetText());
        return false;
    }

    // Validate the prim to protect against crashes in the schema generated 
    // SchemaClass::Apply(const UsdPrim &prim) functions when called with a null
    // prim as these generated functions call prim.ApplyAPI<SchemaClass>
    //
    // While we don't typically validate "this" prim for public UsdPrim C++ API,
    // for performance reasons, we opt to do so here since ApplyAPI isn't 
    // performance critical. If ApplyAPI becomes performance critical in the
    // future, we may have to move this validation elsewhere if this validation
    // is problematic.
    if (!IsValid()) {
        TF_CODING_ERROR("Invalid prim '%s'", GetDescription().c_str());
        return false;
    }

    const TfToken apiName(
        SdfPath::JoinIdentifier(schemaInfo.identifier, instanceName));
    return AddAppliedSchema(apiName);
}

bool
UsdPrim::ApplyAPI(const TfType& schemaType) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaType)) {
        return _ApplySingleApplyAPI(*schemaInfo);
    }

    _ReportInvalidSchemaError("ApplyAPI", schemaType);
    return false;
}

bool
UsdPrim::ApplyAPI(const TfType& schemaType, 
                  const TfToken& instanceName) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaType)) {
        return _ApplyMultipleApplyAPI(*schemaInfo, instanceName);
    }

    _ReportInvalidSchemaError("ApplyAPI", schemaType);
    return false;
}

bool 
UsdPrim::ApplyAPI(const TfToken& schemaIdentifier) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaIdentifier)) {
        return _ApplySingleApplyAPI(*schemaInfo);
    }

    _ReportInvalidSchemaError("ApplyAPI", schemaIdentifier);
    return false;
}

bool 
UsdPrim::ApplyAPI(const TfToken& schemaIdentifier, 
                  const TfToken& instanceName) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaIdentifier)) {
        return _ApplyMultipleApplyAPI(*schemaInfo, instanceName);
    }

    _ReportInvalidSchemaError("ApplyAPI", schemaIdentifier);
    return false;
}

bool 
UsdPrim::ApplyAPI(const TfToken& schemaFamily, 
                  UsdSchemaVersion schemaVersion) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaFamily, schemaVersion)) {
        return _ApplySingleApplyAPI(*schemaInfo);
    }

    _ReportInvalidSchemaError("ApplyAPI", schemaFamily, schemaVersion);
    return false;
}

bool 
UsdPrim::ApplyAPI(const TfToken& schemaFamily, 
                  UsdSchemaVersion schemaVersion, 
                  const TfToken& instanceName) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaFamily, schemaVersion)) {
        return _ApplyMultipleApplyAPI(*schemaInfo, instanceName);
    }

    _ReportInvalidSchemaError("ApplyAPI", schemaFamily, schemaVersion);
    return false;
}

bool 
UsdPrim::_RemoveSingleApplyAPI(
    const UsdSchemaRegistry::SchemaInfo &schemaInfo) const
{
    if (!_ValidateIsSingleApplyAPI("RemoveAPI", schemaInfo)) {
        return false;
    }

    return RemoveAppliedSchema(schemaInfo.identifier);
}

bool 
UsdPrim::_RemoveMultipleApplyAPI(
    const UsdSchemaRegistry::SchemaInfo &schemaInfo,
    const TfToken &instanceName) const
{
    if (!_ValidateIsMultipleApplyAPI("RemoveAPI", schemaInfo)) {
        return false;
    }

    // All API schema functions treat an empty instance for a multiple apply 
    // schema as a coding error.
    if (instanceName.IsEmpty()) {
        TF_CODING_ERROR("RemoveAPI: for mutiple apply API schema %s, a "
                        "non-empty instance name must be provided.",
            schemaInfo.identifier.GetText());
        return false;
    }

    const TfToken apiName(
        SdfPath::JoinIdentifier(schemaInfo.identifier, instanceName));
    return RemoveAppliedSchema(apiName);
}

bool
UsdPrim::RemoveAPI(const TfType& schemaType) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaType)) {
        return _RemoveSingleApplyAPI(*schemaInfo);
    }

    _ReportInvalidSchemaError("RemoveAPI", schemaType);
    return false;
}

bool
UsdPrim::RemoveAPI(const TfType& schemaType, 
                   const TfToken& instanceName) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaType)) {
        return _RemoveMultipleApplyAPI(*schemaInfo, instanceName);
    }

    _ReportInvalidSchemaError("RemoveAPI", schemaType);
    return false;
}

bool
UsdPrim::RemoveAPI(const TfToken& schemaIdentifier) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaIdentifier)) {
        return _RemoveSingleApplyAPI(*schemaInfo);
    }

    _ReportInvalidSchemaError("RemoveAPI", schemaIdentifier);
    return false;
}

bool
UsdPrim::RemoveAPI(const TfToken& schemaIdentifier, 
                   const TfToken& instanceName) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaIdentifier)) {
        return _RemoveMultipleApplyAPI(*schemaInfo, instanceName);
    }

    _ReportInvalidSchemaError("RemoveAPI", schemaIdentifier);
    return false;
}

bool
UsdPrim::RemoveAPI(const TfToken& schemaFamily, 
                   UsdSchemaVersion schemaVersion) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaFamily, schemaVersion)) {
        return _RemoveSingleApplyAPI(*schemaInfo);
    }

    _ReportInvalidSchemaError("RemoveAPI", schemaFamily, schemaVersion);
    return false;
}

bool
UsdPrim::RemoveAPI(const TfToken& schemaFamily, 
                   UsdSchemaVersion schemaVersion, 
                   const TfToken& instanceName) const
{
    if (const UsdSchemaRegistry::SchemaInfo *schemaInfo =
            UsdSchemaRegistry::FindSchemaInfo(schemaFamily, schemaVersion)) {
        return _RemoveMultipleApplyAPI(*schemaInfo, instanceName);
    }

    _ReportInvalidSchemaError("RemoveAPI", schemaFamily, schemaVersion);
    return false;
}

bool 
UsdPrim::AddAppliedSchema(const TfToken &appliedSchemaName) const
{
    // This should find or create the primSpec in the current edit target.
    // It will also issue an error if it's unable to.
    SdfPrimSpecHandle primSpec = _GetStage()->_CreatePrimSpecForEditing(*this);

    // _CreatePrimSpecForEditing would have already issued a runtime error
    // in case of a failure.
    if (!primSpec) {
        TF_WARN("Unable to create primSpec at path <%s> in edit target '%s'. "
                "Failed to add applied API schema.",
            GetPath().GetText(),
            _GetStage()->GetEditTarget().GetLayer()->GetIdentifier().c_str());
        return false;
    }

    auto _HasItem = [](const TfTokenVector &items, const TfToken &item) {
        return std::find(items.begin(), items.end(), item) != items.end();
    };

    SdfTokenListOp listOp =
        primSpec->GetInfo(UsdTokens->apiSchemas).Get<SdfTokenListOp>();

    if (listOp.IsExplicit()) {
        // If the list op is explicit we check if the explicit item to see if
        // our name is already in it. We'll add it to the end of the explicit
        // list if it is not.
        const TfTokenVector &items = listOp.GetExplicitItems();
        if (_HasItem(items, appliedSchemaName)) {
            return true;
        }
        // Use ReplaceOperations to append in place.
        if (!listOp.ReplaceOperations(SdfListOpTypeExplicit, 
                items.size(), 0, {appliedSchemaName})) {
            return false;
        }
    } else {
        // Otherwise our name could be in the append or prepend list (we 
        // purposefully ignore the "add" list which is deprecated) so we check 
        // both before adding it to the end of prepends.
        const TfTokenVector &preItems = listOp.GetPrependedItems();
        const TfTokenVector &appItems = listOp.GetAppendedItems();
        if (_HasItem(preItems, appliedSchemaName) || 
            _HasItem(appItems, appliedSchemaName)) {
            return true;
        }
        // Use ReplaceOperations to append in place.
        if (!listOp.ReplaceOperations(SdfListOpTypePrepended, 
                preItems.size(), 0, {appliedSchemaName})) {
            return false;
        }
    }

    // If we got here, we edited the list op, so author it back to the spec.
    primSpec->SetInfo(UsdTokens->apiSchemas, VtValue::Take(listOp));
    return true;
}

bool 
UsdPrim::RemoveAppliedSchema(const TfToken &appliedSchemaName) const
{
    // This should create the primSpec in the current edit target.
    // It will also issue an error if it's unable to.
    SdfPrimSpecHandle primSpec = _GetStage()->_CreatePrimSpecForEditing(*this);

    // _CreatePrimSpecForEditing would have already issued a runtime error
    // in case of a failure.
    if (!primSpec) {
        TF_WARN("Unable to create primSpec at path <%s> in edit target '%s'. "
                "Failed to remove applied API schema.",
            GetPath().GetText(),
            _GetStage()->GetEditTarget().GetLayer()->GetIdentifier().c_str());
        return false;
    }

    SdfTokenListOp listOp =
        primSpec->GetInfo(UsdTokens->apiSchemas).Get<SdfTokenListOp>();

    // Create a list op that deletes our schema name and apply it to the current
    // apiSchemas list op for the edit prim spec. This will take care of making
    // sure it ends up in the deletes list (for non-explicit list ops) and is
    // removed from any other items list that would add it back.
    SdfTokenListOp editListOp;
    editListOp.SetDeletedItems({appliedSchemaName});
    if (auto result = editListOp.ApplyOperations(listOp)) {
        primSpec->SetInfo(UsdTokens->apiSchemas, VtValue(*result));
        return true;
    } else {
        TF_CODING_ERROR("Failed to apply list op edits to 'apiSchemas' on spec "
                        "at path <%s> in layer '%s'", 
                        primSpec->GetLayer()->GetIdentifier().c_str(), 
                        primSpec->GetPath().GetText());
        return false;
    }
}

std::vector<UsdProperty>
UsdPrim::_MakeProperties(const TfTokenVector &names) const
{
    std::vector<UsdProperty> props;
    UsdStage *stage = _GetStage();
    props.reserve(names.size());
    for (auto const &propName : names) {
        SdfSpecType specType =
            stage->_GetDefiningSpecType(get_pointer(_Prim()), propName);
        if (specType == SdfSpecTypeAttribute) {
            props.push_back(GetAttribute(propName));
        } else if (TF_VERIFY(specType == SdfSpecTypeRelationship)) {
            props.push_back(GetRelationship(propName));
        }
    }
    return props;
}

// Change the order of items in 'names' so that all the things in 'order' that
// are also in 'names' are at the beginning in the order that they appear in
// 'order', followed by any remaining items in 'names' in their existing order.
static void
_ApplyOrdering(const TfTokenVector &order, TfTokenVector *names)
{
    // If order is empty or names is empty, nothing to do.
    if (order.empty() || names->empty())
        return;

    // Perf note: this walks 'order' and linear searches 'names' to find each
    // element, for O(M*N) operations, where M and N are the lengths of 'order'
    // and 'names'.  We hope 1) that propertyOrder stmts are relatively rare and
    // 2) that property lists are relatively short.  If those assumptions fail,
    // this may need revisiting.  In some quick microbenchmarking, this linear
    // search seems to outperform binary search up to about 5000 names.  We
    // suspect this is because linear search does TfToken pointer comparisons,
    // while binary search has to dereference and do string comparisons.

    typedef TfTokenVector::iterator Iter;

    Iter namesRest = names->begin(), namesEnd = names->end();
    for (const TfToken &oName: order) {
        // Look for this name from 'order' in the rest of 'names'.
        Iter i = std::find(namesRest, namesEnd, oName);
        if (i != namesEnd) {
            // Found.  Move to the front by rotating the sub-range.  Using
            // std::rotate invokes swap(), which avoids TfToken refcounting.
            // Also advance 'namesRest' to the next element.
            std::rotate(namesRest++, i, i+1);
        }
    }
}

bool 
UsdPrim::RemoveProperty(const TfToken &propName) 
{
    SdfPath propPath = GetPath().AppendProperty(propName);
    return _GetStage()->_RemoveProperty(propPath);
}

UsdProperty
UsdPrim::GetProperty(const TfToken &propName) const
{
    SdfSpecType specType =
        _GetStage()->_GetDefiningSpecType(get_pointer(_Prim()), propName);
    if (specType == SdfSpecTypeAttribute) {
        return GetAttribute(propName);
    }
    else if (specType == SdfSpecTypeRelationship) {
        return GetRelationship(propName);
    }
    return UsdProperty(UsdTypeProperty, _Prim(), _ProxyPrimPath(), propName);
}

bool
UsdPrim::HasProperty(const TfToken &propName) const 
{
    return static_cast<bool>(GetProperty(propName));
}

bool
UsdPrim::GetKind(TfToken *kind) const
{
    if (IsPseudoRoot()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    return GetMetadata(SdfFieldKeys->Kind, kind);
}

bool
UsdPrim::SetKind(const TfToken &kind) const
{
    if (IsPseudoRoot()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    return SetMetadata(SdfFieldKeys->Kind, kind);
}

TfTokenVector
UsdPrim::GetPropertyOrder() const
{
    TfTokenVector order;
    GetMetadata(SdfFieldKeys->PropertyOrder, &order);
    return order;
}

using TokenRobinSet = pxr_tsl::robin_set<TfToken, TfToken::HashFunctor>;

// This function was copied from Pcp/{PrimIndex,ComposeSite} and optimized for
// Usd.
static void
_ComposePrimPropertyNames( 
    const PcpPrimIndex& primIndex,
    const UsdPrim::PropertyPredicateFunc &predicate,
    TokenRobinSet *inOutNames)
{
    
    auto const &nodeRange = primIndex.GetNodeRange();
    const bool hasPredicate = static_cast<bool>(predicate);

    for (PcpNodeRef node: nodeRange) {
        if (node.IsCulled() || !node.CanContributeSpecs()) {
            continue;
        }
        for (auto const &layer : node.GetLayerStack()->GetLayers()) {
            VtValue namesVal;
            if (layer->HasField(node.GetPath(),
                                SdfChildrenKeys->PropertyChildren,
                                &namesVal) &&
                namesVal.IsHolding<TfTokenVector>()) {
                // If we have a predicate, then check to see if the name is
                // already included to avoid redundantly invoking it.  The most
                // common case for us is repeated names already present.
                TfTokenVector
                    localNames = namesVal.UncheckedRemove<TfTokenVector>();
                if (hasPredicate) {
                    for (auto &name: localNames) {
                        if (!inOutNames->count(name) && predicate(name)) {
                            inOutNames->insert(std::move(name));
                        }
                    }
                } else {
                    inOutNames->insert(
                        std::make_move_iterator(localNames.begin()),
                        std::make_move_iterator(localNames.end()));
                }
            }
        }
    }
}

TfTokenVector
UsdPrim::_GetPropertyNames(
    bool onlyAuthored, 
    bool applyOrder,
    const UsdPrim::PropertyPredicateFunc &predicate) const
{
    TRACE_FUNCTION();
    
    TokenRobinSet names;
    TfTokenVector namesVec;

    // If we're including unauthored properties, take names from definition, if
    // present.
    const UsdPrimDefinition &primDef = _Prim()->GetPrimDefinition();
    if (!onlyAuthored) {   
        const TfTokenVector &builtInNames = primDef.GetPropertyNames();
        for (const auto &builtInName : builtInNames) {
            if (!predicate || predicate(builtInName)) {
                names.insert(builtInName);
            }
        }
    }

    // Add authored names, then sort and apply ordering.
    _ComposePrimPropertyNames(GetPrimIndex(), predicate, &names);

    if (!names.empty()) {
        // Sort and uniquify the names.
        namesVec.resize(names.size());
        std::copy(names.begin(), names.end(), namesVec.begin());
        sort(namesVec.begin(), namesVec.end(), TfDictionaryLessThan());
        if (applyOrder) {
            _ApplyOrdering(GetPropertyOrder(), &namesVec);
        }
    }

    return namesVec;
}

TfTokenVector
UsdPrim::GetAppliedSchemas() const
{
    return GetPrimDefinition().GetAppliedAPISchemas();
}

TfTokenVector
UsdPrim::GetPropertyNames(
    const UsdPrim::PropertyPredicateFunc &predicate) const
{
    return _GetPropertyNames(/*onlyAuthored=*/ false, 
                             /*applyOrder*/ true, 
                             predicate);
}

TfTokenVector
UsdPrim::GetAuthoredPropertyNames(
    const UsdPrim::PropertyPredicateFunc &predicate) const
{
    return _GetPropertyNames(/*onlyAuthored=*/ true, 
                             /*applyOrder*/ true, 
                             predicate);
}

std::vector<UsdProperty>
UsdPrim::GetProperties(const UsdPrim::PropertyPredicateFunc &predicate) const
{
    return _MakeProperties(GetPropertyNames(predicate));
}

std::vector<UsdProperty>
UsdPrim::GetAuthoredProperties(
    const UsdPrim::PropertyPredicateFunc &predicate) const
{
    return _MakeProperties(GetAuthoredPropertyNames(predicate));
}

std::vector<UsdProperty>
UsdPrim::_GetPropertiesInNamespace(
    const std::string &namespaces,
    bool onlyAuthored) const
{
    if (namespaces.empty())
        return onlyAuthored ? GetAuthoredProperties() : GetProperties();

    const char delim = UsdObject::GetNamespaceDelimiter();

    // Set terminator to the expected position of the delimiter after all the
    // supplied namespaces.  We perform an explicit test for this char below
    // so that we don't need to allocate a new string if namespaces does not
    // already end with the delimiter
    const size_t terminator = namespaces.size() -
        (*namespaces.rbegin() == delim);

    TfTokenVector names = _GetPropertyNames(onlyAuthored, 
        /*applyOrder=*/ true,
        /*predicate*/ [&namespaces, terminator, delim](const TfToken &name) {
            const std::string &s = name.GetString();
            return s.size() > terminator &&
                   TfStringStartsWith(s, namespaces)   && 
                   s[terminator] == delim;
        });

    std::vector<UsdProperty> properties(_MakeProperties(names));
    WorkSwapDestroyAsync(names);
    return properties;
}

std::vector<UsdProperty>
UsdPrim::GetPropertiesInNamespace(const std::string &namespaces) const
{
    return _GetPropertiesInNamespace(namespaces, /*onlyAuthored=*/false);
}

std::vector<UsdProperty>
UsdPrim::GetAuthoredPropertiesInNamespace(const std::string &namespaces) const
{
    return _GetPropertiesInNamespace(namespaces, /*onlyAuthored=*/true);
}

std::vector<UsdProperty>
UsdPrim::GetPropertiesInNamespace(const std::vector<std::string> &namespaces) const
{
    return GetPropertiesInNamespace(SdfPath::JoinIdentifier(namespaces));
}

std::vector<UsdProperty>
UsdPrim::GetAuthoredPropertiesInNamespace(
    const std::vector<std::string> &namespaces) const
{
    return GetAuthoredPropertiesInNamespace(
        SdfPath::JoinIdentifier(namespaces));
}

UsdAttribute
UsdPrim::CreateAttribute(const TfToken& name,
                         const SdfValueTypeName &typeName,
                         bool custom,
                         SdfVariability variability) const
{
    UsdAttribute attr = GetAttribute(name);
    attr._Create(typeName, custom, variability);
    return attr;
}

UsdAttribute
UsdPrim::CreateAttribute(const TfToken& name,
                         const SdfValueTypeName &typeName,
                         SdfVariability variability) const
{
    return CreateAttribute(name, typeName, /*custom=*/true, variability);
}

UsdAttribute
UsdPrim::CreateAttribute(const std::vector<std::string> &nameElts,
                         const SdfValueTypeName &typeName,
                         bool custom,
                         SdfVariability variability) const
{
    return CreateAttribute(TfToken(SdfPath::JoinIdentifier(nameElts)),
                           typeName, custom, variability);
}

UsdAttribute
UsdPrim::CreateAttribute(const std::vector<std::string> &nameElts,
                         const SdfValueTypeName &typeName,
                         SdfVariability variability) const
{
    return CreateAttribute(nameElts, typeName, /*custom=*/true, variability);
}

UsdAttributeVector
UsdPrim::_GetAttributes(bool onlyAuthored, bool applyOrder) const
{
    const TfTokenVector names = _GetPropertyNames(onlyAuthored, applyOrder);
    UsdAttributeVector attrs;

    // PERFORMANCE: This is sloppy, since property names are a superset of
    // attribute names, however this vector is likely short lived and worth the
    // trade off of repeated reallocation.
    attrs.reserve(names.size());
    for (const auto& propName : names) {
        if (UsdAttribute attr = GetAttribute(propName)) {
            attrs.push_back(attr);
        }
    }

    return attrs;
}

UsdAttributeVector
UsdPrim::GetAttributes() const
{
    return _GetAttributes(/*onlyAuthored=*/false, /*applyOrder=*/true);
}

UsdAttributeVector
UsdPrim::GetAuthoredAttributes() const
{
    return _GetAttributes(/*onlyAuthored=*/true, /*applyOrder=*/true);
}

UsdAttribute
UsdPrim::GetAttribute(const TfToken& attrName) const
{
    // An invalid prim will present a coding error, and then return an
    // invalid attribute
    return UsdAttribute(_Prim(), _ProxyPrimPath(), attrName);
}

bool
UsdPrim::HasAttribute(const TfToken& attrName) const
{
    return static_cast<bool>(GetAttribute(attrName));
}

UsdRelationship
UsdPrim::CreateRelationship(const TfToken& name, bool custom) const
{
    UsdRelationship rel = GetRelationship(name);
    rel._Create(custom);
    return rel;
}

UsdRelationship
UsdPrim::CreateRelationship(const std::vector<std::string> &nameElts, 
                            bool custom) const
{
    return CreateRelationship(TfToken(SdfPath::JoinIdentifier(nameElts)),
                              custom);
}

UsdRelationshipVector
UsdPrim::_GetRelationships(bool onlyAuthored, bool applyOrder) const
{
    const TfTokenVector names = _GetPropertyNames(onlyAuthored, applyOrder);
    UsdRelationshipVector rels;

    // PERFORMANCE: This is sloppy, since property names are a superset of
    // relationship names, however this vector is likely short lived and worth
    // the trade off of repeated reallocation.
    rels.reserve(names.size());
    for (const auto& propName : names) {
        if (UsdRelationship rel = GetRelationship(propName)) {
            rels.push_back(rel);
        }
    }

    return rels;
}

UsdRelationshipVector
UsdPrim::GetRelationships() const
{
    return _GetRelationships(/*onlyAuthored=*/false, /*applyOrder=*/true);
}

UsdRelationshipVector
UsdPrim::GetAuthoredRelationships() const
{
    return _GetRelationships(/*onlyAuthored=*/true, /*applyOrder=*/true);
}

UsdRelationship
UsdPrim::GetRelationship(const TfToken& relName) const
{
    return UsdRelationship(_Prim(), _ProxyPrimPath(), relName);
}

bool
UsdPrim::HasRelationship(const TfToken& relName) const
{
    return static_cast<bool>(GetRelationship(relName));
} 

template <class PropertyType, class Derived>
struct UsdPrim_TargetFinder
{
    using Predicate = std::function<bool (PropertyType const &)>;
    
    static SdfPathVector
    Find(UsdPrim const &prim, Usd_PrimFlagsPredicate const &traversal,
         Predicate const &pred, bool recurse) {
        UsdPrim_TargetFinder tf(prim, traversal, pred, recurse);
        tf._Find();
        return std::move(tf._result);
    }

private:
    explicit UsdPrim_TargetFinder(
        UsdPrim const &prim, Usd_PrimFlagsPredicate const &traversal,
        Predicate const &pred, bool recurse)
        : _prim(prim)
        , _traversal(traversal)
        , _consumerTask(_dispatcher, [this]() { _ConsumerTask(); })
        , _predicate(pred)
        , _recurse(recurse) {}

    void _Visit(UsdRelationship const &rel) {
        SdfPathVector targets;
        rel._GetForwardedTargets(&targets,
                                 /*includeForwardingRels=*/true);
        _VisitImpl(targets);
    }
    
    void _Visit(UsdAttribute const &attr) {
        SdfPathVector sources;
        attr.GetConnections(&sources);
        _VisitImpl(sources);
    }
    
    void _VisitImpl(SdfPathVector const &paths) {
        if (!paths.empty()) {
            for (SdfPath const &p: paths) {
                _workQueue.push(p);
            }
            _consumerTask.Wake();
        }
        
        if (_recurse) {
            WorkParallelForEach(
                paths.begin(), paths.end(),
                [this](SdfPath const &path) {
                    if (!path.HasPrefix(_prim.GetPath())) {
                        if (UsdPrim owningPrim = _prim.GetStage()->
                            GetPrimAtPath(path.GetPrimPath())) {
                            _VisitSubtree(owningPrim);
                        }
                    }
                });
        }
    }    

    void _VisitPrim(UsdPrim const &prim) {
        if (_seenPrims.insert(prim).second) {
            auto props = static_cast<Derived *>(this)->_GetProperties(prim);
            for (auto const &prop: props) {
                if (!_predicate || _predicate(prop)) {
                    _dispatcher.Run([this, prop]() { _Visit(prop); });
                }
            }
        }
    };

    void _VisitSubtree(UsdPrim const &prim) {
        _VisitPrim(prim);
        auto range = prim.GetFilteredDescendants(_traversal);
        WorkParallelForEach(range.begin(), range.end(),
                            [this](UsdPrim const &desc) { _VisitPrim(desc); });
    }

    void _Find() {
        TF_PY_ALLOW_THREADS_IN_SCOPE();

        WorkWithScopedParallelism([this]() {
                _VisitSubtree(_prim);
                _dispatcher.Wait();
                tbb::parallel_sort(_result.begin(), _result.end(),
                                   SdfPath::FastLessThan());
            });

        _result.erase(unique(_result.begin(), _result.end()), _result.end());
    }

    void _ConsumerTask() {
        SdfPath path;
        while (_workQueue.try_pop(path)) {
            _result.push_back(path);
        }
    }

    UsdPrim _prim;
    Usd_PrimFlagsPredicate _traversal;
    WorkDispatcher _dispatcher;
    WorkSingularTask _consumerTask;
    Predicate const &_predicate;
    tbb::concurrent_queue<SdfPath> _workQueue;
    tbb::concurrent_unordered_set<UsdPrim, TfHash> _seenPrims;
    SdfPathVector _result;
    bool _recurse;
};
          
struct UsdPrim_RelTargetFinder
    : public UsdPrim_TargetFinder<UsdRelationship, UsdPrim_RelTargetFinder>
{
    std::vector<UsdRelationship> _GetProperties(UsdPrim const &prim) const {
        return prim._GetRelationships(/*onlyAuthored=*/true,
                                      /*applyOrder=*/false);
    }
};

struct UsdPrim_AttrConnectionFinder
    : public UsdPrim_TargetFinder<UsdAttribute, UsdPrim_AttrConnectionFinder>
{
    std::vector<UsdAttribute> _GetProperties(UsdPrim const &prim) const {
        return prim._GetAttributes(/*onlyAuthored=*/true,
                                   /*applyOrder=*/false);
    }
};

USD_API
SdfPathVector
UsdPrim::FindAllAttributeConnectionPaths(
    Usd_PrimFlagsPredicate const &traversal,
    std::function<bool (UsdAttribute const &)> const &predicate,
    bool recurseOnSources) const
{
    return UsdPrim_AttrConnectionFinder
        ::Find(*this, traversal, predicate, recurseOnSources);
}

USD_API
SdfPathVector
UsdPrim::FindAllAttributeConnectionPaths(
    std::function<bool (UsdAttribute const &)> const &predicate,
    bool recurseOnSources) const
{
    return FindAllAttributeConnectionPaths(
        UsdPrimDefaultPredicate, predicate, recurseOnSources);
}
    
SdfPathVector
UsdPrim::FindAllRelationshipTargetPaths(
    Usd_PrimFlagsPredicate const &traversal,
    std::function<bool (UsdRelationship const &)> const &predicate,
    bool recurseOnTargets) const
{
    return UsdPrim_RelTargetFinder::Find(
        *this, traversal, predicate, recurseOnTargets);
}

SdfPathVector
UsdPrim::FindAllRelationshipTargetPaths(
    std::function<bool (UsdRelationship const &)> const &predicate,
    bool recurseOnTargets) const
{
    return FindAllRelationshipTargetPaths(
        UsdPrimDefaultPredicate, predicate, recurseOnTargets);
}

bool
UsdPrim::HasVariantSets() const
{
    // Variant sets can't be defined in schema fallbacks as of yet so we only
    // need to check for authored variant sets.
    return HasAuthoredMetadata(SdfFieldKeys->VariantSetNames);
}

UsdVariantSets
UsdPrim::GetVariantSets() const
{
    return UsdVariantSets(*this);
}

UsdVariantSet
UsdPrim::GetVariantSet(const std::string& variantSetName) const
{
    return UsdVariantSet(*this, variantSetName);
}


UsdInherits
UsdPrim::GetInherits() const
{
    return UsdInherits(*this);
}

bool
UsdPrim::HasAuthoredInherits() const
{
    return HasAuthoredMetadata(SdfFieldKeys->InheritPaths);
}

UsdSpecializes
UsdPrim::GetSpecializes() const
{
    return UsdSpecializes(*this);
}

bool
UsdPrim::HasAuthoredSpecializes() const
{
    return HasAuthoredMetadata(SdfFieldKeys->Specializes);
}

UsdReferences
UsdPrim::GetReferences() const
{
    return UsdReferences(*this);
}

bool
UsdPrim::HasAuthoredReferences() const
{
    return HasAuthoredMetadata(SdfFieldKeys->References);
}

// --------------------------------------------------------------------- //
/// \name Payloads, Load and Unload
// --------------------------------------------------------------------- //

bool
UsdPrim::HasPayload() const
{
    return HasAuthoredPayloads();
}

bool
UsdPrim::SetPayload(const SdfPayload& payload) const
{
    UsdPayloads payloads = GetPayloads();
    payloads.ClearPayloads();
    return payloads.SetPayloads(SdfPayloadVector{payload});
}

bool
UsdPrim::SetPayload(const std::string& assetPath, const SdfPath& primPath) const
{
    return SetPayload(SdfPayload(assetPath, primPath));
}

bool
UsdPrim::SetPayload(const SdfLayerHandle& layer, const SdfPath& primPath) const
{
    return SetPayload(SdfPayload(layer->GetIdentifier(), primPath));
}

bool
UsdPrim::ClearPayload() const
{
    return GetPayloads().ClearPayloads();
}

UsdPayloads
UsdPrim::GetPayloads() const
{
    return UsdPayloads(*this);
}

bool
UsdPrim::HasAuthoredPayloads() const
{
    // Unlike the equivalent function for references, we query the prim data
    // for the cached value of HasPayload computed by Pcp instead of querying
    // the composed metadata. This is necessary as this function is called by 
    // _IncludeNewlyDiscoveredPayloadsPredicate in UsdStage which can't safely
    // call back into the querying the composed metatdata.
    return _Prim()->HasPayload();
}

void
UsdPrim::Load(UsdLoadPolicy policy) const
{
    if (IsInPrototype()) {
        TF_CODING_ERROR("Attempted to load a prim in a prototype <%s>",
                        GetPath().GetText());
        return;
    }
    _GetStage()->Load(GetPath(), policy);
}

void
UsdPrim::Unload() const
{
    if (IsInPrototype()) {
        TF_CODING_ERROR("Attempted to unload a prim in a prototype <%s>",
                        GetPath().GetText());
        return;
    }
    _GetStage()->Unload(GetPath());
}

TfTokenVector 
UsdPrim::GetChildrenNames() const
{
    TfTokenVector names;
    for (const auto &child : GetChildren()) {
        names.push_back(child.GetName());
    }
    return names;
}

TfTokenVector 
UsdPrim::GetAllChildrenNames() const
{
    TfTokenVector names;
    for (const auto &child : GetAllChildren()) {
        names.push_back(child.GetName());
    }
    return names;
}

TfTokenVector 
UsdPrim::GetFilteredChildrenNames(const Usd_PrimFlagsPredicate &predicate) const
{
    TfTokenVector names;
    for (const auto &child : GetFilteredChildren(predicate)) {
        names.push_back(child.GetName());
    }
    return names;
}

TfTokenVector
UsdPrim::GetChildrenReorder() const
{
    TfTokenVector reorder;
    GetMetadata(SdfFieldKeys->PrimOrder, &reorder);
    return reorder;
}

UsdPrim
UsdPrim::GetNextSibling() const
{
    return GetFilteredNextSibling(UsdPrimDefaultPredicate);
}

UsdPrim
UsdPrim::GetFilteredNextSibling(const Usd_PrimFlagsPredicate &inPred) const
{
    Usd_PrimDataConstPtr sibling = get_pointer(_Prim());
    SdfPath siblingPath = _ProxyPrimPath();
    const Usd_PrimFlagsPredicate pred = 
        Usd_CreatePredicateForTraversal(sibling, siblingPath, inPred);

    if (Usd_MoveToNextSiblingOrParent(sibling, siblingPath, pred)) {
        return UsdPrim();
    }
    return UsdPrim(sibling, siblingPath);
}

bool
UsdPrim::IsPseudoRoot() const
{
    return GetPath() == SdfPath::AbsoluteRootPath();
}

bool
UsdPrim::IsPrototypePath(const SdfPath& path)
{
    return Usd_InstanceCache::IsPrototypePath(path);
}

bool
UsdPrim::IsPathInPrototype(const SdfPath& path)
{
    return Usd_InstanceCache::IsPathInPrototype(path);
}

UsdPrim
UsdPrim::GetPrototype() const
{
    Usd_PrimDataConstPtr protoPrimData = 
        _GetStage()->_GetPrototypeForInstance(get_pointer(_Prim()));
    return UsdPrim(protoPrimData, SdfPath());
}

std::vector<UsdPrim>
UsdPrim::GetInstances() const
{
    return _GetStage()->_GetInstancesForPrototype(*this);
}

SdfPrimSpecHandleVector 
UsdPrim::GetPrimStack() const
{
    return UsdStage::_GetPrimStack(*this);
}

std::vector<std::pair<SdfPrimSpecHandle, SdfLayerOffset>> 
UsdPrim::GetPrimStackWithLayerOffsets() const
{
    return UsdStage::_GetPrimStackWithLayerOffsets(*this);
}

PcpPrimIndex 
UsdPrim::ComputeExpandedPrimIndex() const
{
    // Get the prim index path to compute from the index stored in the prim
    // data. This ensures we get consistent behavior when dealing with 
    // instancing and instance proxies.
    const PcpPrimIndex& cachedPrimIndex = _Prim()->GetSourcePrimIndex();
    if (!cachedPrimIndex.IsValid()) {
        return PcpPrimIndex();
    }
    
    const SdfPath& primIndexPath = cachedPrimIndex.GetPath();
    PcpCache* cache = _GetStage()->_GetPcpCache();
    
    PcpPrimIndexOutputs outputs;
    PcpComputePrimIndex(
        primIndexPath, cache->GetLayerStack(),
        cache->GetPrimIndexInputs().Cull(false), 
        &outputs);

    _GetStage()->_ReportPcpErrors(
        outputs.allErrors, 
        TfStringPrintf(
            "computing expanded prim index for <%s>", GetPath().GetText()));
    
    return outputs.primIndex;
}

static PcpNodeRef 
_FindStrongestNodeMatchingEditTarget(
    const PcpPrimIndex& index, const UsdEditTarget &editTarget)
{
    // Use the edit target to map the prim's path to the path we expect to find
    // a node for.
    const SdfPath &rootPath = index.GetRootNode().GetPath();
    const SdfPath mappedPath = editTarget.MapToSpecPath(rootPath);

    if (mappedPath.IsEmpty()) {
        return PcpNodeRef();
    }

    // We're looking for the first (strongest) node that would be affected by
    // an edit to the prim using the edit target which means we are looking for
    // the following criteria to be met:
    //  1. The node's path matches the prim path mapped through the edit target.
    //  2. The edit target's layer is in the node's layer stack.
    for (const PcpNodeRef &node : index.GetNodeRange()) {
        if (node.GetPath() != mappedPath) {
            continue;
        }

        if (node.GetLayerStack()->HasLayer(editTarget.GetLayer())) {
            return node;
        }
    }

    return PcpNodeRef();
}

UsdResolveTarget 
UsdPrim::_MakeResolveTargetFromEditTarget(
    const UsdEditTarget &editTarget,
    bool makeAsStrongerThan) const
{
    // Need the expanded prim index to find nodes and layers that may have been 
    // culled out in the cached prim index.
    PcpPrimIndex expandedPrimIndex = ComputeExpandedPrimIndex();
    if (!expandedPrimIndex.IsValid()) {
        return UsdResolveTarget();
    }

    const PcpNodeRef node = _FindStrongestNodeMatchingEditTarget(
        expandedPrimIndex, editTarget);
    if (!node) {
        return UsdResolveTarget();
    }

    // The resolve target needs to hold on to the expanded prim index.
    std::shared_ptr<PcpPrimIndex> resolveIndex = 
        std::make_shared<PcpPrimIndex>(std::move(expandedPrimIndex));

    if (makeAsStrongerThan) {
        // Return a resolve target starting at the root node and stopping at the
        // edit node and layer.
        return UsdResolveTarget(resolveIndex, 
            node.GetRootNode(), nullptr, node, editTarget.GetLayer());
    } else {
        // Return a resolve target starting at the edit node and layer.
        return UsdResolveTarget(resolveIndex, node, editTarget.GetLayer());
    }
}

UsdPrim::_ProtoToInstancePathMap
UsdPrim::_GetProtoToInstancePathMap() const
{
    // Walk up to the root while we're in (nested) instance-land.  When we
    // hit an instance or a prototype, add a mapping for the prototype
    // source prim index path to this particular instance (proxy) path.

    _ProtoToInstancePathMap pathMap;
    if (_Prim()->IsInPrototype()) {
        // This prim might be an instance proxy inside a prototype, if so use
        // its prototype, but be sure to skip up to the parent if *this* prim is
        // an instance.  Target paths on *this* prim are in the "space" of its
        // next ancestral prototype, just as how attribute & metadata values
        // come from the instance itself, not its prototype.

        UsdPrim prim = *this;
        if (prim.IsInstance()) {
            prim = prim.GetParent();
        }
        for (; prim; prim = prim.GetParent()) {
            UsdPrim prototype;
            if (prim.IsInstance()) {
                prototype = prim.GetPrototype();
            } else if (prim.IsPrototype()) {
                prototype = prim;
            }
            if (prototype) {
                pathMap._map.emplace_back(
                    prototype._GetSourcePrimIndex().GetPath(),
                    prim.GetPath());
            }
        };
        std::sort(pathMap._map.begin(), pathMap._map.end());
    }
    return pathMap;
}

UsdResolveTarget 
UsdPrim::MakeResolveTargetUpToEditTarget(
    const UsdEditTarget &editTarget) const
{
    return _MakeResolveTargetFromEditTarget(
        editTarget, /* makeAsStrongerThan = */ false);
}

UsdResolveTarget 
UsdPrim::MakeResolveTargetStrongerThanEditTarget(
    const UsdEditTarget &editTarget) const
{
    return _MakeResolveTargetFromEditTarget(
        editTarget, /* makeAsStrongerThan = */ true);
}

UsdPrim
UsdPrim::GetPrimAtPath(const SdfPath& path) const{
    const SdfPath absolutePath = path.MakeAbsolutePath(GetPath());
    return GetStage()->GetPrimAtPath(absolutePath);
}

UsdObject
UsdPrim::GetObjectAtPath(const SdfPath& path) const{
    const SdfPath absolutePath = path.MakeAbsolutePath(GetPath());
    return GetStage()->GetObjectAtPath(absolutePath);
}

UsdProperty
UsdPrim::GetPropertyAtPath(const SdfPath& path) const{
    return GetObjectAtPath(path).As<UsdProperty>();
}

UsdAttribute
UsdPrim::GetAttributeAtPath(const SdfPath& path) const{
    return GetObjectAtPath(path).As<UsdAttribute>();
}


UsdRelationship
UsdPrim::GetRelationshipAtPath(const SdfPath& path) const{
    return GetObjectAtPath(path).As<UsdRelationship>();
}

PXR_NAMESPACE_CLOSE_SCOPE

