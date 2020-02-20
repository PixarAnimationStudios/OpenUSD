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
                    // Otherwise it's a concrete type. Add the new prim 
                    // definition to the concrete typed schema map also using 
                    // both the USD and TfType name.
                    _concreteTypedPrimDefinitions[typeNameToken] = 
                        _concreteTypedPrimDefinitions[usdTypeNameToken] = 
                        new UsdPrimDefinition(primSpec);
                }
            }
        }
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
SdfPrimSpecHandle
UsdSchemaRegistry::GetSchemaPrimSpec(const TfToken &primType)
{
    auto const &self = GetInstance();
    const UsdPrimDefinition *primDef = 
        self.FindConcretePrimDefinition(primType);
    return primDef ? primDef->GetSchemaPrimSpec() : TfNullPtr;
}

/*static*/
SdfPrimSpecHandle
UsdSchemaRegistry::GetSchemaPrimSpec(const TfType &primType)
{
    return GetSchemaPrimSpec(GetInstance().GetSchemaTypeName(primType));
}

/*static*/
SdfPropertySpecHandle
UsdSchemaRegistry::GetSchemaPropertySpec(const TfToken& primType, 
                                         const TfToken& propName)
{
    auto const &self = GetInstance();
    const UsdPrimDefinition *primDef = 
        self.FindConcretePrimDefinition(primType);
    return primDef ? primDef->GetSchemaPropertySpec(propName) : TfNullPtr;
}

/*static*/
SdfAttributeSpecHandle
UsdSchemaRegistry::GetSchemaAttributeSpec(const TfToken& primType, 
                                          const TfToken& attrName)
{
    auto const &self = GetInstance();
    const UsdPrimDefinition *primDef = 
        self.FindConcretePrimDefinition(primType);
    return primDef ? primDef->GetSchemaAttributeSpec(attrName) : TfNullPtr;
}

/*static*/
SdfRelationshipSpecHandle
UsdSchemaRegistry::GetSchemaRelationshipSpec(const TfToken& primType, 
                                             const TfToken& relName)
{
    auto const &self = GetInstance();
    const UsdPrimDefinition *primDef = 
        self.FindConcretePrimDefinition(primType);
    return primDef ? primDef->GetSchemaRelationshipSpec(relName) : TfNullPtr;
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


PXR_NAMESPACE_CLOSE_SCOPE

