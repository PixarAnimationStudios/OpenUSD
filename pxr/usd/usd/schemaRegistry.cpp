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
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/withScopedParallelism.h"

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

namespace {
// Helper struct for caching a bidirecional mapping between schema TfType and
// USD type name token. This cache is used as a static local instance providing
// this type mapping without having to build the entire schema registry
struct _TypeMapCache {
    _TypeMapCache() {
        const TfType schemaBaseType = TfType::Find<UsdSchemaBase>();

        auto _MapDerivedTypes = [this, &schemaBaseType](
            const TfType &baseType, bool isTyped) 
        {
            set<TfType> types;
            PlugRegistry::GetAllDerivedTypes(baseType, &types);
            for (const TfType &type : types) {
                // The USD type name is the type's alias under UsdSchemaBase. 
                // All schemas should have a type name alias.
                const vector<string> aliases = schemaBaseType.GetAliases(type);
                if (aliases.size() == 1) {
                    TfToken typeName(aliases.front(), TfToken::Immortal);
                    nameToType.insert(std::make_pair(
                        typeName, TypeInfo(type, isTyped)));
                    typeToName.insert(std::make_pair(
                        type, TypeNameInfo(typeName, isTyped)));
                }
            }
        };

        _MapDerivedTypes(TfType::Find<UsdTyped>(), /*isTyped=*/true);
        _MapDerivedTypes(TfType::Find<UsdAPISchemaBase>(), /*isTyped=*/false);
    }

    // For each type and type name mapping we also want to store if it's a 
    // concrete prim type vs an API schema type. 
    struct TypeInfo {
        TfType type;
        bool isTyped;
        TypeInfo(const TfType &type_, bool isTyped_) 
            : type(type_), isTyped(isTyped_) {}
    };

    struct TypeNameInfo {
        TfToken name;
        bool isTyped;
        TypeNameInfo(const TfToken &name_, bool isTyped_) 
            : name(name_), isTyped(isTyped_) {}
    };

    TfHashMap<TfToken, TypeInfo, TfHash> nameToType;
    TfHashMap<TfType, TypeNameInfo, TfHash> typeToName;
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
        for (const auto &valuePair : typeCache.typeToName) {
            const TfType &type = valuePair.first;
            const TfToken &typeName = valuePair.second.name;

            Usd_GetAPISchemaPluginApplyToInfoForType(
                type,
                typeName,
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
    std::map<TfToken, TfTokenVector> autoApplyAPISchemasMap;

    // Mapping of API schema type name to a list of prim type names that it
    // is ONLY allowed to be applied to.
    TfHashMap<TfToken, TfTokenVector, TfHash> canOnlyApplyAPISchemasMap;

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
_IsMultipleApplySchemaKind(const UsdSchemaKind schemaKind)
{
    return schemaKind == UsdSchemaKind::MultipleApplyAPI;
}

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
    PlugPluginPtr plugin =
        PlugRegistry::GetInstance().GetPluginForType(schemaType);
    if (!plugin) {
        TF_CODING_ERROR("Failed to find plugin for schema type '%s'",
                        schemaType.GetTypeName().c_str());
        return UsdSchemaKind::Invalid;
    }

    return _GetSchemaKindFromMetadata(plugin->GetMetadataForType(schemaType));
}

/*static*/
TfToken 
UsdSchemaRegistry::GetSchemaTypeName(const TfType &schemaType) 
{
    const _TypeMapCache & typeMapCache = _GetTypeMapCache();
    auto it = typeMapCache.typeToName.find(schemaType);
    return it != typeMapCache.typeToName.end() ? it->second.name : TfToken();
}

/*static*/
TfToken 
UsdSchemaRegistry::GetConcreteSchemaTypeName(const TfType &schemaType) 
{
    const _TypeMapCache & typeMapCache = _GetTypeMapCache();
    auto it = typeMapCache.typeToName.find(schemaType);
    if (it != typeMapCache.typeToName.end() &&
        it->second.isTyped && 
        _IsConcreteSchemaKind(_GetSchemaKindFromPlugin(schemaType))) {
        return it->second.name;
    }
    return TfToken();
}

/*static*/
TfToken 
UsdSchemaRegistry::GetAPISchemaTypeName(const TfType &schemaType) 
{
    const _TypeMapCache & typeMapCache = _GetTypeMapCache();
    auto it = typeMapCache.typeToName.find(schemaType);
    return it != typeMapCache.typeToName.end() && !it->second.isTyped ? 
        it->second.name : TfToken();
}

/*static*/
TfType 
UsdSchemaRegistry::GetTypeFromSchemaTypeName(const TfToken &typeName) 
{
    const _TypeMapCache & typeMapCache = _GetTypeMapCache();
    auto it = typeMapCache.nameToType.find(typeName);
    return it != typeMapCache.nameToType.end() ? it->second.type : TfType();
}

/*static*/
TfType 
UsdSchemaRegistry::GetConcreteTypeFromSchemaTypeName(const TfToken &typeName) 
{
    const _TypeMapCache & typeMapCache = _GetTypeMapCache();
    auto it = typeMapCache.nameToType.find(typeName);
    if (it != typeMapCache.nameToType.end() && 
        it->second.isTyped && 
        _IsConcreteSchemaKind(_GetSchemaKindFromPlugin(it->second.type))) {
        return it->second.type; 
    }
    return TfType();
}

/*static*/
TfType 
UsdSchemaRegistry::GetAPITypeFromSchemaTypeName(const TfToken &typeName) 
{
    const _TypeMapCache & typeMapCache = _GetTypeMapCache();
    auto it = typeMapCache.nameToType.find(typeName);
    return it != typeMapCache.nameToType.end() && !it->second.isTyped ? 
        it->second.type : TfType();
}

/*static*/
UsdSchemaKind 
UsdSchemaRegistry::GetSchemaKind(const TfType &schemaType)
{
    const _TypeMapCache & typeMapCache = _GetTypeMapCache();
    auto it = typeMapCache.typeToName.find(schemaType);
    if (it == typeMapCache.typeToName.end()) {
        // No schema kind because it is not a schema type.
        return UsdSchemaKind::Invalid;
    }
    // Is a valid schema type.
    return _GetSchemaKindFromPlugin(schemaType);
}

/*static*/
UsdSchemaKind 
UsdSchemaRegistry::GetSchemaKind(const TfToken &typeName)
{
    const _TypeMapCache & typeMapCache = _GetTypeMapCache();
    auto it = typeMapCache.nameToType.find(typeName);
    if (it == typeMapCache.nameToType.end()) {
        // No schema kind because it is not a schema type.
        return UsdSchemaKind::Invalid;
    }
    // Is a valid schema type.
    return _GetSchemaKindFromPlugin(it->second.type);
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

template <class T>
static void
_CopySpec(const T &srcSpec, const T &dstSpec)
{
    TRACE_FUNCTION();
    for (const TfToken& key : srcSpec->ListFields()) {
        if (!UsdSchemaRegistry::IsDisallowedField(key)) {
            dstSpec->SetInfo(key, srcSpec->GetInfo(key));
        }
    }
}

static void
_CopyAttrSpec(const SdfAttributeSpecHandle &srcAttr, 
              const SdfPrimSpecHandle &dstPrim,
              const std::string &dstName)
{
    SdfAttributeSpecHandle newAttr =
        SdfAttributeSpec::New(
            dstPrim, dstName, srcAttr->GetTypeName(),
            srcAttr->GetVariability(), srcAttr->IsCustom());
    _CopySpec(srcAttr, newAttr);
}

static void
_CopyRelSpec(const SdfRelationshipSpecHandle &srcRel, 
             const SdfPrimSpecHandle &dstPrim,
             const std::string &dstName)
{
    SdfRelationshipSpecHandle newRel =
        SdfRelationshipSpec::New(
            dstPrim, dstName, srcRel->IsCustom());
    _CopySpec(srcRel, newRel);
}

static void
_CopyPropSpec(const SdfPropertySpecHandle &srcProp, 
              const SdfPrimSpecHandle &dstPrim,
              const std::string &dstName)
{
    if (SdfAttributeSpecHandle attr = 
            TfDynamic_cast<SdfAttributeSpecHandle>(srcProp)) {
        _CopyAttrSpec(attr, dstPrim, dstName);
    } else if (SdfRelationshipSpecHandle attr = 
            TfDynamic_cast<SdfRelationshipSpecHandle>(srcProp)) {
        _CopyRelSpec(attr, dstPrim, dstName);
    } else {
        TF_CODING_ERROR("Property spec is neither an attribute or a relationship");
    }
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
    std::map<TfToken, TfTokenVector> *autoApplyAPISchemas)
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

    const _TypeMapCache & typeMapCache = _GetTypeMapCache();

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
            const auto it = typeMapCache.nameToType.find(schemaName);
            if (it != typeMapCache.nameToType.end()) {
                const TfType &schemaType = it->second.type;
                if (applyToTypes.insert(schemaType).second) {
                    schemaType.GetAllDerivedTypes(&applyToTypes);
                }
            }
        }

        // With all the apply to types collected we can add the API schema to
        // the list of applied schemas for each Typed schema type.
        // 
        // Note that the auto apply API schemas map is sorted alphabetically by 
        // API schema name so this list for each prim type will also be sorted
        // alphabetically which is intentional. This ordering is arbitrary but 
        // necessary to ensure we get a consistent strength ordering for auto 
        // applied schemas every time. In practice, schema writers should be 
        // careful to make sure that auto applied API schemas have unique 
        // property names so that application order doesn't matter, but this at 
        // least gives us consistent behavior if property name collisions occur.
        for (const TfType &applyToType : applyToTypes) {
            result[applyToType].push_back(apiSchemaName);
        }
    }

    return result;
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

    using _PropNameAndPathsToCompose = std::pair<TfToken, SdfPathVector>;
    using _PropNameAndPathsToComposeVector = 
        std::vector<_PropNameAndPathsToCompose>;

    // Applied API schemas require some processing to determine the entire
    // expanded list of included API schemas that need to be composed into its 
    // prim definition. This structure is used to hold this info.
    struct _BuiltinAPISchemaExpansionInfo {

        // The API schema prim definition that will be expanded in place.
        UsdPrimDefinition *primDef;

        // The expanded list of names of all API schemas that will be present
        // in the final prim definition.
        TfTokenVector allAPISchemaNames;
    };

    // Built-in API schemas are expanded recursively. This is the information
    // about the built-in API schema that is is passed to each step.
    struct _BuiltinAPISchemaInfo {
        // The prim definition for the built-in API schema's type.
        const UsdPrimDefinition *apiSchemaDef;

        // The instance name of the built-in API schema (for multiple apply API
        // schemas only).
        TfToken instanceName;

        // Pointer to the built-in schema info that caused this API schema to 
        // be included. Used for cycle detection.
        const _BuiltinAPISchemaInfo *includedBy;

        // Composition of API schema property overrides only happen across
        // an individual branch of an built-in API schema inclusion hierarchy
        // (i.e. sibling built-ins cannot compose in overrides to each others
        // properties). Thus we have to maintain a stack of the found property
        // overrides during built-in expansion through passing them in this
        // schema info bundle.
        _PropNameAndPathsToComposeVector *propsWithOversToComposePtr;

        // Checks if this info's apiSchemaDef would cause a cycle by checking
        // if it matches any of the recursively expanded API definitions that 
        // caused it to be included.
        bool CheckForCycle() const {
            const _BuiltinAPISchemaInfo *info = includedBy;
            while (info) {
                if (apiSchemaDef == info->apiSchemaDef) {
                    return true;
                }
                info = info->includedBy;
            }
            return false;
        }
    };

    void _InitializePrimDefsAndSchematicsForPluginSchemas();

    void _AddSchemaToSchematics(
        SdfLayerRefPtr const &source);
    void _AddOverridePropertyNamesFromSourceSpec(
        const SdfPrimSpecHandle &prim);

    void _PrependAPISchemasFromSchemaPrim(
        const SdfPath &schematicsPrimPath,
        TfTokenVector *appliedAPISchemas) const;

    void _ExpandBuiltinAPISchemasRecursive(
        const _BuiltinAPISchemaInfo &builtinAPISchema,
        _BuiltinAPISchemaExpansionInfo *expansionInfo) const;

    void _AddSchemaSpecPropertiesAndUpdateOversToCompose(
        const _BuiltinAPISchemaInfo &includedSchemaInfo,
        UsdPrimDefinition *primDef,
        _PropNameAndPathsToComposeVector *propsWithOversToCompose) const;

    _PropNameAndPathVector _GetPropertyPathsForSpec(
        const SdfPath &primSpecPath, 
        _PropNameAndPathVector *overrideProperties = nullptr) const;

    _PropNameAndPathVector _GetPropertyPathsForSpec(
        const SdfPath &primSpecPath, 
        const TfToken &instanceName,
        _PropNameAndPathVector *overrideProperties = nullptr) const;

    void _ComposePropertiesWithOverrides(
        UsdPrimDefinition *primDef,
        _PropNameAndPathsToComposeVector *propsWithOversToCompose) const;

    void _PopulateAppliedAPIPrimDefinitions() const;
    void _PopulateConcretePrimDefinitions() const;

    UsdSchemaRegistry *_registry;

    // Storage for the override property names that may be defined for each
    // schema. See _AddOverridePropertyNamesFromSourceSpec for details.
    std::unordered_map<TfToken, VtTokenArray, TfHash> 
        _overridePropertyNamesPerSchema;
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
    // those types. We need this before initializing our prim definitions. Note
    // the prim definitions will take the API schema lists from this map in the
    // following loop.
    _TypeToTokenVecMap typeToAutoAppliedAPISchemaNames =
        _GetTypeToAutoAppliedAPISchemaNames();

    // Get all the plugins that provide the types and initialize prim defintions
    // for the found schema types.
    std::vector<PlugPluginPtr> plugins;
    for (const auto &valuePair : typeCache.typeToName) {
        const TfType &type = valuePair.first;
        const TfToken &typeName = valuePair.second.name;

        if (PlugPluginPtr plugin =
            PlugRegistry::GetInstance().GetPluginForType(type)) {

            auto insertIt = 
                std::lower_bound(plugins.begin(), plugins.end(), plugin);
            if (insertIt == plugins.end() || *insertIt != plugin) {
                plugins.insert(insertIt, plugin);
            }
        }

        // We register prim definitions by the schema type name which we already
        // grabbed from the TfType alias, and is also the name of the defining 
        // prim in the schema layer. The actual TfType's typename 
        // (i.e. C++ type name) is not a valid typename for a prim.
        SdfPath schematicsPrimPath = 
            SdfPath::AbsoluteRootPath().AppendChild(typeName);

        // Create a new UsdPrimDefinition for this type in the appropriate map 
        // based on the type's schema kind. The prim definitions are not fully 
        // populated by the schematics here; we'll populate all these prim 
        // definitions in the rest FindAndBuildAllSchemaDefinitions.
        const UsdSchemaKind schemaKind = _GetSchemaKindFromPlugin(type);
        if (_IsConcreteSchemaKind(schemaKind)) {
            UsdPrimDefinition *newPrimDef = new UsdPrimDefinition(
                schematicsPrimPath, /* isAPISchema = */ false);
            _registry->_concreteTypedPrimDefinitions.emplace(
                typeName, newPrimDef);
            // Check if there are any API schemas that have been setup to auto 
            // apply to this type. We'll set these in the prim definition's 
            // applied API schemas list so they can be processed when building 
            // out this prim definition in _PopulateConcretePrimDefinitions.
            if (TfTokenVector *autoAppliedAPIs = 
                    TfMapLookupPtr(typeToAutoAppliedAPISchemaNames, type)) {
                TF_DEBUG(USD_AUTO_APPLY_API_SCHEMAS).Msg(
                    "The prim definition for schema type '%s' has "
                    "these additional built-in auto applied API "
                    "schemas: [%s].\n",
                    typeName.GetText(),
                    TfStringJoin(autoAppliedAPIs->begin(), 
                                 autoAppliedAPIs->end(), ", ").c_str());

                // Note that we take the API schema list from the map as the 
                // map was only created help populate these prim definitions.
                newPrimDef->_appliedAPISchemas = std::move(*autoAppliedAPIs);
            }
        } else if (_IsAppliedAPISchemaKind(schemaKind)) {
            UsdPrimDefinition *newPrimDef = new UsdPrimDefinition(
                schematicsPrimPath, /* isAPISchema = */ true);
            if (_IsMultipleApplySchemaKind(schemaKind)) {
                // Multiple apply schemas are actually templates for creating
                // an instance of the schema. We store the prim definition
                // in the applied API definitions map using its template name
                // which is "SchemaName:__INSTANCE_NAME__"
                const TfToken typeNameTemplate = 
                    MakeMultipleApplyNameTemplate(typeName, "");
                _registry->_appliedAPIPrimDefinitions.emplace(
                    typeNameTemplate, newPrimDef);

                // We also store a separate mapping of the multiple apply schema
                // name to the same template prim definition for easy lookup by 
                // type name. Note that we can store these by raw pointer here
                // because the prim definition will be populated in place.
                _registry->_multiApplyAPIPrimDefinitions.emplace(
                    typeName, newPrimDef);

                // Note that all typed and applied API schemas can include 
                // built-in API schemas, but, unlike with single apply and typed
                // schemas, schemas cannot be auto-applied to multiple apply API
                // schemas.
            } else {
                _registry->_appliedAPIPrimDefinitions.emplace(
                    typeName, newPrimDef);

                // Check if there are any API schemas that have been setup to 
                // auto apply to this API schema type. We'll set these in the 
                // prim definition's applied API schemas list so they can be 
                // processed when building out this prim definition in 
                // _PopulateAppliedAPIPrimDefinitions.
                if (TfTokenVector *autoAppliedAPIs = 
                        TfMapLookupPtr(typeToAutoAppliedAPISchemaNames, type)) {
                    TF_DEBUG(USD_AUTO_APPLY_API_SCHEMAS).Msg(
                        "The prim definition for API schema type '%s' has "
                        "these additional built-in auto applied API "
                        "schemas: [%s].\n",
                        typeName.GetText(),
                        TfStringJoin(autoAppliedAPIs->begin(), 
                                     autoAppliedAPIs->end(), ", ").c_str());

                    // Note that we take the API schema list from the map as the 
                    // map was only created help populate these prim definitions.
                    newPrimDef->_appliedAPISchemas = std::move(*autoAppliedAPIs);
                }
            }
        } 
    }

    // For each plugin, if it has generated schema, add it to the schematics.
    std::vector<SdfLayerRefPtr> generatedSchemas(plugins.size());
    WorkWithScopedParallelism([&plugins, &generatedSchemas]() {
            WorkParallelForN(
                plugins.size(), 
                [&plugins, &generatedSchemas](size_t begin, size_t end) {
                    for (; begin != end; ++begin) {
                        generatedSchemas[begin] = 
                            _GetGeneratedSchema(plugins[begin]);
                    }
                });
        });

    SdfChangeBlock block;

    for (const SdfLayerRefPtr& generatedSchema : generatedSchemas) {
        if (generatedSchema) {
            VtDictionary customDataDict = generatedSchema->GetCustomLayerData();

            bool hasErrors = false;

            _AddSchemaToSchematics(generatedSchema);

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
}

void
UsdSchemaRegistry::_SchemaDefInitHelper::_AddSchemaToSchematics(
    SdfLayerRefPtr const &source)
{
    TRACE_FUNCTION();
    const SdfLayerRefPtr &schematicsLayer = _registry->_schematics;
    for (SdfPrimSpecHandle const &prim: source->GetRootPrims()) {
        if (!schematicsLayer->GetPrimAtPath(prim->GetPath())) {

            SdfPrimSpecHandle newPrim = SdfPrimSpec::New(
                schematicsLayer, prim->GetName(), 
                prim->GetSpecifier(), prim->GetTypeName());
            _CopySpec(prim, newPrim);

            for (SdfAttributeSpecHandle const &attr: prim->GetAttributes()) {
                _CopyAttrSpec(attr, newPrim, attr->GetName());
            }

            for (SdfRelationshipSpecHandle const &rel:
                     prim->GetRelationships()) {
                _CopyRelSpec(rel, newPrim, rel->GetName());
            }

            _AddOverridePropertyNamesFromSourceSpec(prim);
        }
    }
}

void 
UsdSchemaRegistry::_SchemaDefInitHelper::_AddOverridePropertyNamesFromSourceSpec(
    const SdfPrimSpecHandle &prim)
{
    // Override property names are stored in a customData field for each schema
    // prim spec in the generatedSchema layers that we combine into the full 
    // schematics layer. But we don't copy customData fields into the final 
    // schematics (nor do we want to) so we have to extract this information 
    // from the prim specs in the generatedSchemas and store it away to used 
    // during the rest of schema initialization.
    static const TfToken apiSchemaOverridePropertyNamesToken(
        "apiSchemaOverridePropertyNames");
    VtTokenArray overridePropertyNames;
    if (prim->GetLayer()->HasFieldDictKey(
            prim->GetPath(), 
            SdfFieldKeys->CustomData, 
            apiSchemaOverridePropertyNamesToken, 
            &overridePropertyNames)) {
        _overridePropertyNamesPerSchema.emplace(
            prim->GetName(), std::move(overridePropertyNames));
    }
}

// Helper that gets the authored API schemas from the schema prim path in the
// schematics layer and prepends them to the given applied API schemas list.
void 
UsdSchemaRegistry::_SchemaDefInitHelper::_PrependAPISchemasFromSchemaPrim(
    const SdfPath &schematicsPrimPath,
    TfTokenVector *appliedAPISchemas) const
{
    // Get the API schemas from the list op field in the schematics.
    SdfTokenListOp apiSchemasListOp;
    if (!_registry->_schematics->HasField(
            schematicsPrimPath, UsdTokens->apiSchemas, &apiSchemasListOp)) {
        return;
    }
    TfTokenVector apiSchemas;
    apiSchemasListOp.ApplyOperations(&apiSchemas);
    if (apiSchemas.empty()) {
        return;
    }

    // If the existing list has API schemas, append them to list we just pulled
    // from the schematics before replacing the existing list with the new list.
    if (!appliedAPISchemas->empty()) {
        apiSchemas.insert(
            apiSchemas.end(),
            appliedAPISchemas->begin(),
            appliedAPISchemas->end());
    }
    *appliedAPISchemas = std::move(apiSchemas);
}

// For the applied API schema prim definition in expansionInfo, that is in the 
// process of being built, this takes the list of appliedAPISchemas and 
// recursively gathers all the API schemas that the prim definition needs to 
// include and composes in the properties directly from their associated 
// schematics prim specs. The expanded list of all included API schemas is
// stored back in the expansionInfo in preparation for the final population
// of the prim definition. This step is expected to be run after all prim 
// definitions representing API schemas have been established with their 
// directly included API schemas, but before any of the API schema prim 
// definitions have been updated with their fully expanded API schemas list.
void 
UsdSchemaRegistry::_SchemaDefInitHelper::_ExpandBuiltinAPISchemasRecursive(
    const _BuiltinAPISchemaInfo &includedSchemaInfo,
    _BuiltinAPISchemaExpansionInfo *expansionInfo) const
{
    // There must always be at least one applied API schema in the list as an 
    // API schema definition will always include itself.
    const TfTokenVector &appliedAPISchemas = 
        includedSchemaInfo.apiSchemaDef->GetAppliedAPISchemas();
    if (!TF_VERIFY(!appliedAPISchemas.empty())) {
        return;
    }

    // Get the name of the API schema for the schema we're including, 
    // applying the instance name if provided.
    const TfToken &includedAPISchemaName = 
        includedSchemaInfo.instanceName.IsEmpty() ? 
        appliedAPISchemas.front() :
        MakeMultipleApplyNameInstance(
            appliedAPISchemas.front(), includedSchemaInfo.instanceName);

    // Add the included API schema to the expanded list.
    expansionInfo->allAPISchemaNames.push_back(includedAPISchemaName);

    // Make a copy the parent's override properties to compose. We need a copy
    // since the overrides from this branch of the API schema expansion do
    // not apply to the other API schemas included by the parent schema. Note
    // that we expect the number of property overrides to be very small (and 
    // frequently empty) and we also expect that the depth of recursion of 
    // API schema expansion to be very minimal, so the copying of this structure
    // is unlikely to have a performance impact.
    _PropNameAndPathsToComposeVector propsWithOversToCompose;
    if (includedSchemaInfo.propsWithOversToComposePtr) {
        propsWithOversToCompose = 
            *(includedSchemaInfo.propsWithOversToComposePtr);
    }

    // Add the properties that come directly from the API schema's class 
    // schematics. Any newly found defined properties that have overrides in
    // propsWithOversToCompose will be fully composed into their final property
    // spec for the expanded prim definition. Existing overrides in 
    // propsWithOversToCompose that were able to be composed with a defined 
    // property will be removed (we are done with them and won't need/want to
    // compose them again). Any new properties that are declared as overrides in 
    // this schematics spec will be added to propsWithOversToCompose so they
    // can be composed with defined properties from API schemas that this 
    // schema includes.
    // \ref Usd_APISchemaStrengthOrdering
    _AddSchemaSpecPropertiesAndUpdateOversToCompose(
        includedSchemaInfo, 
        expansionInfo->primDef,
        &propsWithOversToCompose);

    // Recursively gather the built-in API schemas which are listed after the
    // included API schema in its applied API schemas list.
    // 
    // At this point in initialization, all API schema prim definitions will 
    // have their directly included API schemas set in the definition, but 
    // will not have had them expanded to include APIs included from other
    // APIs. Thus, we can do a depth first recursion on the current applied
    // API schemas of the API prim definition to continue expanding the 
    // full list of API schemas and prim specs to compose in strength order.
    for (auto it = appliedAPISchemas.begin() + 1; 
         it != appliedAPISchemas.end(); ++it) {

        // If we have an instance name, we need to apply it to the built-in 
        // API schema's name as well.
        const TfToken &builtinApiSchemaName = 
            includedSchemaInfo.instanceName.IsEmpty() ? 
            *it : 
            MakeMultipleApplyNameInstance(*it, includedSchemaInfo.instanceName);

        // This is mainly to avoid API most schema inclusion cycles but also has 
        // the added benefit of avoiding duplicates if included API schemas 
        // themselves include the same other API schema.
        // 
        // Note that we linear search the vector of API schema names. This 
        // vector is expected to be small and shouldn't be a performance 
        // concern. But it's something to be aware of in case it does cause 
        // performance problems in the future, especially since by far the most 
        // common case will be that we don't find the name in the list.
        if (std::find(expansionInfo->allAPISchemaNames.begin(), 
                      expansionInfo->allAPISchemaNames.end(), 
                      builtinApiSchemaName) 
                != expansionInfo->allAPISchemaNames.end()) {
            continue;
        }

        // Find the registered prim definition (and instance name if it's a
        // multiple apply instance). Skip this API if we can't find a def.
        TfToken builtinInstanceName;
        const UsdPrimDefinition *builtinApiSchemaDef = 
            _registry->_FindAPIPrimDefinitionByFullName(
                builtinApiSchemaName, &builtinInstanceName);
        if (!builtinApiSchemaDef) {
            TF_WARN("Could not find API schema definition for '%s' included by "
                    "API schema '%s'",
                    builtinApiSchemaName.GetText(), 
                    includedAPISchemaName.GetText());
            continue;
        }

        // There is an additional cycle condition that is not covered by the
        // check above for whether the exact API schema name has already been
        // included. This additional possible cycle may occur only when 
        // expanding a multiple apply API schema template.
        // 
        // A multiple apply schema is allowed to include encapsulated instances
        // of other multiple apply schemas by including them using a 
        // sub-instance name. For instance the multiple apply schema template
        // "MultiApplyAPI:__INSTANCE_NAME__" may include another built-in 
        // API schema "OtherMultiApplyAPI:__INSTANCE_NAME__:builtin". Thus, when
        // MultiApplyAPI is applied with the instance name "foo", it applies the
        // "MultiApplyAPI:foo" instance of MultiApplyAPI and the built-in 
        // "OtherMultiApplyAPI:foo:builtin" instance of OtherMultiApplyAPI.
        //
        // But now say that "OtherMultiApplyAPI:__INSTANCE_NAME__" is set up
        // to have "MultiApplyAPI:__INSTANCE_NAME__:other" included as a 
        // built-in. Unchecked, this would cause an infinite cycle as applying
        // MultiApplyAPI with the "foo" instance would apply "MultiApplyAPI:foo"
        // which includes "OtherMultiApplyAPI:foo:builtin" which then would
        // include "MultiApplyAPI:foo:builtin:other" which in turn includes 
        // "OtherMultiApplyAPI:foo:builtin:other:builtin" and so on infinitely
        // expanding the instance name. The check above doesn't catch this 
        // because each included schema is a different uniquely named instance 
        // of the multiple apply schema and will not have already been included
        // in the list.
        //
        // Thus, we have to do an additional check here to make sure we never
        // include an API schema using the same schema definition as any of the 
        // API schemas in the recursive stack that caused this schema to be 
        // included. We only check the "included by" stack because its perfectly
        // valid for the same multiple apply schema template to be used for 
        // sibling built-in schemas (e.g. if "MultiApplyAPI:__INSTANCE_NAME__"
        // included both "OtherMultiApplyAPI:__INSTANCE_NAME__:foo" and 
        // "OtherMultiApplyAPI:__INSTANCE_NAME__:bar").
        _BuiltinAPISchemaInfo builtinApiSchemaInfo = 
            {builtinApiSchemaDef, builtinInstanceName, 
             &includedSchemaInfo, &propsWithOversToCompose};
        if (builtinApiSchemaInfo.CheckForCycle()) {
            TF_WARN("Found unrecoverable API schema cycle while expanding "
                    "built-in API schema chain '%s'. An API schema of the same "
                    "type as '%s' has caused it to be included again with a "
                    "different instance name. Including it would cause an "
                    "infinite recursion cycle so it must be skipped",
                    includedAPISchemaName.GetText(),
                    builtinApiSchemaName.GetText());
            continue;
        }

        // Gather the built-in schemas prim specs and built-in API schemas.
        _ExpandBuiltinAPISchemasRecursive(builtinApiSchemaInfo, expansionInfo);

        // Each of the API schemas we recursively expand in this loop may have 
        // defined and composed any number of the properties that we have 
        // overrides stored for. We remove the overrides for these composed 
        // properties here so that we don't inadvertently compose over the 
        // property again if one of the weaker sibling API schemas happens to 
        // define the same property.
        if (!propsWithOversToCompose.empty()) {
            const auto removeIt = std::remove_if(
                propsWithOversToCompose.begin(),
                propsWithOversToCompose.end(),
                [&](const _PropNameAndPathsToCompose &propWithOversToCompose) {
                    return expansionInfo->primDef->_GetPropertySpecPath(
                        propWithOversToCompose.first);
                });
            propsWithOversToCompose.erase(
                removeIt, propsWithOversToCompose.end());
        }
    }
}

void 
UsdSchemaRegistry::_SchemaDefInitHelper::
_AddSchemaSpecPropertiesAndUpdateOversToCompose(
    const _BuiltinAPISchemaInfo &includedSchemaInfo,
    UsdPrimDefinition *primDef,
    _PropNameAndPathsToComposeVector *propsWithOversToCompose) const
{
    // Get all the defined property paths and override property paths from the
    // API schema's schematics spec.
    _PropNameAndPathVector overrideProperties;
    _PropNameAndPathVector definedProperties = _GetPropertyPathsForSpec(
        includedSchemaInfo.apiSchemaDef->_schematicsPrimPath,
        includedSchemaInfo.instanceName,
        &overrideProperties);

    // Compose the schema's defined properties into the expanded prim 
    // definition.
    primDef->_AddProperties(std::move(definedProperties));

    // With the new defined properties added, compose any overrides gathered
    // from the schemas that caused this API schema to be included over the 
    // defined property definitions to which they apply. The composed property
    // specs will be added (or updated) under the schematics prim spec of the
    // expanded prim definition's type. Note that this removes any
    // properties that are composed from propsWithOversToCompose so that we
    // don't process these overrides again. Any property overrides that we don't
    // have a defined property for yet will remain in propsWithOversToCompose
    // as they may be defined elsewhere in the schema expansion.
    _ComposePropertiesWithOverrides(primDef, propsWithOversToCompose);

    // Now process the API schema override properties that this schema itself
    // defines.
    for (_PropNameAndPath &overridePropNameAndPath : overrideProperties) {
        TfToken &overridePropName = overridePropNameAndPath.first;
        SdfPath &overridePropPath = overridePropNameAndPath.second;

        // If the property name is already found in the composed prim 
        // definition, then we've already found a def for the property and don't
        // process any more overrides.
        if (primDef->_GetPropertySpecPath(overridePropName)) {
            continue;
        }

        // Add the override property's path to the list of schema specs that 
        // will need to be composed for the property with that name, starting
        // a new list of paths if necessary.
        // 
        // We expect the number of override properties to be extemely small 
        // making linear search efficient.
        auto findIt = std::find_if(
            propsWithOversToCompose->begin(),
            propsWithOversToCompose->end(),
            [&](const _PropNameAndPathsToCompose &propWithOversToCompose){
                return propWithOversToCompose.first == overridePropName;
            });

        if (findIt == propsWithOversToCompose->end()) {
            propsWithOversToCompose->emplace_back(
                std::move(overridePropName), 
                SdfPathVector({std::move(overridePropPath)}));
        } else {
            findIt->second.push_back(std::move(overridePropPath));
        }
    }
}

UsdSchemaRegistry::_SchemaDefInitHelper::_PropNameAndPathVector 
UsdSchemaRegistry::_SchemaDefInitHelper::_GetPropertyPathsForSpec(
    const SdfPath &primSpecPath, 
    _PropNameAndPathVector *overrideProperties) const
{
    _PropNameAndPathVector definedProperties;

    // Get the names of all the properties defined in the prim spec.
    TfTokenVector specPropertyNames;
    if (!_registry->_schematics->HasField<TfTokenVector>(
            primSpecPath, 
            SdfChildrenKeys->PropertyChildren, 
            &specPropertyNames)) {
        // While its possible for the spec to have no properties, we expect 
        // the prim spec itself to exist.
        if (!_registry->_schematics->HasSpec(primSpecPath)) {
            TF_WARN("No prim spec exists at path '%s' in schematics layer.",
                    primSpecPath.GetText());
        }
        return definedProperties;
    }

    definedProperties.reserve(specPropertyNames.size());

    // Get the override property names for this schema if there are any. If 
    // there aren't any, return the path for each property name.
    const VtTokenArray *overridePropertyNames = TfMapLookupPtr(
        _overridePropertyNamesPerSchema, primSpecPath.GetNameToken());
    if (!overridePropertyNames) {
        for (TfToken &propName : specPropertyNames) {
            definedProperties.emplace_back(
                std::move(propName), 
                primSpecPath.AppendProperty(propName));
        }
        return definedProperties;
    } 

    // Otherwish we have to filter out the override properties from the list
    // and optionally return their paths via the output parameter.
    if (overrideProperties) {
        overrideProperties->reserve(overridePropertyNames->size());
    }
    for (TfToken &propName : specPropertyNames) {
        if (std::find(overridePropertyNames->begin(),
                      overridePropertyNames->end(), 
                      propName) == overridePropertyNames->end()) {
            definedProperties.emplace_back(
                std::move(propName), 
                primSpecPath.AppendProperty(propName));
        } else if (overrideProperties) {
            overrideProperties->emplace_back(
                std::move(propName), 
                primSpecPath.AppendProperty(propName));
        }
    }

    return definedProperties;
}

UsdSchemaRegistry::_SchemaDefInitHelper::_PropNameAndPathVector 
UsdSchemaRegistry::_SchemaDefInitHelper::_GetPropertyPathsForSpec(
    const SdfPath &primSpecPath, 
    const TfToken &instanceName,
    _PropNameAndPathVector *overrideProperties) const
{
    // First get the property names and specs without the instance name.
    _PropNameAndPathVector definedProperties = 
        _GetPropertyPathsForSpec(primSpecPath, overrideProperties);
    if (instanceName.IsEmpty()) {
        return definedProperties;
    }

    // Apply the instance to all the property names before returning property
    // paths.
    for (_PropNameAndPath &propNameAndPath : definedProperties) {
        propNameAndPath.first = 
            UsdSchemaRegistry::MakeMultipleApplyNameInstance(
                propNameAndPath.first, instanceName);
    }
    if (overrideProperties) {
        for (_PropNameAndPath &propNameAndPath : *overrideProperties) {
            propNameAndPath.first = 
                UsdSchemaRegistry::MakeMultipleApplyNameInstance(
                    propNameAndPath.first, instanceName);
        }
    }

    return definedProperties;
}

// Returns true if the property with the given name in these two separate prim
// definitions have the same type. "Same type" here means that they are both
// the same kind of property (attribute or relationship), have the same 
// variability (varying or uniform) and if they are attributes, that their 
// attribute type names are the same.
static bool _PropertyTypesMatch(
    const SdfLayerRefPtr &layer,
    const SdfPath &strongerPropPath,
    const SdfPath &weakerPropPath)
{
    const SdfSpecType specType = layer->GetSpecType(strongerPropPath);
    const bool specIsAttribute = (specType == SdfSpecTypeAttribute);

    // Compare spec types (relationship vs attribute)
    if (specType != layer->GetSpecType(weakerPropPath)) {
        TF_WARN("%s at path '%s' from stronger schema failed to override %s at "
                "'%s' from weaker schema during schema prim definition "
                "composition because of the property spec types do not match.",
                specIsAttribute ? "Attribute" : "Relationsip",
                strongerPropPath.GetText(),
                specIsAttribute ? "relationsip" : "attribute",
                weakerPropPath.GetText());
        return false;
    }

    // Compare variability
    SdfVariability strongerVariability, weakerVariability;
    layer->HasField(
        strongerPropPath, SdfFieldKeys->Variability, &strongerVariability);
    layer->HasField(
        weakerPropPath, SdfFieldKeys->Variability, &weakerVariability);
    if (weakerVariability != strongerVariability) {
        TF_WARN("Property at path '%s' from stronger schema failed to override "
                "property at path '%s' from weaker schema during schema prim "
                "definition composition because their variability does not "
                "match.",
                strongerPropPath.GetText(),
                weakerPropPath.GetText());
        return false;
    }

    // Done comparing if its not an attribute.
    if (!specIsAttribute) {
        return true;
    }

    // Compare the type name field of the attributes.
    TfToken strongerTypeName;
    layer->HasField(strongerPropPath, SdfFieldKeys->TypeName, &strongerTypeName);
    TfToken weakerTypeName;
    layer->HasField(weakerPropPath, SdfFieldKeys->TypeName, &weakerTypeName);
    if (weakerTypeName != strongerTypeName) {
        TF_WARN("Attribute at path '%s' with type name '%s' from stronger "
                "schema failed to override attribute at path '%s' with type "
                "name '%s' from weaker schema during schema prim definition "
                "composition because of the attribute type names do not match.",
                strongerPropPath.GetText(),
                strongerTypeName.GetText(),
                weakerPropPath.GetText(),
                weakerTypeName.GetText());
        return false;
    }
    return true;
}

void
UsdSchemaRegistry::_SchemaDefInitHelper::
_ComposePropertiesWithOverrides(
    UsdPrimDefinition *primDef,
    _PropNameAndPathsToComposeVector *propsWithOversToCompose) const
{
    if (propsWithOversToCompose->empty()) {
        return;
    }

    _PropNameAndPathsToComposeVector uncomposedPropsWithOversToCompose;
    const SdfLayerRefPtr &schematicsLayer = _registry->_schematics;

    for (auto &propWithOversToCompose : *propsWithOversToCompose) {
        const TfToken &propName = propWithOversToCompose.first;
        SdfPathVector &propertyPaths = propWithOversToCompose.second;

        // Get the defined property spec for the override property spec. If 
        // there isn't one yet, move the override properties to the uncomposed
        // list so we can return them back at the end.
        SdfPath *defPath = primDef->_GetPropertySpecPath(propName);
        if (!defPath) {
            uncomposedPropsWithOversToCompose.push_back(
                std::move(propWithOversToCompose));
            continue;
        }

        // Property overrides are not allowed to change the type of a property
        // from its defining spec so remove any override specs that are 
        // invalid.
        const auto badPropsIt = std::remove_if(
            propertyPaths.begin(), propertyPaths.end(),
            [&](const SdfPath &propPath) {
                return !_PropertyTypesMatch(
                    schematicsLayer, propPath, *defPath);
            }
        );
        if (badPropsIt != propertyPaths.end()) {
            propertyPaths.erase(badPropsIt, propertyPaths.end());
            if (propertyPaths.empty()) {
                continue;
            }
        }

        // The composed property will always live under the prim definition's
        // schematics prim spec, regardless of where the defs it is 
        // composed from come from.
        const SdfPath dstPath = 
            primDef->_schematicsPrimPath.AppendProperty(propName);

        // If the first override path is not from the composed prim 
        // definition itself, then the schematics prim doesn't have a
        // spec for this prorperty yet. Copy the first override to 
        // create the needed property spec.
        if (dstPath != propertyPaths.front()) {
            SdfPrimSpecHandle dstPrim = 
                schematicsLayer->GetPrimAtPath(primDef->_schematicsPrimPath);
            SdfPropertySpecHandle srcProp = 
                schematicsLayer->GetPropertyAtPath(propertyPaths.front());
            _CopyPropSpec(srcProp, dstPrim, propName);
        }

        // Compose function. Any fields from the srcPath spec that aren't 
        // already in the dstPath spec are copied into the dstPath spec.
        auto composeFn = [&](const SdfPath &srcPath) {
            for (const TfToken srcField : schematicsLayer->ListFields(srcPath)) {
                if (!schematicsLayer->HasField(dstPath, srcField)) {
                    schematicsLayer->SetField(dstPath, srcField, 
                        schematicsLayer->GetField(srcPath, srcField));
                }
            }
        };

        // Now compose in the rest of the override property specs under
        // our destination. We always skip the first entry as that will
        // already be the spec the destination path starts with.
        for (auto pathIt = propertyPaths.begin() + 1; 
             pathIt != propertyPaths.end(); ++pathIt) {
            composeFn(*pathIt);
        }

        // Last compose in the property definition itself.
        composeFn(*defPath);

        // Now that the spec is fully composed, set the definition's 
        // path for the property to the composed property's path.
        *defPath = dstPath;
    }

    // Update the propsWithOversToCompose to be the remaining prop overs that 
    // weren't able to compose here.
    propsWithOversToCompose->swap(uncomposedPropsWithOversToCompose);
}

void
UsdSchemaRegistry::_SchemaDefInitHelper::
_PopulateAppliedAPIPrimDefinitions() const
{
    TRACE_FUNCTION();
    // All applied API schemas may contain other applied API schemas which may 
    // also include other API schemas. To populate their properties correctly,
    // we must do this in multiple passes.
    std::vector<_BuiltinAPISchemaExpansionInfo> defsToExpand;

    // Step 1. For each applied API schema, we determine what (if any) built-in
    // applied API schemas it has. If it has none, we can just populate its 
    // prim definition from the schematics prim spec and be done. Otherwise we
    // need to store the directly included built-in API schemas now and process
    // them in the next pass once we know ALL the API schemas that every other
    // API schema includes.
    for (auto &nameAndDefPtr : _registry->_appliedAPIPrimDefinitions) {
        const TfToken &usdTypeNameToken = nameAndDefPtr.first;
        UsdPrimDefinition *primDef = nameAndDefPtr.second.get();
        if (!TF_VERIFY(primDef)) {
            continue;
        }

        // During initialization, any auto applied API schema names relevant
        // to this API schema type were put in prim definition's applied API 
        // schema list. There may be applied API schemas defined in the metadata
        // for the type in the schematics. If so, these must be prepended to the
        // collected auto applied schema names (auto apply API schemas are 
        // weaker) to get full list of API schemas that need to be composed into
        // this prim definition.
        _PrependAPISchemasFromSchemaPrim(
            primDef->_schematicsPrimPath,
            &primDef->_appliedAPISchemas);

        // We always include the API schema itself as the first applied API 
        // schema in its prim definition.
        primDef->_appliedAPISchemas.insert(
            primDef->_appliedAPISchemas.begin(), usdTypeNameToken);

        // If this API schema has no built-in API schemas (its only applied 
        // schema is itself), we can just add the prim spec's properties and be
        // done. 
        if (primDef->_appliedAPISchemas.size() == 1) {
            _PropNameAndPathVector definedProperties = 
                _GetPropertyPathsForSpec(primDef->_schematicsPrimPath);
            primDef->_AddProperties(std::move(definedProperties));
            continue;
        }

        // Otherwise schema def has additional applied built-in API schemas 
        // that need to be expanded and composed in the next step.
        defsToExpand.push_back({primDef});

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
        // template. Otherwise if this API schema is not a tempate, then each
        // built-in must also not be a template.
        //
        // Note that usdGenSchema will always generate schemas that conform to
        // this, but it's worthwhile to detect and report this invalid 
        // condition if it does occur.
        const bool isMultipleApplyTemplateSchema = 
            IsMultipleApplyNameTemplate(usdTypeNameToken);
        const auto it = std::remove_if(
            primDef->_appliedAPISchemas.begin(),
            primDef->_appliedAPISchemas.end(),
            [isMultipleApplyTemplateSchema](const TfToken &apiSchemaName)
                { return IsMultipleApplyNameTemplate(apiSchemaName) !=
                    isMultipleApplyTemplateSchema; });

        if (it != primDef->_appliedAPISchemas.end()) {
            TF_WARN("Invalid inclusion of API schemas (%s) by API schema "
                    "'%s'. Multiple apply API schema templates can only "
                    "include or be included by other multiple apply API "
                    "schema templates. These schemas will not be included as "
                    "built-in schemas of '%s'",
                    TfStringJoin(
                        it, primDef->_appliedAPISchemas.end(), ", ").c_str(),
                    usdTypeNameToken.GetText(),
                    usdTypeNameToken.GetText());
            primDef->_appliedAPISchemas.erase(
                it, primDef->_appliedAPISchemas.end());
        }
    }

    // Step 2. For each applied API schema that has other built-in applied API 
    // schemas, recursively gather the fully expanded list of API schemas and 
    // compose their properties into the definition's properties. 
    // 
    // We can safely compose in all the properties here but we can't set the 
    // final expanded list of included API schemas in the prim definition until 
    // we've computed the expanded API schemas for every API schema prim 
    // definition as this step expects each API schema definition to ONLY 
    // list its direct built-in API schemas so that we can recurse without 
    // cycling. Only once we've gathered what will be the fully expanded list 
    // of API schemas for all of them can we set the fully expanded API schema 
    // list in the definition itself. 
    for (_BuiltinAPISchemaExpansionInfo &expansionInfo : defsToExpand) {
        _ExpandBuiltinAPISchemasRecursive(
            {expansionInfo.primDef, TfToken(), nullptr}, &expansionInfo);
    }

    // Step 3. For each API schema definition from step 2, we can now set the 
    // fully expanded list of API schemas.
    for (_BuiltinAPISchemaExpansionInfo &expansionInfo : defsToExpand) {
        expansionInfo.primDef->_appliedAPISchemas = 
            std::move(expansionInfo.allAPISchemaNames);
    }
}

void 
UsdSchemaRegistry::_SchemaDefInitHelper::
_PopulateConcretePrimDefinitions() const
{
    TRACE_FUNCTION();
    // Populate all concrete API schema definitions; it is expected that all 
    // API schemas, which these may depend on, have already been populated.
    for (auto &nameAndDefPtr : _registry->_concreteTypedPrimDefinitions) {
        UsdPrimDefinition *primDef = nameAndDefPtr.second.get();
        if (!TF_VERIFY(primDef)) {
            continue;
        }

        // During initialization, any auto applied API schema names relevant
        // to this prim type were put in prim definition's applied API schema
        // list. There may be applied API schemas defined in the metadata for 
        // the type in the schematics. If so, these must be prepended to the
        // collected auto applied schema names (auto apply API schemas are 
        // weaker) to get full list of API schemas that need to be composed into
        // this prim definition.
        _PrependAPISchemasFromSchemaPrim(
            primDef->_schematicsPrimPath,
            &primDef->_appliedAPISchemas);

        // Get both the defined properties and API schema override properties
        // from the concrete prim spec. We compose the defined properties from 
        // the prim spec to the prim definition first as these are stronger 
        // than the built-in API schema properties.
        _PropNameAndPathVector overrideProperties;
        _PropNameAndPathVector definedProperties = 
            _GetPropertyPathsForSpec(primDef->_schematicsPrimPath, 
                                     &overrideProperties);
        primDef->_AddProperties(std::move(definedProperties));

        // If there are any applied API schemas in the list, compose them 
        // in now
        if (!primDef->_appliedAPISchemas.empty()) {
            // We've stored the names of all the API schemas we're going to 
            // compose in the primDef's _appliedAPISchemas even though we 
            // haven't composed in any of their properties yet. In addition to 
            // composing in properties, _ComposeAPISchemasIntoPrimDefinition 
            // will also append the API schemas it receives to the primDef's 
            // _appliedAPISchemas. Thus, we copy _appliedAPISchemas to a 
            // temporary and clear it first so that we don't end up with the 
            // entire contents of list appended to itself again.
            TfTokenVector apiSchemasToCompose = 
                std::move(primDef->_appliedAPISchemas);
            primDef->_appliedAPISchemas.clear();
            _registry->_ComposeAPISchemasIntoPrimDefinition(
                primDef, apiSchemasToCompose);
        }

        // With all the built-in API schemas applied, we can now compose any
        // API schema property overrides declared in the types schema over the 
        // current defined properties.
        if (!overrideProperties.empty()) {
            _PropNameAndPathsToComposeVector overridePropertiesToCompose;
            for (_PropNameAndPath &overrideProperty : overrideProperties) {
                overridePropertiesToCompose.emplace_back(
                    std::move(overrideProperty.first),
                    SdfPathVector({std::move(overrideProperty.second)}));
            }
            _ComposePropertiesWithOverrides(
                primDef, &overridePropertiesToCompose);
        }
    }
}

UsdSchemaRegistry::UsdSchemaRegistry()
{
    _schematics = SdfLayer::CreateAnonymous("registry.usda");
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
const std::map<TfToken, TfTokenVector> &
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
    const TfHashMap<TfToken, TfTokenVector, TfHash> &canOnlyApplyToMap = 
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
        return std::unique_ptr<UsdPrimDefinition>();
    }

    // Start with a new prim definition mapped to the same prim spec path as the 
    // prim type's definition. This does not yet add the prim type's properties.
    // Note that its perfectly valid for there to be no prim definition for the 
    // given prim type, in which case we compose API schemas over an empty prim 
    // definition.
    const UsdPrimDefinition *primDef = FindConcretePrimDefinition(primType);
    std::unique_ptr<UsdPrimDefinition> composedPrimDef(primDef ? 
        new UsdPrimDefinition(
            primDef->_schematicsPrimPath, /* isAPISchema = */ false) : 
        new UsdPrimDefinition());

    // We compose in the new API schemas first as these API schema property 
    // opinions need to be stronger than the prim type's prim definition's 
    // opinions.
    _ComposeAPISchemasIntoPrimDefinition(
        composedPrimDef.get(), appliedAPISchemas);
        
    // Now compose in the prim type's properties if we can.        
    if (primDef) {
        // When building a definition from authored applied API schemas we 
        // don't want API schemas applied on top of a typed prim definition to 
        // change any of property types from the typed prim definition. That's 
        // why we compose in the weaker typed prim definition with 
        // useWeakerPropertyForTypeConflict set to true. 
        // 
        // Note that the strongest API schema wins for a property type conflict
        // amongst the authored applied API schemas themselves. But this 
        // "winning" property will be ignored if it tries to changed the type
        // of an existing property in the typed prim definition.
        composedPrimDef->_ComposePropertiesFromPrimDef(
            *primDef, /* useWeakerPropertyForTypeConflict = */ true);

        // The prim type's prim definition may have its own built-in API 
        // schemas (which were already composed into its definition). We need
        // to append these to applied API schemas list for our composed prim
        // definition.
        composedPrimDef->_appliedAPISchemas.insert(
            composedPrimDef->_appliedAPISchemas.end(),
            primDef->_appliedAPISchemas.begin(),
            primDef->_appliedAPISchemas.end());
    }
            
    return composedPrimDef;
}

const UsdPrimDefinition *
UsdSchemaRegistry::_FindAPIPrimDefinitionByFullName(
    const TfToken &apiSchemaName, 
    TfToken *instanceName) const
{
    // Applied schemas may be single or multiple apply so we have to parse
    // the full schema name into a type and possibly an instance name.
    auto typeNameAndInstance = GetTypeNameAndInstance(apiSchemaName);
    const TfToken &typeName = typeNameAndInstance.first;
    *instanceName = typeNameAndInstance.second;

    // If the instance name is empty we expect a single apply API schema 
    // otherwise it should be a multiple apply API.
    if (instanceName->IsEmpty()) {
        if (std::unique_ptr<UsdPrimDefinition> const *apiSchemaTypeDef = 
                TfMapLookupPtr(_appliedAPIPrimDefinitions, typeName)) {
            return apiSchemaTypeDef->get();
        }
    } else {
        if (const UsdPrimDefinition * const *multiApplyDef = 
                TfMapLookupPtr(_multiApplyAPIPrimDefinitions, typeName)) {
            return *multiApplyDef;
        }
    }

    return nullptr;
}

void UsdSchemaRegistry::_ComposeAPISchemasIntoPrimDefinition(
    UsdPrimDefinition *primDef, const TfTokenVector &appliedAPISchemas) const
{
    // Add in properties from each new applied API schema. Applied API schemas 
    // are ordered strongest to weakest so we compose, in order, each weaker 
    // schema's properties.
    for (const TfToken &apiSchemaName : appliedAPISchemas) {

        TfToken instanceName;
        const UsdPrimDefinition *apiSchemaTypeDef = 
            _FindAPIPrimDefinitionByFullName(apiSchemaName, &instanceName);

        if (apiSchemaTypeDef) {
            // Compose in the properties from the API schema def.
            primDef->_ComposePropertiesFromPrimDef(
                *apiSchemaTypeDef, 
                /* useWeakerPropertyForTypeConflict = */ false, 
                instanceName);

            // Append all the API schemas included in the schema def to the 
            // prim def's API schemas list. This list will always include the 
            // schema itself followed by all other API schemas that were 
            // composed into its definition.
            const TfTokenVector &apiSchemasToAppend = 
                apiSchemaTypeDef->GetAppliedAPISchemas();

            if (instanceName.IsEmpty()) {
                primDef->_appliedAPISchemas.insert(
                    primDef->_appliedAPISchemas.end(),
                    apiSchemasToAppend.begin(), apiSchemasToAppend.end());
            } else {
                // An instance name indicates a multiple apply schema so we
                // have to apply the instance name to all the included API
                // schemas being added.
                for (const TfToken &apiSchema : apiSchemasToAppend) {
                    primDef->_appliedAPISchemas.push_back(
                        MakeMultipleApplyNameInstance(
                            apiSchema, instanceName));
                }
            }
        }
    }
}

void 
Usd_GetAPISchemaPluginApplyToInfoForType(
    const TfType &apiSchemaType,
    const TfToken &apiSchemaName,
    std::map<TfToken, TfTokenVector> *autoApplyAPISchemasMap,
    TfHashMap<TfToken, TfTokenVector, TfHash> *canOnlyApplyAPISchemasMap,
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

