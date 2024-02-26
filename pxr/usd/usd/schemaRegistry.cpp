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
#include "pxr/pxr.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/primDefinition.h"

#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/clip.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/withScopedParallelism.h"

#include <cctype>
#include <set>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using std::set;
using std::string;
using std::vector;

TF_DEFINE_ENV_SETTING(
    USD_DISABLE_PRIM_DEFINITIONS_FOR_USDGENSCHEMA, false,
    "Set to true to disable the generation of prim definitions for schema "
    "types in the schema registry. This is used is to prevent the processing "
    "of generatedSchema.usda files during schema generation as it's the "
    "process used to create, update, or fix generatedSchema.usda files. "
    "This should only be used by usdGenSchema.py as this can cause crashes in "
    "most contexts which expect prim definitions for schema types.");

TF_DEFINE_ENV_SETTING(
    USD_DISABLE_AUTO_APPLY_API_SCHEMAS, false,
    "Set to true to disable the application of all auto-apply API schemas.");

TF_INSTANTIATE_SINGLETON(UsdSchemaRegistry);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (appliedAPISchemas)
    (multipleApplyAPISchemas)
    (multipleApplyAPISchemaPrefixes)
    (autoApplyAPISchemas)

    (apiSchemaAutoApplyTo)
    (apiSchemaCanOnlyApplyTo)
    (apiSchemaAllowedInstanceNames)
    (apiSchemaInstances)
    (schemaKind)
    (nonAppliedAPI)
    (singleApplyAPI)
    (multipleApplyAPI)
    (concreteTyped)
    (abstractTyped)
    (abstractBase)

    ((PluginAutoApplyAPISchemasKey, "AutoApplyAPISchemas"))
);

using _TypeToTokenVecMap = 
    TfHashMap<TfType, TfTokenVector, TfHash>;
using _TokenToTokenMap = 
    TfHashMap<TfToken, TfToken, TfToken::HashFunctor>;

static UsdSchemaKind 
_GetSchemaKindFromMetadata(const JsObject &dict)
{
    const JsValue *kindValue = TfMapLookupPtr(dict, _tokens->schemaKind);
    if (!kindValue) {
        return UsdSchemaKind::Invalid;
    }

    const TfToken schemaTypeToken(kindValue->GetString());
    if (schemaTypeToken == _tokens->nonAppliedAPI) {
        return UsdSchemaKind::NonAppliedAPI;
    } else if (schemaTypeToken == _tokens->singleApplyAPI) {
        return UsdSchemaKind::SingleApplyAPI;
    } else if (schemaTypeToken == _tokens->multipleApplyAPI) {
        return UsdSchemaKind::MultipleApplyAPI;
    } else if (schemaTypeToken == _tokens->concreteTyped) {
        return UsdSchemaKind::ConcreteTyped;
    } else if (schemaTypeToken == _tokens->abstractTyped) {
        return UsdSchemaKind::AbstractTyped;
    } else if (schemaTypeToken == _tokens->abstractBase) {
        return UsdSchemaKind::AbstractBase;
    }

    TF_CODING_ERROR("Invalid schema kind name '%s' found for plugin metadata "
                    "key '%s'.", 
                    schemaTypeToken.GetText(), _tokens->schemaKind.GetText());
    return UsdSchemaKind::Invalid;
}

static UsdSchemaKind
_GetSchemaKindFromPlugin(const TfType &schemaType)
{
    TRACE_FUNCTION();
    PlugPluginPtr plugin =
        PlugRegistry::GetInstance().GetPluginForType(schemaType);
    if (!plugin) {
        TF_CODING_ERROR("Failed to find plugin for schema type '%s'",
                        schemaType.GetTypeName().c_str());
        return UsdSchemaKind::Invalid;
    }

    return _GetSchemaKindFromMetadata(plugin->GetMetadataForType(schemaType));
}

namespace {
// Helper struct for caching a bidirecional mapping between schema TfType and
// USD type name token. This cache is used as a static local instance providing
// this type mapping without having to build the entire schema registry
struct _TypeMapCache {
    _TypeMapCache() {
        const TfType schemaBaseType = TfType::Find<UsdSchemaBase>();

        set<TfType> types;
        PlugRegistry::GetAllDerivedTypes(schemaBaseType, &types);
        types.insert(schemaBaseType);

        for (const TfType &type : types) {
            // The schema's identifier is the type's alias under UsdSchemaBase. 
            // All schemas should have a type name alias.
            const vector<string> aliases = schemaBaseType.GetAliases(type);
            if (aliases.size() != 1) {
                continue;
            }

            // Generate all the components of the schema info.
            const TfToken schemaIdentifier(aliases.front(), TfToken::Immortal);
            const UsdSchemaKind schemaKind = _GetSchemaKindFromPlugin(type);
            const std::pair<TfToken, UsdSchemaVersion> familyAndVersion = 
                UsdSchemaRegistry::ParseSchemaFamilyAndVersionFromIdentifier(
                    schemaIdentifier);

            // Add primary mapping of schema info by type.
            auto inserted = schemaInfoByType.emplace(type, 
                UsdSchemaRegistry::SchemaInfo{
                    schemaIdentifier, 
                    type, 
                    familyAndVersion.first, 
                    familyAndVersion.second, 
                    schemaKind});

            // Add secondary mapping of schema info pointer by identifier.
            schemaInfoByIdentifier.emplace(
                schemaIdentifier, &inserted.first->second);
        }
    }

    // Primary mapping of schema info by TfType.
    std::unordered_map<TfType, UsdSchemaRegistry::SchemaInfo, TfHash> 
        schemaInfoByType;

    // Secondary mapping of schema info by identifier token.
    std::unordered_map<TfToken, const UsdSchemaRegistry::SchemaInfo *, TfHash>
        schemaInfoByIdentifier;
};

// Static singleton accessor
static const _TypeMapCache &
_GetTypeMapCache() {
    static _TypeMapCache typeCache;
    return typeCache;
}

// Helper struct for caching the information extracted from plugin metadata
// about how API schema types are applied.
struct _APISchemaApplyToInfoCache {
    _APISchemaApplyToInfoCache()
    {
        TRACE_FUNCTION();

        // Get all types that derive UsdSchemaBase by getting the type map 
        // cache.
        const _TypeMapCache &typeCache = _GetTypeMapCache();

        // For each schema type, extract the can apply and auto apply plugin 
        // info into the cache.
        for (const auto &valuePair : typeCache.schemaInfoByType) {
            const UsdSchemaRegistry::SchemaInfo &schemaInfo = valuePair.second;

            Usd_GetAPISchemaPluginApplyToInfoForType(
                schemaInfo.type,
                schemaInfo.identifier,
                &autoApplyAPISchemasMap,
                &canOnlyApplyAPISchemasMap,
                &allowedInstanceNamesMap);
        }

        // Collect any plugin auto apply API schema mappings. These can be 
        // defined in any plugin to auto apply schemas in a particular 
        // application context instead of the type itself being defined to 
        // always auto apply whenever it is present.
        UsdSchemaRegistry::CollectAddtionalAutoApplyAPISchemasFromPlugins(
            &autoApplyAPISchemasMap);
    }
    
    // Mapping of API schema type name to a list of type names it should auto 
    // applied to.
    UsdSchemaRegistry::TokenToTokenVectorMap autoApplyAPISchemasMap;

    // Mapping of API schema type name to a list of prim type names that it
    // is ONLY allowed to be applied to.
    UsdSchemaRegistry::TokenToTokenVectorMap canOnlyApplyAPISchemasMap;

    // Mapping of multiple apply API schema type name to a set of instance names
    // that are the only allowed instance names for that type.
    TfHashMap<TfToken, TfToken::Set, TfHash> allowedInstanceNamesMap;
};

// Static singleton accessor
static const _APISchemaApplyToInfoCache &
_GetAPISchemaApplyToInfoCache() {
    static _APISchemaApplyToInfoCache applyToInfo;
    return applyToInfo;
}

}; // end anonymous namespace

static bool 
_IsConcreteSchemaKind(const UsdSchemaKind schemaKind)
{
    return schemaKind == UsdSchemaKind::ConcreteTyped;
}

static bool
_IsAbstractSchemaKind(const UsdSchemaKind schemaKind)
{
    return (schemaKind == UsdSchemaKind::AbstractTyped) || 
        (schemaKind == UsdSchemaKind::AbstractBase);
}

static bool 
_IsAppliedAPISchemaKind(const UsdSchemaKind schemaKind)
{
    return schemaKind == UsdSchemaKind::SingleApplyAPI ||
           schemaKind == UsdSchemaKind::MultipleApplyAPI;
}

static bool
_IsAPISchemaKind(const UsdSchemaKind schemaKind) 
{
    return schemaKind == UsdSchemaKind::SingleApplyAPI ||
           schemaKind == UsdSchemaKind::MultipleApplyAPI ||
           schemaKind == UsdSchemaKind::NonAppliedAPI;
}

static bool 
_IsMultipleApplySchemaKind(const UsdSchemaKind schemaKind)
{
    return schemaKind == UsdSchemaKind::MultipleApplyAPI;
}

/* static */
TfToken
UsdSchemaRegistry::MakeSchemaIdentifierForFamilyAndVersion(
    const TfToken &schemaFamily, 
    UsdSchemaVersion schemaVersion)
{
    // Version 0, the family is the identifier.
    if (schemaVersion == 0) {
        return schemaFamily;
    }

    // All other versions, append the version suffix and find by identifier.
    std::string idStr = schemaFamily.GetString();
    idStr.append("_");
    idStr.append(TfStringify(schemaVersion));
    return TfToken(idStr);
}

// Search from the end of the string for an underscore character that is ONLY
// followed by one or more digits. This is beginning of the version suffix if
// found.
static size_t
_FindVersionDelimiter(const std::string &idString) 
{
    static const char versionDelimiter = '_';

    // A version suffix is at least 2 characters long (underscore and 1+ digits)
    const size_t idLen =  idString.size();
    if (idLen < 2) {
        return std::string::npos;
    }

    size_t delim = idLen - 1;
    if (!std::isdigit(idString[delim])) {
        return std::string::npos;
    }
    while (--delim >= 0) {
        if (idString[delim] == versionDelimiter) {
            return delim;
        }
        if (!std::isdigit(idString[delim])) {
            return std::string::npos;
        }
    }
    return std::string::npos;
}

/* static */
std::pair<TfToken, UsdSchemaVersion> 
UsdSchemaRegistry::ParseSchemaFamilyAndVersionFromIdentifier(
    const TfToken &schemaIdentifier)
{
    const std::string &idString = schemaIdentifier.GetString();

    const size_t delim = _FindVersionDelimiter(idString);

    if (delim == std::string::npos) {
        // If the identifier has no version suffix, the family is the identifier
        // and the version is zero.
        return std::make_pair(schemaIdentifier, 0);
    }

    // Successfully parsed a family and version. Return them.
    return std::make_pair(
        TfToken(idString.substr(0, delim)), 
        TfUnstringify<UsdSchemaVersion>(idString.substr(delim + 1)));
}

/* static */
bool
UsdSchemaRegistry::IsAllowedSchemaFamily(const TfToken &schemaFamily)
{
    return TfIsValidIdentifier(schemaFamily) &&
        _FindVersionDelimiter(schemaFamily) == std::string::npos;
}

/* static */
bool
UsdSchemaRegistry::IsAllowedSchemaIdentifier(const TfToken &schemaIdentifier)
{
    const auto familyAndVersion =
        ParseSchemaFamilyAndVersionFromIdentifier(schemaIdentifier);
    return IsAllowedSchemaFamily(familyAndVersion.first) &&
        MakeSchemaIdentifierForFamilyAndVersion(
            familyAndVersion.first, familyAndVersion.second) == schemaIdentifier;
}

/*static*/
const UsdSchemaRegistry::SchemaInfo *
UsdSchemaRegistry::FindSchemaInfo(const TfType &schemaType)
{
    const _TypeMapCache & typeMapCache = _GetTypeMapCache();
    return TfMapLookupPtr(typeMapCache.schemaInfoByType, schemaType);
}

/*static*/
const UsdSchemaRegistry::SchemaInfo *
UsdSchemaRegistry::FindSchemaInfo(const TfToken &schemaIdentifier)
{
    const _TypeMapCache & typeMapCache = _GetTypeMapCache();
    const auto it = typeMapCache.schemaInfoByIdentifier.find(schemaIdentifier);
    return it != typeMapCache.schemaInfoByIdentifier.end() ?
        it->second : nullptr;
}

/*static*/
const UsdSchemaRegistry::SchemaInfo *
UsdSchemaRegistry::FindSchemaInfo(
    const TfToken &schemaFamily, UsdSchemaVersion schemaVersion)
{
    // It is possible to pass an invalid schema family with version 0 that 
    // produces a registered schema's valid identifier. An example would be 
    // FindSchemaInfo("Foo_1", 0) would be able to find schema info for a 
    // schema named "Foo_1" if it exists even though "Foo_1" is family "Foo"
    // version 1. This check is to prevent returning the schema info in this
    // case as it wouldn't represent the passed in family and version.
    if (ARCH_UNLIKELY(!IsAllowedSchemaFamily(schemaFamily))) {
        return nullptr;
    }

    return FindSchemaInfo(
        MakeSchemaIdentifierForFamilyAndVersion(schemaFamily, schemaVersion));
}

namespace {

// Helper class for storing and retrieving a vector of schema info pointers
// sorted from highest version to lowest. One of these will be created for each
// schema family.
class _VersionOrderedSchemas 
{
public:
    using SchemaInfoConstPtr = const UsdSchemaRegistry::SchemaInfo *;
    using SchemaInfoConstPtrVector = std::vector<SchemaInfoConstPtr>;

    // Insert schema info, maintaining highest to lowest order
    void Insert(const UsdSchemaRegistry::SchemaInfo * schemaInfo)
    {
        _orderedSchemas.insert(_LowerBound(schemaInfo->version), schemaInfo);
    }

    // Get the entire vector of ordered schemas.
    const SchemaInfoConstPtrVector &
    GetSchemaInfos() const
    {
        return _orderedSchemas;
    }

    // Get a copy of the subrange of schemas that match the version predicate.
    SchemaInfoConstPtrVector
    GetFilteredSchemaInfos(
        UsdSchemaVersion schemaVersion,
        UsdSchemaRegistry::VersionPolicy versionPolicy) const
    {
        // Note again that the schemas are ordered highest version to lowest, 
        // thus the backwards seeming subranges.
        switch (versionPolicy) {
        case UsdSchemaRegistry::VersionPolicy::All:
            return _orderedSchemas;
        case UsdSchemaRegistry::VersionPolicy::GreaterThan:
            return {_orderedSchemas.cbegin(), _LowerBound(schemaVersion)};
        case UsdSchemaRegistry::VersionPolicy::GreaterThanOrEqual:
            return {_orderedSchemas.cbegin(), _UpperBound(schemaVersion)};
        case UsdSchemaRegistry::VersionPolicy::LessThan:
            return {_UpperBound(schemaVersion), _orderedSchemas.cend()};
        case UsdSchemaRegistry::VersionPolicy::LessThanOrEqual:
            return {_LowerBound(schemaVersion), _orderedSchemas.cend()};
        };
        return {};
    }

private:
    // Lower bound for highest to lowest version order.
    SchemaInfoConstPtrVector::const_iterator
    _LowerBound(UsdSchemaVersion schemaVersion) const
    {
        return std::lower_bound(_orderedSchemas.begin(), _orderedSchemas.end(), 
            schemaVersion, 
            [](SchemaInfoConstPtr lhs, UsdSchemaVersion schemaVersion) {
                return lhs->version > schemaVersion;
            });
    }

    // Upper bound for highest to lowest version order.
    SchemaInfoConstPtrVector::const_iterator
    _UpperBound(UsdSchemaVersion schemaVersion) const
    {
        return std::upper_bound(_orderedSchemas.begin(), _orderedSchemas.end(), 
            schemaVersion, 
            [](UsdSchemaVersion schemaVersion, SchemaInfoConstPtr rhs) {
                return schemaVersion > rhs->version;
            });
    }

    // Highest to lowest ordered vector.
    SchemaInfoConstPtrVector _orderedSchemas;
};

};

// Map of schema family token to schema info ordered from highest to lowest 
// version.
using _SchemasByFamilyCache = 
    std::unordered_map<TfToken, _VersionOrderedSchemas, TfHash>;

static const _SchemasByFamilyCache &
_GetSchemasByFamilyCache()
{
    // Create the schemas by family singleton from the registered schema types.
    static const _SchemasByFamilyCache schemaFamilyCache = [](){
        const _TypeMapCache &typeCache = _GetTypeMapCache();
        _SchemasByFamilyCache result;
        for (const auto& keyValPair : typeCache.schemaInfoByType) {
            const UsdSchemaRegistry::SchemaInfo &schemaInfo = keyValPair.second;
            result[schemaInfo.family].Insert(&schemaInfo);
        }
        return result;
    }();
    return schemaFamilyCache;
}

/*static*/
const std::vector<const UsdSchemaRegistry::SchemaInfo *> &
UsdSchemaRegistry::FindSchemaInfosInFamily(
    const TfToken &schemaFamily)
{
    const _SchemasByFamilyCache &schemaFamilyCache = _GetSchemasByFamilyCache();
    auto it = schemaFamilyCache.find(schemaFamily);
    if (it == schemaFamilyCache.end()) {
        static const std::vector<const UsdSchemaRegistry::SchemaInfo *> empty;
        return empty;
    }
    return it->second.GetSchemaInfos();
}    

/*static*/
std::vector<const UsdSchemaRegistry::SchemaInfo *>
UsdSchemaRegistry::FindSchemaInfosInFamily(
    const TfToken &schemaFamily, 
    UsdSchemaVersion schemaVersion, 
    VersionPolicy versionPolicy)
{
    const _SchemasByFamilyCache &schemaFamilyCache = _GetSchemasByFamilyCache();
    auto it = schemaFamilyCache.find(schemaFamily);
    if (it == schemaFamilyCache.end()) {
        return {};
    }
    return it->second.GetFilteredSchemaInfos(schemaVersion, versionPolicy);
}

/*static*/
TfToken 
UsdSchemaRegistry::GetSchemaTypeName(const TfType &schemaType) 
{
    const SchemaInfo *schemaInfo = FindSchemaInfo(schemaType);
    return schemaInfo ? schemaInfo->identifier : TfToken();
}

/*static*/
TfToken 
UsdSchemaRegistry::GetConcreteSchemaTypeName(const TfType &schemaType) 
{
    const SchemaInfo *schemaInfo = FindSchemaInfo(schemaType);
    return schemaInfo && _IsConcreteSchemaKind(schemaInfo->kind) ?
        schemaInfo->identifier : TfToken();
}

/*static*/
TfToken 
UsdSchemaRegistry::GetAPISchemaTypeName(const TfType &schemaType) 
{
    const SchemaInfo *schemaInfo = FindSchemaInfo(schemaType);
    return schemaInfo && _IsAPISchemaKind(schemaInfo->kind) ?
        schemaInfo->identifier : TfToken();
}

/*static*/
TfType 
UsdSchemaRegistry::GetTypeFromSchemaTypeName(const TfToken &typeName) 
{
    const SchemaInfo *schemaInfo = FindSchemaInfo(typeName);
    return schemaInfo ? schemaInfo->type : TfType();
}

/*static*/
TfType 
UsdSchemaRegistry::GetConcreteTypeFromSchemaTypeName(const TfToken &typeName) 
{
    const SchemaInfo *schemaInfo = FindSchemaInfo(typeName);
    return schemaInfo && _IsConcreteSchemaKind(schemaInfo->kind) ?
        schemaInfo->type : TfType();
}

/*static*/
TfType 
UsdSchemaRegistry::GetAPITypeFromSchemaTypeName(const TfToken &typeName) 
{
    const SchemaInfo *schemaInfo = FindSchemaInfo(typeName);
    return schemaInfo && _IsAPISchemaKind(schemaInfo->kind) ?
        schemaInfo->type : TfType();
}

/*static*/
UsdSchemaKind 
UsdSchemaRegistry::GetSchemaKind(const TfType &schemaType)
{
    const SchemaInfo *schemaInfo = FindSchemaInfo(schemaType);
    return schemaInfo ? schemaInfo->kind : UsdSchemaKind::Invalid;
}

/*static*/
UsdSchemaKind 
UsdSchemaRegistry::GetSchemaKind(const TfToken &typeName)
{
    const SchemaInfo *schemaInfo = FindSchemaInfo(typeName);
    return schemaInfo ? schemaInfo->kind : UsdSchemaKind::Invalid;
}

/*static*/
bool 
UsdSchemaRegistry::IsConcrete(const TfType& primType)
{
    return _IsConcreteSchemaKind(GetSchemaKind(primType));
}

/*static*/
bool 
UsdSchemaRegistry::IsConcrete(const TfToken& primType)
{
    return _IsConcreteSchemaKind(GetSchemaKind(primType));
}

/*static*/
bool
UsdSchemaRegistry::IsAbstract(const TfType& primType)
{
    return _IsAbstractSchemaKind(GetSchemaKind(primType));
}

/*static*/
bool
UsdSchemaRegistry::IsAbstract(const TfToken& primType)
{
    return _IsAbstractSchemaKind(GetSchemaKind(primType));
}

/*static*/
bool 
UsdSchemaRegistry::IsMultipleApplyAPISchema(const TfType& apiSchemaType)
{
    return _IsMultipleApplySchemaKind(GetSchemaKind(apiSchemaType));
}

/*static*/
bool 
UsdSchemaRegistry::IsMultipleApplyAPISchema(const TfToken& apiSchemaType)
{
    return _IsMultipleApplySchemaKind(GetSchemaKind(apiSchemaType));
}

/*static*/
bool 
UsdSchemaRegistry::IsAppliedAPISchema(const TfType& apiSchemaType)
{
    return _IsAppliedAPISchemaKind(GetSchemaKind(apiSchemaType));
}

/*static*/
bool 
UsdSchemaRegistry::IsAppliedAPISchema(const TfToken& apiSchemaType)
{
    return _IsAppliedAPISchemaKind(GetSchemaKind(apiSchemaType));
}

static const std::string &_GetInstanceNamePlaceholder()
{
    static std::string instanceNamePlaceHolder("__INSTANCE_NAME__");
    return instanceNamePlaceHolder;
}

// Finds the first occurrence of the instance name placeholder that is fully
// contained as a single substring between name space delimiters (including the 
// beginning and ends of the name template).
static std::string::size_type _FindInstanceNamePlaceholder(
    const std::string &nameTemplate)
{
    static const std::string::size_type placeholderSize =
        _GetInstanceNamePlaceholder().size();
    std::string::size_type substrStart = 0;
    while (substrStart < nameTemplate.size()) {
        // The substring ends at the next delimeter (or the end of the name
        // template if no next delimiter is found).
        std::string::size_type substrEnd = nameTemplate.find(':', substrStart);
        if (substrEnd == std::string::npos) {
            substrEnd = nameTemplate.size();
        }
        // If the substring is an exact full word match with the instance name 
        // placeholder, return the beginning of this substring.
        if (substrEnd - substrStart == placeholderSize &&
            nameTemplate.compare(substrStart, placeholderSize, 
                                 _GetInstanceNamePlaceholder()) == 0) {
            return substrStart;
        }
        // Otherwise move to the next substring which starts after the namespace
        // delimiter.
        substrStart = substrEnd + 1;
    }
    return std::string::npos;
}

/*static*/
TfToken
UsdSchemaRegistry::MakeMultipleApplyNameTemplate(
    const std::string &namespacePrefix, 
    const std::string &baseName)
{
    return TfToken(SdfPath::JoinIdentifier(SdfPath::JoinIdentifier(
        namespacePrefix, _GetInstanceNamePlaceholder()), baseName));
}

/*static*/
TfToken 
UsdSchemaRegistry::MakeMultipleApplyNameInstance(
    const std::string &nameTemplate,
    const std::string &instanceName)
{
    // Find the first occurence of the instance name placeholder and replace
    // it with the instance name if found.
    const std::string::size_type pos = 
        _FindInstanceNamePlaceholder(nameTemplate);
    if (pos == std::string::npos) {
        return TfToken(nameTemplate);
    }
    std::string result = nameTemplate;
    result.replace(pos, _GetInstanceNamePlaceholder().size(), instanceName);
    return TfToken(result); 
}

/*static*/
TfToken 
UsdSchemaRegistry::GetMultipleApplyNameTemplateBaseName(
    const std::string &nameTemplate)
{
    // Find the first occurence of the instance name placeholder.
    const std::string::size_type pos = 
        _FindInstanceNamePlaceholder(nameTemplate);
    if (pos == std::string::npos) {
        return TfToken(nameTemplate);
    }

    // The base name is the rest of the name after the instance name 
    // placeholder. If the instance name placeholder is the end of the name, the
    // base name is the empty string.
    const std::string::size_type baseNamePos = 
        pos + _GetInstanceNamePlaceholder().size() + 1;
    if (baseNamePos >= nameTemplate.size()) {
        return TfToken();
    }
    return TfToken(nameTemplate.substr(baseNamePos)); 
}

/*static*/
bool 
UsdSchemaRegistry::IsMultipleApplyNameTemplate(
    const std::string &nameTemplate)
{
    return _FindInstanceNamePlaceholder(nameTemplate) != std::string::npos;
}

static SdfLayerRefPtr
_GetGeneratedSchema(const PlugPluginPtr &plugin)
{
    // Look for generatedSchema in Resources.
    const string fname = TfStringCatPaths(plugin->GetResourcePath(),
                                          "generatedSchema.usda");
    SdfLayerRefPtr layer = SdfLayer::OpenAsAnonymous(fname);

    TF_DEBUG(USD_SCHEMA_REGISTRATION).Msg(
       "Looking up generated schema for plugin %s at path %s. "
       "Generated schema %s.\n",
       plugin->GetName().c_str(),
       fname.c_str(),
       (layer ? "valid" : "invalid") 
    );

    if (!layer) {
        TF_WARN("Failed to open schema layer at path '%s'. "
            "Any schemas defined in plugin library '%s' will not have "
            "valid prim definitions.",
            fname.c_str(), plugin->GetName().c_str());

        // If the layer is invalid, create an empty layer so that we don't have
        // to check for null layers elsewhere in the schema registry or prim
        // definitions.
        layer = SdfLayer::CreateAnonymous(fname);
    }
    return layer;
}

static TfTokenVector
_GetNameListFromMetadata(const JsObject &dict, const TfToken &key)
{
    const JsValue *value = TfMapLookupPtr(dict, key);
    if (!value) {
        return TfTokenVector();
    }

    if (!value->IsArrayOf<std::string>()) {
        TF_CODING_ERROR("Plugin metadata value for key '%s' does not hold a "
                        "string array", key.GetText());
        return TfTokenVector();
    }
    return TfToTokenVector(value->GetArrayOf<std::string>());
}

/*static*/
void
UsdSchemaRegistry::CollectAddtionalAutoApplyAPISchemasFromPlugins(
    TokenToTokenVectorMap *autoApplyAPISchemas)
{
    TRACE_FUNCTION();

    // Skip if auto apply API schemas have been disabled.
    if (TfGetEnvSetting(USD_DISABLE_AUTO_APPLY_API_SCHEMAS)) {
        return;
    }

    // Check all registered plugins for metadata that may supply additional
    // auto apply API schem mappings.
    const PlugPluginPtrVector& plugins =
        PlugRegistry::GetInstance().GetAllPlugins();
    for (const PlugPluginPtr &plug : plugins) {

        // The metadata will contain a dictionary with entries of the form:
        // "AutoApplyAPISchemas": {
        //     "<APISchemaName1>": {
        //         "apiSchemaAutoApplyTo": [
        //             "<TypedSchema1>", "<TypedSchema2>"
        //         ]
        //     },
        //     "<APISchemaName2>": {
        //         "apiSchemaAutoApplyTo": [
        //             "<TypedSchema1>", "<TypedSchema2>"
        //         ]
        //     }
        // }
        const JsObject &metadata = plug->GetMetadata();
        const JsValue *autoApplyMetadataValue = TfMapLookupPtr(
            metadata, _tokens->PluginAutoApplyAPISchemasKey);
        if (!autoApplyMetadataValue) {
            continue;
        }

        TF_DEBUG(USD_AUTO_APPLY_API_SCHEMAS).Msg(
            "Collecting additional auto apply API schemas from "
            "'AutoApplyAPISchemas' metadata in plugin '%s' at path '%s'.",
            plug->GetName().c_str(), plug->GetPath().c_str());

        const JsObject &autoApplyMetadata =
            autoApplyMetadataValue->GetJsObject();
        for(const auto &entry : autoApplyMetadata) {
            if (!entry.second.IsObject()) {
                continue;
            }

            TfToken apiSchemaName(entry.first);

            // The metadata for the apiSchemaAutoApplyTo list is the same as
            // for the auto apply built in to the schema type info.
            TfTokenVector apiSchemaAutoApplyToNames =
                _GetNameListFromMetadata(
                    entry.second.GetJsObject(), _tokens->apiSchemaAutoApplyTo);

            if (!apiSchemaAutoApplyToNames.empty()) {

                TF_DEBUG(USD_AUTO_APPLY_API_SCHEMAS).Msg(
                    "Plugin '%s' is adding automatic application of API schema "
                    "'%s' to the following schema types: [%s].\n",
                    plug->GetName().c_str(), apiSchemaName.GetText(),
                    TfStringJoin(apiSchemaAutoApplyToNames.begin(), 
                                 apiSchemaAutoApplyToNames.end(), ", ").c_str());

                // The API schema may already have an entry in the map, in which
                // case we have to append to the existing entry.
                auto it = autoApplyAPISchemas->find(apiSchemaName);
                if (it == autoApplyAPISchemas->end()) {
                    autoApplyAPISchemas->emplace(
                        apiSchemaName, std::move(apiSchemaAutoApplyToNames));
                } else {
                    it->second.insert(it->second.end(),
                                      apiSchemaAutoApplyToNames.begin(),
                                      apiSchemaAutoApplyToNames.end());
                }
            }
        }
    }
}

static _TypeToTokenVecMap
_GetTypeToAutoAppliedAPISchemaNames()
{
    _TypeToTokenVecMap result;

    for (const auto &valuePair : UsdSchemaRegistry::GetAutoApplyAPISchemas()) {
        const TfToken &apiSchemaName = valuePair.first;
        const TfTokenVector &autoApplyToSchemas = valuePair.second;

        // Collect all the types to apply the API schema to which includes any
        // derived types of each of the listed types. 
        std::set<TfType> applyToTypes;
        for (const TfToken &schemaName : autoApplyToSchemas) {
            // The names listed are the USD type names (not the full TfType 
            // name) for abstract and concrete Typed schemas, so we need to get 
            // the actual TfType of the schema and its derived types.
            const UsdSchemaRegistry::SchemaInfo *schemaInfo =
                UsdSchemaRegistry::FindSchemaInfo(schemaName);
            if (schemaInfo) {
                if (applyToTypes.insert(schemaInfo->type).second) {
                    schemaInfo->type.GetAllDerivedTypes(&applyToTypes);
                }
            }
        }

        // With all the apply to types collected we can add the API schema to
        // the list of applied schemas for each Typed schema type.
        for (const TfType &applyToType : applyToTypes) {
            result[applyToType].push_back(apiSchemaName);
        }
    }

    // We have to sort the auto apply API schemas for each type here to be in
    // reverse "dictionary order" for two reasons.
    // 1. To ensure that if multiple versions of an API schema exist and 
    //    auto-apply to the same schema, then the latest version of the API
    //    schema that is auto-applied will always be stronger than any of the 
    //    earlier versions that are also auto-applied.
    // 2. To enforce an arbitrary, but necessary, strength ordering for auto 
    //    applied schemas that is consistent every time the schema registry is
    //    initialized. In practice, schema writers should be careful to make
    //    sure that auto applied API schemas have unique property names so that
    //    application order doesn't matter, but this at least gives us
    //    consistent behavior if property name collisions occur.
    for (auto &typeAndAutoAppliedSchemas : result) {
        Usd_SortAutoAppliedAPISchemas(&typeAndAutoAppliedSchemas.second);
    }

    return result;
}

void Usd_SortAutoAppliedAPISchemas(TfTokenVector *autoAppliedAPISchemas) {
    if (autoAppliedAPISchemas->size() < 2) {
        return;
    }
    // Sort in reverse dictionary order. This ensures that later versions of
    // a schema will always appear before earlier versions of the same schema
    // family if present in this list. Outside of this, the ordering is 
    // arbitrary.
    std::sort(autoAppliedAPISchemas->begin(), autoAppliedAPISchemas->end(),
        [](const TfToken &lhs, const TfToken &rhs) {
            return TfDictionaryLessThan()(rhs, lhs);
        });
}

// Helper class for initializing the schema registry by finding all generated 
// schema types in plugin libraries and creating the static prim definitions 
// for all concrete and applied API schema types.
class UsdSchemaRegistry::_SchemaDefInitHelper
{
public:
    _SchemaDefInitHelper(UsdSchemaRegistry *registry) 
        : _registry(registry)
    {};

    void FindAndBuildAllSchemaDefinitions()
    {
        TRACE_FUNCTION();
        // Find and load all the generated schema in plugin libraries.  We find 
        // these files adjacent to pluginfo files in libraries that provide 
        // subclasses of UsdSchemaBase.
        _InitializePrimDefsAndSchematicsForPluginSchemas();

        // Populate all applied API schema definitions second. These can 
        // include other API schemas with single apply API schemas including 
        // other single apply or instances of multiple apply API schemas, or
        // additionally, multiple apply schemas including other multiple apply
        // schemas.
        _PopulateAppliedAPIPrimDefinitions();

        // Populate all concrete API schema definitions after all API schemas
        // they may depend on have been populated.
        _PopulateConcretePrimDefinitions();
    }

private:
    using _PropNameAndPath = std::pair<TfToken, SdfPath>;
    using _PropNameAndPathVector = std::vector<_PropNameAndPath>;

    struct _PropertyPathsForSchematicsPrim {
        _PropNameAndPathVector definedProperties;
        _PropNameAndPathVector overrideProperties;
    };

    // Applied API schemas may depend on each other when building a complete
    // prim definition due to inclusion via built-in API schemas. This structure
    // helps us build nested API schema prim definitions correctly while 
    // handling possible schema inclusion cycles consistently.
    struct _APISchemaPrimDefBuilder {
        // Schema info about the schema the prim definition is for.
        const SchemaInfo *schemaInfo;

        // Index into the schematics layer vector of the layer that should 
        // hold the prim spec for this API schema.
        size_t schemaLayerIndex;

        // Raw pointer to the schema's prim definition for consistent access to
        // the prim definition regardless of whether it is owned by this
        // structure or if it has been moved to the schema registry.
        UsdPrimDefinition *primDef = nullptr;

        // Pointer to hold ownership of the prim definition while and after it 
        // is built, before it is transferred to schema registry.
        std::unique_ptr<UsdPrimDefinition> ownedPrimDef;

        // Build status flag for preventing rebuilding a prim definition 
        // over again when not needed and for cycle protection.
        enum _BuildStatus {
            NotBuilding,
            Building,
            Complete
        };
        _BuildStatus buildStatus = NotBuilding;

        // Creates and expands the prim definition for this API schema.
        bool BuildPrimDefinition(_SchemaDefInitHelper *defInitHelper);
    };

    void _InitializePrimDefsAndSchematicsForPluginSchemas();

    // Returns the list of property names that are tagged as API schema override
    // properties in the given schematics prim spec.
    static
    VtTokenArray _GetOverridePropertyNames(
        const SdfLayerRefPtr &schematicsLayer,
        const SdfPath &primSpecPath);

    // Gets the list of direct built-in API schemas from the schematics prim, 
    // plus the direct auto apply API schemas for the schema type.
    TfTokenVector _GetDirectBuiltinAPISchemas(
        const SdfLayerRefPtr &schematicsLayer,
        const SdfPath &schematicsPrimPath,
        const SchemaInfo &schemaInfo) const;

    void _PopulateAppliedAPIPrimDefinitions();
    void _PopulateConcretePrimDefinitions() const;

    UsdSchemaRegistry *_registry;

    // Map holding the builders for the API schema prim definitions that will
    // be built.
    std::unordered_map<TfToken, _APISchemaPrimDefBuilder, TfHash>
        _apiSchemaDefsToBuild;

    // A list of concrete typed schemas that will have prim definitions built
    // for them paired with the index to the schematics layer which holds the 
    // schema's prim spec.
    std::vector<std::pair<const SchemaInfo *, size_t>> _concreteSchemaDefsToBuild;

    _TypeToTokenVecMap _typeToAutoAppliedAPISchemaNames;
};

void
UsdSchemaRegistry::_SchemaDefInitHelper::
_InitializePrimDefsAndSchematicsForPluginSchemas()
{
    TRACE_FUNCTION();
    // Get all types that derive from UsdSchemaBase by getting the type map 
    // cache.
    const _TypeMapCache &typeCache = _GetTypeMapCache();

    // Gather the mapping of TfTypes to the schemas that are auto applied to
    // those types. We'll need this for building the final prim definitions.
    _typeToAutoAppliedAPISchemaNames = _GetTypeToAutoAppliedAPISchemaNames();

    // Get all the plugins that provide the types and initialize prim 
    // definitions for the found schema types.
    std::vector<std::pair<PlugPluginPtr, size_t>> plugins;
    for (const auto &valuePair : typeCache.schemaInfoByType) {
        const SchemaInfo &schemaInfo = valuePair.second;

        // Skip schema kinds that don't need a prim definition (and therefore
        // don't need a schematics layer).
        const bool needsPrimDefinition = 
            schemaInfo.kind == UsdSchemaKind::ConcreteTyped ||
            schemaInfo.kind == UsdSchemaKind::MultipleApplyAPI ||
            schemaInfo.kind == UsdSchemaKind::SingleApplyAPI;
        if (!needsPrimDefinition) {
            continue;
        }

        // Each plugin will have it's own generatedSchema layer that we'll 
        // open in parallel later. But each prim definition will need to know 
        // where to find the generatedSchema layer it needs. Thus we
        // decide now where in the generateSchemas vector each plugin's layer
        // will live (once it's loaded) so we can tell the schema builders where
        // the loaded layer will be.
        size_t generatedSchemaIndex = plugins.size();
        if (PlugPluginPtr plugin =
            PlugRegistry::GetInstance().GetPluginForType(schemaInfo.type)) {

            auto insertIt = 
                std::lower_bound(plugins.begin(), plugins.end(), plugin,
                [](const decltype(plugins)::value_type &lhs,
                   const PlugPluginPtr &rhs) {
                        return lhs.first < rhs;
                });
            if (insertIt == plugins.end() || insertIt->first != plugin) {
                plugins.insert(
                    insertIt, std::make_pair(plugin, generatedSchemaIndex));
            } else {
                generatedSchemaIndex = insertIt->second;
            }
        }

        // Add the schemas that need prim definitions to the appropriate 
        // list/map of prim definitions we need to build.
        if (schemaInfo.kind == UsdSchemaKind::ConcreteTyped) {
            _concreteSchemaDefsToBuild.emplace_back(
                &schemaInfo, generatedSchemaIndex);
        } else {
            _apiSchemaDefsToBuild.emplace(
                schemaInfo.identifier, 
                _APISchemaPrimDefBuilder{&schemaInfo, generatedSchemaIndex});
        }
    }

    // Drop the GIL if we have it, so that any tasks we create that might
    // require it (e.g. for python lifetime management on TfRefBase) won't
    // deadlock waiting for the GIL.
    TF_PY_ALLOW_THREADS_IN_SCOPE();
    
    // For each plugin, load the generated schema.
    std::vector<SdfLayerRefPtr> &generatedSchemas = 
        _registry->_schematicsLayers;
    generatedSchemas.resize(plugins.size());
    WorkWithScopedParallelism(
        [&plugins, &generatedSchemas]() {
            WorkParallelForN(
                plugins.size(), 
                [&plugins, &generatedSchemas](size_t begin, size_t end) {
                    for (; begin != end; ++begin) {
                        // We determined above where in the generatedSchemas
                        // vector each plugin's layer should live.
                        generatedSchemas[plugins[begin].second] = 
                            _GetGeneratedSchema(plugins[begin].first);
                    }
                });
        });

    for (const SdfLayerRefPtr& generatedSchema : generatedSchemas) {
        VtDictionary customDataDict = generatedSchema->GetCustomLayerData();

        bool hasErrors = false;

        // Schema generation will have added any defined fallback prim 
        // types as a dictionary in layer metadata which will be composed
        // into the single fallback types dictionary.
        VtDictionary generatedFallbackPrimTypes;
        if (generatedSchema->HasField(SdfPath::AbsoluteRootPath(), 
                                        UsdTokens->fallbackPrimTypes,
                                        &generatedFallbackPrimTypes)) {
            for (const auto &it: generatedFallbackPrimTypes) {
                if (it.second.IsHolding<VtTokenArray>()) {
                    _registry->_fallbackPrimTypes.insert(it);
                } else {
                    TF_CODING_ERROR("Found a VtTokenArray value for type "
                        "name key '%s' in fallbackPrimTypes layer metadata "
                        "dictionary in generated schema file '%s'.",
                        it.first.c_str(),
                        generatedSchema->GetRealPath().c_str());
                }
            }
        }

        if (hasErrors) {
            TF_CODING_ERROR(
                "Encountered errors in layer metadata from generated "
                "schema file '%s'. This schema must be regenerated.",
                generatedSchema->GetRealPath().c_str());
        }
    }
}

/*static*/
VtTokenArray
UsdSchemaRegistry::_SchemaDefInitHelper::_GetOverridePropertyNames(
    const SdfLayerRefPtr &schematicsLayer,
    const SdfPath &primSpecPath)
{
    static const TfToken apiSchemaOverridePropertyNamesToken(
        "apiSchemaOverridePropertyNames");

    // Override property names for a schema will be specified in the customData
    // of the schema's prim spec if there are any.
    VtTokenArray overridePropertyNames;
    schematicsLayer->HasFieldDictKey(
        primSpecPath, 
        SdfFieldKeys->CustomData, 
        apiSchemaOverridePropertyNamesToken, 
        &overridePropertyNames);
    return overridePropertyNames;
}

TfTokenVector 
UsdSchemaRegistry::_SchemaDefInitHelper::_GetDirectBuiltinAPISchemas(
    const SdfLayerRefPtr &schematicsLayer,
    const SdfPath &schematicsPrimPath,
    const SchemaInfo &schemaInfo) const
{
    TfTokenVector result;

    // Get the API schemas from the list op field in the schematics.
    SdfTokenListOp apiSchemasListOp;
    if (schematicsLayer->HasField(
            schematicsPrimPath, UsdTokens->apiSchemas, &apiSchemasListOp)) {
        apiSchemasListOp.ApplyOperations(&result);
    }

    // Check if there are any API schemas that have been setup to 
    // auto apply to this API schema type and append them to end.
    if (const TfTokenVector *autoAppliedAPIs = TfMapLookupPtr(
            _typeToAutoAppliedAPISchemaNames, schemaInfo.type)) {
        TF_DEBUG(USD_AUTO_APPLY_API_SCHEMAS).Msg(
            "The prim definition for schema type '%s' has these additional "
            "built-in auto applied API schemas: [%s].\n",
            schemaInfo.identifier.GetText(),
            TfStringJoin(autoAppliedAPIs->begin(), 
                         autoAppliedAPIs->end(), ", ").c_str());

        result.insert(result.end(), 
            autoAppliedAPIs->begin(), autoAppliedAPIs->end());
    }

    if (result.empty()) {
        return result;
    }

    // This next piece is validity checking of the directly included 
    // built-in API schemas, particularly related to restrictions on how
    // multiple apply and single apply schemas are allowed to include each
    // other. 
    // 
    // The prim definition of a multiple apply schema is actually a template
    // for applying any number of named instances of the schema to a prim 
    // definition. Thus we store the multiple apply schema definition using
    // a template name such as "MultiApplyAPI:__INSTANCE_NAME__" where 
    // "__INSTANCE_NAME__" is the placeholder that is replaced with the 
    // instance name when the schema is applied. Because of the template
    // nature of these schemas, we only allow multiple apply API schemas to
    // have built-in schemas that are also multiple apply schema templates. 
    // 
    // So "MultiApplyAPI:__INSTANCE_NAME__" may have in its 
    // appliedAPISchemas list entries like 
    // "OtherMultiApplyAPI:__INSTANCE_NAME__" (which behaves like it 
    // "inherits" OtherMultiApply) or 
    // "OtherMultiApplyAPI:__INSTANCE_NAME__:foo" (where each instance of 
    // MultiApplyAPI will contain an encapsulated instance of 
    // OtherMultiApplyAPI using the instance name template 
    // "__INSTANCE_NAME__:foo"). But they are not allowed to contain names 
    // of single apply schemas or named instances of multiple apply schemas
    // (e.g. "SingleApplyAPI" or "OtherMultiApplyAPI:bar"). 
    // 
    // On the flip side, single apply schemas can have built-in named
    // instances of multiple apply schemas (like "MultiApplyAPI:foo") but
    // cannot include the multiple apply schema templates themselves (like 
    // "MultiApplyAPI:__INSTANCE_NAME__" or
    // "MultiApplyAPI:__INSTANCE_NAME__:foo"). So before expanding our 
    // built-in API schemas we make sure that if this API schema is a 
    // multiple apply template, then each built-in schema must also be a 
    // template. Otherwise if this API schema is not a template, then each
    // built-in must also not be a template.
    //
    // Note that usdGenSchema will always generate schemas that conform to
    // this, but it's worthwhile to detect and report this invalid 
    // condition if it does occur.
    const bool isMultipleApplyTemplateSchema = 
        schemaInfo.kind == UsdSchemaKind::MultipleApplyAPI;
    const auto it = std::remove_if(
        result.begin(), result.end(),
        [isMultipleApplyTemplateSchema](const TfToken &apiSchemaName)
            { return IsMultipleApplyNameTemplate(apiSchemaName) !=
                isMultipleApplyTemplateSchema; });

    if (it != result.end()) {
        TF_WARN("Invalid inclusion of API schemas (%s) by schema "
                "'%s'. Multiple apply API schema templates can only "
                "include or be included by other multiple apply API "
                "schema templates. These schemas will not be included as "
                "built-in schemas of '%s'",
                TfStringJoin(it, result.end(), ", ").c_str(),
                schemaInfo.identifier.GetText(),
                schemaInfo.identifier.GetText());
        result.erase(it, result.end());
    }

    return result;
}

// Helper that builds a complete prim definition for an API schema. This may
// be recursive in the sense that any include built-in API schemas will also 
// be built before being composed into the definition this building.
// This returns true if the schema's prim definition is fully built to 
// completion. This returns false if a cycle is encountered that causes any of
// the included API schema built-ins to not be fully built to completion.
bool 
UsdSchemaRegistry::_SchemaDefInitHelper::_APISchemaPrimDefBuilder::
BuildPrimDefinition(_SchemaDefInitHelper *defInitHelper)
{
    // Early out if the prim definition has already been fully built for this
    // API schema.
    if (buildStatus == Complete) {
        return true;
    }

    // Mark this schema as building. This will help determine if we end up 
    // in a schema inclusion cycle.
    buildStatus = Building;

    // The schema identifier is also the name of the defining prim in the 
    // schematics layer.
    const SdfLayerRefPtr &schematicsLayer = 
        defInitHelper->_registry->_schematicsLayers[schemaLayerIndex];
    const SdfPath schematicsPrimPath = SdfPath::AbsoluteRootPath().AppendChild(
        schemaInfo->identifier);

    // Get the list of names of any override properties this schema may have as
    // we want to skip these at first when initializing the prim definition.
    const VtTokenArray overridePropertyNames = _GetOverridePropertyNames(
        schematicsLayer, schematicsPrimPath);

    // Multiple apply schemas are actually templates for creating an instance of
    // the schema so the name we need to use in its prim definition is its 
    // template name, "SchemaIdentifier:__INSTANCE_NAME__". For single apply we
    // just use the identifier.
    TfToken apiSchemaName = 
        schemaInfo->kind == UsdSchemaKind::MultipleApplyAPI ?
            MakeMultipleApplyNameTemplate(schemaInfo->identifier, "") :
            schemaInfo->identifier;

    // Create and initialize a new UsdPrimDefinition.
    // This adds the schema's defined properties into the prim definition.
    primDef = new UsdPrimDefinition();
    primDef->_IntializeForAPISchema(apiSchemaName, 
        schematicsLayer, schematicsPrimPath, 
        /* propertiesToIgnore = */ overridePropertyNames);

    // Also hold ownership of the prim definition until is either taken by the
    // registry or we delete it to rebuild the prim definition (in the case 
    // partial builds due to inclusion cycles). Note that the primDef pointer
    // will remain valid after the registry takes ownership of the definition.
    ownedPrimDef.reset(primDef);

    // Get the list of API schemas that have defined as built-ins for this prim
    // definition. This includes the API schemas included from the schematics
    // prim spec followed by any API schemas that are auto applied to this
    // prim definition's type.
    //
    // Note that this list only includes the direct built-ins and not yet any of
    // the expanded API schemas that the built-ins include.
    const TfTokenVector builtinAPISchemaNames = 
        defInitHelper->_GetDirectBuiltinAPISchemas(
            schematicsLayer,
            schematicsPrimPath, *schemaInfo);

    // If this API schema has no built-in API schemas, we're done. Mark this
    // prim definition as complete and return success.
    if (builtinAPISchemaNames.empty()) {
        buildStatus = Complete;
        return true;
    }

    // Otherwise, we have built-in API schemas. We'll need to build the, or get
    // the already built, expanded prim definition for each and compose it into
    // our prim definition. 

    // We need to keep track of the schema family and version for every direct 
    // and indirect API schema definition that we compose. This is to prevent
    // a prim definition from having more than one version of the same API 
    // schema family applied at the same time. We start by adding this schema's 
    // family and version.
    // XXX: May need to revisit what the instance name here needs to be for
    // multiple apply schemas.
    _FamilyAndInstanceToVersionMap seenAPISchemaVersions;
    seenAPISchemaVersions.emplace(
        std::make_pair(schemaInfo->family, TfToken()), schemaInfo->version);

    // Build and compose the built-in API prim definitions in strength order.
    bool foundCycle = false;
    for (const TfToken &builtinApiSchemaName : builtinAPISchemaNames) {

        // The built-in API schema name may be single apply or an instance of a 
        // multiple apply schema so we have to parse the full name into an
        // identifier (typeName) and a possible instance name.
        auto typeNameAndInstance = GetTypeNameAndInstance(builtinApiSchemaName);
        const TfToken &typeName = typeNameAndInstance.first;
        const TfToken &builtinInstanceName = typeNameAndInstance.second;

        // Look up prim definition builder for the built-in schema type. We 
        // always look up the schema in the builders as its prim definition may
        // or may not have been built yet itself.
        _APISchemaPrimDefBuilder *builtinAPIPrimDefBuilder =
             TfMapLookupPtr(defInitHelper->_apiSchemaDefsToBuild, typeName);
        if (!builtinAPIPrimDefBuilder) {
            TF_WARN("Could not find API schema definition for '%s' included by "
                    "API schema '%s'",
                    builtinApiSchemaName.GetText(), 
                    schemaInfo->identifier.GetText());
            continue;
        }

        // If the built-in API schema's prim definition is already building, 
        // then we've encountered it in a cycle where this API schema is 
        // directly or indirectly trying to include itself. Mark that we've
        // encountered a cycle and skip including this built-in schema.
        if (builtinAPIPrimDefBuilder->buildStatus == Building) {
            TF_WARN("Skipping the inclusion of the API schema definition for "
                "schema '%s' as a built-in for API schema '%s' as '%s' is "
                "being built to be included directly or indirectly by the API "
                "schema for '%s' itself. Including this schema again would "
                "result in cycle.",
                builtinApiSchemaName.GetText(),
                schemaInfo->identifier.GetText(),
                schemaInfo->identifier.GetText(),
                builtinApiSchemaName.GetText());
            foundCycle = true;
            continue;
        }

        // Try to build the fully expanded prim definition for the built-in 
        // schema. If it does not successfully complete, that means it 
        // encountered a cycle during the expansion. We don't skip the schema in
        // this case; we'll just compose in what it was able to build before it
        // had to stop for cycle prevention.
        if (!builtinAPIPrimDefBuilder->BuildPrimDefinition(defInitHelper)) {
            foundCycle = true;
        }

        // Compose in the built-in API schema's expanded prim definition for the
        // built-in instance into our prim definition.
        if (!primDef->_ComposeWeakerAPIPrimDefinition(
            *builtinAPIPrimDefBuilder->primDef, builtinInstanceName, 
            &seenAPISchemaVersions)) {
            TF_WARN("Could not add API schema definition for '%s' included by "
                    "API schema '%s'",
                    builtinApiSchemaName.GetText(), 
                    schemaInfo->identifier.GetText());
        }
    }

    // With all the built-in API schemas composed in, we can now compose any
    // API schema property overrides declared in this API schema over the 
    // defined properties.
    for (const TfToken &overridePropertyName :  overridePropertyNames) {
        primDef->_ComposeOverAndReplaceExistingProperty(
            overridePropertyName,
            schematicsLayer,
            schematicsPrimPath);
    }

    // If we found a cycle anywhere in the built-in expansion process, return 
    // the expansion status back to NotBuilding instead of Complete. This is to 
    // ensure that this API schema definition is built again from the top the
    // next time it is requested. If we don't do this the API prim definition 
    // would be inconsistent depending whether it was first built from the top
    // itself vs being built within the expansion of another API that directly
    // or indirectly includes it, given that API schema inclusions
    // can be skipped in the presence of cycles. 
    if (foundCycle) {
        buildStatus = NotBuilding;
        TF_WARN("API schema inclusion cycle encountered while building API "
            "schema definition for API schema '%s'",
            schemaInfo->identifier.GetText());
        return false;
    }

    // Otherwise we successfully completed expanding the prim definition and 
    // won't have to rebuild it again.
    buildStatus = Complete;
    return true;
}

void
UsdSchemaRegistry::_SchemaDefInitHelper::
_PopulateAppliedAPIPrimDefinitions()
{
    TRACE_FUNCTION();
    // Build each of the API schema prim definitions and add it to the registry.
    for (auto &identifierAndBuilder : _apiSchemaDefsToBuild) {
        const TfToken &schemaIdentifier = identifierAndBuilder.first;
        _APISchemaPrimDefBuilder &apiPrimDefBuilder = 
            identifierAndBuilder.second;

        // Build the prim definition for this API schema. Since API schemas may
        // include any number of other API schemas as built-ins this may build
        // those API schema definitions as well. 
        apiPrimDefBuilder.BuildPrimDefinition(this);

        // Multiple apply schemas are actually templates for creating an 
        // instance of the schema while single apply schemas cannnot be applied
        // with instances. We store this instance name requirement along with
        // the definition.
        const bool applyExpectsInstanceName = 
            apiPrimDefBuilder.schemaInfo->kind == UsdSchemaKind::MultipleApplyAPI;

        // Note, that registry takes ownership of the prim definition from
        // the builder here. The builder still retains a raw pointer to 
        // this prim definition in service of other API schemas that may need 
        // to include this schema's completed definition when building.
        _registry->_appliedAPIPrimDefinitions.emplace(schemaIdentifier, 
            _APISchemaDefinitionInfo{
                std::move(apiPrimDefBuilder.ownedPrimDef), 
                applyExpectsInstanceName});
    }
}

void 
UsdSchemaRegistry::_SchemaDefInitHelper::
_PopulateConcretePrimDefinitions() const
{
    TRACE_FUNCTION();
    // Populate all concrete API schema definitions; it is expected that all 
    // API schemas, which these may depend on, have already been populated.
    for (const auto &infoAndIndex : _concreteSchemaDefsToBuild) {
        const SchemaInfo *schemaInfo = infoAndIndex.first;

        // The schema identifier is also the name of the defining prim in the 
        // schematics layer.
        const SdfLayerRefPtr &schematicsLayer = 
            _registry->_schematicsLayers[infoAndIndex.second];
        const SdfPath schematicsPrimPath = 
            SdfPath::AbsoluteRootPath().AppendChild(schemaInfo->identifier);

        const VtTokenArray overridePropertyNames = _GetOverridePropertyNames(
            schematicsLayer, schematicsPrimPath);

        // Create and initialize a new prim definition for the concrete schema.
        // This adds the defined properties from the prim spec to the prim 
        // definition first as these are stronger than the built-in API schema 
        // properties.
        std::unique_ptr<UsdPrimDefinition> primDef(new UsdPrimDefinition());
        primDef->_IntializeForTypedSchema(
            schematicsLayer, schematicsPrimPath, overridePropertyNames);

        // Get the directly built-in and auto applied API schema for this 
        // concrete schema and compose them into the prim definition. Since all
        // API schema prim definitions have been fully  expanded; each direct 
        // built-in API schema will automatically also include every API schema
        // it includes.
        TfTokenVector apiSchemasToCompose = _GetDirectBuiltinAPISchemas(
            schematicsLayer, schematicsPrimPath, *schemaInfo);
        if (!apiSchemasToCompose.empty()) {
            // Note that we check for API schema version conflicts and will skip
            // all schemas under a directly built-in API schema if any would 
            // cause a version conflict.
            _FamilyAndInstanceToVersionMap seenSchemaFamilyVersions;
            _registry->_ComposeAPISchemasIntoPrimDefinition(
                primDef.get(), apiSchemasToCompose, &seenSchemaFamilyVersions);
        }

        // With all the built-in API schemas applied, we can now compose any
        // API schema property overrides declared in the typed schema over the 
        // current defined properties.
        for (const TfToken &overridePropertyName : overridePropertyNames) {
            primDef->_ComposeOverAndReplaceExistingProperty(
                overridePropertyName,
                schematicsLayer,
                schematicsPrimPath);
        }

        // Move the completed definition into the registry.
        _registry->_concreteTypedPrimDefinitions.emplace(
            schemaInfo->identifier, std::move(primDef));
    }
}

UsdSchemaRegistry::UsdSchemaRegistry()
{
    _emptyPrimDefinition = new UsdPrimDefinition();

    // Find and load all the generated schema in plugin libraries and build all
    // the schema prim definitions.
    if (!TfGetEnvSetting(USD_DISABLE_PRIM_DEFINITIONS_FOR_USDGENSCHEMA)) {
        _SchemaDefInitHelper schemaDefHelper(this);
        schemaDefHelper.FindAndBuildAllSchemaDefinitions();
    }

    TfSingleton<UsdSchemaRegistry>::SetInstanceConstructed(*this);
    TfRegistryManager::GetInstance().SubscribeTo<UsdSchemaRegistry>();
}

/*static*/
bool 
UsdSchemaRegistry::IsDisallowedField(const TfToken &fieldName)
{
    static auto &disallowedFields = *[]() {
        auto *disallowedFields = new TfHashSet<TfToken, TfToken::HashFunctor>;

        // Disallow fallback values for composition arc fields, since they
        // won't be used during composition.
        disallowedFields->insert(SdfFieldKeys->InheritPaths);
        disallowedFields->insert(SdfFieldKeys->Payload);
        disallowedFields->insert(SdfFieldKeys->References);
        disallowedFields->insert(SdfFieldKeys->Specializes);
        disallowedFields->insert(SdfFieldKeys->VariantSelection);
        disallowedFields->insert(SdfFieldKeys->VariantSetNames);

        // Disallow customData, since it contains information used by
        // usdGenSchema that isn't relevant to other consumers.
        disallowedFields->insert(SdfFieldKeys->CustomData);

        // Disallow fallback values for these fields, since they won't be
        // used during scenegraph population or value resolution.
        disallowedFields->insert(SdfFieldKeys->Active);
        disallowedFields->insert(SdfFieldKeys->Instanceable);
        disallowedFields->insert(SdfFieldKeys->TimeSamples);
        disallowedFields->insert(SdfFieldKeys->ConnectionPaths);
        disallowedFields->insert(SdfFieldKeys->TargetPaths);

        // Disallow fallback values for specifier. Even though it will always
        // be present, it has no meaning as a fallback value.
        disallowedFields->insert(SdfFieldKeys->Specifier);

        // Disallow fallback values for children fields.
        disallowedFields->insert(SdfChildrenKeys->allTokens.begin(),
                                 SdfChildrenKeys->allTokens.end());

        // Disallow fallback values for clip-related fields, since they won't
        // be used during value resolution.
        const std::vector<TfToken> clipFields = UsdGetClipRelatedFields();
        disallowedFields->insert(clipFields.begin(), clipFields.end());

        // Disallow fallback values for prim "kind" metadata as prim composition
        // currently intentionally ignores the "kind" fallback value
        disallowedFields->insert(SdfFieldKeys->Kind);

        return disallowedFields;
    }();

    return (disallowedFields.find(fieldName) != disallowedFields.end());
}

/*static*/
bool 
UsdSchemaRegistry::IsTyped(const TfType& primType)
{
    return primType.IsA<UsdTyped>();
}

TfType
UsdSchemaRegistry::GetTypeFromName(const TfToken& typeName){
    static const TfType schemaBaseType = TfType::Find<UsdSchemaBase>();
    return PlugRegistry::GetInstance().FindDerivedTypeByName(
        schemaBaseType, typeName.GetString());
}

std::pair<TfToken, TfToken>
UsdSchemaRegistry::GetTypeNameAndInstance(const TfToken &apiSchemaName)
{
    // Try to split the string at the first namespace delimiter. We always use
    // the first as type names can not have embedded namespaces but instances 
    // names can.
    const char namespaceDelimiter =
        SdfPathTokens->namespaceDelimiter.GetText()[0];
    const std::string &typeString = apiSchemaName.GetString();
    size_t delim = typeString.find(namespaceDelimiter);
    // If the delimiter is not found, we have a single apply API schema and 
    // no instance name.
    if (delim == std::string::npos) {
        return std::make_pair(apiSchemaName, TfToken());
    } else {
        return std::make_pair(TfToken(typeString.substr(0, delim)),
                              TfToken(typeString.c_str() + delim + 1));
    }
}

/*static*/
const UsdSchemaRegistry::TokenToTokenVectorMap &
UsdSchemaRegistry::GetAutoApplyAPISchemas()
{
    return _GetAPISchemaApplyToInfoCache().autoApplyAPISchemasMap;
}

/*static*/
bool 
UsdSchemaRegistry::IsAllowedAPISchemaInstanceName(
    const TfToken &apiSchemaName, const TfToken &instanceName)
{
    // Verify we have a multiple apply API schema and a non-empty instance name.
    if (instanceName.IsEmpty() || !IsMultipleApplyAPISchema(apiSchemaName)) {
        return false;
    }

    // A multiple apply schema may specify a list of instance names that
    // are allowed for it. If so we check for that here. If no list of 
    // instance names exist or it is empty, then any valid instance name is 
    // allowed.
    const TfHashMap<TfToken, TfToken::Set, TfHash> &allowedInstanceNamesMap = 
        _GetAPISchemaApplyToInfoCache().allowedInstanceNamesMap;
    if (const TfToken::Set *allowedInstanceNames = 
            TfMapLookupPtr(allowedInstanceNamesMap, apiSchemaName)) {
        if (!allowedInstanceNames->empty() && 
            !allowedInstanceNames->count(instanceName)) {
            return false;
        }
    }

    // In all cases, we don't allow instance names whose base name matches the 
    // name of a property of the API schema. We check the prim definition for 
    // this.
    const UsdPrimDefinition *apiSchemaDef = 
        GetInstance().FindAppliedAPIPrimDefinition(apiSchemaName);
    if (!apiSchemaDef) {
        TF_CODING_ERROR("Could not find UsdPrimDefinition for multiple apply "
                        "API schema '%s'", apiSchemaName.GetText());
        return false;
    }

    const TfTokenVector tokens = 
        SdfPath::TokenizeIdentifierAsTokens(instanceName);
    if (tokens.empty()) {
        return false;
    }

    const TfToken &baseName = tokens.back();
    // Since the property names for multiple apply schemas will have an 
    // instanceable template prefix we need to check against the computed base 
    // name for each of the schema's properties.
    // Note that we have to check against the base name of each property (as 
    // opposed to prepending the template prefix to the name and searching for
    // that in the properties map) because we can't guarantee that all 
    // properties will have the same prefix if they come from another built-in
    // multiple apply API schema.
    for (const TfToken &propName : apiSchemaDef->GetPropertyNames()) {
        const TfToken propBaseName = 
            GetMultipleApplyNameTemplateBaseName(propName);
        if (baseName == propBaseName) {
            return false;
        }
    }

    return true;
}

const TfTokenVector &
UsdSchemaRegistry::GetAPISchemaCanOnlyApplyToTypeNames(
    const TfToken &apiSchemaName, const TfToken &instanceName)
{
    const TokenToTokenVectorMap &canOnlyApplyToMap = 
        _GetAPISchemaApplyToInfoCache().canOnlyApplyAPISchemasMap;

    if (!instanceName.IsEmpty()) {
        // It's possible that specific instance names of the schema can only be 
        // applied to the certain types. If a list of "can only apply to" types
        // is exists for the given instance, we use it.
        TfToken fullApiSchemaName(
            SdfPath::JoinIdentifier(apiSchemaName, instanceName));
        if (const TfTokenVector *canOnlyApplyToTypeNames = 
            TfMapLookupPtr(canOnlyApplyToMap, fullApiSchemaName)) {
            return *canOnlyApplyToTypeNames;
        }
    }

    // Otherwise, no there's no instance specific list, so try to find one just
    // from the API schema type name.
    if (const TfTokenVector *canOnlyApplyToTypeNames = 
        TfMapLookupPtr(canOnlyApplyToMap, apiSchemaName)) {
        return *canOnlyApplyToTypeNames;
    }

    static const TfTokenVector empty;
    return empty;
}

std::unique_ptr<UsdPrimDefinition>
UsdSchemaRegistry::BuildComposedPrimDefinition(
    const TfToken &primType, const TfTokenVector &appliedAPISchemas) const
{
    if (appliedAPISchemas.empty()) {
        TF_CODING_ERROR("BuildComposedPrimDefinition without applied API "
                        "schemas is not allowed. If you want a prim definition "
                        "for a single prim type with no appied schemas, use "
                        "FindConcretePrimDefinition instead.");
        return nullptr;
    }

   _FamilyAndInstanceToVersionMap seenSchemaFamilyVersions;

    // Find the existing concrete typed prim definition for the prim's type.
    const UsdPrimDefinition *primDef = FindConcretePrimDefinition(primType);
    // Make a copy of the typed prim definition to start.
    // Its perfectly valid for there to be no prim definition for the 
    // given prim type, in which case we compose API schemas into an empty
    // prim definition.
    std::unique_ptr<UsdPrimDefinition> composedPrimDef(
        primDef ? new UsdPrimDefinition(*primDef) : new UsdPrimDefinition());

    // We do not allow authored API schemas to cause a different version of an
    // API schema to be added if an API schema in that family is already 
    // built in to the prim type's prim definition. Thus, we have to populate
    // the seen schema family versions with API schemas found in the prim type's
    // definition before trying to add any authored API schemas.
    for (const TfToken &apiSchema : composedPrimDef->_appliedAPISchemas) {
        // Applied schemas may be single or multiple apply so we have to parse
        // the full schema name into a type and possibly an instance name.
        std::pair<TfToken, TfToken> typeNameAndInstance = 
            UsdSchemaRegistry::GetTypeNameAndInstance(apiSchema);
        const UsdSchemaRegistry::SchemaInfo *schemaInfo = 
            UsdSchemaRegistry::FindSchemaInfo(typeNameAndInstance.first);
        typeNameAndInstance.first = schemaInfo->family;

        seenSchemaFamilyVersions.emplace(
            std::move(typeNameAndInstance), schemaInfo->version);
    }

    // We compose in the weaker authored API schemas.
    _ComposeAPISchemasIntoPrimDefinition(
        composedPrimDef.get(), appliedAPISchemas, &seenSchemaFamilyVersions);
           
    return composedPrimDef;
}

void 
UsdSchemaRegistry::_ComposeAPISchemasIntoPrimDefinition(
    UsdPrimDefinition *primDef, 
    const TfTokenVector &appliedAPISchemas,
    _FamilyAndInstanceToVersionMap *seenSchemaFamilyVersions) const
{
    // Add in properties from each new applied API schema. Applied API schemas 
    // are ordered strongest to weakest so we compose, in order, each weaker 
    // schema's properties.
    for (const TfToken &apiSchemaName : appliedAPISchemas) {
        // Applied schemas may be single or multiple apply so we have to parse
        // the full schema name into a type and possibly an instance name.
        auto typeNameAndInstance = GetTypeNameAndInstance(apiSchemaName);
        const TfToken &typeName = typeNameAndInstance.first;
        const TfToken &instanceName = typeNameAndInstance.second;

        if (const _APISchemaDefinitionInfo *apiSchemaDefInfo = 
                TfMapLookupPtr(_appliedAPIPrimDefinitions, typeName)) {
            // Multiple apply schemas must always be applied with an instance 
            // name while single apply schemas must never have an instance
            // name. Skip the API schema def if the presence of an instance 
            // does not match what we expect for the applied schema.
            const bool hasInstanceName = !instanceName.IsEmpty();
            if (apiSchemaDefInfo->applyExpectsInstanceName != hasInstanceName) {
                TF_WARN("API schema '%s' can not be added to a prim definition "
                    "%s an instance name.",
                    apiSchemaName.GetText(),
                    apiSchemaDefInfo->applyExpectsInstanceName ? 
                        "without" : "with");
                continue;
            }

            primDef->_ComposeWeakerAPIPrimDefinition(
                *apiSchemaDefInfo->primDef, instanceName, 
                seenSchemaFamilyVersions);
        }
    }
}

void 
Usd_GetAPISchemaPluginApplyToInfoForType(
    const TfType &apiSchemaType,
    const TfToken &apiSchemaName,
    UsdSchemaRegistry::TokenToTokenVectorMap *autoApplyAPISchemasMap,
    UsdSchemaRegistry::TokenToTokenVectorMap *canOnlyApplyAPISchemasMap,
    TfHashMap<TfToken, TfToken::Set, TfHash> *allowedInstanceNamesMap)
{
    PlugPluginPtr plugin =
        PlugRegistry::GetInstance().GetPluginForType(apiSchemaType);
    if (!plugin) {
        TF_CODING_ERROR("Failed to find plugin for schema type '%s'",
                        apiSchemaType.GetTypeName().c_str());
        return;
    }

    // We don't load the plugin, we just use its metadata.
    const JsObject dict = plugin->GetMetadataForType(apiSchemaType);

    // Skip types that aren't applied API schemas
    const UsdSchemaKind schemaKind = _GetSchemaKindFromMetadata(dict);
    if (!_IsAppliedAPISchemaKind(schemaKind)) {
        return;
    }

    // Both single and multiple apply API schema types can have metadata 
    // specifying the list that the type can only be applied to.
    TfTokenVector canOnlyApplyToTypeNames =
        _GetNameListFromMetadata(dict, _tokens->apiSchemaCanOnlyApplyTo);
    if (!canOnlyApplyToTypeNames.empty()) {
        (*canOnlyApplyAPISchemasMap)[apiSchemaName] = 
            std::move(canOnlyApplyToTypeNames);
    }

    if (schemaKind == UsdSchemaKind::SingleApplyAPI) {
        // Skip if auto apply API schemas have been disabled.
        if (TfGetEnvSetting(USD_DISABLE_AUTO_APPLY_API_SCHEMAS)) {
            return;
        }

        // For single apply API schemas, we can get the types it should auto
        // apply to.
        TfTokenVector autoApplyToTypeNames =
            _GetNameListFromMetadata(dict, _tokens->apiSchemaAutoApplyTo);
        if (!autoApplyToTypeNames.empty()) {
             TF_DEBUG(USD_AUTO_APPLY_API_SCHEMAS).Msg(
                 "API schema '%s' is defined to auto apply to the following "
                 "schema types: [%s].\n",
                 apiSchemaName.GetText(),
                 TfStringJoin(autoApplyToTypeNames.begin(),
                              autoApplyToTypeNames.end(), ", ").c_str());
            (*autoApplyAPISchemasMap)[apiSchemaName] =
                std::move(autoApplyToTypeNames);
        }
    } else {
        // For multiple apply schemas, the metadata may specify a list of 
        // allowed instance names.
        TfTokenVector allowedInstanceNames =
            _GetNameListFromMetadata(dict, _tokens->apiSchemaAllowedInstanceNames);
        if (!allowedInstanceNames.empty()) {
            (*allowedInstanceNamesMap)[apiSchemaName].insert(
                allowedInstanceNames.begin(), allowedInstanceNames.end());
        }

        // Multiple apply API schema metadata may specify a dictionary of 
        // additional apply info for individual instance names. Right now this
        // will only contain additional "can only apply to" types for individual
        // instances names, but in the future we can add auto-apply metadata
        // here as well.
        const JsValue *apiSchemaInstancesValue = 
            TfMapLookupPtr(dict, _tokens->apiSchemaInstances);
        if (!apiSchemaInstancesValue) {
            return;
        }

        if (!apiSchemaInstancesValue->IsObject()) {
            TF_CODING_ERROR("Metadata value for key '%s' for API schema type "
                            "'%s' is not holding a dictionary. PlugInfo may "
                            "need to be regenerated.",
                            _tokens->apiSchemaInstances.GetText(),
                            apiSchemaName.GetText());
            return;
        }

        // For each instance name in the metadata dictionary we grab any 
        // "can only apply to" types specified for and add it to 
        // "can only apply to" types map under the fully joined API schema name
        for (const auto &entry : apiSchemaInstancesValue->GetJsObject()) {
            const std::string &instanceName = entry.first;

            if (!entry.second.IsObject()) {
                TF_CODING_ERROR("%s value for instance name '%s' for API "
                                "schema type '%s' is not holding a dictionary. "
                                "PlugInfo may need to be regenerated.",
                                _tokens->apiSchemaInstances.GetText(),
                                instanceName.c_str(),
                                apiSchemaName.GetText());
                continue;
            }
            const JsObject &instanceDict = entry.second.GetJsObject();

            const TfToken schemaInstanceName(
                SdfPath::JoinIdentifier(apiSchemaName, instanceName));

            TfTokenVector instanceCanOnlyApplyToTypeNames =
                _GetNameListFromMetadata(instanceDict, 
                                         _tokens->apiSchemaCanOnlyApplyTo);
            if (!instanceCanOnlyApplyToTypeNames.empty()) {
                (*canOnlyApplyAPISchemasMap)[schemaInstanceName] = 
                    std::move(instanceCanOnlyApplyToTypeNames);
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

