//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/usd/ar/resolver.h"

PXR_NAMESPACE_OPEN_SCOPE

ArResolverScopedCache::ArResolverScopedCache()
{
    ArGetResolver().BeginCacheScope(&_cacheScopeData);
}

ArResolverScopedCache::ArResolverScopedCache(const ArResolverScopedCache* parent)
    : _cacheScopeData(parent->_cacheScopeData)
{
    ArGetResolver().BeginCacheScope(&_cacheScopeData);
}

ArResolverScopedCache::~ArResolverScopedCache()
{
    ArGetResolver().EndCacheScope(&_cacheScopeData);
}

PXR_NAMESPACE_CLOSE_SCOPE
