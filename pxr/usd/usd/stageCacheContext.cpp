//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/stageCacheContext.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/instantiateStacked.h"

PXR_NAMESPACE_OPEN_SCOPE


using std::vector;

TF_INSTANTIATE_DEFINED_STACKED(UsdStageCacheContext);

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(UsdBlockStageCaches);
    TF_ADD_ENUM_NAME(UsdBlockStageCachePopulation);
    TF_ADD_ENUM_NAME(Usd_NoBlock);
}

/* static */
vector<const UsdStageCache *>
UsdStageCacheContext::_GetReadOnlyCaches()
{
    const Stack &stack = GetStack();
    vector<const UsdStageCache *> caches;
    caches.reserve(stack.size());
    for (auto ctxIter = stack.rbegin(); ctxIter != stack.rend(); ++ctxIter) {
        const auto& ctx = *ctxIter;
        if (ctx->_blockType == UsdBlockStageCaches) {
            break;
        } else if (ctx->_blockType == UsdBlockStageCachePopulation) {
            continue;
        } else if (ctx->_isReadOnlyCache) {
            caches.push_back(ctx->_roCache);
        }
    }
    return caches;
}

/* static */
vector<const UsdStageCache *>
UsdStageCacheContext::_GetReadableCaches()
{
    const Stack &stack = GetStack();
    vector<const UsdStageCache *> caches;
    caches.reserve(stack.size());
    for (auto ctxIter = stack.rbegin(); ctxIter != stack.rend(); ++ctxIter) {
        const auto& ctx = *ctxIter;
        if (ctx->_blockType == UsdBlockStageCaches) {
            break;
        } else if (ctx->_blockType == UsdBlockStageCachePopulation) {
            continue;
        } else {
            caches.push_back(ctx->_isReadOnlyCache ?
                             ctx->_roCache : ctx->_rwCache);
        }
    }
    return caches;
}

/* static */
std::vector<UsdStageCache *>
UsdStageCacheContext::_GetWritableCaches()
{
    const Stack &stack = GetStack();
    vector<UsdStageCache *> caches;
    caches.reserve(stack.size());
    for (auto ctxIter = stack.rbegin(); ctxIter != stack.rend(); ++ctxIter) {
        const auto& ctx = *ctxIter;
        if (ctx->_blockType == UsdBlockStageCaches ||
            ctx->_blockType == UsdBlockStageCachePopulation) {
            break;
        } else if (!ctx->_isReadOnlyCache) {
            caches.push_back(ctx->_rwCache);
        }
    }
    return caches;
}

PXR_NAMESPACE_CLOSE_SCOPE

