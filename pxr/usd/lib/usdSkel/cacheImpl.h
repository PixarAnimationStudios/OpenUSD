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
#ifndef USDSKEL_CACHEIMPL
#define USDSKEL_CACHEIMPL

/// \file usdSkel/cacheImpl.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/usdSkel/animMapper.h"
#include "pxr/usd/usdSkel/animQueryImpl.h"
#include "pxr/usd/usdSkel/animQuery.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/skelDefinition.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"
#include "pxr/usd/usdSkel/skinningQuery.h"

#include <boost/optional.hpp>

#include <tbb/concurrent_hash_map.h>
#include <tbb/queuing_rw_mutex.h>

#include <unordered_map>


PXR_NAMESPACE_OPEN_SCOPE


class UsdGeomXformCache;
class UsdSkelRoot;


struct UsdSkel_HashPrim {
    inline size_t operator()(const UsdPrim& prim) const {
        return hash_value(prim);
    }

    static bool equal(const UsdPrim& a, const UsdPrim& b) {
        return a == b;
    }

    static size_t hash(const UsdPrim& prim) {
        return hash_value(prim);
    }
};


/// Internal cache implementation.
class UsdSkel_CacheImpl
{
public:
    using RWMutex = tbb::queuing_rw_mutex;

    struct SkinningQueryKey {
        UsdAttribute jointIndicesAttr;
        UsdAttribute jointWeightsAttr;
        UsdAttribute geomBindTransformAttr;
        UsdAttribute jointsAttr;
        UsdAttribute blendShapesAttr;
        UsdRelationship blendShapeTargetsRel;
        UsdPrim skel;
    };
    
    /// Scope for performing read-only operations on the cache.
    /// Any thread-safe operations should be called here.
    struct ReadScope {
        ReadScope(UsdSkel_CacheImpl* cache);

        // Getters for properties with a direct prim association.
        // These are produced on-demand rather than through Populate().

        UsdSkelAnimQuery
        FindOrCreateAnimQuery(const UsdPrim& prim);

        UsdSkel_SkelDefinitionRefPtr
        FindOrCreateSkelDefinition(const UsdPrim& prim);

        UsdSkelSkeletonQuery
        FindOrCreateSkelQuery(const UsdPrim& prim);

        /// Method for populating the cache with cache properties, for
        /// the set of properties that depend on inherited state.
        /// Returns true if any skinnable prims were populated.
        bool Populate(const UsdSkelRoot& root);

        // Getters for properties added to the cache through Populate().

        UsdSkelSkinningQuery
        GetSkinningQuery(const UsdPrim& prim) const;

    private:

        UsdSkelSkinningQuery
        _FindOrCreateSkinningQuery(const UsdPrim& skinnedPrim,
                                   const SkinningQueryKey& key);

        using _PrimToSkinMap =
            std::unordered_map<UsdPrim,SkinningQueryKey,UsdSkel_HashPrim>;

        /// Recursively populate the cache beneath \p prim.
        /// Returns the number of skinnable prims populated beneath \p prim.
        void _RecursivePopulate(const SdfPath& rootPath,
                                const UsdPrim& prim,
                                SkinningQueryKey key,
                                UsdSkelAnimQuery animQuery,
                                _PrimToSkinMap* skinBindingMap,
                                size_t depth=1);

    private:
        UsdSkel_CacheImpl* _cache;
        RWMutex::scoped_lock _lock;
    };

    /// Scope for performing write operations on the cache.
    /// This is used for non-threadsafe operations, like cache clearing.
    struct WriteScope {
        WriteScope(UsdSkel_CacheImpl* cache);

        void Clear();

    private:
        UsdSkel_CacheImpl* _cache;
        RWMutex::scoped_lock _lock;
    };

private:

    using _PrimToAnimMap =
        tbb::concurrent_hash_map<UsdPrim,
                                 UsdSkel_AnimQueryImplRefPtr,
                                 UsdSkel_HashPrim>;

    using _PrimToSkelDefinitionMap =
        tbb::concurrent_hash_map<UsdPrim,
                                 UsdSkel_SkelDefinitionRefPtr,
                                 UsdSkel_HashPrim>;

    using _PrimToSkelQueryMap =
        tbb::concurrent_hash_map<UsdPrim,
                                 UsdSkelSkeletonQuery,
                                 UsdSkel_HashPrim>;

    using _PrimToSkinningQueryMap =
        tbb::concurrent_hash_map<UsdPrim,
                                 UsdSkelSkinningQuery,
                                 UsdSkel_HashPrim>;

    using _RWMutex = tbb::queuing_rw_mutex;

    _PrimToAnimMap _animQueryCache;
    _PrimToSkelDefinitionMap _skelDefinitionCache;
    _PrimToSkelQueryMap _skelQueryCache;
    _PrimToSkinningQueryMap _primSkinningQueryCache;

    /// Mutex around unsafe operations (eg., clearing the maps)
    mutable _RWMutex _mutex; // XXX: Not recursive!
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKEL_CACHEIMPL
