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
#include "pxr/usd/ar/defaultResolver.h"

#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/resolverContext.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/value.h"

#if MAYA_TBB_HANG_WORKAROUND_HACK
#include <unordered_map>
#include <tbb/spin_mutex.h>
#else
#include <tbb/concurrent_hash_map.h>
#endif // MAYA_TBB_HANG_WORKAROUND_HACK

#include <boost/foreach.hpp>

static const char* _FileRelativePathPrefix = "./";

#if MAYA_TBB_HANG_WORKAROUND_HACK
struct Ar_DefaultResolver::_Cache
{
    using _PathToResolvedPathMap = 
        std::unordered_map<std::string, std::string>;
    _PathToResolvedPathMap _pathToResolvedPathMap;
    tbb::spin_mutex _mutex;
};
#else
struct Ar_DefaultResolver::_Cache
{
    using _PathToResolvedPathMap = 
        tbb::concurrent_hash_map<std::string, std::string>;
    _PathToResolvedPathMap _pathToResolvedPathMap;
};
#endif // MAYA_TBB_HANG_WORKAROUND_HACK

Ar_DefaultResolver::Ar_DefaultResolver()
{
    _searchPath.push_back(ArchGetCwd());

    const std::string envPath = TfGetenv("PXR_AR_DEFAULT_SEARCH_PATH");
    if (not envPath.empty()) {
        BOOST_FOREACH(const std::string& p, TfStringTokenize(envPath, ":")) {
            _searchPath.push_back(TfAbsPath(p));
        }
    }
}

Ar_DefaultResolver::~Ar_DefaultResolver()
{
}

void
Ar_DefaultResolver::ConfigureResolverForAsset(const std::string& path)
{
    // no configuration takes place in search path resolver
}

bool
Ar_DefaultResolver::IsRelativePath(const std::string& path)
{
    return (not path.empty() and TfIsRelativePath(path));
}

bool
Ar_DefaultResolver::IsRepositoryPath(const std::string& path)
{
    return false;
}

std::string
Ar_DefaultResolver::AnchorRelativePath(
    const std::string& anchorPath, 
    const std::string& path)
{
    if (path.empty() or not IsRelativePath(path)) {
        return path;
    }

    if (anchorPath.empty() or IsRelativePath(anchorPath)) {
        return path;
    }

    // If anchorPath does not end with a '/', we assume it is specifying
    // a file, strip off the last component, and anchor the path to that
    // directory.
    std::string anchoredPath = TfStringCatPaths(
        TfStringGetBeforeSuffix(anchorPath, '/'), path);
    return TfNormPath(anchoredPath);
}

bool
Ar_DefaultResolver::IsSearchPath(const std::string& path)
{
    return IsRelativePath(path)
        and not TfStringStartsWith(path, _FileRelativePathPrefix);
}

std::string
Ar_DefaultResolver::GetExtension(const std::string& path)
{
    if (path.empty()) {
        return path;
    }

    const std::string fileName = TfGetBaseName(path);

    // If this is a dot file with no extension (e.g. /some/path/.folder), then
    // we return an empty string.
    if (TfStringGetBeforeSuffix(fileName).empty()) {
        return std::string();
    }

    return TfStringGetSuffix(fileName);
}

std::string
Ar_DefaultResolver::ComputeNormalizedPath(const std::string& path)
{
    return TfNormPath(path);
}

std::string
Ar_DefaultResolver::ComputeRepositoryPath(const std::string& path)
{
    return std::string();
}

static std::string
_Resolve(
    const std::string& anchorPath,
    const std::string& path)
{
    std::string resolvedPath = path;
    if (not anchorPath.empty()) {
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
Ar_DefaultResolver::_ResolveNoCache(const std::string& path)
{
    if (path.empty()) {
        return path;
    }

    if (IsRelativePath(path)) {
        // First try to resolve relative paths against the current
        // working directory.
        std::string resolvedPath = _Resolve(ArchGetCwd(), path);
        if (not resolvedPath.empty()) {
            return resolvedPath;
        }

        // If that fails and the path is a search path, try to resolve
        // against each directory in the specified search paths.
        if (IsSearchPath(path)) {
            BOOST_FOREACH(const std::string& searchPath, _searchPath) {
                resolvedPath = _Resolve(searchPath, path);
                if (not resolvedPath.empty()) {
                    return resolvedPath;
                }
            }
        }

        return std::string();
    }

    return _Resolve(std::string(), path);
}

std::string
Ar_DefaultResolver::Resolve(const std::string& path)
{
    return ResolveWithAssetInfo(path, /* assetInfo = */ nullptr);
}

std::string
Ar_DefaultResolver::ResolveWithAssetInfo(
    const std::string& path, 
    ArAssetInfo* assetInfo)
{
    if (path.empty()) {
        return path;
    }

#if MAYA_TBB_HANG_WORKAROUND_HACK
    if (_CachePtr currentCache = _GetCurrentCache()) {
        tbb::spin_mutex::scoped_lock lock(currentCache->_mutex);
        std::pair<_Cache::_PathToResolvedPathMap::iterator, bool> accessor =
            currentCache->_pathToResolvedPathMap.insert(
                std::make_pair(path, std::string()));
        if (accessor.second) {
            accessor.first->second = _ResolveNoCache(path);
        }
        return accessor.first->second;
    }
#else
    if (_CachePtr currentCache = _GetCurrentCache()) {
        _Cache::_PathToResolvedPathMap::accessor accessor;
        if (currentCache->_pathToResolvedPathMap.insert(
                accessor, std::make_pair(path, std::string()))) {
            accessor->second = _ResolveNoCache(path);
        }
        return accessor->second;
    }
#endif // MAYA_TBB_HANG_WORKAROUND_HACK    

    return _ResolveNoCache(path);
}

std::string
Ar_DefaultResolver::ComputeLocalPath(const std::string& path)
{
    return path.empty() ? path : TfAbsPath(path);
}

void
Ar_DefaultResolver::UpdateAssetInfo(
    const std::string& identifier,
    const std::string& filePath,
    const std::string& fileVersion,
    ArAssetInfo* resolveInfo)
{
    if (resolveInfo) {
        if (not fileVersion.empty()) {
            resolveInfo->version = fileVersion;
        }
    }
}

bool
Ar_DefaultResolver::CanWriteLayerToPath(
    const std::string& path,
    std::string* whyNot)
{
    return true;
}

bool
Ar_DefaultResolver::CanCreateNewLayerWithIdentifier(
    const std::string& identifier, 
    std::string* whyNot)
{
    return true;
}

ArResolverContext 
Ar_DefaultResolver::CreateDefaultContext()
{
    return ArResolverContext();
}

ArResolverContext 
Ar_DefaultResolver::CreateDefaultContextForAsset(
    const std::string& filePath)
{
    return ArResolverContext();
}

ArResolverContext
Ar_DefaultResolver::CreateDefaultContextForDirectory(
    const std::string& fileDirectory)
{
    return ArResolverContext();
}

void 
Ar_DefaultResolver::RefreshContext(const ArResolverContext& context)
{
}

ArResolverContext
Ar_DefaultResolver::GetCurrentContext()
{
    return ArResolverContext();
}

void 
Ar_DefaultResolver::_BeginCacheScope(
    VtValue* cacheScopeData)
{
    // cacheScopeData is held by ArResolverScopedCache instances
    // but is only populated by this function, so we know it must 
    // be empty (when constructing a regular ArResolverScopedCache)
    // or holding on to a _CachePtr (when constructing an 
    // ArResolverScopedCache that shares data with another one).
    TF_VERIFY(cacheScopeData and
              (cacheScopeData->IsEmpty() or 
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
Ar_DefaultResolver::_EndCacheScope(
    VtValue* cacheScopeData)
{
    _CachePtrStack& cacheStack = _threadCacheStack.local();
    if (TF_VERIFY(not cacheStack.empty())) {
        cacheStack.pop_back();
    }
}

Ar_DefaultResolver::_CachePtr 
Ar_DefaultResolver::_GetCurrentCache()
{
    _CachePtrStack& cacheStack = _threadCacheStack.local();
    return (cacheStack.empty() ? _CachePtr() : cacheStack.back());
}

void 
Ar_DefaultResolver::_BindContext(
    const ArResolverContext& context,
    VtValue* bindingData)
{
}

void 
Ar_DefaultResolver::_UnbindContext(
    const ArResolverContext& context,
    VtValue* bindingData)
{
}

