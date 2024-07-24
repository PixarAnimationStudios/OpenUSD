//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_USDZ_RESOLVER_H
#define PXR_USD_USD_USDZ_RESOLVER_H

#include "pxr/pxr.h"
#include "pxr/usd/ar/packageResolver.h"
#include "pxr/usd/ar/threadLocalScopedCache.h"
#include "pxr/usd/usd/zipFile.h"

PXR_NAMESPACE_OPEN_SCOPE

class ArAsset;

/// \class Usd_UsdzResolver
///
/// Package resolver responsible for resolving assets in
/// .usdz files.
class Usd_UsdzResolver
    : public ArPackageResolver
{
public:
    Usd_UsdzResolver();

    virtual std::string Resolve(
        const std::string& packagePath,
        const std::string& packagedPath) override;

    virtual std::shared_ptr<ArAsset> OpenAsset(
        const std::string& packagePath,
        const std::string& packagedPath) override;

    virtual void BeginCacheScope(
        VtValue* cacheScopeData) override;

    virtual void EndCacheScope(
        VtValue* cacheScopeData) override;
};

/// \class Usd_UsdzResolverCache
///
/// Singleton thread-local scoped cache used by Usd_UsdzResolver. This
/// allows other clients besides Usd_UsdzResolver to take advantage of
/// caching of zip files while a resolver scoped cache is active.
class Usd_UsdzResolverCache
{
public:
    static Usd_UsdzResolverCache& GetInstance();

    Usd_UsdzResolverCache(const Usd_UsdzResolverCache&) = delete;
    Usd_UsdzResolverCache& operator=(const Usd_UsdzResolverCache&) = delete;

    using AssetAndZipFile = std::pair<std::shared_ptr<ArAsset>, UsdZipFile>;

    /// Returns the ArAsset and UsdZipFile for the given package path.
    /// If a cache scope is active in the current thread, the returned
    /// values will be cached and returned on subsequent calls to this
    /// function for the same packagePath.
    AssetAndZipFile FindOrOpenZipFile(
        const std::string& packagePath);

    /// Open a cache scope in the current thread. While a cache scope 
    /// is opened, the results of FindOrOpenZipFile will be cached and 
    /// reused.
    void BeginCacheScope(
        VtValue* cacheScopeData);

    /// Close cache scope in the current thread. Once all cache scopes
    /// in the current thread are closed, cached zip files will be
    /// dropped.
    void EndCacheScope(
        VtValue* cacheScopeData);

private:
    Usd_UsdzResolverCache();

    struct _Cache;
    using _ThreadLocalCaches = ArThreadLocalScopedCache<_Cache>;
    using _CachePtr = _ThreadLocalCaches::CachePtr;
    _CachePtr _GetCurrentCache();

    AssetAndZipFile _OpenZipFile(const std::string& packagePath);

    _ThreadLocalCaches _caches;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_USDZ_RESOLVER_H
