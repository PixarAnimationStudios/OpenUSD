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

#include "pxr/usd/ar/api.h"
#include "pxr/base/vt/value.h"
#include <boost/noncopyable.hpp>

/// \class ArResolverScopedCache
///
/// Helper object for managing asset resolver cache scopes.
///
/// \see ArResolverContext::_BeginCacheScope
/// \see ArResolverContext::_EndCacheScope
class AR_API ArResolverScopedCache
    : public boost::noncopyable
{
public:
    /// Begin an asset resolver cache scope.
    ArResolverScopedCache();

    /// Begin an asset resolver cache scope that shares data
    /// with the given \p parent scope.
    explicit ArResolverScopedCache(const ArResolverScopedCache* parent);

    /// End an asset resolver cache scope.
    ~ArResolverScopedCache();

private:
    VtValue _cacheScopeData;
};

#endif // AR_RESOLVER_SCOPED_CACHE_H
