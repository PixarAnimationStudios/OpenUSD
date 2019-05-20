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
            const ArDefaultResolverContext* contexts[2] =
                {_GetCurrentContext(), &_fallbackContext};
            for (const ArDefaultResolverContext* ctx : contexts) {
                if (ctx) {
                    for (const auto& searchPath : ctx->GetSearchPath()) {
                        resolvedPath = _Resolve(searchPath, path);
                        if (!resolvedPath.empty()) {
                            return resolvedPath;
                        }
                    }
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

std::shared_ptr<ArAsset> 
ArDefaultResolver::OpenAsset(
    const std::string& resolvedPath)
{
    FILE* f = ArchOpenFile(resolvedPath.c_str(), "rb");
    if (!f) {
        return nullptr;
    }

    return std::shared_ptr<ArAsset>(new ArFilesystemAsset(f));
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
    return _defaultContext;
}

ArResolverContext 
ArDefaultResolver::CreateDefaultContextForAsset(
    const std::string& filePath)
{
    if (filePath.empty()){
        return ArResolverContext(ArDefaultResolverContext());
    }

    std::string assetDir = TfGetPathName(TfAbsPath(filePath));
    
    return ArResolverContext(ArDefaultResolverContext(
                                 std::vector<std::string>(1, assetDir)));
}

void 
ArDefaultResolver::RefreshContext(const ArResolverContext& context)
{
}

ArResolverContext
ArDefaultResolver::GetCurrentContext()
{
    const ArDefaultResolverContext* ctx = _GetCurrentContext();
    return ctx ? ArResolverContext(*ctx) : ArResolverContext();
}

void 
ArDefaultResolver::BeginCacheScope(
    VtValue* cacheScopeData)
{
    _threadCache.BeginCacheScope(cacheScopeData);
}

void 
ArDefaultResolver::EndCacheScope(
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
ArDefaultResolver::BindContext(
    const ArResolverContext& context,
    VtValue* bindingData)
{
    const ArDefaultResolverContext* ctx = 
        context.Get<ArDefaultResolverContext>();

    if (!context.IsEmpty() && !ctx) {
        TF_CODING_ERROR(
            "Unknown resolver context object: %s", 
            context.GetDebugString().c_str());
    }

    _ContextStack& contextStack = _threadContextStack.local();
    contextStack.push_back(ctx);
}

void 
ArDefaultResolver::UnbindContext(
    const ArResolverContext& context,
    VtValue* bindingData)
{
    _ContextStack& contextStack = _threadContextStack.local();
    if (contextStack.empty() ||
        contextStack.back() != context.Get<ArDefaultResolverContext>()) {
        TF_CODING_ERROR(
            "Unbinding resolver context in unexpected order: %s",
            context.GetDebugString().c_str());
    }

    if (!contextStack.empty()) {
        contextStack.pop_back();
    }
}

const ArDefaultResolverContext* 
ArDefaultResolver::_GetCurrentContext()
{
    _ContextStack& contextStack = _threadContextStack.local();
    return contextStack.empty() ? nullptr : contextStack.back();
}

PXR_NAMESPACE_CLOSE_SCOPE
