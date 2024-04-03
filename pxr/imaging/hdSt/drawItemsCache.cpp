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
#include "pxr/imaging/hdSt/drawItemsCache.h"
#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/renderParam.h"
#include "pxr/imaging/hdSt/renderDelegate.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/perfLog.h"

#include <sstream>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


std::ostream &operator <<(std::ostream &os, TfTokenVector const &tags)
{
    os << "[";
    for (TfToken const &tag : tags) {
        os << tag << ", ";
    }
    os << "]";
    return os;
}

//------------------------------------------------------------------------------
// Helper methods

static size_t
_GetMaterialTagsVersion(HdRenderIndex *renderIndex)
{
    HdStRenderParam *stRenderParam = static_cast<HdStRenderParam*>(
        renderIndex->GetRenderDelegate()->GetRenderParam());

    return size_t(stRenderParam->GetMaterialTagsVersion());
}

static size_t
_GetGeomSubsetDrawItemsVersion(HdRenderIndex *renderIndex)
{
    HdStRenderParam *stRenderParam = static_cast<HdStRenderParam*>(
        renderIndex->GetRenderDelegate()->GetRenderParam());

    return size_t(stRenderParam->GetGeomSubsetDrawItemsVersion());
}

//------------------------------------------------------------------------------

HdDrawItemConstPtrVectorSharedPtr
HdSt_DrawItemsCache::GetDrawItems(
    HdRprimCollection const &collection,
    TfTokenVector const &renderTags,
    HdRenderIndex *renderIndex,
    HdDrawItemConstPtrVectorSharedPtr const &curDrawItems)
{
    TRACE_FUNCTION();

    if (!renderIndex) {
        TF_CODING_ERROR("Received a null render index\n");
        return HdDrawItemConstPtrVectorSharedPtr();
    }

    const _CacheKey key(collection, renderTags, renderIndex);
    std::pair<_Cache::iterator, bool> ret =
        _cache.insert(
            std::pair<_CacheKey, _CacheValue>(key, _CacheValue()) );
    _CacheValue &val = ret.first->second;
    const bool cacheMiss = ret.second; // Insertion success
    const bool staleEntry = !cacheMiss &&
        _IsCacheEntryStale(val, collection.GetName(), renderIndex);
    
    // 3 possibilities:
    // 1. Doesn't exist in cache => Query render index and add entry
    // 2, Exists in cache and stale => Query render index and update entry
    // 3. Exists in cache and up-to-date => Return cached entry
    if (TfDebug::IsEnabled(HDST_DRAWITEMS_CACHE)) {
        std::stringstream ss;

        if (cacheMiss) {
            ss << "[MISS] Didn't find cache entry for collection " << collection
            << ", render tags " << renderTags << ". Fetching draw items...\n";
        } else if (staleEntry) {
            ss << "[MISS] Found stale cache entry for collection " << collection
            << ", render tags " << renderTags
            << ". Fetching updated draw items...\n";
        } else {
            ss << "[HIT] Found up-to-date cache entry for collection " 
            << collection << ", render tags " << renderTags << "\n";
        }

        TfDebug::Helper().Msg(ss.str());
    }
    
    if (cacheMiss || staleEntry) {
        _UpdateCacheEntry(collection, renderTags, renderIndex, &val);

        if (cacheMiss) {
            HD_PERF_COUNTER_INCR(HdStPerfTokens->drawItemsCacheMiss);
        } else {
            HD_PERF_COUNTER_INCR(HdStPerfTokens->drawItemsCacheStale);
        }
    } else if (val.drawItems != curDrawItems) {
        // The metric we care about is the number of times the cache has the
        // up-to-date draw items while the render pass doesn't.
        HD_PERF_COUNTER_INCR(HdStPerfTokens->drawItemsCacheHit);
    }
    
    return val.drawItems;
}

void
HdSt_DrawItemsCache::GarbageCollect()
{
    TRACE_FUNCTION();

    // Remove map entries wherein the draw items are not referred to from
    // anywhere else (i.e., the cache entry is the only reference).
    // NOTE: We could use a more sophisticated policy based on last use, memory
    // limits and such, but for now, this simple policy evicts entries as soon
    // as no render passes refer to them.
    for (typename _Cache::iterator it = _cache.begin();
         it != _cache.end();) {
        _CacheValue const &val = it->second;
        if (val.drawItems.use_count() == 1) {
            it = _cache.erase(it);
        } else {
            ++it;
        }
    }
}

bool
HdSt_DrawItemsCache::_IsCacheEntryStale(
    HdSt_DrawItemsCache::_CacheValue const &val,
    TfToken const &collectionName,
    HdRenderIndex *renderIndex) const
{
    HdChangeTracker const &tracker = renderIndex->GetChangeTracker();
    const bool collectionVersionChanged =
        val.collectionVersion != tracker.GetCollectionVersion(collectionName);
    const bool renderTagsVersionChanged =
        val.renderTagsVersion != tracker.GetRenderTagVersion();
    const bool materialTagsVersionChanged =
        val.materialTagsVersion != _GetMaterialTagsVersion(renderIndex);
    const bool geomSubsetDrawItemsChanged =
        val.geomSubsetDrawItemsVersion != _GetGeomSubsetDrawItemsVersion(
            renderIndex);

    return collectionVersionChanged ||
           renderTagsVersionChanged ||
           materialTagsVersionChanged ||
           geomSubsetDrawItemsChanged;
}

void
HdSt_DrawItemsCache::_UpdateCacheEntry(
    HdRprimCollection const &collection,
    TfTokenVector const &renderTags,
    HdRenderIndex *renderIndex,
    HdSt_DrawItemsCache::_CacheValue *val)
{
    TRACE_FUNCTION();

    HdChangeTracker const &tracker = renderIndex->GetChangeTracker();

    val->collectionVersion =
        tracker.GetCollectionVersion(collection.GetName());
    val->renderTagsVersion = tracker.GetRenderTagVersion();
    val->materialTagsVersion = _GetMaterialTagsVersion(renderIndex);
    val->geomSubsetDrawItemsVersion = _GetGeomSubsetDrawItemsVersion(
        renderIndex);

    val->drawItems = std::make_shared<HdDrawItemConstPtrVector>();

    const HdStRenderParam * const renderParam =
        static_cast<HdStRenderParam *>(
            renderIndex->GetRenderDelegate()->GetRenderParam());
    if (renderParam->HasMaterialTag(collection.GetMaterialTag()) &&
        (renderTags.empty() || renderParam->HasAnyRenderTag(renderTags)))
    {
            HdDrawItemConstPtrVector drawItems = 
                renderIndex->GetDrawItems(collection, renderTags);
            val->drawItems->swap(drawItems);
    }
    // No need to even call GetDrawItems when we know that
    // there is no prim with the desired tags.
}


PXR_NAMESPACE_CLOSE_SCOPE
