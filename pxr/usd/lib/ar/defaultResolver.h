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

#include "pxr/usd/ar/resolver.h"

#include <tbb/enumerable_thread_specific.h>

#include <memory>
#include <string>
#include <vector>

/// \class Ar_DefaultResolver
///
/// Default asset resolution implementation used when no plugin
/// implementation is provided.
///
/// In order to resolve assets specified by relative paths, this resolver
/// implements a simple "search path" scheme. The resolver will anchor the
/// relative path to a series of directories and return the first absolute
/// path where the asset exists.
///
/// The first directory searched is always the current working directory.
/// Consumers can specify additional directories by setting the 
/// PXR_AR_DEFAULT_SEARCH_PATH environment variable to a list of
/// directories delimited by ':'.
///
class Ar_DefaultResolver
    : public ArResolver
{
public:
    Ar_DefaultResolver();
    virtual ~Ar_DefaultResolver();

    // ArResolver overrides
    virtual void ConfigureResolverForAsset(const std::string& path);

    virtual std::string AnchorRelativePath(
        const std::string& anchorPath, 
        const std::string& path); 

    virtual bool IsRelativePath(const std::string& path);
    virtual bool IsRepositoryPath(const std::string& path);
    virtual bool IsSearchPath(const std::string& path);

    virtual std::string GetExtension(const std::string& path);

    virtual std::string ComputeNormalizedPath(const std::string& path);

    virtual std::string ComputeRepositoryPath(const std::string& path);

    virtual std::string ComputeLocalPath(const std::string& path);

    virtual std::string Resolve(const std::string& path);

    virtual std::string ResolveWithAssetInfo(
        const std::string& path, 
        ArAssetInfo* assetInfo);

    virtual void UpdateAssetInfo(
       const std::string& identifier,
       const std::string& filePath,
       const std::string& fileVersion,
       ArAssetInfo* assetInfo);

    virtual bool CanWriteLayerToPath(
        const std::string& path,
        std::string* whyNot);

    virtual bool CanCreateNewLayerWithIdentifier(
        const std::string& identifier, 
        std::string* whyNot);

    virtual ArResolverContext CreateDefaultContext();

    virtual ArResolverContext CreateDefaultContextForAsset(
        const std::string& filePath);

    virtual ArResolverContext CreateDefaultContextForDirectory(
        const std::string& fileDirectory);

    virtual void RefreshContext(const ArResolverContext& context);

    virtual ArResolverContext GetCurrentContext();

protected:
    virtual void _BeginCacheScope(
        VtValue* cacheScopeData);

    virtual void _EndCacheScope(
        VtValue* cacheScopeData);

    virtual void _BindContext(
        const ArResolverContext& context,
        VtValue* bindingData);

    virtual void _UnbindContext(
        const ArResolverContext& context,
        VtValue* bindingData);

private:
    struct _Cache;
    using _CachePtr = std::shared_ptr<_Cache>;
    _CachePtr _GetCurrentCache();

    std::string _ResolveNoCache(const std::string& path);

private:
    std::vector<std::string> _searchPath;

    using _CachePtrStack = std::vector<_CachePtr>;
    using _PerThreadCachePtrStack = 
        tbb::enumerable_thread_specific<_CachePtrStack>;

    _PerThreadCachePtrStack _threadCacheStack;
};

#endif // AR_DEFAULT_RESOLVER_H
