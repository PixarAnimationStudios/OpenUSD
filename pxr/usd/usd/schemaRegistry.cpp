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
);

namespace {
// Helper struct for caching a bidirecional mapping between schema TfType and
// USD type name token. This cache is used as a static local instance providing
// this type mapping without having to build the entire schema registry
struct _TypeMapCache {
    _TypeMapCache() {
        const TfType schemaBaseType = TfType::Find<UsdSchemaBase>();

        auto _MapDerivedTypes = [this, &schemaBaseType](
            const TfType &baseType, bool isConcrete) 
        {
            set<TfType> types;
            PlugRegistry::GetAllDerivedTypes(baseType, &types);
            for (const TfType &type : types) {
                // The USD type name is the type's alias under UsdSchemaBase. 
                // Only concrete typed and API schemas should have a type name 
                // alias.
                const vector<string> aliases = schemaBaseType.GetAliases(type);
                if (aliases.size() == 1) {
                    TfToken typeName(aliases.front(), TfToken::Immortal);
                    nameToType.insert(std::make_pair(
                        typeName, TypeInfo(type, isConcrete)));
                    typeToName.insert(std::make_pair(
                        type, TypeNameInfo(typeName, isConcrete)));
                }
            }
        };

        _MapDerivedTypes(TfType::Find<UsdTyped>(), /*isConcrete=*/true);
        _MapDerivedTypes(TfType::Find<UsdAPISchemaBase>(), /*isConcrete=*/false);
    }

    // For each type and type name mapping we also want to store if it's a 
    // concrete prim type vs an API schema type. 
    struct TypeInfo {
        TfType type;
        bool isConcrete;
        TypeInfo(const TfType &type_, bool isConcrete_) 
            : type(type_), isConcrete(isConcrete_) {}
    };

    struct TypeNameInfo {
        TfToken name;
        bool isConcrete;
        TypeNameInfo(const TfToken &name_, bool isConcrete_) 
            : name(name_), isConcrete(isConcrete_) {}
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
    return it != typeMapCache.typeToName.end() && it->second.isConcrete ? 
        it->second.name : TfToken();
}

/*static*/
TfToken 
UsdSchemaRegistry::GetAPISchemaTypeName(const TfType &schemaType) 
{
    const _TypeMapCache & typeMapCache = _GetTypeMapCache();
    auto it = typeMapCache.typeToName.find(schemaType);
    return it != typeMapCache.typeToName.end() && !it->second.isConcrete ? 
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
    return it != typeMapCache.nameToType.end() && it->second.isConcrete ? 
        it->second.type : TfType();
}

/*static*/
TfType 
UsdSchemaRegistry::GetAPITypeFromSchemaTypeName(const TfToken &typeName) 
{
    const _TypeMapCache & typeMapCache = _GetTypeMapCache();
    auto it = typeMapCache.nameToType.find(typeName);
    return it != typeMapCache.nameToType.end() && !it->second.isConcrete ? 
        it->second.type : TfType();
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
    TfToken::HashSet appliedAPISchemaNames;
    for (const SdfLayerRefPtr& generatedSchema : generatedSchemas) {
        if (generatedSchema) {
            VtDictionary customDataDict = generatedSchema->GetCustomLayerData();

            if (VtDictionaryIsHolding<VtStringArray>(customDataDict, 
                    _tokens->appliedAPISchemas)) {
                        
                const VtStringArray &appliedAPISchemas = 
                        VtDictionaryGet<VtStringArray>(customDataDict, 
                            _tokens->appliedAPISchemas);
                for (const auto &apiSchemaName : appliedAPISchemas) {
                    appliedAPISchemaNames.insert(TfToken(apiSchemaName));
                }
            }

            // Names of multiple apply API schemas are stored in their schemas
            // in a dictionary mapping them to their property namespace prefixes. 
            // These will be useful in mapping schema instance property names 
            // to the schema property specs.
            auto it = customDataDict.find(_tokens->multipleApplyAPISchemas);
            if (VtDictionaryIsHolding<VtDictionary>(customDataDict, 
                    _tokens->multipleApplyAPISchemas)) {
                if (!it->second.IsHolding<VtDictionary>()) {
                    TF_CODING_ERROR("Found a non-dictionary value for layer "
                        "metadata key '%s' in generated schema file '%s'. "
                        "Any multiple apply API schemas from this file will "
                        "be incorrect. This schema must be regenerated.",
                        _tokens->multipleApplyAPISchemas.GetText(),
                        generatedSchema->GetRealPath().c_str());
                    continue;
                }
                const VtDictionary &multipleApplyAPISchemas = 
                    it->second.UncheckedGet<VtDictionary>();
                for (const auto &it : multipleApplyAPISchemas) {
                    if (it.second.IsHolding<std::string>()) {
                        _multipleApplyAPISchemaNamespaces[TfToken(it.first)] = 
                            TfToken(it.second.UncheckedGet<std::string>());
                    }
                }
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
        TfTokenVector fallbackAPISchemas;

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
                // Otherwise it's a concrete type. If it has no API schemas,
                // add the new prim definition to the concrete typed schema 
                // map also using both the USD and TfType name. Otherwise
                // collect the API schemas and defer.
                SdfTokenListOp fallbackAPISchemaListOp;
                if (_schematics->HasField(primPath, UsdTokens->apiSchemas, 
                                          &fallbackAPISchemaListOp)) {
                    primTypesWithAPISchemas.emplace_back(
                        usdTypeNameToken, primSpec);
                    fallbackAPISchemaListOp.ApplyOperations(
                        &primTypesWithAPISchemas.back().fallbackAPISchemas);
                } else {
                    _concreteTypedPrimDefinitions[usdTypeNameToken] = 
                        new UsdPrimDefinition(primSpec, 
                                              /*isAPISchema=*/ false);
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
        _ApplyAPISchemasToPrimDefinition(primDef, it.fallbackAPISchemas);
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
UsdSchemaRegistry::IsConcrete(const TfType& primType) const
{
    return IsConcrete(GetSchemaTypeName(primType));
}

bool 
UsdSchemaRegistry::IsConcrete(const TfToken& primType) const
{
    return _concreteTypedPrimDefinitions.find(primType) != 
        _concreteTypedPrimDefinitions.end();
}

bool 
UsdSchemaRegistry::IsMultipleApplyAPISchema(const TfType& apiSchemaType) const
{
    return IsMultipleApplyAPISchema(GetSchemaTypeName(apiSchemaType));
}

bool 
UsdSchemaRegistry::IsMultipleApplyAPISchema(const TfToken& apiSchemaType) const
{
    return IsAppliedAPISchema(apiSchemaType) && 
        (_multipleApplyAPISchemaNamespaces.find(apiSchemaType) !=
         _multipleApplyAPISchemaNamespaces.end());
}

bool 
UsdSchemaRegistry::IsAppliedAPISchema(const TfType& apiSchemaType) const
{
    return IsAppliedAPISchema(GetSchemaTypeName(apiSchemaType));
}

bool 
UsdSchemaRegistry::IsAppliedAPISchema(const TfToken& apiSchemaType) const
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

// The type name for an applied API schema will consist of a schema type name
// and an instance name if the API schema is multiple apply. This function 
// parses that for us.
static std::pair<TfToken, TfToken> _GetTypeAndInstance(const TfToken &typeName)
{
    // Try to split the string at the first namespace delimiter. We always use
    // the first as type names can not have embedded namespaces but instances 
    // names can.
    const char namespaceDelimiter =
        SdfPathTokens->namespaceDelimiter.GetText()[0];
    const std::string &typeString = typeName.GetString();
    size_t delim = typeString.find(namespaceDelimiter);
    // If the delimiter is not found, we have a single apply API schema and 
    // no instance name.
    if (delim == std::string::npos) {
        return std::make_pair(typeName, TfToken());
    } else {
        return std::make_pair(TfToken(typeString.substr(0, delim)),
                              TfToken(typeString.c_str() + delim + 1));
    }
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
        auto typeAndInstance = _GetTypeAndInstance(schema);

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
                // the this instance and apply it to each property name and map
                // the prefix name to the definition's property.
                const std::string propPrefix = 
                    SdfPath::JoinIdentifier(prefix, typeAndInstance.second);
                primDef->_ApplyPropertiesFromPrimDef(
                    *apiSchemaTypeDef, propPrefix);
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

