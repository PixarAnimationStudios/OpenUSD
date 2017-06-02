//
// Copyright 2017 Pixar
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
#ifndef __GUSD_BOUNDSCACHE_H__
#define __GUSD_BOUNDSCACHE_H__

#include <pxr/pxr.h>
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/base/tf/token.h"

#include "USD_DataCache.h"

#include <UT/UT_BoundingBox.h>
#include <UT/UT_IntrusivePtr.h>
#include <UT/UT_ConcurrentHashMap.h>

PXR_NAMESPACE_OPEN_SCOPE

/// A wrapper arround UsdGeomBBoxCache. 
///
/// This singleton class keeps a cache per stage and per purpose.
/// It will be flushed when the stage cache is flushed. 
///
/// Unfortunaly UsdGeomBBoxCaches only store a single frame at
/// a time. I considered creating a cache per frame but I thought
/// that would defeat optimizations for non animated geometry.

class GusdBoundsCache : public GusdUSD_DataCache {
public:
    static GusdBoundsCache& GetInstance();

    GusdBoundsCache();
    virtual ~GusdBoundsCache();

    bool ComputeWorldBound(
            const UsdPrim &prim,
            UsdTimeCode time,
            const TfTokenVector &includedPurposes,
            UT_BoundingBox &bounds );

    bool ComputeUntransformedBound(
            const UsdPrim &prim,
            UsdTimeCode time,
            const TfTokenVector &includedPurposes,
            UT_BoundingBox &bounds );

    virtual void Clear();
    virtual int64 Clear(const UT_Set<std::string>& stageNames) override;

private:

    // Key that hashes the stage file name and a set of purposes.
    struct Key 
    {
        Key() : hash(0) {}
        
        Key(const TfToken &path, TfTokenVector purposes)
            : path(path), purposes( purposes ), hash(ComputeHash(path,purposes)) {}

        static std::size_t  ComputeHash(const TfToken &path, TfTokenVector purposes)
                            {
                                std::size_t h = hash_value(path);
                                boost::hash_combine(h, purposes);
                                return h; 
                            }

        bool                operator==(const Key& o) const
                            { return path == o.path &&
                                     purposes == o.purposes ; }

        friend size_t       hash_value(const Key& o)
                            { return o.hash; }

        struct HashCmp
        {
            static std::size_t  hash(const Key& key)
                                { return key.hash; }
            static bool         equal(const Key& a,
                                      const Key& b)
                                { return a == b; }
        };

        TfToken             path;
        TfTokenVector       purposes;
        std::size_t         hash;
    };

    struct Item : public UT_IntrusiveRefCounter<Item>
    {
        Item( UsdTimeCode time, TfTokenVector includedPurposes ) 
            : bboxCache( time, includedPurposes )
        {
        }
        
        UsdGeomBBoxCache bboxCache;
        std::mutex lock;
    };

    typedef GfBBox3d (UsdGeomBBoxCache::*ComputeFunc)(const UsdPrim& prim);

    bool _ComputeBound(
            const UsdPrim &prim,
            UsdTimeCode time,
            const TfTokenVector &includedPurposes,
            ComputeFunc boundFunc,
            UT_BoundingBox &bounds );   

    typedef UT_IntrusivePtr<Item> ItemHandle;

    typedef UT_ConcurrentHashMap<Key,ItemHandle,Key::HashCmp> MapType;
    MapType   m_map;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // __GUSD_BOUNDSCACHE_H__
