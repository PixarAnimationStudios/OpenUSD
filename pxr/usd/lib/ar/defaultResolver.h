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
#ifndef AR_DEFAULT_RESOLVER_H
#define AR_DEFAULT_RESOLVER_H

/// \file ar/defaultResolver.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/usd/ar/defaultResolverContext.h"
#include "pxr/usd/ar/resolver.h"

#include <tbb/enumerable_thread_specific.h>

#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class ArDefaultResolver
///
/// Default asset resolution implementation used when no plugin
/// implementation is provided.
///
/// In order to resolve assets specified by relative paths, this resolver
/// implements a simple "search path" scheme. The resolver will anchor the
/// relative path to a series of directories and return the first absolute
/// path where the asset exists.
///
/// The first directory will always be the current working directory. The
/// resolver will then examine the directories specified via the following
/// mechanisms (in order):
///
///    - The currently-bound ArDefaultResolverContext
///    - ArDefaultResolver::SetDefaultSearchPath
///    - The environment variable PXR_AR_DEFAULT_SEARCH_PATH. This is
///      expected to be a list of directories delimited by the platform's 
///      standard path separator.
///
class ArDefaultResolver
    : public ArResolver
{
public:
    AR_API 
    ArDefaultResolver();

    AR_API 
    virtual ~ArDefaultResolver();

    /// Set the default search path that will be used during asset
    /// resolution. This must be called before the first call
    /// to \ref ArGetResolver.
    AR_API
    static void SetDefaultSearchPath(
        const std::vector<std::string>& searchPath);

    // ArResolver overrides
    AR_API
    virtual void ConfigureResolverForAsset(const std::string& path) override;

    AR_API
    virtual std::string AnchorRelativePath(
        const std::string& anchorPath, 
        const std::string& path) override; 

    AR_API
    virtual bool IsRelativePath(const std::string& path) override;

    AR_API
    virtual bool IsRepositoryPath(const std::string& path) override;

    AR_API
    virtual bool IsSearchPath(const std::string& path) override;

    AR_API
    virtual std::string GetExtension(const std::string& path) override;

    AR_API
    virtual std::string ComputeNormalizedPath(const std::string& path) override;

    AR_API
    virtual std::string ComputeRepositoryPath(const std::string& path) override;

    AR_API
    virtual std::string ComputeLocalPath(const std::string& path) override;

    AR_API
    virtual std::string Resolve(const std::string& path) override;

    AR_API
    virtual std::string ResolveWithAssetInfo(
        const std::string& path, 
        ArAssetInfo* assetInfo) override;

    AR_API
    virtual void UpdateAssetInfo(
       const std::string& identifier,
       const std::string& filePath,
       const std::string& fileVersion,
       ArAssetInfo* assetInfo) override;

    AR_API
    virtual VtValue GetModificationTimestamp(
        const std::string& path,
        const std::string& resolvedPath) override;

    AR_API
    virtual bool FetchToLocalResolvedPath(
        const std::string& path,
        const std::string& resolvedPath) override;

    AR_API
    virtual bool CanWriteLayerToPath(
        const std::string& path,
        std::string* whyNot) override;

    AR_API
    virtual bool CanCreateNewLayerWithIdentifier(
        const std::string& identifier, 
        std::string* whyNot) override;

    AR_API
    virtual ArResolverContext CreateDefaultContext() override;

    AR_API
    virtual ArResolverContext CreateDefaultContextForAsset(
        const std::string& filePath) override;

    AR_API
    virtual ArResolverContext CreateDefaultContextForDirectory(
        const std::string& fileDirectory) override;

    AR_API
    virtual void RefreshContext(const ArResolverContext& context) override;

    AR_API
    virtual ArResolverContext GetCurrentContext() override;

protected:
    AR_API
    virtual void _BeginCacheScope(
        VtValue* cacheScopeData) override;

    AR_API
    virtual void _EndCacheScope(
        VtValue* cacheScopeData) override;

    AR_API
    virtual void _BindContext(
        const ArResolverContext& context,
        VtValue* bindingData) override;

    AR_API
    virtual void _UnbindContext(
        const ArResolverContext& context,
        VtValue* bindingData) override;

private:
    struct _Cache;
    using _CachePtr = std::shared_ptr<_Cache>;
    _CachePtr _GetCurrentCache();

    const ArDefaultResolverContext* _GetCurrentContext();

    std::string _ResolveNoCache(const std::string& path);

private:
    ArDefaultResolverContext _fallbackContext;

    using _CachePtrStack = std::vector<_CachePtr>;
    using _PerThreadCachePtrStack = 
        tbb::enumerable_thread_specific<_CachePtrStack>;
    _PerThreadCachePtrStack _threadCacheStack;

    using _ContextStack = std::vector<const ArDefaultResolverContext*>;
    using _PerThreadContextStack = 
        tbb::enumerable_thread_specific<_ContextStack>;
    _PerThreadContextStack _threadContextStack;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // AR_DEFAULT_RESOLVER_H
