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
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

#include <boost/foreach.hpp>

#include <set>
#include <utility>
#include <vector>

using std::make_pair;
using std::set;
using std::string;
using std::vector;

TF_INSTANTIATE_SINGLETON(UsdSchemaRegistry);

TF_MAKE_STATIC_DATA(TfType, _schemaBaseType) {
    *_schemaBaseType = TfType::Find<UsdSchemaBase>();
}

static void
_AddSchema(SdfLayerRefPtr const &source, SdfLayerRefPtr const &target)
{
    // XXX: Seems like it might be better to copy everything from source into
    // target (with possible exceptions) rather than cherry-picking certain
    // data.
    static TfToken allowedTokens("allowedTokens");
    BOOST_FOREACH(SdfPrimSpecHandle const &prim, source->GetRootPrims()) {
        if (not target->GetPrimAtPath(prim->GetPath())) {

            SdfPrimSpecHandle targetPrim =
                SdfPrimSpec::New(target, prim->GetName(), prim->GetSpecifier(),
                                 prim->GetTypeName());

            string doc = prim->GetDocumentation();
            if (not doc.empty())
                targetPrim->SetDocumentation(doc);

            BOOST_FOREACH(SdfAttributeSpecHandle const &attr,
                          prim->GetAttributes()) {
                SdfAttributeSpecHandle newAttr =
                    SdfAttributeSpec::New(
                        targetPrim, attr->GetName(), attr->GetTypeName(),
                        attr->GetVariability(), attr->IsCustom());
                if (attr->HasDefaultValue())
                    newAttr->SetDefaultValue(attr->GetDefaultValue());
                if (attr->HasInfo(allowedTokens))
                    newAttr->SetInfo(allowedTokens,
                                     attr->GetInfo(allowedTokens));
                string doc = attr->GetDocumentation();
                if (not doc.empty())
                    newAttr->SetDocumentation(doc);
                string displayName = attr->GetDisplayName();
                if (not displayName.empty())
                    newAttr->SetDisplayName(displayName);
                string displayGroup = attr->GetDisplayGroup();
                if (not displayGroup.empty())
                    newAttr->SetDisplayGroup(displayGroup);
                if (attr->GetHidden())
                    newAttr->SetHidden(true);
            }

            BOOST_FOREACH(SdfRelationshipSpecHandle const &rel,
                          prim->GetRelationships()) {
                SdfRelationshipSpecHandle newRel =
                    SdfRelationshipSpec::New(
                        targetPrim, rel->GetName(), rel->IsCustom());
                string doc = rel->GetDocumentation();
                if (not doc.empty())
                    newRel->SetDocumentation(doc);
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
    return TfIsFile(fname) ? SdfLayer::OpenAsAnonymous(fname) : TfNullPtr;
}

void
UsdSchemaRegistry::_BuildPrimTypePropNameToSpecIdMap(
    const TfToken &typeName, const SdfPath &primPath)
{
    // Add this prim and its properties.  Note that the spec path and
    // propertyName are intentionally leaked.  It's okay, since there's a fixed
    // set that we'd like to persist forever.
    SdfPrimSpecHandle prim = _schematics->GetPrimAtPath(primPath);
    if (not prim or prim->GetTypeName().IsEmpty())
        return;

    _primTypePropNameToSpecIdMap[make_pair(typeName, TfToken())] = 
        new SdfAbstractDataSpecId(new SdfPath(prim->GetPath()));

    BOOST_FOREACH(SdfPropertySpecHandle prop, prim->GetProperties()) {
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
    set<PlugPluginPtr> plugins;
    BOOST_FOREACH(const TfType &type, types) {
        if (PlugPluginPtr plugin =
            PlugRegistry::GetInstance().GetPluginForType(type))
            plugins.insert(plugin);
    }

    // For each plugin, if it has generated schema, add it to the schematics.
    SdfChangeBlock block;
    BOOST_FOREACH(const PlugPluginPtr &plugin, plugins) {
        if (SdfLayerRefPtr generatedSchema = _GetGeneratedSchema(plugin))
            _AddSchema(generatedSchema, _schematics);
    }

    // Add them to the type -> path and typeName -> path maps, and the type ->
    // SpecId and typeName -> SpecId maps.
    BOOST_FOREACH(const TfType &type, types) {
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
