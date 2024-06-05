//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/defaultResolverContext.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/filesystemAsset.h"
#include "pxr/usd/ar/filesystemWritableAsset.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

/// This Test resolver is setup in order to exercise various aspects of
/// UsdUtils with custom "Non-Filesystem based resolvers"
/// This is simulated by registering prefixing filesystem paths and removing
/// the URI before operating on the paths
/// It is intentionally configured to use two separate URI's for identifier 
/// creation and asset resolution.
/// Identifiers are in form of test:path
/// Resolved paths are in form testresolved:path
class CustomResolver
    : public ArResolver
{
public:
    const std::string identifierUri = "test:";
    const std::string resolvedPathUri = "testresolved:";

    CustomResolver()
    {
    }

protected:
    std::string 
    _CreateIdentifier(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const final
    {
        if (anchorAssetPath.empty()) {
            return assetPath;
        }

        const std::string anchorDir =
            TfGetPathName(_RemoveUri(anchorAssetPath.GetPathString()));
        
        const std::string assetFilesystemPath = 
            identifierUri + TfStringCatPaths(anchorDir, _RemoveUri(assetPath));

        return identifierUri + 
            TfStringCatPaths(anchorDir, _RemoveUri(assetPath));
    }

    std::string 
    _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const final
    {
        return _CreateIdentifier(assetPath, anchorAssetPath);
    }

    ArResolvedPath 
    _Resolve(
        const std::string& assetPath) const final
    {
        const std::string rawPath = _RemoveUri(assetPath);

        // After removing the URI, we will defer to filesystem path resolver
        ArResolvedPath resolved = ArGetResolver().Resolve(rawPath);

        if (resolved.IsEmpty()) {
            return resolved;
        }

        return ArResolvedPath(resolvedPathUri + resolved.GetPathString());
    }

    ArResolvedPath
    _ResolveForNewAsset(
        const std::string& assetPath) const final
    {
        return _Resolve(assetPath);
    }

    std::shared_ptr<ArAsset> 
    _OpenAsset(
        const ArResolvedPath& resolvedPath) const final
    {
        if (resolvedPath.empty()) {
            return nullptr;
        }

        const std::string filesystemPath = 
            _RemoveUri(resolvedPath.GetPathString());

        return ArFilesystemAsset::Open(ArResolvedPath(filesystemPath));
    }

    std::shared_ptr<ArWritableAsset>
    _OpenAssetForWrite(
        const ArResolvedPath& resolvedPath,
        WriteMode writeMode) const final
    {
        if (resolvedPath.empty()) {
            return nullptr;
        }

        const std::string filesystemPath = 
            _RemoveUri(resolvedPath.GetPathString());

        return ArFilesystemWritableAsset::Create(
                ArResolvedPath(filesystemPath), writeMode);
    }

private:
    std::string
    _RemoveUri(
        const std::string& path) const
    {
        size_t index = path.find_first_of(':');
        return index != std::string::npos ? path.substr(index + 1) : path;
    }

};

AR_DEFINE_RESOLVER(CustomResolver, ArResolver);
