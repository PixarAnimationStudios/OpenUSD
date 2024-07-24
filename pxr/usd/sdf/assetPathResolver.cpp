//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file Sdf/AssetPathResolver.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/assetPathResolver.h"
#include "pxr/usd/sdf/debugCodes.h"
#include "pxr/usd/sdf/fileFormat.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/packageUtils.h"
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
        && (lhs.resolvedPath == rhs.resolvedPath)
        && (lhs.resolverContext == rhs.resolverContext)
        && (lhs.assetInfo == rhs.assetInfo);
}

bool
Sdf_CanCreateNewLayerWithIdentifier(
    const string& identifier,
    string* whyNot)
{
    if (identifier.empty()) {
        if (whyNot) {
            *whyNot = "cannot use empty identifier.";
        }
        return false;
    }

    if (Sdf_IsAnonLayerIdentifier(identifier)) {
        if (whyNot) {
            *whyNot = "cannot use anonymous layer identifier.";
        }
        return false;
    }

    if (Sdf_IdentifierContainsArguments(identifier)) {
        if (whyNot) {
            *whyNot = "cannot use arguments in the identifier.";
        }
        return false;
    }

    return true;
}

ArResolvedPath
Sdf_ResolvePath(
    const string& layerPath,
    ArAssetInfo* assetInfo)
{
    TRACE_FUNCTION();
    return ArGetResolver().Resolve(layerPath);
}

bool
Sdf_CanWriteLayerToPath(
    const ArResolvedPath& resolvedPath)
{
    return ArGetResolver().CanWriteAssetToPath(
        resolvedPath, /* whyNot = */ nullptr);
}

ArResolvedPath
Sdf_ComputeFilePath(
    const string& layerPath,
    ArAssetInfo* assetInfo)
{
    TRACE_FUNCTION();

    ArResolvedPath resolvedPath = Sdf_ResolvePath(layerPath, assetInfo);  
    if (resolvedPath.empty()) {
        // If we can't resolve layerPath, it means no layer currently
        // exists at that location. Use ResolveForNewAsset to figure
        // out where this layer would go if we were to create a new
        // one. 
        ArResolver& resolver = ArGetResolver();
        resolvedPath = resolver.ResolveForNewAsset(layerPath);
    }

    return resolvedPath;
}

VtValue
Sdf_ComputeLayerModificationTimestamp(
    const SdfLayer& layer)
{
    std::string layerPath, args;
    Sdf_SplitIdentifier(layer.GetIdentifier(), &layerPath, &args);

    return VtValue(ArGetResolver().GetModificationTimestamp(
        layerPath, layer.GetResolvedPath()));
}

VtDictionary
Sdf_ComputeExternalAssetModificationTimestamps(
    const SdfLayer& layer)
{
    VtDictionary result;
    std::set<std::string> externalAssetDependencies = 
        layer.GetExternalAssetDependencies();
    for (const std::string& resolvedPath : externalAssetDependencies) {
        // Get the modification timestamp for the path. Note that external
        // asset dependencies only returns resolved paths so pass the same
        // path for both params.
        result[resolvedPath] = ArGetResolver().GetModificationTimestamp(
            resolvedPath, ArResolvedPath(resolvedPath));
    }
    return result;
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
        assetInfo->identifier = identifier;

        string layerPath, arguments;
        Sdf_SplitIdentifier(assetInfo->identifier, &layerPath, &arguments);
        if (filePath.empty()) {
            assetInfo->resolvedPath = 
                Sdf_ComputeFilePath(layerPath, &resolveInfo);
        } else {
            assetInfo->resolvedPath = ArResolvedPath(filePath);
        }

        resolveInfo = ArGetResolver().GetAssetInfo(
            layerPath, assetInfo->resolvedPath);
    }

    assetInfo->resolverContext = 
        ArGetResolver().GetCurrentContext();
    assetInfo->assetInfo = resolveInfo;

    TF_DEBUG(SDF_ASSET).Msg("Sdf_ComputeAssetInfoFromIdentifier:\n"
        "  assetInfo->identifier = '%s'\n"
        "  assetInfo->resolvedPath = '%s'\n"
        "  assetInfo->repoPath = '%s'\n"
        "  assetInfo->assetName = '%s'\n"
        "  assetInfo->version = '%s'\n",
        assetInfo->identifier.c_str(),
        assetInfo->resolvedPath.GetPathString().c_str(),
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
    // We want to find the second occurence of ':', traversing from the left,
    // in our identifier which is of the form anon:0x4rfs23:displayName
    auto fst = std::find(identifier.begin(), identifier.end(), ':');
    if (fst == identifier.end()) {
        return std::string();
    }

    auto snd = std::find(fst + 1, identifier.end(), ':');
    if (snd == identifier.end()) {
        return std::string();
    }

    return identifier.substr(std::distance(identifier.begin(), snd) + 1);
}

string
Sdf_GetAnonLayerIdentifierTemplate(
    const string& tag)
{
    string idTag = tag.empty() ? tag : TfStringTrim(tag);

    // Ensure that URL-encoded characters are not misinterpreted as
    // format strings to TfStringPrintf in Sdf_ComputeAnonLayerIdentifier.
    // See discussion in https://github.com/PixarAnimationStudios/OpenUSD/pull/2022
    idTag = TfStringReplace(idTag, "%", "%%");

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

bool Sdf_StripIdentifierArgumentsIfPresent(
    const std::string &identifier,
    std::string *strippedIdentifier)
{
    size_t argPos = identifier.find(_Tokens->ArgsDelimiter.GetString());
    if (argPos == string::npos) {
        return false;
    }
    
    *strippedIdentifier = string(identifier, 0, argPos);
    return true;
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

    string layerPath, arguments;
    Sdf_SplitIdentifier(identifier, &layerPath, &arguments);

    if (Sdf_IsAnonLayerIdentifier(layerPath)) {
        return Sdf_GetAnonLayerDisplayName(layerPath);
    }

    // If the layer path is a package-relative path, we want
    // the basename of the outermost package combined with
    // the packaged path. For example, given:
    //    "/tmp/asset.package[sub/dir/file.sdf]", 
    // we want:
    //    "asset.package[sub/dir/file.sdf]".
    if (ArIsPackageRelativePath(layerPath)) {
        std::pair<std::string, std::string> packagePath =
            ArSplitPackageRelativePathOuter(layerPath);
        packagePath.first = TfGetBaseName(packagePath.first);
        return ArJoinPackageRelativePath(packagePath);
    }

    return TfGetBaseName(layerPath);
}

string
Sdf_GetExtension(
    const string& identifier)
{
    // Split the identifier to get the layer asset path without
    // any file format arguments.
    string strippedPath;
    const string &assetPath =
        Sdf_StripIdentifierArgumentsIfPresent(identifier, &strippedPath)
        ? strippedPath
        : identifier;

    if (Sdf_IsAnonLayerIdentifier(assetPath)) {
        // Strip off the "anon:0x...:" portion of the anonymous layer
        // identifier and look for an extension in the remainder. This
        // allows clients to create anonymous layers using tags that
        // match their asset path scheme and retrieve the extension
        // via ArResolver.
        return Sdf_GetExtension(Sdf_GetAnonLayerDisplayName(assetPath));
    }

    // XXX: If the asset path is a dot file (e.g. ".sdf"), we append
    // a temporary name so that the path we pass to Ar is not 
    // interpreted as a directory name. This is legacy behavior that
    // should be fixed.
    if (!assetPath.empty() && assetPath[0] == '.') {
        return Sdf_GetExtension("temp_file_name" + assetPath);
    }

    return ArGetResolver().GetExtension(assetPath);
}

bool
Sdf_IsPackageOrPackagedLayer(
    const SdfLayerHandle& layer)
{
    return Sdf_IsPackageOrPackagedLayer(
        layer->GetFileFormat(), layer->GetIdentifier());
}

bool
Sdf_IsPackageOrPackagedLayer(
    const SdfFileFormatConstPtr& fileFormat,
    const std::string& identifier)
{
    return fileFormat->IsPackage() || ArIsPackageRelativePath(identifier);
}

PXR_NAMESPACE_CLOSE_SCOPE
