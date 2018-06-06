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
#include "pxr/usd/usdSkel/cache.h"

#include "pxr/usd/usd/primRange.h"

#include "pxr/usd/usdSkel/cacheImpl.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"
#include "pxr/usd/usdSkel/skinningQuery.h"


PXR_NAMESPACE_OPEN_SCOPE


UsdSkelCache::UsdSkelCache()
    : _impl(new UsdSkel_CacheImpl)
{}


void
UsdSkelCache::Clear()
{
    return UsdSkel_CacheImpl::WriteScope(_impl.get()).Clear();
}


UsdSkelAnimQuery
UsdSkelCache::GetAnimQuery(const UsdPrim& prim)
{
    return UsdSkel_CacheImpl::ReadScope(_impl.get())
        .FindOrCreateAnimQuery(prim);
}


bool
UsdSkelCache::Populate(const UsdSkelRoot& root)
{
    return UsdSkel_CacheImpl::ReadScope(_impl.get()).Populate(root);
}


UsdSkelSkeletonQuery
UsdSkelCache::GetSkelQuery(const UsdPrim& prim) const
{
    return UsdSkel_CacheImpl::ReadScope(_impl.get()).GetSkelQuery(prim);
}


UsdSkelSkeletonQuery
UsdSkelCache::GetInheritedSkelQuery(const UsdPrim& prim) const
{
    return UsdSkel_CacheImpl::ReadScope(_impl.get())
        .GetInheritedSkelQuery(prim);
}


UsdSkelSkinningQuery
UsdSkelCache::GetSkinningQuery(const UsdPrim& prim) const
{
    return UsdSkel_CacheImpl::ReadScope(_impl.get()).GetSkinningQuery(prim);
}


bool
UsdSkelCache::ComputeSkinnedPrims(
    const UsdPrim& prim,
    std::vector<std::pair<UsdPrim,UsdSkelSkinningQuery> >* pairs) const
{
    TRACE_FUNCTION();

    if(!pairs) {
        TF_CODING_ERROR("'pairs' pointer is null.");
        return false;
    }
    
    pairs->clear();
    
    auto range = UsdPrimRange(prim);
    for(auto it = range.begin(); it != range.end(); ++it) {
        if(*it != prim) {
            if(GetSkelQuery(*it)) {
                // Found another bound skel.
                // Any skinnable prims found at or beneath this scope
                // would be deformed by the other skel.
                it.PruneChildren();
                continue;
            }
        }

        if(UsdSkelSkinningQuery query = GetSkinningQuery(*it)) {
            pairs->emplace_back(*it, query);
        }
    }
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
