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
#include "pxr/usd/usdUtils/pipeline.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_ENV_SETTING(
    USD_FORCE_DEFAULT_MATERIALS_SCOPE_NAME,
    false,
    "Disables the ability to configure the materials scope name with a "
    "plugInfo.json value and forces the use of the built-in default instead. "
    "This is primarily used for unit testing purposes as a way to ignore any "
    "site-based configuration.");


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (UsdUtilsPipeline)
        (MaterialsScopeName)
        (PrimaryCameraName)
        (RegisteredVariantSets)
            (selectionExportPolicy)
                // lowerCamelCase of the enums.
                (never)
                (ifAuthored)
                (always)

    ((DefaultMaterialsScopeName, "Looks"))
    ((DefaultPrimaryCameraName, "main_cam"))
);


TfToken UsdUtilsGetAlphaAttributeNameForColor(TfToken const &colorAttrName)
{
    return TfToken(colorAttrName.GetString()+std::string("_A"));
}

TfToken
UsdUtilsGetModelNameFromRootLayer(
    const SdfLayerHandle& rootLayer)
{
    // First check if if we have the metadata.
    TfToken modelName = rootLayer->GetDefaultPrim();
    if (!modelName.IsEmpty()) {
        return modelName;
    }

    // If no default prim, see if there is a prim w/ the same "name" as the
    // file.  "name" here means the string before the first ".".
    const std::string& filePath = rootLayer->GetRealPath();
    std::string baseName = TfGetBaseName(filePath);
    modelName = TfToken(baseName.substr(0, baseName.find('.')));

    if (!modelName.IsEmpty() &&
            SdfPath::IsValidIdentifier(modelName) && 
            rootLayer->GetPrimAtPath(
                SdfPath::AbsoluteRootPath().AppendChild(modelName))) {
        return modelName;
    }

    // Otherwise, fallback to getting the first non-class child in the layer.
    TF_FOR_ALL(rootChildrenIter, rootLayer->GetRootPrims()) {
        const SdfPrimSpecHandle& rootPrim = *rootChildrenIter;
        if (rootPrim->GetSpecifier() != SdfSpecifierClass) {
            return rootPrim->GetNameToken();
        }
    }

    return modelName;
}

TF_MAKE_STATIC_DATA(std::set<UsdUtilsRegisteredVariantSet>, _regVarSets)
{
    PlugPluginPtrVector plugs = PlugRegistry::GetInstance().GetAllPlugins();
    TF_FOR_ALL(plugIter, plugs) {
        PlugPluginPtr plug = *plugIter;
        JsObject metadata = plug->GetMetadata();
        JsValue pipelineUtilsDictValue;
        if (TfMapLookup(metadata, _tokens->UsdUtilsPipeline, &pipelineUtilsDictValue)) {
            if (!pipelineUtilsDictValue.Is<JsObject>()) {
                TF_CODING_ERROR(
                        "%s[UsdUtilsPipeline] was not a dictionary.",
                        plug->GetName().c_str());
                continue;
            }

            JsObject pipelineUtilsDict =
                pipelineUtilsDictValue.Get<JsObject>();

            JsValue registeredVariantSetsValue;
            if (TfMapLookup(pipelineUtilsDict,
                        _tokens->RegisteredVariantSets,
                        &registeredVariantSetsValue)) {
                if (!registeredVariantSetsValue.IsObject()) {
                    TF_CODING_ERROR(
                            "%s[UsdUtilsPipeline][RegisteredVariantSets] was not a dictionary.",
                            plug->GetName().c_str());
                    continue;
                }

                const JsObject& registeredVariantSets =
                    registeredVariantSetsValue.GetJsObject();
                for (const auto& i: registeredVariantSets) {
                    const std::string& variantSetName = i.first;
                    const JsValue& v = i.second;
                    if (!v.IsObject()) {
                        TF_CODING_ERROR(
                                "%s[UsdUtilsPipeline][RegisteredVariantSets][%s] was not a dictionary.",
                                plug->GetName().c_str(),
                                variantSetName.c_str());
                        continue;
                    }

                    JsObject info = v.GetJsObject();
                    std::string variantSetType = info[_tokens->selectionExportPolicy].GetString();


                    UsdUtilsRegisteredVariantSet::SelectionExportPolicy selectionExportPolicy;
                    if (variantSetType == _tokens->never) {
                        selectionExportPolicy = 
                            UsdUtilsRegisteredVariantSet::SelectionExportPolicy::Never;
                    }
                    else if (variantSetType == _tokens->ifAuthored) {
                        selectionExportPolicy = 
                            UsdUtilsRegisteredVariantSet::SelectionExportPolicy::IfAuthored;
                    }
                    else if (variantSetType == _tokens->always) {
                        selectionExportPolicy = 
                            UsdUtilsRegisteredVariantSet::SelectionExportPolicy::Always;
                    }
                    else {
                        TF_CODING_ERROR(
                                "%s[UsdUtilsPipeline][RegisteredVariantSets][%s] was not valid.",
                                plug->GetName().c_str(),
                                variantSetName.c_str());
                        continue;
                    }
                    _regVarSets->insert(UsdUtilsRegisteredVariantSet(
                                variantSetName, selectionExportPolicy));
                }
            }
        }
    }
}

const std::set<UsdUtilsRegisteredVariantSet>&
UsdUtilsGetRegisteredVariantSets()
{
    return *_regVarSets;
}

UsdPrim 
UsdUtilsGetPrimAtPathWithForwarding(const UsdStagePtr &stage, 
                                    const SdfPath &path)
{
    // If the given path refers to a prim beneath an instance,
    // UsdStage::GetPrimAtPath will return an instance proxy
    // from which we can retrieve the corresponding prim in
    // the master.
    UsdPrim p = stage->GetPrimAtPath(path);
    return (p && p.IsInstanceProxy()) ? p.GetPrimInMaster() : p;
}

UsdPrim 
UsdUtilsUninstancePrimAtPath(const UsdStagePtr &stage, 
                             const SdfPath &path)
{
    // If a valid prim exists at the requested path, simply return it.
    // If the prim is an instance proxy, it means this path indicates
    // a prim beneath an instance. In order to uninstance it, we need
    // to uninstance all ancestral instances.
    UsdPrim p = stage->GetPrimAtPath(path);
    if (!p || !p.IsInstanceProxy()) {
        return p;
    }

    // Skip the last element in prefixes, since that's our own
    // path and we only want to uninstance ancestors.
    SdfPathVector prefixes = path.GetPrefixes();
    if (!prefixes.empty()) {
        prefixes.pop_back();
    }

    for (const SdfPath& prefixPath : prefixes) {
        UsdPrim prim = stage->GetPrimAtPath(prefixPath);
        if (!prim) {
            break;
        }

        if (prim.IsInstance()) {
            prim.SetInstanceable(false);
        }
    }

    // Uninstancing should ensure that the prim at the given
    // path, if it exists, is no longer inside an instance.
    p = stage->GetPrimAtPath(path);
    TF_VERIFY(!p || !p.IsInstanceProxy());
    return p;
}

TfToken UsdUtilsGetPrimaryUVSetName()
{
    return TfToken("st");
}

TfToken UsdUtilsGetPrefName()
{
    return TfToken("pref");
}

using _TokenToTokenMap = TfHashMap<TfToken, TfToken, TfToken::HashFunctor>;

/// Looks through the metadata dictionaries of all registered plugins for
/// string values that match the key path:
///     [UsdUtilsPipeline][<identifierKey>]
///
/// A string value will be looked up for each of the tokens in identifierKeys.
///
/// The first valid string value identifier found for each identifierKey, if
/// any, is inserted into the returned map.
static
_TokenToTokenMap
_GetPipelineIdentifierTokens(const TfTokenVector& identifierKeys)
{
    const TfToken metadataDictKey = _tokens->UsdUtilsPipeline;

    _TokenToTokenMap identifierMap;

    const PlugPluginPtrVector plugs =
        PlugRegistry::GetInstance().GetAllPlugins();
    for (const PlugPluginPtr plug : plugs) {
        JsObject metadata = plug->GetMetadata();
        JsValue metadataDictValue;
        if (!TfMapLookup(metadata, metadataDictKey, &metadataDictValue)) {
            continue;
        }

        if (!metadataDictValue.Is<JsObject>()) {
            TF_CODING_ERROR(
                "%s[%s] was not a dictionary.",
                plug->GetName().c_str(),
                metadataDictKey.GetText());
            continue;
        }

        JsObject metadataDict = metadataDictValue.Get<JsObject>();

        for (const TfToken& identifierKey : identifierKeys) {
            JsValue stringJsValue;
            if (!TfMapLookup(metadataDict, identifierKey, &stringJsValue)) {
                continue;
            }

            if (!stringJsValue.IsString()) {
                TF_CODING_ERROR(
                    "%s[%s][%s] was not a string.",
                    plug->GetName().c_str(),
                    metadataDictKey.GetText(),
                    identifierKey.GetText());
                continue;
            }

            const std::string valueString = stringJsValue.GetString();
            if (!SdfPath::IsValidIdentifier(valueString)) {
                TF_CODING_ERROR(
                    "%s[%s][%s] was not a valid identifier: \"%s\".",
                    plug->GetName().c_str(),
                    metadataDictKey.GetText(),
                    identifierKey.GetText(),
                    valueString.c_str());
                continue;
            }

            identifierMap.insert({identifierKey, TfToken(valueString)});
        }

        if (identifierMap.size() == identifierKeys.size()) {
            // We got an identifier for all of the given keys, so stop looking
            // through plugin metadata.
            break;
        }
    }

    return identifierMap;
}

TF_MAKE_STATIC_DATA(_TokenToTokenMap, _pipelineIdentifiersMap)
{
    const TfTokenVector identifierKeys({
        _tokens->MaterialsScopeName,
        _tokens->PrimaryCameraName
    });

    *_pipelineIdentifiersMap = _GetPipelineIdentifierTokens(identifierKeys);
}

TfToken
UsdUtilsGetMaterialsScopeName(const bool forceDefault)
{
    if (TfGetEnvSetting(USD_FORCE_DEFAULT_MATERIALS_SCOPE_NAME) ||
            forceDefault) {
        return _tokens->DefaultMaterialsScopeName;
    }

    return TfMapLookupByValue(
        *_pipelineIdentifiersMap,
        _tokens->MaterialsScopeName,
        _tokens->DefaultMaterialsScopeName);
}

TfToken
UsdUtilsGetPrimaryCameraName(const bool forceDefault)
{
    if (forceDefault) {
        return _tokens->DefaultPrimaryCameraName;
    }

    return TfMapLookupByValue(
        *_pipelineIdentifiersMap,
        _tokens->PrimaryCameraName,
        _tokens->DefaultPrimaryCameraName);
}


PXR_NAMESPACE_CLOSE_SCOPE
