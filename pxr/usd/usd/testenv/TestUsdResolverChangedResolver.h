//
// Copyright 2021 Pixar
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
