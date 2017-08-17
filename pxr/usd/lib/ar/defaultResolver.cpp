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
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/resolverContext.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
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

static TfStaticData<std::vector<std::string>> _SearchPath;

struct ArDefaultResolver::_Cache
{
    using _PathToResolvedPathMap = 
        tbb::concurrent_hash_map<std::string, std::string>;
    _PathToResolvedPathMap _pathToResolvedPathMap;
};

ArDefaultResolver::ArDefaultResolver()
{
    std::vector<std::string> searchPath = *_SearchPath;

    const std::string envPath = TfGetenv("PXR_AR_DEFAULT_SEARCH_PATH");
    if (!envPath.empty()) {
        const std::vector<std::string> envSearchPath = 
            TfStringTokenize(envPath, ARCH_PATH_LIST_SEP);
        searchPath.insert(
            searchPath.end(), envSearchPath.begin(), envSearchPath.end());
    }

    _searchPath.reserve(searchPath.size());
    for (const std::string& p : searchPath) {
        if (p.empty()) {
            continue;
        }

        const std::string absPath = TfAbsPath(p);
        if (absPath.empty()) {
            TF_WARN(
                "Could not determine absolute path for search path prefix "
                "'%s'", p.c_str());
            continue;
        }

        _searchPath.push_back(absPath);
    }
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
    // no configuration takes place in search path resolver
}

bool
ArDefaultResolver::IsRelativePath(const std::string& path)
{
    return (!path.empty() && TfIsRelativePath(path));
}

bool
ArDefaultResolver::IsRepositoryPath(const std::string& path)
{
    return false;
}

std::string
ArDefaultResolver::AnchorRelativePath(
    const std::string& anchorPath, 
    const std::string& path)
{
    if (TfIsRelativePath(anchorPath) ||
        !ArDefaultResolver::IsRelativePath(path) ||
        !_IsFileRelative(path)) {
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

bool
ArDefaultResolver::IsSearchPath(const std::string& path)
{
    return IsRelativePath(path) && !_IsFileRelative(path);
}

std::string
ArDefaultResolver::GetExtension(const std::string& path)
{
    return TfGetExtension(path);
}

std::string
ArDefaultResolver::ComputeNormalizedPath(const std::string& path)
{
    return TfNormPath(path);
}

std::string
ArDefaultResolver::ComputeRepositoryPath(const std::string& path)
{
    return std::string();
}

static std::string
_Resolve(
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
    return TfPathExists(resolvedPath) ? resolvedPath : std::string();
}

std::string
ArDefaultResolver::_ResolveNoCache(const std::string& path)
{
    if (path.empty()) {
        return path;
    }

    if (IsRelativePath(path)) {
        // First try to resolve relative paths against the current
        // working directory.
        std::string resolvedPath = _Resolve(ArchGetCwd(), path);
        if (!resolvedPath.empty()) {
            return resolvedPath;
        }

        // If that fails and the path is a search path, try to resolve
        // against each directory in the specified search paths.
        if (IsSearchPath(path)) {
            for (const auto& searchPath : _searchPath) {
                resolvedPath = _Resolve(searchPath, path);
                if (!resolvedPath.empty()) {
                    return resolvedPath;
                }
            }
        }

        return std::string();
    }

    return _Resolve(std::string(), path);
}

std::string
ArDefaultResolver::Resolve(const std::string& path)
{
    return ResolveWithAssetInfo(path, /* assetInfo = */ nullptr);
}

std::string
ArDefaultResolver::ResolveWithAssetInfo(
    const std::string& path, 
    ArAssetInfo* assetInfo)
{
    if (path.empty()) {
        return path;
    }

    if (_CachePtr currentCache = _GetCurrentCache()) {
        _Cache::_PathToResolvedPathMap::accessor accessor;
        if (currentCache->_pathToResolvedPathMap.insert(
                accessor, std::make_pair(path, std::string()))) {
            accessor->second = _ResolveNoCache(path);
        }
        return accessor->second;
    }

    return _ResolveNoCache(path);
}

std::string
ArDefaultResolver::ComputeLocalPath(const std::string& path)
{
    return path.empty() ? path : TfAbsPath(path);
}

void
ArDefaultResolver::UpdateAssetInfo(
    const std::string& identifier,
    const std::string& filePath,
    const std::string& fileVersion,
    ArAssetInfo* resolveInfo)
{
    if (resolveInfo) {
        if (!fileVersion.empty()) {
            resolveInfo->version = fileVersion;
        }
    }
}

VtValue
ArDefaultResolver::GetModificationTimestamp(
    const std::string& path,
    const std::string& resolvedPath)
{
    // Since the default resolver always resolves paths to local
    // paths, we can just look at the mtime of the file indicated
    // by resolvedPath.
    double time;
    if (ArchGetModificationTime(resolvedPath.c_str(), &time)) {
        return VtValue(time);
    }
    return VtValue();
}

bool 
ArDefaultResolver::FetchToLocalResolvedPath(
    const std::string& path,
    const std::string& resolvedPath)
{
    // ArDefaultResolver always resolves paths to a file on the
    // local filesystem. Because of this, we know the asset specified 
    // by the given path already exists on the filesystem at 
    // resolvedPath, so no further data fetching is needed.
    return true;
}

bool
ArDefaultResolver::CanWriteLayerToPath(
    const std::string& path,
    std::string* whyNot)
{
    return true;
}

bool
ArDefaultResolver::CanCreateNewLayerWithIdentifier(
    const std::string& identifier, 
    std::string* whyNot)
{
    return true;
}

ArResolverContext 
ArDefaultResolver::CreateDefaultContext()
{
    return ArResolverContext();
}

ArResolverContext 
ArDefaultResolver::CreateDefaultContextForAsset(
    const std::string& filePath)
{
    return ArResolverContext();
}

ArResolverContext
ArDefaultResolver::CreateDefaultContextForDirectory(
    const std::string& fileDirectory)
{
    return ArResolverContext();
}

void 
ArDefaultResolver::RefreshContext(const ArResolverContext& context)
{
}

ArResolverContext
ArDefaultResolver::GetCurrentContext()
{
    return ArResolverContext();
}

void 
ArDefaultResolver::_BeginCacheScope(
    VtValue* cacheScopeData)
{
    // cacheScopeData is held by ArResolverScopedCache instances
    // but is only populated by this function, so we know it must 
    // be empty (when constructing a regular ArResolverScopedCache)
    // or holding on to a _CachePtr (when constructing an 
    // ArResolverScopedCache that shares data with another one).
    TF_VERIFY(cacheScopeData &&
              (cacheScopeData->IsEmpty() ||
               cacheScopeData->IsHolding<_CachePtr>()));

    _CachePtrStack& cacheStack = _threadCacheStack.local();

    if (cacheScopeData->IsHolding<_CachePtr>()) {
        cacheStack.push_back(cacheScopeData->UncheckedGet<_CachePtr>());
    }
    else {
        if (cacheStack.empty()) {
            cacheStack.push_back(std::make_shared<_Cache>());
        }
        else {
            cacheStack.push_back(cacheStack.back());
        }
    }

    *cacheScopeData = cacheStack.back();
}

void 
ArDefaultResolver::_EndCacheScope(
    VtValue* cacheScopeData)
{
    _CachePtrStack& cacheStack = _threadCacheStack.local();
    if (TF_VERIFY(!cacheStack.empty())) {
        cacheStack.pop_back();
    }
}

ArDefaultResolver::_CachePtr 
ArDefaultResolver::_GetCurrentCache()
{
    _CachePtrStack& cacheStack = _threadCacheStack.local();
    return (cacheStack.empty() ? _CachePtr() : cacheStack.back());
}

void 
ArDefaultResolver::_BindContext(
    const ArResolverContext& context,
    VtValue* bindingData)
{
}

void 
ArDefaultResolver::_UnbindContext(
    const ArResolverContext& context,
    VtValue* bindingData)
{
}

PXR_NAMESPACE_CLOSE_SCOPE
