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
#ifndef PXR_USD_USD_SKEL_CACHE_H
#define PXR_USD_USD_SKEL_CACHE_H

/// \file usdSkel/cache.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primFlags.h"

#include "pxr/usd/usdSkel/animQuery.h"
#include "pxr/usd/usdSkel/binding.h"

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


class UsdSkelRoot;
class UsdSkelAnimation;
class UsdSkelSkeleton;
class UsdSkelSkeletonQuery;
class UsdSkelSkinningQuery;

TF_DECLARE_REF_PTRS(UsdSkelBinding);


/// Thread-safe cache for accessing query objects for evaluating skeletal data.
///
/// This provides caching of major structural components, such as skeletal
/// topology. In a streaming context, this cache is intended to persist.
class UsdSkelCache
{
public:
    USDSKEL_API
    UsdSkelCache();

    USDSKEL_API
    void Clear();

    /// Populate the cache for the skeletal data beneath prim \p root,
    /// as traversed using \p predicate.
    ///
    /// Population resolves inherited skel bindings set using the
    /// UsdSkelBindingAPI, making resolved bindings available through
    /// GetSkinningQuery(), ComputeSkelBdining() and ComputeSkelBindings().
    USDSKEL_API
    bool Populate(const UsdSkelRoot& root,
                  Usd_PrimFlagsPredicate predicate) const;

    /// Get a skel query for computing properties of \p skel.
    ///
    /// This does not require Populate() to be called on the cache.
    USDSKEL_API
    UsdSkelSkeletonQuery GetSkelQuery(const UsdSkelSkeleton& skel) const;

    /// Get an anim query corresponding to \p anim.
    ///
    /// This does not require Populate() to be called on the cache.
    USDSKEL_API
    UsdSkelAnimQuery GetAnimQuery(const UsdSkelAnimation& anim) const;

    /// \overload
    /// \deprecated
    USDSKEL_API
    UsdSkelAnimQuery GetAnimQuery(const UsdPrim& prim) const;

    /// Get a skinning query at \p prim.
    ///
    /// Skinning queries are defined at any skinnable prims (I.e., boundable
    /// prims with fully defined joint influences).
    ///
    /// The caller must first Populate() the cache with the skel root containing
    /// \p prim, with a predicate that will visit \p prim, in order for a
    /// skinning query to be discoverable.
    USDSKEL_API
    UsdSkelSkinningQuery GetSkinningQuery(const UsdPrim& prim) const;

    /// Compute the set of skeleton bindings beneath \p skelRoot,
    /// as discovered through a traversal using \p predicate.
    ///
    /// Skinnable prims are only discoverable by this method if Populate()
    /// has already been called for \p skelRoot, with an equivalent predicate.
    USDSKEL_API
    bool ComputeSkelBindings(const UsdSkelRoot& skelRoot,
                             std::vector<UsdSkelBinding>* bindings,
                             Usd_PrimFlagsPredicate predicate) const;

    /// Compute the bindings corresponding to a single skeleton, bound beneath
    /// \p skelRoot, as discovered through a traversal using \p predicate.
    ///
    /// Skinnable prims are only discoverable by this method if Populate()
    /// has already been called for \p skelRoot, with an equivalent predicate.
    USDSKEL_API
    bool ComputeSkelBinding(const UsdSkelRoot& skelRoot,
                            const UsdSkelSkeleton& skel,
                            UsdSkelBinding* binding,
                            Usd_PrimFlagsPredicate predicate) const;

private:
    std::shared_ptr<class UsdSkel_CacheImpl> _impl;
 
    friend class UsdSkelAnimQuery;
    friend class UsdSkelSkeletonQuery;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKEL_EVALCACHE_H
