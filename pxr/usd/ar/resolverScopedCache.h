//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_AR_RESOLVER_SCOPED_CACHE_H
#define PXR_USD_AR_RESOLVER_SCOPED_CACHE_H

/// \file ar/resolverScopedCache.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class ArResolverScopedCache
///
/// Helper object for managing asset resolver cache scopes.
///
/// A scoped resolution cache indicates to the resolver that results of
/// calls to Resolve should be cached for a certain scope. This is
/// important for performance and also for consistency -- it ensures 
/// that repeated calls to Resolve with the same parameters will
/// return the same result.
///
/// \see \ref ArResolver_scopedCache "Scoped Resolution Cache"
class ArResolverScopedCache
{
public:

    // Disallow copies
    ArResolverScopedCache(const ArResolverScopedCache&) = delete;
    ArResolverScopedCache& operator=(const ArResolverScopedCache&) = delete;

    /// Begin an asset resolver cache scope. 
    ///
    /// Calls ArResolver::BeginCacheScope on the configured asset resolver
    /// and saves the cacheScopeData populated by that function.
    AR_API
    ArResolverScopedCache();

    /// Begin an asset resolver cache scope that shares data
    /// with the given \p parent scope.
    ///
    /// Calls ArResolver::BeginCacheScope on the configured asset resolver,
    /// saves the cacheScopeData stored in \p parent and passes that to that
    /// function.
    AR_API
    explicit ArResolverScopedCache(const ArResolverScopedCache* parent);

    /// End an asset resolver cache scope.
    ///
    /// Calls ArResolver::EndCacheScope on the configured asset resolver,
    /// passing the saved cacheScopeData to that function.
    AR_API
    ~ArResolverScopedCache();

private:
    VtValue _cacheScopeData;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_AR_RESOLVER_SCOPED_CACHE_H
