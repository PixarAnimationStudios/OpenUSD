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
#ifndef USDSKEL_CACHE_H
#define USDSKEL_CACHE_H

/// \file usdSkel/cache.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"

#include "pxr/usd/usd/prim.h"

#include "pxr/usd/usdSkel/animQuery.h"

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


class UsdSkelRoot;
class UsdSkelSkeletonQuery;
class UsdSkelSkinningQuery;

TF_DECLARE_REF_PTRS(UsdSkelBinding);


/// \class UsdSkelCache
///
/// Thread-safe cache for accessing query objects for evaluating skeletal data.
/// This provides caching of major structural components, such as skeletal
/// topology.
/// In a streaming context, this cache is intended to persist.
class UsdSkelCache
{
public:
    USDSKEL_API
    UsdSkelCache();

    USDSKEL_API
    void Clear();

    /// Populate the cache for the skeletal data beneath prim \p root.
    USDSKEL_API
    bool Populate(const UsdSkelRoot& root);

    /// Get a skel query at \p prim, if any is defined.
    /// Skel queries are created wherever \em skel:skeleton relationships are set.
    /// The caller must first Populate() the cache with the skel root containing
    /// \p prim in order for any skel queries to be discoverabble.
    USDSKEL_API
    UsdSkelSkeletonQuery GetSkelQuery(const UsdPrim& prim) const;

    /// Get a skel query at \p prim, or any of its ancestor (within the skel root),
    /// if any is defined. Skel queries are created wherever \em skel:skeleton
    /// relationships are set.
    /// The caller must first Populate() the cache with the skel root containing
    /// \p prim in order for any skel queries to be discoverabble.
    USDSKEL_API
    UsdSkelSkeletonQuery GetInheritedSkelQuery(const UsdPrim& prim) const;

    /// Get a skinning query at \p prim. Skinning queries are defined at any
    /// skinnable prims (I.e., boundable prims with fully defined joint
    /// influences).
    ///
    /// The caller must first Populate() the cache with the skel root containing
    /// \p prim in order for any skinning queries to be discoverabble.
    USDSKEL_API
    UsdSkelSkinningQuery GetSkinningQuery(const UsdPrim& prim) const;

    /// Get an anim query corresponding to \p prim.
    /// This does not require Populate() to be called on the cache.
    USDSKEL_API
    UsdSkelAnimQuery GetAnimQuery(const UsdPrim& prim);

    /// Get a vector of (prim,skinningQuery) pairs identifying the set of
    /// prims that would be deformed by a skeleton bound at \p prim,
    /// along with a query object that can be used to access skinning-related
    /// information.
    USDSKEL_API
    bool ComputeSkinnedPrims(
        const UsdPrim& prim,
        std::vector<std::pair<UsdPrim,UsdSkelSkinningQuery> >* pairs) const;

private:
    std::shared_ptr<class UsdSkel_CacheImpl> _impl;

    friend class UsdSkelAnimQuery;
    friend class UsdSkelSkeletonQuery;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKEL_EVALCACHE_H
