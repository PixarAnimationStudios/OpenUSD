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

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/work/loops.h"

#include <set>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using std::set;
using std::string;
using std::vector;

TF_INSTANTIATE_SINGLETON(UsdSchemaRegistry);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (appliedAPISchemas)
    (multipleApplyAPISchemas)
    (multipleApplyAPISchemaPrefixes)
    (autoApplyAPISchemas)

    (apiSchemaAutoApplyTo)
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
}; 

// Static singleton accessor
static const _TypeMapCache &_GetTypeMapCache() {
    static _TypeMapCache typeCache;
    return typeCache;
}

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

// This result struct is useful for handling the fact that we're going to 
// support backwards compatibility (for a bit) with schemas that were generated 
// before usdGenSchema has started putting schema kind in the plugInfo. When we
// can't find schema kind metadata we need to know if it's because the metadata
// doesn't exist yet or if the type is just not a schema type.
namespace {
struct _SchemaKindResult {
    UsdSchemaKind schemaKind;
    bool isSchemaType;
};
}

static _SchemaKindResult 
_GetSchemaKind(const TfType &schemaType)
{
    const _TypeMapCache & typeMapCache = _GetTypeMapCache();
    auto it = typeMapCache.typeToName.find(schemaType);
    if (it == typeMapCache.typeToName.end()) {
        // No schema kind because it is not a schema type.
        return {UsdSchemaKind::Invalid, false};
    }
    // Is a valid schema type.
    return {_GetSchemaKindFromPlugin(schemaType), true};
}

static _SchemaKindResult 
_GetSchemaKind(const TfToken &typeName)
{
    const _TypeMapCache & typeMapCache = _GetTypeMapCache();
    auto it = typeMapCache.nameToType.find(typeName);
    if (it == typeMapCache.nameToType.end()) {
        // No schema kind because it is not a schema type.
        return {UsdSchemaKind::Invalid, false};
    }
    // Is a valid schema type.
    return {_GetSchemaKindFromPlugin(it->second.type), true};
}

/*static*/
UsdSchemaKind 
UsdSchemaRegistry::GetSchemaKind(const TfType &schemaType)
{
    return _GetSchemaKind(schemaType).schemaKind;
}

/*static*/
UsdSchemaKind 
UsdSchemaRegistry::GetSchemaKind(const TfToken &typeName)
{
    return _GetSchemaKind(typeName).schemaKind;
}

/*static*/
bool 
UsdSchemaRegistry::IsConcrete(const TfType& primType)
{
    const _SchemaKindResult result = _GetSchemaKind(primType);
    if (!result.isSchemaType) {
        return false;
    }
    // XXX: Backward compatibility with valid schema types that haven't been 
    // regenerated since schemaKind was added to the plugInfo. We have to 
    // instantiate the registry and look for a prim definition.
    if (result.schemaKind == UsdSchemaKind::Invalid) {
        return UsdSchemaRegistry::GetInstance()._HasConcretePrimDefinition(
            GetSchemaTypeName(primType));
    }
    return _IsConcreteSchemaKind(result.schemaKind);
}

/*static*/
bool 
UsdSchemaRegistry::IsConcrete(const TfToken& primType)
{
    const _SchemaKindResult result = _GetSchemaKind(primType);
    if (!result.isSchemaType) {
        return false;
    }
    // XXX: Backward compatibility with valid schema types that haven't been 
    // regenerated since schemaKind was added to the plugInfo. We have to 
    // instantiate the registry and look for a prim definition.
    if (result.schemaKind == UsdSchemaKind::Invalid) {
        return UsdSchemaRegistry::GetInstance()._HasConcretePrimDefinition(
            primType);
    }
    return _IsConcreteSchemaKind(result.schemaKind);
}

/*static*/
bool 
UsdSchemaRegistry::IsMultipleApplyAPISchema(const TfType& apiSchemaType)
{
    const _SchemaKindResult result = _GetSchemaKind(apiSchemaType);
    if (!result.isSchemaType) {
        return false;
    }
    // XXX: Backward compatibility with valid schema types that haven't been 
    // regenerated since schemaKind was added to the plugInfo. We have to 
    // instantiate the registry and look for a prim definition.
    if (result.schemaKind == UsdSchemaKind::Invalid) {
        return UsdSchemaRegistry::GetInstance().
            _HasMultipleApplyAPIPrimDefinition(
                GetSchemaTypeName(apiSchemaType));
    }
    return _IsMultipleApplySchemaKind(result.schemaKind);
}

/*static*/
bool 
UsdSchemaRegistry::IsMultipleApplyAPISchema(const TfToken& apiSchemaType)
{
    const _SchemaKindResult result = _GetSchemaKind(apiSchemaType);
    if (!result.isSchemaType) {
        return false;
    }
    // XXX: Backward compatibility with valid schema types that haven't been 
    // regenerated since schemaKind was added to the plugInfo. We have to 
    // instantiate the registry and look for a prim definition.
    if (result.schemaKind == UsdSchemaKind::Invalid) {
        return UsdSchemaRegistry::GetInstance().
            _HasMultipleApplyAPIPrimDefinition(apiSchemaType);
    }
    return _IsMultipleApplySchemaKind(result.schemaKind);
}

/*static*/
bool 
UsdSchemaRegistry::IsAppliedAPISchema(const TfType& apiSchemaType)
{
    const _SchemaKindResult result = _GetSchemaKind(apiSchemaType);
    if (!result.isSchemaType) {
        return false;
    }
    // XXX: Backward compatibility with valid schema types that haven't been 
    // regenerated since schemaKind was added to the plugInfo. We have to 
    // instantiate the registry and look for a prim definition.
    if (result.schemaKind == UsdSchemaKind::Invalid) {
        return UsdSchemaRegistry::GetInstance().
            _HasAppliedAPIPrimDefinition(
                GetSchemaTypeName(apiSchemaType));
    }
    return _IsAppliedAPISchemaKind(result.schemaKind);
}

/*static*/
bool 
UsdSchemaRegistry::IsAppliedAPISchema(const TfToken& apiSchemaType)
{
    const _SchemaKindResult result = _GetSchemaKind(apiSchemaType);
    if (!result.isSchemaType) {
        return false;
    }
    // XXX: Backward compatibility with valid schema types that haven't been 
    // regenerated since schemaKind was added to the plugInfo. We have to 
    // instantiate the registry and look for a prim definition.
    if (result.schemaKind == UsdSchemaKind::Invalid) {
        return UsdSchemaRegistry::GetInstance().
            _HasAppliedAPIPrimDefinition(apiSchemaType);
    }
    return _IsAppliedAPISchemaKind(result.schemaKind);
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

// Gets the names of all applied API schema types.
// XXX: Note that this only gets the API schema types that have their kind set
// in plugin metadata. This will eventually be all schemas once they've been 
// regenerated with usdGenSchema.
static TfToken::HashSet
_GetAppliedAPISchemaNames()
{
    TfToken::HashSet result;

    // Get all types that derive UsdSchemaBase by getting the type map cache.
    const _TypeMapCache &typeCache = _GetTypeMapCache();

    for (const auto &valuePair : typeCache.typeToName) {
        const TfType &type = valuePair.first;
        const TfToken &typeName = valuePair.second.name;

        if (!valuePair.second.isTyped &&
            _IsAppliedAPISchemaKind(_GetSchemaKind(type).schemaKind)) {
            result.insert(typeName);
        }
    }
    return result;
}

static bool
_CollectAppliedAPISchemaNames(
    const VtDictionary &customDataDict,
    TfToken::HashSet *appliedAPISchemaNames)
{
    auto it = customDataDict.find(_tokens->appliedAPISchemas);
    if (it == customDataDict.end()) {
        return true;
    }

    if (!it->second.IsHolding<VtStringArray>()) {
        TF_CODING_ERROR("Found an unexpected value type for layer customData "
            "key '%s'; expected a string array. Applied API schemas may be "
            "incorrect.",
            _tokens->appliedAPISchemas.GetText());
        return false;
    }

    const VtStringArray &appliedAPISchemas = 
        it->second.UncheckedGet<VtStringArray>();
    for (const auto &apiSchemaName : appliedAPISchemas) {
        appliedAPISchemaNames->insert(TfToken(apiSchemaName));
    }
    return true;
}

static bool
_CollectMultipleApplyAPISchemaNamespaces(
    const VtDictionary &customDataDict,
    _TokenToTokenMap *multipleApplyAPISchemaNamespaces)
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

        (*multipleApplyAPISchemaNamespaces)[apiSchemaName] = 
             TfToken(it.second.UncheckedGet<std::string>());
    }
    return success;
}

static TfTokenVector
_GetApiSchemaAutoApplyToNamesFromMetadata(const JsObject &dict)
{
    const JsValue *autoApplyToValue = 
        TfMapLookupPtr(dict, _tokens->apiSchemaAutoApplyTo);
    if (!autoApplyToValue) {
        return TfTokenVector();
    }

    if (!autoApplyToValue->IsArrayOf<std::string>()) {
        TF_CODING_ERROR("Plugin metadata value for key '%s' does not hold a "
                        "string array", 
                        _tokens->apiSchemaAutoApplyTo.GetText());
        return TfTokenVector();
    }
    return TfToTokenVector(autoApplyToValue->GetArrayOf<std::string>());
}

/*static*/
void
UsdSchemaRegistry::CollectAddtionalAutoApplyAPISchemasFromPlugins(
    std::map<TfToken, TfTokenVector> *autoApplyAPISchemas)
{
    TRACE_FUNCTION();

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
                _GetApiSchemaAutoApplyToNamesFromMetadata(
                    entry.second.GetJsObject());

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

static std::map<TfToken, TfTokenVector> 
_GetAutoApplyAPISchemas()
{
    TRACE_FUNCTION();

    std::map<TfToken, TfTokenVector> result;

    // Get all types that derive UsdSchemaBase by getting the type map cache.
    const _TypeMapCache &typeCache = _GetTypeMapCache();

    for (const auto &valuePair : typeCache.typeToName) {
        const TfType &type = valuePair.first;
        PlugPluginPtr plugin =
            PlugRegistry::GetInstance().GetPluginForType(type);
        if (!plugin) {
            TF_CODING_ERROR("Failed to find plugin for schema type '%s'",
                            type.GetTypeName().c_str());
            continue;
        }

        // We don't load the plugin, we just use its metadata.
        const JsObject dict = plugin->GetMetadataForType(type);

        // Only single apply API schemas can be auto applied
        if (_GetSchemaKindFromMetadata(dict) != UsdSchemaKind::SingleApplyAPI) {
            continue;
        }

        TfTokenVector apiSchemaAutoApplyToNames = 
            _GetApiSchemaAutoApplyToNamesFromMetadata(dict);

        if (!apiSchemaAutoApplyToNames.empty()) {
            TF_DEBUG(USD_AUTO_APPLY_API_SCHEMAS).Msg(
                "API schema '%s' is defined to auto apply to the following "
                "schema types: [%s].\n",
                valuePair.second.name.GetText(),
                TfStringJoin(apiSchemaAutoApplyToNames.begin(), 
                             apiSchemaAutoApplyToNames.end(), ", ").c_str());

            result.emplace(
                valuePair.second.name, std::move(apiSchemaAutoApplyToNames));
        }
    }

    // Collect any plugin auto apply API schema mappings. These can be defined 
    // in any plugin to auto apply schemas in a particular application context 
    // instead of the type itself being defined to always auto apply whenever 
    // it is present.
    UsdSchemaRegistry::CollectAddtionalAutoApplyAPISchemasFromPlugins(&result);

    return result;
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
            if (it != typeMapCache.nameToType.end() && it->second.isTyped) {
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

void
UsdSchemaRegistry::_FindAndAddPluginSchema()
{
    // Get all types that derive UsdSchemaBase by getting the type map cache.
    const _TypeMapCache &typeCache = _GetTypeMapCache();

    // Get all the plugins that provide the types.
    std::vector<PlugPluginPtr> plugins;
    for (const auto &valuePair : typeCache.typeToName) {
        if (PlugPluginPtr plugin =
            PlugRegistry::GetInstance().GetPluginForType(valuePair.first)) {

            auto insertIt = 
                std::lower_bound(plugins.begin(), plugins.end(), plugin);
            if (insertIt == plugins.end() || *insertIt != plugin) {
                plugins.insert(insertIt, plugin);
            }
        }
    }
    
    // For each plugin, if it has generated schema, add it to the schematics.
    std::vector<SdfLayerRefPtr> generatedSchemas(plugins.size());
    {
        WorkArenaDispatcher dispatcher;
        dispatcher.Run([&plugins, &generatedSchemas]() {
            WorkParallelForN(
                plugins.size(), 
                [&plugins, &generatedSchemas](size_t begin, size_t end) {
                    for (; begin != end; ++begin) {
                        generatedSchemas[begin] = 
                            _GetGeneratedSchema(plugins[begin]);
                    }
                });
            });
    }

    SdfChangeBlock block;
    TfToken::HashSet appliedAPISchemaNames = _GetAppliedAPISchemaNames();
    _TypeToTokenVecMap typeToAutoAppliedAPISchemaNames =
        _GetTypeToAutoAppliedAPISchemaNames();

    for (const SdfLayerRefPtr& generatedSchema : generatedSchemas) {
        if (generatedSchema) {
            VtDictionary customDataDict = generatedSchema->GetCustomLayerData();

            bool hasErrors = false;

            // XXX: For backwards compatibility with schemas that haven't been
            // regenerated, we still collect API schema names from the 
            // generated schemas if present as they won't be obtained from 
            // _GetAppliedSchemaNames with no schema kind plugin metadata 
            // defined.
            if (!_CollectAppliedAPISchemaNames(
                    customDataDict, &appliedAPISchemaNames)) {
                hasErrors = true;
            }

            if (!_CollectMultipleApplyAPISchemaNamespaces(
                    customDataDict, &_multipleApplyAPISchemaNamespaces)) {
                hasErrors = true;
            }

            _AddSchema(generatedSchema, _schematics);

            // Schema generation will have added any defined fallback prim 
            // types as a dictionary in layer metadata which will be composed
            // into the single fallback types dictionary.
            VtDictionary generatedFallbackPrimTypes;
            if (generatedSchema->HasField(SdfPath::AbsoluteRootPath(), 
                                          UsdTokens->fallbackPrimTypes,
                                          &generatedFallbackPrimTypes)) {
                for (const auto &it: generatedFallbackPrimTypes) {
                    if (it.second.IsHolding<VtTokenArray>()) {
                        _fallbackPrimTypes.insert(it);
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

    // Concrete typed prim schemas may contain a list of apiSchemas in their 
    // schema prim definition which affect their set of fallback properties. 
    // For these prim types, we'll need to defer the creation of their prim 
    // definitions until all the API schema prim definitions have been created.
    // So we'll store the necessary info about these prim types in this struct
    // so we can create their definitions after the main loop.
    struct _PrimDefInfo {
        TfToken usdTypeNameToken;
        SdfPrimSpecHandle primSpec;
        TfTokenVector apiSchemasToApply;

        _PrimDefInfo(const TfToken &usdTypeNameToken_,
                     const SdfPrimSpecHandle &primSpec_)
        : usdTypeNameToken(usdTypeNameToken_)
        , primSpec(primSpec_) {}
    };
    std::vector<_PrimDefInfo> primTypesWithAPISchemas;

    // Create the prim definitions for all the named concrete and API schemas
    // we found types for.
    for (const auto &valuePair: typeCache.nameToType) {
        // We register prim definitions by the schema type name which we already
        // grabbed from the TfType alias, and is also the name of the defining 
        // prim in the schema layer. The actual TfType's typename 
        // (i.e. C++ type name) is not a valid typename for a prim.
        const TfToken &usdTypeNameToken = valuePair.first;
        SdfPath primPath = SdfPath::AbsoluteRootPath().
            AppendChild(usdTypeNameToken);

        // We only map type names for types that have an underlying prim
        // spec, i.e. concrete and API schema types.
        SdfPrimSpecHandle primSpec = _schematics->GetPrimAtPath(primPath);
        if (primSpec) {
            // If the prim spec doesn't have a type name, then it's an
            // API schema
            if (primSpec->GetTypeName().IsEmpty()) {
                // Non-apply API schemas also have prim specs so make sure
                // this is actually an applied schema before adding the 
                // prim definition to applied API schema map.
                if (appliedAPISchemaNames.find(usdTypeNameToken) != 
                        appliedAPISchemaNames.end()) {
                    // Add it to the map using the USD type name.
                    _appliedAPIPrimDefinitions[usdTypeNameToken] = 
                        new UsdPrimDefinition(primSpec, 
                                              /*isAPISchema=*/ true);
                }
            } else {
                // Otherwise it's a concrete type. We need to see if it requires
                // any applied API schemas.
                TfTokenVector apiSchemasToApply;

                // First check for any applied API schemas defined in the 
                // metadata for the type in the schematics.
                SdfTokenListOp apiSchemasListOp;
                if (_schematics->HasField(primPath, UsdTokens->apiSchemas, 
                                          &apiSchemasListOp)) {
                    apiSchemasListOp.ApplyOperations(&apiSchemasToApply);
                }
                // Next, check if there are any API schemas that have been 
                // setup to apply to this type. We add these after the metadata
                // metadata defined API schemas so that auto applied APIs are
                // weaker.
                if (const TfTokenVector *autoAppliedAPIs = 
                        TfMapLookupPtr(typeToAutoAppliedAPISchemaNames, 
                                       valuePair.second.type)) {
                    TF_DEBUG(USD_AUTO_APPLY_API_SCHEMAS).Msg(
                        "The prim definition for schema type '%s' has "
                        "these additional built-in auto applied API "
                        "schemas: [%s].\n",
                        usdTypeNameToken.GetText(),
                        TfStringJoin(autoAppliedAPIs->begin(), 
                                     autoAppliedAPIs->end(), ", ").c_str());

                    apiSchemasToApply.insert(apiSchemasToApply.end(), 
                                             autoAppliedAPIs->begin(),
                                             autoAppliedAPIs->end());
                }

                // If it has no API schemas, add the new prim definition to the 
                // concrete typed schema map also using both the USD and TfType 
                // name. Otherwise we defer the creation of the prim definition
                // until all API schema definitions have processed..
                if (apiSchemasToApply.empty()) {
                    _concreteTypedPrimDefinitions[usdTypeNameToken] = 
                        new UsdPrimDefinition(primSpec, 
                                              /*isAPISchema=*/ false);
                } else {
                    primTypesWithAPISchemas.emplace_back(
                        usdTypeNameToken, primSpec);
                    primTypesWithAPISchemas.back().apiSchemasToApply = 
                        std::move(apiSchemasToApply);
                }
            }
        }
    }

    // All valid API schema prim definitions now exist so create the concrete
    // typed prim definitions that require API schemas.
    for (const auto &it : primTypesWithAPISchemas) {
        // We create an empty prim definition, apply the API schemas and then
        // add the typed prim spec. This is specifically because the authored
        // opinions on the prim spec are stronger than the API schema fallbacks
        // here.
        UsdPrimDefinition *primDef =
            _concreteTypedPrimDefinitions[it.usdTypeNameToken] = 
            new UsdPrimDefinition();
        _ApplyAPISchemasToPrimDefinition(primDef, it.apiSchemasToApply);
        primDef->_SetPrimSpec(it.primSpec, /*providesPrimMetadata=*/ true);
    }
}

UsdSchemaRegistry::UsdSchemaRegistry()
{
    _schematics = SdfLayer::CreateAnonymous("registry.usda");
    _emptyPrimDefinition = new UsdPrimDefinition();

    // Find and load all the generated schema in plugin libraries.  We find thes
    // files adjacent to pluginfo files in libraries that provide subclasses of
    // UsdSchemaBase.
    _FindAndAddPluginSchema();

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

bool 
UsdSchemaRegistry::_HasConcretePrimDefinition(const TfToken& primType) const
{
    return _concreteTypedPrimDefinitions.find(primType) != 
        _concreteTypedPrimDefinitions.end();
}

bool 
UsdSchemaRegistry::_HasMultipleApplyAPIPrimDefinition(
    const TfToken& apiSchemaType) const
{
    return IsAppliedAPISchema(apiSchemaType) && 
        (_multipleApplyAPISchemaNamespaces.find(apiSchemaType) !=
         _multipleApplyAPISchemaNamespaces.end());
}

bool 
UsdSchemaRegistry::_HasAppliedAPIPrimDefinition(
    const TfToken& apiSchemaType) const
{
    return _appliedAPIPrimDefinitions.find(apiSchemaType) != 
        _appliedAPIPrimDefinitions.end();
}

TfType
UsdSchemaRegistry::GetTypeFromName(const TfToken& typeName){
    static const TfType schemaBaseType = TfType::Find<UsdSchemaBase>();
    return PlugRegistry::GetInstance().FindDerivedTypeByName(
        schemaBaseType, typeName.GetString());
}

std::pair<TfToken, TfToken>
UsdSchemaRegistry::GetTypeAndInstance(const TfToken &apiSchemaName)
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
    static const std::map<TfToken, TfTokenVector> result = 
        _GetAutoApplyAPISchemas();
    return result;
}

TfToken 
UsdSchemaRegistry::GetPropertyNamespacePrefix(
    const TfToken &multiApplyAPISchemaName) const
{
    const TfToken *prefix = TfMapLookupPtr(
        _multipleApplyAPISchemaNamespaces, multiApplyAPISchemaName);
    return prefix ? *prefix : TfToken();
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

    // Start with a copy of the prim definition for the typed prim type. Note 
    // that its perfectly valid for there to be no prim definition for the given
    // type, in which case we start with an empty prim definition.
    const UsdPrimDefinition *primDef = FindConcretePrimDefinition(primType);
    std::unique_ptr<UsdPrimDefinition> composedPrimDef(
        primDef ? new UsdPrimDefinition(*primDef) : new UsdPrimDefinition());

    // Now we'll add in properties from each applied API schema in order. Note
    // that in this loop, if we encounter a property name that already exists
    // we overwrite it. This will be rare and discouraged in practice, but this
    // is policy in property name conflicts from applied schemas.
    _ApplyAPISchemasToPrimDefinition(composedPrimDef.get(), appliedAPISchemas);
        
    return composedPrimDef;
}

void UsdSchemaRegistry::_ApplyAPISchemasToPrimDefinition(
    UsdPrimDefinition *primDef, const TfTokenVector &appliedAPISchemas) const
{
    // Prepend the new applied schema names to the existing applied schemas for
    // prim definition.
    primDef->_appliedAPISchemas.insert(primDef->_appliedAPISchemas.begin(), 
        appliedAPISchemas.begin(), appliedAPISchemas.end());

    // Now we'll add in properties from each new applied API schema in order. 
    // Note that applied API schemas are ordered strongest to weakest so we
    // apply in reverse order, overwriting a property's path if we encounter a 
    // duplicate property name.
    for (auto it = appliedAPISchemas.crbegin(); 
         it != appliedAPISchemas.crend(); ++it) {
        const TfToken &schema = *it;

        // Applied schemas may be single or multiple apply so we have to parse
        // the schema name into a type and possibly an instance name.
        auto typeAndInstance = GetTypeAndInstance(schema);

        // From the type we should able to find an existing prim definition for
        // the API schema type if it is valid.
        const UsdPrimDefinition *apiSchemaTypeDef = 
            FindAppliedAPIPrimDefinition(typeAndInstance.first);
        if (!apiSchemaTypeDef) {
            continue;
        }

        if (typeAndInstance.second.IsEmpty()) {
            // An empty instance name indicates a single apply schema. Just 
            // copy its properties into the new prim definition.
            primDef->_ApplyPropertiesFromPrimDef(*apiSchemaTypeDef);
        } else {
            // Otherwise we have a multiple apply schema. We need to use the 
            // instance name and the property prefix to map and add the correct
            // properties for this instance.
            auto it = _multipleApplyAPISchemaNamespaces.find(
                typeAndInstance.first);
            if (it == _multipleApplyAPISchemaNamespaces.end()) {
                // Warn that this not actually a multiple apply schema type?
                continue;
            }
            const TfToken &prefix = it->second;
            if (TF_VERIFY(!prefix.IsEmpty())) {
                // The prim definition for a multiple apply schema will have its
                // properties stored with no prefix. We generate the prefix for 
                // this instance and apply it to each property name and map the
                // prefix name to the definition's property.
                const std::string propPrefix = 
                    SdfPath::JoinIdentifier(prefix, typeAndInstance.second);
                primDef->_ApplyPropertiesFromPrimDef(
                    *apiSchemaTypeDef, propPrefix);
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

