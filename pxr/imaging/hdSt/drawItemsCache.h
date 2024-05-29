//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDST_DRAW_ITEMS_CACHE_H
#define PXR_IMAGING_HDST_DRAW_ITEMS_CACHE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/rprimCollection.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/hash.h"
#include "pxr/usd/sdf/path.h"

#include <vector>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class HdChangeTracker;
class HdRenderIndex;
using HdDrawItemConstPtrVector = std::vector<class HdDrawItem const*>;
using HdDrawItemConstPtrVectorSharedPtr =
    std::shared_ptr<HdDrawItemConstPtrVector>;


// This class provides a caching mechanism for the filtered draw items returned
// by the render index given a collection and a list of render tags.
//
// The cache is owned by the Storm render delegate.
// Storm render passes may query the cache using GetDrawItems to obtain a
// shared pointer to the updated vector of draw items.
//
// The public interface is designed to be simple:
// * GetDrawItems(render index, collection, render tags, curDrawItems):
//   Render passes may simply call this function to get a shared pointer to the
//   filtered vector of draw items.
//   The render pass' current draw items (`curDrawItems`) is used only for
//   performance tracking to determine when the cache has the up-to-date
//   draw items while the render pass does not.
//
// * GarbageCollect(): The render delegate may call this during the
//   CommitResources step to free entries that are no longer used by any render
//   pass.
//
// Performance note:
// This caching is useful when different tasks use the same query,
// which may arise in several scenarios that are application dependent.
// One example is an app with multiple viewers, each of which manages their
// own set of (similar) Hydra tasks. Another example is shadow map generation
// where the same set of shadow caster prims may be rendered repeatedly to
// generate a shadow map for each light.
//
class HdSt_DrawItemsCache final
{
public:
    HdSt_DrawItemsCache() = default;

    // See comments above.
    HDST_API
    HdDrawItemConstPtrVectorSharedPtr GetDrawItems(
        HdRprimCollection const &collection,
        TfTokenVector const &renderTags,
        HdRenderIndex *renderIndex,
        HdDrawItemConstPtrVectorSharedPtr const &curDrawItems);

    // See comments above.
    HDST_API
    void GarbageCollect();

private:
    struct _CacheKey
    {
        _CacheKey(HdRprimCollection const &collection,
                  TfTokenVector const &renderTags,
                  HdRenderIndex *renderIndex)
        : _collection(collection)
        , _renderTags(renderTags)
        , _renderIndex(renderIndex) {}

        // TfHash support.
        template <class HashState>
        friend void TfHashAppend(HashState &h, _CacheKey const &key) {
            h.Append(key._collection, key._renderTags, key._renderIndex);
        }

        bool operator==(_CacheKey const &other) const {
            return _collection == other._collection &&
                   _renderTags == other._renderTags &&
                   _renderIndex == other._renderIndex;
        }

    private:
        HdRprimCollection const _collection;
        TfTokenVector const _renderTags;
        HdRenderIndex * const _renderIndex;
    };

    struct _CacheValue
    {
        _CacheValue()
        : collectionVersion(0),
          renderTagsVersion(0),
          materialTagsVersion(0),
          geomSubsetDrawItemsVersion(0) {}
        
        HdDrawItemConstPtrVectorSharedPtr drawItems;
        size_t collectionVersion;
        size_t renderTagsVersion;
        size_t materialTagsVersion;
        size_t geomSubsetDrawItemsVersion;
    };

    bool _IsCacheEntryStale(_CacheValue const &val,
                            TfToken const &collectionName,
                            HdRenderIndex *renderIndex) const;

    void _UpdateCacheEntry(
        HdRprimCollection const &collection,
        TfTokenVector const &renderTags,
        HdRenderIndex *renderIndex,
        _CacheValue *val);

    HdSt_DrawItemsCache(const HdSt_DrawItemsCache &) = delete;
    HdSt_DrawItemsCache &operator =(const HdSt_DrawItemsCache &) = delete;

    //--------------------------------------------------------------------------
    using _Cache = std::unordered_map<_CacheKey, _CacheValue, TfHash>;
    _Cache _cache;
};

using HdStDrawItemsCachePtr = HdSt_DrawItemsCache *;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDST_DRAW_ITEMS_CACHE_H
