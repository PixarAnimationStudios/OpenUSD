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

template <class T>
static void
_CopySpec(const T &srcSpec, const T &dstSpec)
{
    for (const TfToken& key : srcSpec->ListInfoKeys()) {
        if (!UsdSchemaRegistry::IsDisallowedField(key)) {
            dstSpec->SetInfo(key, srcSpec->GetInfo(key));
        }
    }
}

static void
_AddSchema(SdfLayerRefPtr const &source, SdfLayerRefPtr const &target)
{
    for (SdfPrimSpecHandle const &prim: source->GetRootPrims()) {
        if (!target->GetPrimAtPath(prim->GetPath())) {

            SdfPrimSpecHandle newPrim =
                SdfPrimSpec::New(target, prim->GetName(), prim->GetSpecifier(),
                                 prim->GetTypeName());
            _CopySpec(prim, newPrim);

            for (SdfAttributeSpecHandle const &attr: prim->GetAttributes()) {
                SdfAttributeSpecHandle newAttr =
                    SdfAttributeSpec::New(
                        newPrim, attr->GetName(), attr->GetTypeName(),
                        attr->GetVariability(), attr->IsCustom());
                _CopySpec(attr, newAttr);
            }

            for (SdfRelationshipSpecHandle const &rel:
                     prim->GetRelationships()) {
                SdfRelationshipSpecHandle newRel =
                    SdfRelationshipSpec::New(
                        newPrim, rel->GetName(), rel->IsCustom());
                _CopySpec(rel, newRel);
            }
        }
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
        // Find and load all the generated schema in plugin libraries.  We find 
        // these files adjacent to pluginfo files in libraries that provide 
        // subclasses of UsdSchemaBase.
        _InitializePrimDefsAndSchematicsForPluginSchemas();

        // Populate multiple apply API schema definitions first. These can't 
        // include other API schemas so they're populated directly from the 
        // their prim spec in the schematics.
        _PopulateMultipleApplyAPIPrimDefinitions();

        // Populate single apply API schema definitions second. These can 
        // include other API schemas including instances of multiple apply
        // API schemas.
        _PopulateSingleApplyAPIPrimDefinitions();

        // Populate all concrete API schema definitions after all API schemas
        // they may depend on have been populated.
        _PopulateConcretePrimDefinitions();
    }

private:
    // Single apply API schemas require some processing to determine all 
    // the included API schemas and all the prim specs that need to be composed
    // into its prim definition. This structure is used to hold this info.
    struct _SchemaDefCompositionInfo {

        // The prim definition to compose.
        UsdPrimDefinition *primDef;

        // The ordered list of prim specs from the schematics to compose into
        // the prim definition. The list is actually a vector of pairs, where
        // the pair is the prim spec and a possible instance name (for included
        // instances of multiple apply API schemas).
        using _SchemaPrimSpecToCompose = 
            std::pair<const SdfPrimSpecHandle, TfToken>;
        using _SchemaPrimSpecsToCompose = std::vector<_SchemaPrimSpecToCompose>;
        _SchemaPrimSpecsToCompose schemaPrimSpecsToCompose;

        // The expanded list of names of all API schemas that will be present
        // in the prim definition.
        TfTokenVector allAPISchemaNames;

        _SchemaDefCompositionInfo(
            UsdPrimDefinition *primDef_,
            const TfToken &apiSchemaName,
            const SdfPrimSpecHandle &apiSchemaPrimSpec) 
        : primDef(primDef_) 
        {
            // We'll always compose the schema type's own prim spec.
            schemaPrimSpecsToCompose.emplace_back(apiSchemaPrimSpec, TfToken());
            // The schema's own name will always be the first entry in its own
            // list of applied API schemas.
            allAPISchemaNames.push_back(apiSchemaName);
        }
    };

    void _InitializePrimDefsAndSchematicsForPluginSchemas();

    bool _CollectMultipleApplyAPISchemaNamespaces(
        const VtDictionary &customDataDict);

    void _PrependAPISchemasFromSchemaPrim(
        const SdfPath &schematicsPrimPath,
        TfTokenVector *appliedAPISchemas);

    void _GatherAllAPISchemaPrimSpecsToCompose(
        _SchemaDefCompositionInfo *apiDefInfo,
        const TfTokenVector &appliedAPISchemas) const;

    void _PopulateMultipleApplyAPIPrimDefinitions();
    void _PopulateSingleApplyAPIPrimDefinitions();
    void _PopulateConcretePrimDefinitions();

    UsdSchemaRegistry *_registry;
};

bool
UsdSchemaRegistry::_SchemaDefInitHelper::
_CollectMultipleApplyAPISchemaNamespaces(
    const VtDictionary &customDataDict)
{
    // Names of multiple apply API schemas are stored in their schemas
    // in a dictionary mapping them to their property namespace prefixes. 
    // These will be useful in mapping schema instance property names 
    // to the schema property specs.

    auto it = customDataDict.find(_tokens->multipleApplyAPISchemas);
    if (it == customDataDict.end()) {
        return true;
    }

    if (!it->second.IsHolding<VtDictionary>()) {
        TF_CODING_ERROR("Found an unexpected value type for layer "
            "customData key '%s'; expected a dictionary. Multiple apply API "
            "schemas may be incorrect.",
            _tokens->multipleApplyAPISchemas.GetText());
        return false;
    }

    bool success = true;
    const VtDictionary &multipleApplyAPISchemas = 
        it->second.UncheckedGet<VtDictionary>();
    for (const auto &it : multipleApplyAPISchemas) {
        const TfToken apiSchemaName(it.first);

        if (!it.second.IsHolding<std::string>()) {
            TF_CODING_ERROR("Found an unexpected value type for key '%s' in "
                "the dictionary for layer customData field '%s'; expected a "
                "string. Multiple apply API schema of type '%s' will not be "
                "correctly registered.",
                apiSchemaName.GetText(),
                _tokens->multipleApplyAPISchemas.GetText(),
                apiSchemaName.GetText());
            success = false;
            continue;
        }

        // The property namespace is stored along side where the prim definition
        // defining the schema's properties will be.
        _registry->_multiApplyAPIPrimDefinitions[apiSchemaName]
            .propertyNamespace = TfToken(it.second.UncheckedGet<std::string>());
    }
    return success;
}

void
UsdSchemaRegistry::_SchemaDefInitHelper::
_InitializePrimDefsAndSchematicsForPluginSchemas()
{
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
            // Check if there are any API schemas that have been setup to apply 
            // to this type. We'll set these in the prim definition's applied 
            // API schemas list so they can be processed when building out this
            // prim definition in _PopulateConcretePrimDefinitions.
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
                // The multiple apply schema map stores both the prim definition
                // and the property prefix for the schema, but we won't know
                // the prefix until we read the generated schemas.
                _registry->_multiApplyAPIPrimDefinitions[typeName].primDef = 
                    newPrimDef;
            } else {
                _registry->_singleApplyAPIPrimDefinitions.emplace(
                    typeName, newPrimDef);

                // Check if there are any API schemas that have been setup to 
                // apply to this API schema type. We'll set these in the prim 
                // definition's applied API schemas list so they can be 
                // processed when building out this prim definition in 
                // _PopulateSingleApplyAPIPrimDefinitions.
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

            // This collects all the multiple apply API schema namespaces 
            // prefixes that are defined in the generated schema and stores
            // them in the prim definition map entry that will contain the 
            // prim definition for the corresponding multiple apply API schema
            // name.
            if (!_CollectMultipleApplyAPISchemaNamespaces(customDataDict)) {
                hasErrors = true;
            }

            _AddSchema(generatedSchema, _registry->_schematics);

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

// Helper that gets the authored API schemas from the schema prim path in the
// schematics layer and prepends them to the given applied API schemas list.
void 
UsdSchemaRegistry::_SchemaDefInitHelper::_PrependAPISchemasFromSchemaPrim(
    const SdfPath &schematicsPrimPath,
    TfTokenVector *appliedAPISchemas)
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

// For the single API schema prim definition in depCompInfo, that is in the 
// process of being built, this takes the list of appliedAPISchemas and 
// recursively gathers all the API schemas that the prim definition needs to 
// include as well as the prim specs that will provide the properties. This
// is all stored back in the depCompInfo in preparation for the final population
// of the prim definition. This step is expected to be run after all prim 
// definitions representing API schemas have been established with their 
// directly included API schemas. but before any of the API schema prim 
// definitions have been updated with their fully expanded API schemas list.
void 
UsdSchemaRegistry::_SchemaDefInitHelper::_GatherAllAPISchemaPrimSpecsToCompose(
    _SchemaDefCompositionInfo *defCompInfo,
    const TfTokenVector &appliedAPISchemas) const
{
    for (const TfToken &apiSchemaName : appliedAPISchemas) {
        // This mainly to avoid API schema inclusion cycles but also has the
        // added benefit of avoiding duplicates if included API schemas 
        // themselves include the same other API schema.
        // 
        // Note that we linear search the vector of API schema names. This 
        // vector is expected to be small and shouldn't be a performance 
        // concern. But it's something to be aware of in case it does cause 
        // performance problems in the future, especially since by far the most 
        // common case will be that we don't find the name in the list.
        if (std::find(defCompInfo->allAPISchemaNames.begin(), 
                      defCompInfo->allAPISchemaNames.end(), 
                      apiSchemaName) 
                != defCompInfo->allAPISchemaNames.end()) {
            continue;
        }

        // Find the registered prim definition (and property prefix if it's a
        // multiple apply instance). Skip this API if we can't find a def.
        std::string propPrefix;
        const UsdPrimDefinition *apiSchemaTypeDef = 
            _registry->_FindAPIPrimDefinitionByFullName(
                apiSchemaName, &propPrefix);
        if (!apiSchemaTypeDef) {
            continue;
        }

        // The found API schema def may not be fully populated at this point,
        // but it will always have its prim spec path stored. Get the prim spec 
        // for this API def.
        SdfPrimSpecHandle primSpec = 
            _registry->_schematics->GetPrimAtPath(
                apiSchemaTypeDef->_schematicsPrimPath);
        if (!primSpec) {
            continue;
        }

        // Add this API schema and its prim spec to the composition 
        defCompInfo->schemaPrimSpecsToCompose.emplace_back(
            primSpec, propPrefix);
        defCompInfo->allAPISchemaNames.push_back(apiSchemaName);

        // At this point in initialization, all API schemas prim defs will 
        // have their directly included API schemas set in the definition, but 
        // will not have had them expanded to include APIs included from other
        // APIs. Thus, we can do a depth first recursion on the current applied
        // API schemas of the API prim definition to continue expanding the 
        // full list of API schemas and prim specs to compose in strength order.
        _GatherAllAPISchemaPrimSpecsToCompose(
            defCompInfo,
            apiSchemaTypeDef->GetAppliedAPISchemas());
    }
}

void
UsdSchemaRegistry::_SchemaDefInitHelper::
_PopulateMultipleApplyAPIPrimDefinitions()
{
    // Populate all multiple apply API schema definitions. These can't 
    // include other API schemas so they're populated directly from the 
    // their prim spec in the schematics.
    for (auto &nameAndDefPtr : _registry->_multiApplyAPIPrimDefinitions) {
        UsdPrimDefinition *&primDef = nameAndDefPtr.second.primDef;
        if (!TF_VERIFY(primDef)) {
            continue;
        }

        SdfPrimSpecHandle primSpec = 
            _registry->_schematics->GetPrimAtPath(primDef->_schematicsPrimPath);
        if (!primSpec) {
            // XXX: This, and the warnings below, should probably be coding
            // errors. However a coding error here causes usdGenSchema to 
            // fail when there are plugin schema types that are missing a 
            // generated schema prim spec. Since running usdGenSchema is how 
            // you'd fix this error, that's not helpful. 
            TF_WARN("Could not find a prim spec at path '%s' in the "
                    "schematics layer for registered multiple apply "
                    "API schema '%s'. Schemas need to be regenerated.",
                    primDef->_schematicsPrimPath.GetText(),
                    nameAndDefPtr.first.GetText());
            continue;
        }

        // Compose the properties from the prim spec to the prim definition.
        primDef->_ComposePropertiesFromPrimSpec(primSpec);
    }
}

void
UsdSchemaRegistry::_SchemaDefInitHelper::
_PopulateSingleApplyAPIPrimDefinitions()
{
    // Single apply schemas are more complicated. These may contain other 
    // applied API schemas which may also include other API schemas. To populate
    // their properties correctly, we must do this in multiple passes.
    std::vector<_SchemaDefCompositionInfo> apiSchemasWithAppliedSchemas;

    // Step 1. For each single apply API schema, we determine what (if any) 
    // applied API schemas it has. If it has none, we can just populate its 
    // prim definition from the schematics prim spec and be done. Otherwise we
    // need to store the directly included built-in API schemas now and process
    // them in the next pass once we know ALL the API schemas that every single
    // apply API includes.
    for (auto &nameAndDefPtr : _registry->_singleApplyAPIPrimDefinitions) {
        const TfToken &usdTypeNameToken = nameAndDefPtr.first;
        UsdPrimDefinition *&primDef = nameAndDefPtr.second;
        if (!TF_VERIFY(primDef)) {
            continue;
        }

        SdfPrimSpecHandle primSpec = 
            _registry->_schematics->GetPrimAtPath(primDef->_schematicsPrimPath);
        if (!primSpec) {
            TF_WARN("Could not find a prim spec at path '%s' in the "
                    "schematics layer for registered single apply "
                    "API schema '%s'. Schemas need to be regenerated.",
                    primDef->_schematicsPrimPath.GetText(),
                    nameAndDefPtr.first.GetText());
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

        if (primDef->_appliedAPISchemas.empty()) {
            // If there are no API schemas to apply to this schema, we can just
            // apply the prim specs properties and be done.
            primDef->_ComposePropertiesFromPrimSpec(primSpec);
            // We always include the API schema itself as an applied API schema
            // in its prim definition. Note that we don't do this for multiple
            // apply schema definitions which cannot be applied without an
            // instance name.
            primDef->_appliedAPISchemas = {usdTypeNameToken};
        } else {
            apiSchemasWithAppliedSchemas.emplace_back(
                primDef, usdTypeNameToken, primSpec);
        }
    }

    // Step 2. For each single apply API that has other applied API schemas,
    // recursively gather the fully expanded list of API schemas and their 
    // corresponding prim specs that will be used to populate the prim 
    // definition's properties.
    // 
    // We specifically do this step here because each API schema prim 
    // definition will have only its direct built-in API schemas in its list
    // allowing us to recurse without cycling. Only once we've gathered what 
    // will be the fully expanded list of API schemas for all of them can we
    // start to populate the API schemas with all their properties. 
    for (_SchemaDefCompositionInfo &defCompInfo : apiSchemasWithAppliedSchemas) {
        _GatherAllAPISchemaPrimSpecsToCompose(
            &defCompInfo,
            defCompInfo.primDef->_appliedAPISchemas);
    }

    // Step 3. For each single apply API schema from step 2, we can update the 
    // prim definition by applying the properties from all the gathered prim 
    // specs and setting the fully expanded list of API schemas (which will 
    // include itself).
    for (_SchemaDefCompositionInfo &defCompInfo : apiSchemasWithAppliedSchemas) {
        for (const auto &primSpecAndPropPrefix : 
                defCompInfo.schemaPrimSpecsToCompose) {
            defCompInfo.primDef->_ComposePropertiesFromPrimSpec(
                primSpecAndPropPrefix.first, 
                primSpecAndPropPrefix.second);
        }
        defCompInfo.primDef->_appliedAPISchemas = 
            std::move(defCompInfo.allAPISchemaNames);
    }
}

void 
UsdSchemaRegistry::_SchemaDefInitHelper::
_PopulateConcretePrimDefinitions()
{
    // Populate all concrete API schema definitions; it is expected that all 
    // API schemas, which these may depend on, have already been populated.
    for (auto &nameAndDefPtr : _registry->_concreteTypedPrimDefinitions) {
        UsdPrimDefinition *&primDef = nameAndDefPtr.second;
        if (!TF_VERIFY(primDef)) {
            continue;
        }

        SdfPrimSpecHandle primSpec = 
            _registry->_schematics->GetPrimAtPath(primDef->_schematicsPrimPath);
        if (!primSpec) {
            TF_WARN("Could not find a prim spec at path '%s' in the "
                    "schematics layer for registered concrete typed "
                    "schema '%s'. Schemas need to be regenerated.",
                    primDef->_schematicsPrimPath.GetText(),
                    nameAndDefPtr.first.GetText());
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

        // Compose the properties from the prim spec to the prim definition
        // first as these are stronger than the built-in API schema properties.
        primDef->_ComposePropertiesFromPrimSpec(primSpec);

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
    static TfHashSet<TfToken, TfToken::HashFunctor> disallowedFields;

    // XXX -- Use this instead of an initializer list in case TfHashSet
    //        doesn't support initializer lists.  Should ensure that
    //        TfHashSet does support them.
    static std::once_flag once;
    std::call_once(once, [](){
        // Disallow fallback values for composition arc fields, since they
        // won't be used during composition.
        disallowedFields.insert(SdfFieldKeys->InheritPaths);
        disallowedFields.insert(SdfFieldKeys->Payload);
        disallowedFields.insert(SdfFieldKeys->References);
        disallowedFields.insert(SdfFieldKeys->Specializes);
        disallowedFields.insert(SdfFieldKeys->VariantSelection);
        disallowedFields.insert(SdfFieldKeys->VariantSetNames);

        // Disallow customData, since it contains information used by
        // usdGenSchema that isn't relevant to other consumers.
        disallowedFields.insert(SdfFieldKeys->CustomData);

        // Disallow fallback values for these fields, since they won't be
        // used during scenegraph population or value resolution.
        disallowedFields.insert(SdfFieldKeys->Active);
        disallowedFields.insert(SdfFieldKeys->Instanceable);
        disallowedFields.insert(SdfFieldKeys->TimeSamples);
        disallowedFields.insert(SdfFieldKeys->ConnectionPaths);
        disallowedFields.insert(SdfFieldKeys->TargetPaths);

        // Disallow fallback values for specifier. Even though it will always
        // be present, it has no meaning as a fallback value.
        disallowedFields.insert(SdfFieldKeys->Specifier);

        // Disallow fallback values for children fields.
        disallowedFields.insert(SdfChildrenKeys->allTokens.begin(),
                                SdfChildrenKeys->allTokens.end());

        // Disallow fallback values for clip-related fields, since they won't
        // be used during value resolution.
        const std::vector<TfToken> clipFields = UsdGetClipRelatedFields();
        disallowedFields.insert(clipFields.begin(), clipFields.end());
    });

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
    if (apiSchemaDef->_propPathMap.count(baseName)) {
        return false;
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

TfToken 
UsdSchemaRegistry::GetPropertyNamespacePrefix(
    const TfToken &multiApplyAPISchemaName) const
{
    if (const _MultipleApplyAPIDefinition *def = TfMapLookupPtr(
            _multiApplyAPIPrimDefinitions, multiApplyAPISchemaName)) {
        return def->propertyNamespace;
    }
    return TfToken();
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
        composedPrimDef->_ComposePropertiesFromPrimDef(*primDef);
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
    std::string *propertyPrefix) const
{
    // Applied schemas may be single or multiple apply so we have to parse
    // the full schema name into a type and possibly an instance name.
    auto typeNameAndInstance = GetTypeNameAndInstance(apiSchemaName);
    const TfToken &typeName = typeNameAndInstance.first;
    const TfToken &instanceName = typeNameAndInstance.second;

    // If the instance name is empty we expect a single apply API schema 
    // otherwise it should be a multiple apply API.
    if (instanceName.IsEmpty()) {
        if (const UsdPrimDefinition * const *apiSchemaTypeDef = 
                TfMapLookupPtr(_singleApplyAPIPrimDefinitions, typeName)) {
            return *apiSchemaTypeDef;
        }
    } else {
        const _MultipleApplyAPIDefinition *multiApplyDef =
            TfMapLookupPtr(_multiApplyAPIPrimDefinitions, typeName);
        if (multiApplyDef) {
            // We also provide the full property namespace prefix for this 
            // particular instance of the multiple apply API.
            *propertyPrefix = SdfPath::JoinIdentifier(
                multiApplyDef->propertyNamespace, instanceName);
            return multiApplyDef->primDef;
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

        std::string propPrefix;
        const UsdPrimDefinition *apiSchemaTypeDef = 
            _FindAPIPrimDefinitionByFullName(apiSchemaName, &propPrefix);

        if (apiSchemaTypeDef) {
            // Compose in the properties from the API schema def.
            primDef->_ComposePropertiesFromPrimDef(
                *apiSchemaTypeDef, propPrefix);

            // Append all the API schemas included in the schema def to the 
            // prim def's API schemas list.
            const TfTokenVector &apiSchemasToAppend = 
                apiSchemaTypeDef->GetAppliedAPISchemas();

            if (apiSchemasToAppend.empty()) {
                // The API def's applied API schemas list will be empty if it's 
                // multiple apply API so in that case we append the schema name.
                primDef->_appliedAPISchemas.push_back(apiSchemaName);
            } else {
                // Otherwise, it's a single apply API and its definition's API 
                // schemas list will hold itself followed by all other API 
                // schemas that were composed into its definition.
                primDef->_appliedAPISchemas.insert(
                    primDef->_appliedAPISchemas.end(),
                    apiSchemasToAppend.begin(), apiSchemasToAppend.end());
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

