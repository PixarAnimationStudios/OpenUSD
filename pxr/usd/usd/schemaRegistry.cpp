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

using std::make_pair;
using std::set;
using std::string;
using std::vector;

TF_INSTANTIATE_SINGLETON(UsdSchemaRegistry);

TF_MAKE_STATIC_DATA(TfType, _schemaBaseType) {
    *_schemaBaseType = TfType::Find<UsdSchemaBase>();
}

TF_MAKE_STATIC_DATA(TfType, _apiSchemaBaseType) {
    *_apiSchemaBaseType = TfType::Find<UsdAPISchemaBase>();
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (appliedAPISchemas)
    (multipleApplyAPISchemas)
    (multipleApplyAPISchemaPrefixes)
);

template <class T>
static void
_CopySpec(const T &srcSpec, const T &dstSpec, 
          const std::vector<TfToken> &disallowedFields)
{
    for (const TfToken& key : srcSpec->ListInfoKeys()) {
        const bool isDisallowed = std::binary_search(
            disallowedFields.begin(), disallowedFields.end(), key,
            TfTokenFastArbitraryLessThan());
        if (!isDisallowed) {
            dstSpec->SetInfo(key, srcSpec->GetInfo(key));
        }
    }
}

static void
_AddSchema(SdfLayerRefPtr const &source, SdfLayerRefPtr const &target,
           std::vector<TfToken> const &disallowedFields)
{
    for (SdfPrimSpecHandle const &prim: source->GetRootPrims()) {
        if (!target->GetPrimAtPath(prim->GetPath())) {

            SdfPrimSpecHandle newPrim =
                SdfPrimSpec::New(target, prim->GetName(), prim->GetSpecifier(),
                                 prim->GetTypeName());
            _CopySpec(prim, newPrim, disallowedFields);

            for (SdfAttributeSpecHandle const &attr: prim->GetAttributes()) {
                SdfAttributeSpecHandle newAttr =
                    SdfAttributeSpec::New(
                        newPrim, attr->GetName(), attr->GetTypeName(),
                        attr->GetVariability(), attr->IsCustom());
                _CopySpec(attr, newAttr, disallowedFields);
            }

            for (SdfRelationshipSpecHandle const &rel:
                     prim->GetRelationships()) {
                SdfRelationshipSpecHandle newRel =
                    SdfRelationshipSpec::New(
                        newPrim, rel->GetName(), rel->IsCustom());
                _CopySpec(rel, newRel, disallowedFields);
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
    // Get all types that derive UsdSchemaBase.
    set<TfType> types;
    PlugRegistry::GetAllDerivedTypes(*_schemaBaseType, &types);

    // Get all the plugins that provide the types.
    std::vector<PlugPluginPtr> plugins;
    for (const TfType &type: types) {
        if (PlugPluginPtr plugin =
            PlugRegistry::GetInstance().GetPluginForType(type)) {

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

    // Get list of disallowed fields in schemas and sort them so that
    // helper functions in _AddSchema can binary search through them.
    std::vector<TfToken> disallowedFields = GetDisallowedFields();
    std::sort(disallowedFields.begin(), disallowedFields.end(),
              TfTokenFastArbitraryLessThan());

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

            _AddSchema(generatedSchema, _schematics, disallowedFields);
        }
    }

    // Concrete typed prim schemas may contain a list of apiSchemas in their 
    // schema prim definition which affect their set of fallback properties. 
    // For these prim types, we'll need to defer the creation of their prim 
    // definitions until all the API schema prim definitions have been created.
    // So we'll store the necessary info about these prim types in this struct
    // so we can create their definitions after the main loop.
    struct _PrimDefInfo {
        TfToken typeNameToken;
        TfToken usdTypeNameToken;
        SdfPrimSpecHandle primSpec;
        TfTokenVector fallbackAPISchemas;

        _PrimDefInfo(const TfToken &typeNameToken_,
                     const TfToken &usdTypeNameToken_,
                     const SdfPrimSpecHandle &primSpec_)
        : typeNameToken(typeNameToken_)
        , usdTypeNameToken(usdTypeNameToken_)
        , primSpec(primSpec_) {}
    };
    std::vector<_PrimDefInfo> primTypesWithAPISchemas;

    // Add them to the type -> path and typeName -> path maps, and the prim type
    // & prop name -> path maps.
    for (const TfType &type: types) {
        // The path in the schema is the type's alias under UsdSchemaBase.
        vector<string> aliases = _schemaBaseType->GetAliases(type);
        if (aliases.size() == 1) {
            SdfPath primPath = SdfPath::AbsoluteRootPath().
                AppendChild(TfToken(aliases.front()));
            // XXX: Using tokens as keys means we can look up by prim
            //      type token, rather than converting a prim type token
            //      to a TfType and looking up by that, which requires
            //      an expensive lookup (including a lock).
            //
            //      We should be registering by name only but TfType
            //      doesn't return the type name (or aliases) by TfToken
            //      and we can't afford to construct a TfToken from the
            //      string returned by TfType when looking up by
            //      type.  TfType should be fixed/augmented.
            TfToken typeNameToken(type.GetTypeName());
            const TfToken &usdTypeNameToken = primPath.GetNameToken();

            // We only map type names for types that have an underlying prim
            // spec, i.e. concrete and API schema types.
            SdfPrimSpecHandle primSpec = _schematics->GetPrimAtPath(primPath);
            if (primSpec) {
                // Map TfType to the USD type name
                _typeToUsdTypeNameMap[type] = usdTypeNameToken;

                // If the prim spec doesn't have a type name, then it's an
                // API schema
                if (primSpec->GetTypeName().IsEmpty()) {
                    // Non-apply API schemas also have prim specs so make sure
                    // this is actually an applied schema before adding the 
                    // prim definition to applied API schema map.
                    if (appliedAPISchemaNames.find(usdTypeNameToken) != 
                            appliedAPISchemaNames.end()) {
                        // Add it to the map using both the USD type name and 
                        // TfType name.
                        _appliedAPIPrimDefinitions[typeNameToken] = 
                            _appliedAPIPrimDefinitions[usdTypeNameToken] = 
                            new UsdPrimDefinition(primSpec);
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
                            typeNameToken, usdTypeNameToken, primSpec);
                        fallbackAPISchemaListOp.ApplyOperations(
                            &primTypesWithAPISchemas.back().fallbackAPISchemas);
                    } else {
                        _concreteTypedPrimDefinitions[typeNameToken] = 
                            _concreteTypedPrimDefinitions[usdTypeNameToken] = 
                            new UsdPrimDefinition(primSpec);
                    }
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
            _concreteTypedPrimDefinitions[it.typeNameToken] = 
            _concreteTypedPrimDefinitions[it.usdTypeNameToken] = 
            new UsdPrimDefinition();
        _ApplyAPISchemasToPrimDefinition(primDef, it.fallbackAPISchemas);
        _ApplyPrimSpecToPrimDefinition(primDef, it.primSpec);
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
std::vector<TfToken>
UsdSchemaRegistry::GetDisallowedFields()
{
    std::vector<TfToken> result = {
        // Disallow fallback values for composition arc fields, since they
        // won't be used during composition.
        SdfFieldKeys->InheritPaths,
        SdfFieldKeys->Payload,
        SdfFieldKeys->References,
        SdfFieldKeys->Specializes,
        SdfFieldKeys->VariantSelection,
        SdfFieldKeys->VariantSetNames,

        // Disallow customData, since it contains information used by
        // usdGenSchema that isn't relevant to other consumers.
        SdfFieldKeys->CustomData,

        // Disallow fallback values for these fields, since they won't be
        // used during scenegraph population or value resolution.
        SdfFieldKeys->Active,
        SdfFieldKeys->Instanceable,
        SdfFieldKeys->TimeSamples,
        SdfFieldKeys->ConnectionPaths,
        SdfFieldKeys->TargetPaths
    };

    // Disallow fallback values for clip-related fields, since they won't
    // be used during value resolution.
    const std::vector<TfToken> clipFields = UsdGetClipRelatedFields();
    result.insert(result.end(), clipFields.begin(), clipFields.end());

    return result;
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
    return PlugRegistry::GetInstance().FindDerivedTypeByName(
        *_schemaBaseType, typeName.GetString());
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

void UsdSchemaRegistry::_ApplyPrimSpecToPrimDefinition(
    UsdPrimDefinition *primDef, const SdfPrimSpecHandle &primSpec) const
{
    primDef->_primSpec = primSpec;
    // Adds the path for each property overwriting the property if one with
    // that name already exists but without adding duplicates to the property
    // names list.
    for (SdfPropertySpecHandle prop: primSpec->GetProperties()) {
        const TfToken &propName = prop->GetNameToken();
        const SdfPath &propPath = prop->GetPath();
        auto it = primDef->_propPathMap.insert(
            std::make_pair(propName, propPath));
        if (it.second) {
            primDef->_properties.push_back(propName);
        } else {
            it.first->second = propPath;
        }
    }
}

void UsdSchemaRegistry::_ApplyAPISchemasToPrimDefinition(
    UsdPrimDefinition *primDef, const TfTokenVector &appliedAPISchemas) const
{
    // Adds the property name with schema path to the prim def. This makes sure
    // we overwrite the original property path with the new path if it already
    // exists, but makes sure we don't end up with duplicate names in the 
    // property names list.
    auto _AddProperty = [&primDef](const std::pair<TfToken, SdfPath> &value)
    {
        auto it = primDef->_propPathMap.insert(value);
        if (it.second) {
            primDef->_properties.push_back(value.first);
        } else {
            it.first->second = value.second;
        }
    };

    // Append the new applied schema names to the existing applied schemas for
    // prim definition.
    primDef->_appliedAPISchemas.insert(primDef->_appliedAPISchemas.end(), 
        appliedAPISchemas.begin(), appliedAPISchemas.end());

    // Now we'll add in properties from each new applied API schema in order. 
    // Note that applied API schemas are ordered weakest to strongest so we 
    // overwrite a property's path if we encounter a duplicate property name.
    for (const TfToken &schema : appliedAPISchemas) {

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
            for (const auto &it : apiSchemaTypeDef->_propPathMap) {
                _AddProperty(it);
            }
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
                for (const auto &it : apiSchemaTypeDef->_propPathMap) {
                    const TfToken prefixedPropName(
                        SdfPath::JoinIdentifier(propPrefix, it.first.GetString()));
                    _AddProperty(std::make_pair(prefixedPropName, it.second));
                }
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

