//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_TEST_USD_RESOLVER_CHANGED_RESOLVER_H
#define PXR_USD_USD_TEST_USD_RESOLVER_CHANGED_RESOLVER_H

#include "pxr/pxr.h"

#include "pxr/usd/ar/defineResolverContext.h"
#include "pxr/usd/ar/resolver.h"

#include "pxr/base/plug/staticInterface.h"
#include "pxr/base/tf/hash.h"

#include <string>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

/// \class _TestResolverContext
///
class _TestResolverContext
{
public:
    _TestResolverContext(const std::string& configName_)
        : configName(configName_)
    {
    }

    bool operator<(const _TestResolverContext& rhs) const
    { return configName < rhs.configName; }

    bool operator==(const _TestResolverContext& rhs) const
    { return configName == rhs.configName; }

    friend size_t hash_value(const _TestResolverContext& ctx)
    { return TfHash()(ctx.configName); }

    std::string configName;
};

AR_DECLARE_RESOLVER_CONTEXT(_TestResolverContext);

/// \class _TestResolverPluginInterface
///
class _TestResolverPluginInterface
{
public:
    virtual ~_TestResolverPluginInterface() { }

    virtual void SetAssetPathsForConfig(
        const std::string& configName,
        const std::unordered_map<std::string, std::string>& assetPathMap) = 0;

    virtual void SetVersionForConfig(
        const std::string& configName,
        const std::string& version) = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
