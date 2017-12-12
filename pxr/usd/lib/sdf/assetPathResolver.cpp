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
///
/// \file Sdf/AssetPathResolver.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/assetPathResolver.h"
#include "pxr/usd/sdf/debugCodes.h"

#include "pxr/base/tracelite/trace.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"

#include <utility>
#include <vector>

using std::make_pair;
using std::pair;
using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_Tokens,
    ((AnonLayerPrefix,  "anon:"))
    ((ArgsDelimiter, ":SDF_FORMAT_ARGS:"))
    );

bool
operator==(
    const Sdf_AssetInfo& lhs,
    const Sdf_AssetInfo& rhs)
{
    return (lhs.identifier == rhs.identifier)
        && (lhs.realPath == rhs.realPath)
        && (lhs.resolverContext == rhs.resolverContext)
        && (lhs.assetInfo == rhs.assetInfo);
}

bool
Sdf_CanCreateNewLayerWithIdentifier(
    const string& identifier,
    string* whyNot)
{
    if (identifier.empty()) {
        if (whyNot)
            *whyNot = "cannot create a new layer with an empty identifier.";
        return false;
    }

    if (Sdf_IdentifierContainsArguments(identifier)) {
        if (whyNot)
            *whyNot = "cannot create a new layer with arguments in the "
                "identifier";
        return false;
    }

    return ArGetResolver().CanCreateNewLayerWithIdentifier(identifier, whyNot);
}

string
Sdf_ResolvePath(
    const string& layerPath,
    ArAssetInfo* assetInfo)
{
    TRACE_FUNCTION();
    return ArGetResolver().ResolveWithAssetInfo(layerPath, assetInfo);
}

bool
Sdf_CanWriteLayerToPath(
    const string& layerPath)
{
    return ArGetResolver().CanWriteLayerToPath(
        layerPath, /* whyNot = */ nullptr);
}

string
Sdf_ComputeFilePath(
    const string& layerPath,
    ArAssetInfo* assetInfo)
{
    TRACE_FUNCTION();

    string resolvedPath = Sdf_ResolvePath(layerPath, assetInfo);  
    if (resolvedPath.empty()) {
        // If we can't resolve layerPath, it means no layer currently
        // exists at that location. Compute the local path to figure
        // out where this layer would go if we were to create a new
        // one. 
        //
        // However, we skip this for search paths since the real path
        // is ambiguous if we can't resolve the search path above.
        // This is important for layers with search path identifiers,
        // because otherwise we may compute a confusing real path
        // for these layers.
        ArResolver& resolver = ArGetResolver();
        if (!resolver.IsSearchPath(layerPath)) {
            resolvedPath = resolver.ComputeLocalPath(layerPath);
        }
    }

    return resolvedPath;
}

Sdf_AssetInfo*
Sdf_ComputeAssetInfoFromIdentifier(
    const string& identifier,
    const string& filePath,
    const ArAssetInfo& inResolveInfo,
    const string& fileVersion)
{
    // Allocate a new asset info object. The caller is responsible for
    // managing the returned object.
    Sdf_AssetInfo* assetInfo = new Sdf_AssetInfo;
    ArAssetInfo resolveInfo = inResolveInfo; 

    TF_DEBUG(SDF_ASSET).Msg(
        "Sdf_ComputeAssetInfoFromIdentifier('%s', '%s', '%s')\n",
        identifier.c_str(),
        filePath.c_str(),
        fileVersion.c_str());

    if (Sdf_IsAnonLayerIdentifier(identifier)) {
        // If the identifier is an anonymous layer identifier, don't
        // normalize, and also don't set any of the other assetInfo fields.
        // Anonymous layers do not have repository, overlay, or real paths.
        assetInfo->identifier = identifier;
    } else {
        assetInfo->identifier = ArGetResolver()
            .ComputeNormalizedPath(identifier);

        if (filePath.empty()) {
            string layerPath, arguments;
            Sdf_SplitIdentifier(assetInfo->identifier, &layerPath, &arguments);
            assetInfo->realPath = Sdf_ComputeFilePath(layerPath, &resolveInfo);
        } else {
            assetInfo->realPath = filePath;
        }

        ArGetResolver().UpdateAssetInfo(
            assetInfo->identifier, assetInfo->realPath, fileVersion,
            &resolveInfo);
    }

    assetInfo->resolverContext = 
        ArGetResolver().GetCurrentContext();
    assetInfo->assetInfo = resolveInfo;

    TF_DEBUG(SDF_ASSET).Msg("Sdf_ComputeAssetInfoFromIdentifier:\n"
        "  assetInfo->identifier = '%s'\n"
        "  assetInfo->realPath = '%s'\n"
        "  assetInfo->repoPath = '%s'\n"
        "  assetInfo->assetName = '%s'\n"
        "  assetInfo->version = '%s'\n",
        assetInfo->identifier.c_str(),
        assetInfo->realPath.c_str(),
        resolveInfo.repoPath.c_str(),
        resolveInfo.assetName.c_str(),
        resolveInfo.version.c_str());

    return assetInfo;
}

string
Sdf_ComputeAnonLayerIdentifier(
    const string& identifierTemplate,
    const SdfLayer* layer)
{
    TF_VERIFY(layer);
    return TfStringPrintf(identifierTemplate.c_str(), layer);
}

bool
Sdf_IsAnonLayerIdentifier(
    const string& identifier)
{
    return TfStringStartsWith(identifier,
        _Tokens->AnonLayerPrefix.GetString());
}

string
Sdf_GetAnonLayerDisplayName(
    const string& identifier)
{
    if (std::count(identifier.begin(), identifier.end(), ':') == 2)
        return identifier.substr(identifier.rfind(':') + 1);
    return std::string();
}

string
Sdf_GetAnonLayerIdentifierTemplate(
    const string& tag)
{
    string idTag = tag.empty() ? tag : TfStringTrim(tag);
    return _Tokens->AnonLayerPrefix.GetString() + "%p" +
        (idTag.empty() ? idTag : ":" + idTag);
}

string
Sdf_CreateIdentifier(
    const string& layerPath,
    const string& arguments)
{
    return layerPath + arguments;
}

// XXX: May need to escape characters in the arguments map
// when encoding arguments and unescape then when decoding?
static string
Sdf_EncodeArguments(
    const SdfLayer::FileFormatArguments& args)
{
    const char* delimiter = _Tokens->ArgsDelimiter.GetText();
    string argString;
    for (const auto& entry : args) {
        argString += delimiter;
        argString += entry.first;
        argString += '=';
        argString += entry.second;

        delimiter = "&";
    }

    return argString;
}

static bool
Sdf_DecodeArguments(
    const string& argString,
    SdfLayer::FileFormatArguments* args)
{
    if (argString.empty() || argString.size() == _Tokens->ArgsDelimiter.size()) {
        args->clear();
        return true;
    }

    const size_t argStringLength = argString.size();
    if (!TF_VERIFY(argStringLength > _Tokens->ArgsDelimiter.size())) {
        return false;
    }

    SdfLayer::FileFormatArguments tmpArgs;

    size_t startIdx = _Tokens->ArgsDelimiter.size();
    while (startIdx < argStringLength) {
        const size_t eqIdx = argString.find('=', startIdx);
        if (eqIdx == string::npos) {
            TF_CODING_ERROR(
                "Invalid file format arguments: %s", argString.c_str());
            return false;
        }

        const string key = argString.substr(startIdx, eqIdx - startIdx);
        startIdx = eqIdx + 1;

        const size_t sepIdx = argString.find('&', startIdx);
        if (sepIdx == string::npos) {
            tmpArgs[key] = argString.substr(startIdx);
            break;
        }
        else {
            tmpArgs[key] = argString.substr(startIdx, sepIdx - startIdx);
            startIdx = sepIdx + 1;
        }
    }

    args->swap(tmpArgs);
    return true;
}

string 
Sdf_CreateIdentifier(
    const string& layerPath,
    const SdfLayer::FileFormatArguments& arguments)
{
    return layerPath + Sdf_EncodeArguments(arguments);
}

bool
Sdf_SplitIdentifier(
    const string& identifier,
    string* layerPath,
    string* arguments)
{
    size_t argPos = identifier.find(_Tokens->ArgsDelimiter.GetString());
    if (argPos == string::npos) {
        argPos = identifier.size();
    }
    
    *layerPath = string(identifier, 0, argPos);
    *arguments = string(identifier, argPos, string::npos);
    return true;
}

bool 
Sdf_SplitIdentifier(
    const string& identifier,
    string* layerPath,
    SdfLayer::FileFormatArguments* args)
{
    string tmpLayerPath, tmpArgs;
    if (!Sdf_SplitIdentifier(identifier, &tmpLayerPath, &tmpArgs)) {
        return false;
    }

    if (!Sdf_DecodeArguments(tmpArgs, args)) {
        return false;
    }

    layerPath->swap(tmpLayerPath);
    return true;
}

bool 
Sdf_IdentifierContainsArguments(
    const string& identifier)
{
    return identifier.find(_Tokens->ArgsDelimiter.GetString()) != string::npos;
}

string 
Sdf_GetLayerDisplayName(
    const string& identifier)
{
    if (Sdf_IsAnonLayerIdentifier(identifier)) {
        return Sdf_GetAnonLayerDisplayName(identifier);
    }

    string layerPath, arguments;
    Sdf_SplitIdentifier(identifier, &layerPath, &arguments);
    return TfGetBaseName(layerPath);
}

PXR_NAMESPACE_CLOSE_SCOPE
