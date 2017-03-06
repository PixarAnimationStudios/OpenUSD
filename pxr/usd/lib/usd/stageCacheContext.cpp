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

