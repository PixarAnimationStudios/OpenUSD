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
    return SdfLayer::OpenAsAnonymous(fname);
}

void
UsdSchemaRegistry::_BuildPrimTypePropNameToSpecIdMap(
    const TfToken &typeName, const SdfPath &primPath)
{
    // Add this prim and its properties.  Note that the spec path and
    // propertyName are intentionally leaked.  It's okay, since there's a fixed
    // set that we'd like to persist forever.
    SdfPrimSpecHandle prim = _schematics->GetPrimAtPath(primPath);
    if (!prim || prim->GetTypeName().IsEmpty())
        return;

    _primTypePropNameToSpecIdMap[make_pair(typeName, TfToken())] = 
        new SdfAbstractDataSpecId(new SdfPath(prim->GetPath()));

    for (SdfPropertySpecHandle prop: prim->GetProperties()) {
        _primTypePropNameToSpecIdMap[make_pair(typeName, prop->GetNameToken())] =
            new SdfAbstractDataSpecId(new SdfPath(prop->GetPath()));
    }
}

const SdfAbstractDataSpecId *
UsdSchemaRegistry::_GetSpecId(const TfToken &primType,
                              const TfToken &propName) const
{
    return TfMapLookupByValue(_primTypePropNameToSpecIdMap,
                              make_pair(primType, propName),
                              static_cast<const SdfAbstractDataSpecId *>(NULL));
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
    for (const SdfLayerRefPtr& generatedSchema : generatedSchemas) {
        if (generatedSchema) {
            VtDictionary customDataDict = generatedSchema->GetCustomLayerData();

            if (VtDictionaryIsHolding<VtStringArray>(customDataDict, 
                    _tokens->appliedAPISchemas)) {
                        
                const VtStringArray &appliedAPISchemas = 
                        VtDictionaryGet<VtStringArray>(customDataDict, 
                            _tokens->appliedAPISchemas);
                for (const auto &apiSchemaName : appliedAPISchemas) {
                    _appliedAPISchemaNames.insert(TfToken(apiSchemaName));
                }
            }

            if (VtDictionaryIsHolding<VtStringArray>(customDataDict, 
                    _tokens->multipleApplyAPISchemas)) {
                const VtStringArray &multipleApplyAPISchemas = 
                        VtDictionaryGet<VtStringArray>(customDataDict, 
                            _tokens->multipleApplyAPISchemas);
                for (const auto &apiSchemaName : multipleApplyAPISchemas) {
                    _multipleApplyAPISchemaNames.insert(TfToken(apiSchemaName));
                }
            }

            _AddSchema(generatedSchema, _schematics, disallowedFields);
        }
    }

    // Add them to the type -> path and typeName -> path maps, and the type ->
    // SpecId and typeName -> SpecId maps.
    for (const TfType &type: types) {
        // The path in the schema is the type's alias under UsdSchemaBase.
        vector<string> aliases = _schemaBaseType->GetAliases(type);
        if (aliases.size() == 1) {
            SdfPath primPath = SdfPath::AbsoluteRootPath().
                AppendChild(TfToken(aliases.front()));
            _typeToPathMap[type] = primPath;
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
            _typeNameToPathMap[typeNameToken] = primPath;
            _typeNameToPathMap[primPath.GetNameToken()] = primPath;

            _BuildPrimTypePropNameToSpecIdMap(typeNameToken, primPath);
            _BuildPrimTypePropNameToSpecIdMap(
                primPath.GetNameToken(), primPath);
        }
    }
}

UsdSchemaRegistry::UsdSchemaRegistry()
{
    _schematics = SdfLayer::CreateAnonymous("registry.usda");

    // Find and load all the generated schema in plugin libraries.  We find thes
    // files adjacent to pluginfo files in libraries that provide subclasses of
    // UsdSchemaBase.
    _FindAndAddPluginSchema();

    TfSingleton<UsdSchemaRegistry>::SetInstanceConstructed(*this);
    TfRegistryManager::GetInstance().SubscribeTo<UsdSchemaRegistry>();
}

const SdfPath&
UsdSchemaRegistry::_GetSchemaPrimPath(const TfType &primType) const
{
    // Look up in registered types.
    _TypeToPathMap::const_iterator i = _typeToPathMap.find(primType);
    return i == _typeToPathMap.end() ? SdfPath::EmptyPath() : i->second;
}

const SdfPath&
UsdSchemaRegistry::_GetSchemaPrimPath(const TfToken &primType) const
{
    _TypeNameToPathMap::const_iterator i = _typeNameToPathMap.find(primType);
    return i == _typeNameToPathMap.end() ? SdfPath::EmptyPath() : i->second;
}

/*static*/
SdfPrimSpecHandle
UsdSchemaRegistry::GetPrimDefinition(const TfToken &primType)
{
    return GetSchematics()->GetPrimAtPath(
        GetInstance()._GetSchemaPrimPath(primType));
}

/*static*/
SdfPrimSpecHandle
UsdSchemaRegistry::GetPrimDefinition(const TfType &primType)
{
    return GetSchematics()->GetPrimAtPath(
        GetInstance()._GetSchemaPrimPath(primType));
}

/*static*/
SdfPropertySpecHandle
UsdSchemaRegistry::GetPropertyDefinition(const TfToken& primType, 
                                         const TfToken& propName)
{
    auto const &self = GetInstance();
    if (auto specId = self._GetSpecId(primType, propName)) {
        return self._schematics->GetPropertyAtPath(specId->GetFullSpecPath());
    }
    return TfNullPtr;
}

/*static*/
SdfAttributeSpecHandle
UsdSchemaRegistry::GetAttributeDefinition(const TfToken& primType, 
                                          const TfToken& attrName)
{
    return TfDynamic_cast<SdfAttributeSpecHandle>(
        GetPropertyDefinition(primType, attrName));
}

/*static*/
SdfRelationshipSpecHandle
UsdSchemaRegistry::GetRelationshipDefinition(const TfToken& primType, 
                                             const TfToken& relName)
{
    return TfDynamic_cast<SdfRelationshipSpecHandle>(
        GetPropertyDefinition(primType, relName));
}

// Helper function invoked by generated Schema classes, used to avoid dynamic
// SdfPath construction when looking up prim definitions.
SdfPrimSpecHandle
Usd_SchemaRegistryGetPrimDefinitionAtPath(SdfPath const &path)
{
    return UsdSchemaRegistry::GetInstance().
        GetSchematics()->GetPrimAtPath(path);
}

/*static*/
SdfPrimSpecHandle
UsdSchemaRegistry::_GetPrimDefinitionAtPath(const SdfPath &path)
{
    return Usd_SchemaRegistryGetPrimDefinitionAtPath(path);
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

/*static*/
bool 
UsdSchemaRegistry::IsConcrete(const TfType& primType)
{
    auto primSpec = GetPrimDefinition(primType);
    return (primSpec && !primSpec->GetTypeName().IsEmpty());
}

bool 
UsdSchemaRegistry::IsMultipleApplyAPISchema(const TfType& apiSchemaType)
{
    // Return false if apiSchemaType is not an API schema.
    if (!apiSchemaType.IsA(*_apiSchemaBaseType)) {
        return false;
    }

    for (const auto& alias : _schemaBaseType->GetAliases(apiSchemaType)) {
        if (_multipleApplyAPISchemaNames.find(TfToken(alias)) !=  
                _multipleApplyAPISchemaNames.end()) {
            return true;
        }
    }

    return false;
}

bool 
UsdSchemaRegistry::IsAppliedAPISchema(const TfType& apiSchemaType)
{
    // Return false if apiSchemaType is not an API schema.
    if (!apiSchemaType.IsA(*_apiSchemaBaseType)) {
        return false;
    }

    for (const auto& alias : _schemaBaseType->GetAliases(apiSchemaType)) {
        if (_appliedAPISchemaNames.find(TfToken(alias)) !=  
                _appliedAPISchemaNames.end()) {
            return true;
        }
    }

    return false;
}

TfType
UsdSchemaRegistry::GetTypeFromName(const TfToken& typeName){
    return (*_schemaBaseType).FindDerivedByName(typeName);
}


PXR_NAMESPACE_CLOSE_SCOPE

