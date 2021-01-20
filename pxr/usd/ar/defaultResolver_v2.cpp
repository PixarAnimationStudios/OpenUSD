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
#include "pxr/usd/ar/defaultResolver.h"

#include "pxr/usd/ar/defaultResolverContext.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/filesystemAsset.h"
#include "pxr/usd/ar/filesystemWritableAsset.h"
#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/ar/writableAsset.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/safeOutputFile.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/value.h"

#include <tbb/concurrent_hash_map.h>

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(ArDefaultResolver, ArResolver);

static bool
_IsFileRelative(const std::string& path) {
    return path.find("./") == 0 || path.find("../") == 0;
}

static std::vector<std::string>
_ParseSearchPaths(const std::string& pathStr)
{
    return TfStringTokenize(pathStr, ARCH_PATH_LIST_SEP);
}

static TfStaticData<std::vector<std::string>> _SearchPath;

struct ArDefaultResolver::_Cache
{
    using _PathToResolvedPathMap = 
        tbb::concurrent_hash_map<std::string, ArResolvedPath>;
    _PathToResolvedPathMap _pathToResolvedPathMap;
};

ArDefaultResolver::ArDefaultResolver()
{
    std::vector<std::string> searchPath = *_SearchPath;

    const std::string envPath = TfGetenv("PXR_AR_DEFAULT_SEARCH_PATH");
    if (!envPath.empty()) {
        const std::vector<std::string> envSearchPath = 
            _ParseSearchPaths(envPath);
        searchPath.insert(
            searchPath.end(), envSearchPath.begin(), envSearchPath.end());
    }

    _fallbackContext = ArDefaultResolverContext(searchPath);
}

ArDefaultResolver::~ArDefaultResolver()
{
}

void
ArDefaultResolver::SetDefaultSearchPath(
    const std::vector<std::string>& searchPath)
{
    *_SearchPath = searchPath;
}

void
ArDefaultResolver::ConfigureResolverForAsset(const std::string& path)
{
    _defaultContext = CreateDefaultContextForAsset(path);
}

bool
ArDefaultResolver::IsRelativePath(const std::string& path)
{
    return (!path.empty() && TfIsRelativePath(path));
}

std::string
ArDefaultResolver::AnchorRelativePath(
    const std::string& anchorPath, 
    const std::string& path)
{
    if (TfIsRelativePath(anchorPath) ||
        !IsRelativePath(path)) {
        return path;
    }

    // Ensure we are using forward slashes and not back slashes.
    std::string forwardPath = anchorPath;
    std::replace(forwardPath.begin(), forwardPath.end(), '\\', '/');

    // If anchorPath does not end with a '/', we assume it is specifying
    // a file, strip off the last component, and anchor the path to that
    // directory.
    const std::string anchoredPath = TfStringCatPaths(
        TfStringGetBeforeSuffix(forwardPath, '/'), path);
    return TfNormPath(anchoredPath);
}

std::string
ArDefaultResolver::_CreateIdentifier(
    const std::string& assetPath,
    const ArResolvedPath& anchorAssetPath)
{
    if (assetPath.empty()) {
        return assetPath;
    }

    if (!anchorAssetPath) {
        return TfNormPath(assetPath);
    }

    // If assetPath is a search path, we want to use that as the asset's
    // identifier instead of turning it into an absolute path by anchoring
    // it. This ensures that resolving that identifier will always invoke
    // the search path mechanism, so that assets that are added/removed
    // from directories in the search path can be discovered by re-resolving
    // the identifier.
    //
    // This is determined using the look-here-first scheme. For any relative
    // path, we first look relative to the specified anchor. If an asset 
    // exists there, we just return the anchored path. Otherwise, we return
    // the asset path as-is.
    const std::string anchoredAssetPath =
        AnchorRelativePath(anchorAssetPath, assetPath);

    if (IsSearchPath(assetPath) && Resolve(anchoredAssetPath).empty()) {
        return TfNormPath(assetPath);
    }

    return TfNormPath(anchoredAssetPath);
}

std::string
ArDefaultResolver::_CreateIdentifierForNewAsset(
    const std::string& assetPath,
    const ArResolvedPath& anchorAssetPath)
{
    if (assetPath.empty()) {
        return assetPath;
    }

    if (IsRelativePath(assetPath)) {
        return TfNormPath(anchorAssetPath ? 
            AnchorRelativePath(anchorAssetPath, assetPath) :
            TfAbsPath(assetPath));
    }

    return TfNormPath(assetPath);
}

bool
ArDefaultResolver::IsSearchPath(const std::string& path)
{
    return IsRelativePath(path) && !_IsFileRelative(path);
}

std::string
ArDefaultResolver::_GetExtension(const std::string& path)
{
    return TfGetExtension(path);
}

static ArResolvedPath
_ResolveAnchored(
    const std::string& anchorPath,
    const std::string& path)
{
    std::string resolvedPath = path;
    if (!anchorPath.empty()) {
        // XXX - CLEANUP:
        // It's tempting to use AnchorRelativePath to combine the two
        // paths here, but that function's file-relative anchoring
        // causes consumers to break. 
        // 
        // Ultimately what we should do is specify whether anchorPath 
        // in both Resolve and AnchorRelativePath can be files or directories 
        // and fix up all the callers to accommodate this.
        resolvedPath = TfStringCatPaths(anchorPath, path);
    }

    // Use TfAbsPath to ensure we return an absolute path using the
    // platform-specific representation (e.g. '\' as path separators
    // on Windows.
    return TfPathExists(resolvedPath) ?
        ArResolvedPath(TfAbsPath(resolvedPath)) : ArResolvedPath();
}

ArResolvedPath
ArDefaultResolver::_ResolveNoCache(const std::string& path)
{
    if (path.empty()) {
        return ArResolvedPath();
    }

    if (IsRelativePath(path)) {
        // First try to resolve relative paths against the current
        // working directory.
        ArResolvedPath resolvedPath = _ResolveAnchored(ArchGetCwd(), path);
        if (resolvedPath) {
            return resolvedPath;
        }

        // If that fails and the path is a search path, try to resolve
        // against each directory in the specified search paths.
        if (IsSearchPath(path)) {
            const ArDefaultResolverContext* contexts[2] =
                {_GetCurrentContextPtr(), &_fallbackContext};
            for (const ArDefaultResolverContext* ctx : contexts) {
                if (ctx) {
                    for (const auto& searchPath : ctx->GetSearchPath()) {
                        resolvedPath = _ResolveAnchored(searchPath, path);
                        if (resolvedPath) {
                            return resolvedPath;
                        }
                    }
                }
            }
        }

        return ArResolvedPath();
    }

    return _ResolveAnchored(std::string(), path);
}

ArResolvedPath
ArDefaultResolver::_Resolve(
    const std::string& assetPath)
{
    if (assetPath.empty()) {
        return ArResolvedPath();
    }

    if (_CachePtr currentCache = _GetCurrentCache()) {
        _Cache::_PathToResolvedPathMap::accessor accessor;
        if (currentCache->_pathToResolvedPathMap.insert(
                accessor, std::make_pair(assetPath, ArResolvedPath()))) {
            accessor->second = _ResolveNoCache(assetPath);
        }
        return accessor->second;
    }

    return _ResolveNoCache(assetPath);
}

ArResolvedPath
ArDefaultResolver::_ResolveForNewAsset(
    const std::string& assetPath)
{
    return ArResolvedPath(assetPath.empty() ? assetPath : TfAbsPath(assetPath));
}

VtValue
ArDefaultResolver::_GetModificationTimestamp(
    const std::string& path,
    const ArResolvedPath& resolvedPath)
{
    // Since the default resolver always resolves paths to local
    // paths, we can just look at the mtime of the file indicated
    // by resolvedPath.
    double time;
    if (ArchGetModificationTime(resolvedPath.GetPathString().c_str(), &time)) {
        return VtValue(time);
    }
    return VtValue();
}

std::shared_ptr<ArAsset> 
ArDefaultResolver::_OpenAsset(
    const ArResolvedPath& resolvedPath)
{
    FILE* f = ArchOpenFile(resolvedPath.GetPathString().c_str(), "rb");
    if (!f) {
        return nullptr;
    }

    return std::shared_ptr<ArAsset>(new ArFilesystemAsset(f));
}

std::shared_ptr<ArWritableAsset>
ArDefaultResolver::_OpenAssetForWrite(
    const ArResolvedPath& resolvedPath,
    WriteMode writeMode)
{
    return ArFilesystemWritableAsset::Create(resolvedPath, writeMode);
}

bool
ArDefaultResolver::_IsContextDependentPath(
    const std::string& assetPath)
{
    return IsSearchPath(assetPath);
}

bool
ArDefaultResolver::CreatePathForLayer(
    const std::string& path)
{
    const std::string layerDir = TfGetPathName(path);
    return layerDir.empty() || TfIsDir(layerDir) || TfMakeDirs(layerDir);
}

ArResolverContext 
ArDefaultResolver::_CreateDefaultContext()
{
    return _defaultContext;
}

ArResolverContext
ArDefaultResolver::_CreateContextFromString(
    const std::string& contextStr)
{
    return ArDefaultResolverContext(_ParseSearchPaths(contextStr));
}

ArResolverContext 
ArDefaultResolver::_CreateDefaultContextForAsset(
    const std::string& assetPath)
{
    if (assetPath.empty()){
        return ArResolverContext(ArDefaultResolverContext());
    }

    std::string assetDir = TfGetPathName(TfAbsPath(assetPath));
    
    return ArResolverContext(ArDefaultResolverContext(
                                 std::vector<std::string>(1, assetDir)));
}

ArResolverContext
ArDefaultResolver::_GetCurrentContext()
{
    const ArDefaultResolverContext* ctx = _GetCurrentContextPtr();
    return ctx ? ArResolverContext(*ctx) : ArResolverContext();
}

void 
ArDefaultResolver::_BeginCacheScope(
    VtValue* cacheScopeData)
{
    _threadCache.BeginCacheScope(cacheScopeData);
}

void 
ArDefaultResolver::_EndCacheScope(
    VtValue* cacheScopeData)
{
    _threadCache.EndCacheScope(cacheScopeData);
}

ArDefaultResolver::_CachePtr 
ArDefaultResolver::_GetCurrentCache()
{
    return _threadCache.GetCurrentCache();
}

void 
ArDefaultResolver::_BindContext(
    const ArResolverContext& context,
    VtValue* bindingData)
{
    const ArDefaultResolverContext* ctx = 
        context.Get<ArDefaultResolverContext>();

    _ContextStack& contextStack = _threadContextStack.local();
    contextStack.push_back(ctx);
}

void 
ArDefaultResolver::_UnbindContext(
    const ArResolverContext& context,
    VtValue* bindingData)
{
    _ContextStack& contextStack = _threadContextStack.local();
    if (contextStack.empty()) {
        TF_CODING_ERROR(
            "No context was bound, cannot unbind context: %s",
            context.GetDebugString().c_str());
    }

    if (!contextStack.empty()) {
        contextStack.pop_back();
    }
}

const ArDefaultResolverContext* 
ArDefaultResolver::_GetCurrentContextPtr()
{
    _ContextStack& contextStack = _threadContextStack.local();
    return contextStack.empty() ? nullptr : contextStack.back();
}

PXR_NAMESPACE_CLOSE_SCOPE
