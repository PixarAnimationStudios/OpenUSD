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
#ifndef AR_RESOLVER_SCOPED_CACHE_H
#define AR_RESOLVER_SCOPED_CACHE_H

/// \file ar/resolverScopedCache.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/base/vt/value.h"
#include <boost/noncopyable.hpp>

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
/// \see \ref ArResolver_scopedCache_impl "Scoped Resolution Cache"
class ArResolverScopedCache
    : public boost::noncopyable
{
public:
    /// Begin an asset resolver cache scope.
    AR_API
    ArResolverScopedCache();

    /// Begin an asset resolver cache scope that shares data
    /// with the given \p parent scope.
    AR_API
    explicit ArResolverScopedCache(const ArResolverScopedCache* parent);

    /// End an asset resolver cache scope.
    AR_API
    ~ArResolverScopedCache();

private:
    VtValue _cacheScopeData;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // AR_RESOLVER_SCOPED_CACHE_H
