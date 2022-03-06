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


PXR_NAMESPACE_OPEN_SCOPE

namespace pxrUsdSdfAssetPathResolver {

TF_DEFINE_PRIVATE_TOKENS(_Tokens,
    ((AnonLayerPrefix,  "anon:"))
    ((ArgsDelimiter, ":SDF_FORMAT_ARGS:"))
    );

} // pxrUsdSdfAssetPathResolver

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
    const std::string& identifier,
    std::string* whyNot)
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

#if AR_VERSION == 1
    return ArGetResolver().CanCreateNewLayerWithIdentifier(identifier, whyNot);
#else
    return true;
#endif
}

ArResolvedPath
Sdf_ResolvePath(
    const std::string& layerPath,
    ArAssetInfo* assetInfo)
{
    TRACE_FUNCTION();
#if AR_VERSION == 1
    return ArResolvedPath(
        ArGetResolver().ResolveWithAssetInfo(layerPath, assetInfo));
#else
    return ArGetResolver().Resolve(layerPath);
#endif
}

bool
Sdf_CanWriteLayerToPath(
    const ArResolvedPath& resolvedPath)
{
#if AR_VERSION == 1
    return ArGetResolver().CanWriteLayerToPath(
        resolvedPath, /* whyNot = */ nullptr);
#else
    return ArGetResolver().CanWriteAssetToPath(
        resolvedPath, /* whyNot = */ nullptr);
#endif
}

ArResolvedPath
Sdf_ComputeFilePath(
    const std::string& layerPath,
    ArAssetInfo* assetInfo)
{
    TRACE_FUNCTION();

    ArResolvedPath resolvedPath = Sdf_ResolvePath(layerPath, assetInfo);  
    if (resolvedPath.empty()) {
#if AR_VERSION == 1
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
            resolvedPath = ArResolvedPath(resolver.ComputeLocalPath(layerPath));
        }
#else
        // If we can't resolve layerPath, it means no layer currently
        // exists at that location. Use ResolveForNewAsset to figure
        // out where this layer would go if we were to create a new
        // one. 
        ArResolver& resolver = ArGetResolver();
        resolvedPath = resolver.ResolveForNewAsset(layerPath);
#endif
    }

    return resolvedPath;
}

Sdf_AssetInfo*
Sdf_ComputeAssetInfoFromIdentifier(
    const std::string& identifier,
    const std::string& filePath,
    const ArAssetInfo& inResolveInfo,
    const std::string& fileVersion)
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
#if AR_VERSION == 1
        assetInfo->identifier = ArGetResolver()
            .ComputeNormalizedPath(identifier);
#else
        assetInfo->identifier = identifier;
#endif

        std::string layerPath, arguments;
        Sdf_SplitIdentifier(assetInfo->identifier, &layerPath, &arguments);
        if (filePath.empty()) {
            assetInfo->resolvedPath = 
                Sdf_ComputeFilePath(layerPath, &resolveInfo);
        } else {
            assetInfo->resolvedPath = ArResolvedPath(filePath);
        }

#if AR_VERSION == 1
        assetInfo->resolvedPath = ArResolvedPath(
            Sdf_CanonicalizeRealPath(assetInfo->resolvedPath));

        ArGetResolver().UpdateAssetInfo(
            assetInfo->identifier, assetInfo->resolvedPath, fileVersion,
            &resolveInfo);
#else
        resolveInfo = ArGetResolver().GetAssetInfo(
            layerPath, assetInfo->resolvedPath);
#endif
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

std::string
Sdf_ComputeAnonLayerIdentifier(
    const std::string& identifierTemplate,
    const SdfLayer* layer)
{
    TF_VERIFY(layer);
    return TfStringPrintf(identifierTemplate.c_str(), layer);
}

bool
Sdf_IsAnonLayerIdentifier(
    const std::string& identifier)
{
    return TfStringStartsWith(identifier,
        pxrUsdSdfAssetPathResolver::_Tokens->AnonLayerPrefix.GetString());
}

std::string
Sdf_GetAnonLayerDisplayName(
    const std::string& identifier)
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

std::string
Sdf_GetAnonLayerIdentifierTemplate(
    const std::string& tag)
{
    std::string idTag = tag.empty() ? tag : TfStringTrim(tag);
    return pxrUsdSdfAssetPathResolver::_Tokens->AnonLayerPrefix.GetString() + "%p" +
        (idTag.empty() ? idTag : ":" + idTag);
}

std::string
Sdf_CreateIdentifier(
    const std::string& layerPath,
    const std::string& arguments)
{
    return layerPath + arguments;
}

// XXX: May need to escape characters in the arguments map
// when encoding arguments and unescape then when decoding?
static std::string
Sdf_EncodeArguments(
    const SdfLayer::FileFormatArguments& args)
{
    const char* delimiter = pxrUsdSdfAssetPathResolver::_Tokens->ArgsDelimiter.GetText();
    std::string argString;
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
    const std::string& argString,
    SdfLayer::FileFormatArguments* args)
{
    if (argString.empty() || argString.size() == pxrUsdSdfAssetPathResolver::_Tokens->ArgsDelimiter.size()) {
        args->clear();
        return true;
    }

    const size_t argStringLength = argString.size();
    if (!TF_VERIFY(argStringLength > pxrUsdSdfAssetPathResolver::_Tokens->ArgsDelimiter.size())) {
        return false;
    }

    SdfLayer::FileFormatArguments tmpArgs;

    size_t startIdx = pxrUsdSdfAssetPathResolver::_Tokens->ArgsDelimiter.size();
    while (startIdx < argStringLength) {
        const size_t eqIdx = argString.find('=', startIdx);
        if (eqIdx == std::string::npos) {
            TF_CODING_ERROR(
                "Invalid file format arguments: %s", argString.c_str());
            return false;
        }

        const std::string key = argString.substr(startIdx, eqIdx - startIdx);
        startIdx = eqIdx + 1;

        const size_t sepIdx = argString.find('&', startIdx);
        if (sepIdx == std::string::npos) {
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

std::string 
Sdf_CreateIdentifier(
    const std::string& layerPath,
    const SdfLayer::FileFormatArguments& arguments)
{
    return layerPath + Sdf_EncodeArguments(arguments);
}

bool
Sdf_SplitIdentifier(
    const std::string& identifier,
    std::string* layerPath,
    std::string* arguments)
{
    size_t argPos = identifier.find(pxrUsdSdfAssetPathResolver::_Tokens->ArgsDelimiter.GetString());
    if (argPos == std::string::npos) {
        argPos = identifier.size();
    }
    
    *layerPath = std::string(identifier, 0, argPos);
    *arguments = std::string(identifier, argPos, std::string::npos);
    return true;
}

bool 
Sdf_SplitIdentifier(
    const std::string& identifier,
    std::string* layerPath,
    SdfLayer::FileFormatArguments* args)
{
    std::string tmpLayerPath, tmpArgs;
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
    const std::string& identifier)
{
    return identifier.find(pxrUsdSdfAssetPathResolver::_Tokens->ArgsDelimiter.GetString()) != std::string::npos;
}

std::string 
Sdf_GetLayerDisplayName(
    const std::string& identifier)
{

    std::string layerPath, arguments;
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

std::string
Sdf_GetExtension(
    const std::string& identifier)
{
    // Split the identifier to get the layer asset path without
    // any file format arguments.
    std::string assetPath;
    std::string dummyArgs;
    Sdf_SplitIdentifier(identifier, &assetPath, &dummyArgs);

    if (Sdf_IsAnonLayerIdentifier(assetPath)) {
        // Strip off the "anon:0x...:" portion of the anonymous layer
        // identifier and look for an extension in the remainder. This
        // allows clients to create anonymous layers using tags that
        // match their asset path scheme and retrieve the extension
        // via ArResolver.
        assetPath = Sdf_GetAnonLayerDisplayName(assetPath);
    }

    // XXX: If the asset path is a dot file (e.g. ".sdf"), we append
    // a temporary name so that the path we pass to Ar is not 
    // interpreted as a directory name. This is legacy behavior that
    // should be fixed.
    if (!assetPath.empty() && assetPath[0] == '.') {
        assetPath = "temp_file_name" + assetPath;
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

std::string 
Sdf_CanonicalizeRealPath(
    const std::string& realPath)
{
    // Use the given realPath as-is if it's a relative path, otherwise
    // use TfAbsPath to compute a platform-dependent real path.
    //
    // XXX: This method needs to be re-examined as we move towards a
    // less filesystem-dependent implementation.

    // If realPath is a package-relative path, absolutize just the
    // outer path; the packaged path has a specific format defined in
    // Ar that we don't want to modify.
    if (ArIsPackageRelativePath(realPath)) {
        std::pair<std::string, std::string> packagePath = 
            ArSplitPackageRelativePathOuter(realPath);
        return TfIsRelativePath(packagePath.first) ?
            realPath : ArJoinPackageRelativePath(
                TfAbsPath(packagePath.first), packagePath.second);
    }

    return TfIsRelativePath(realPath) ? realPath : TfAbsPath(realPath);
}

PXR_NAMESPACE_CLOSE_SCOPE
