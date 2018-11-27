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
#include "pxr/usd/usdSkel/cacheImpl.h"

#include "pxr/base/arch/hints.h"

#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usd/primRange.h"

#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdGeom/imageable.h"

#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/debugCodes.h"
#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/utils.h"


PXR_NAMESPACE_OPEN_SCOPE


// ------------------------------------------------------------
// UsdSkel_CacheImpl::WriteScope
// ------------------------------------------------------------


UsdSkel_CacheImpl::WriteScope::WriteScope(UsdSkel_CacheImpl* cache)
    : _cache(cache), _lock(cache->_mutex, /*write*/ true)
{}


void
UsdSkel_CacheImpl::WriteScope::Clear()
{
    _cache->_animQueryCache.clear();
    _cache->_skelDefinitionCache.clear();
    _cache->_skelQueryCache.clear();
    _cache->_primSkinningQueryCache.clear();
}


// ------------------------------------------------------------
// UsdSkel_CacheImpl::ReadScope
// ------------------------------------------------------------


UsdSkel_CacheImpl::ReadScope::ReadScope(UsdSkel_CacheImpl* cache)
    : _cache(cache), _lock(cache->_mutex, /*write*/ false)
{}


UsdSkelAnimQuery
UsdSkel_CacheImpl::ReadScope::FindOrCreateAnimQuery(const UsdPrim& prim)
{
    TRACE_FUNCTION();

    if(ARCH_UNLIKELY(!prim || !prim.IsActive()))
        return UsdSkelAnimQuery();

    if(prim.IsInstanceProxy())
        return FindOrCreateAnimQuery(prim.GetPrimInMaster());

    {
        _PrimToAnimMap::const_accessor a;
        if(_cache->_animQueryCache.find(a, prim))
            return UsdSkelAnimQuery(a->second);
    }

    if (UsdSkelIsSkelAnimationPrim(prim)) {
        _PrimToAnimMap::accessor a;
        if(_cache->_animQueryCache.insert(a, prim)) {
            a->second = UsdSkel_AnimQueryImpl::New(prim);
        }
        return UsdSkelAnimQuery(a->second);
    }
    return UsdSkelAnimQuery();
}


UsdSkel_SkelDefinitionRefPtr
UsdSkel_CacheImpl::ReadScope::FindOrCreateSkelDefinition(const UsdPrim& prim)
{
    TRACE_FUNCTION();

    if(ARCH_UNLIKELY(!prim || !prim.IsActive()))
        return nullptr;

    if(prim.IsInstanceProxy())
        return FindOrCreateSkelDefinition(prim.GetPrimInMaster());

    {
        _PrimToSkelDefinitionMap::const_accessor a;
        if(_cache->_skelDefinitionCache.find(a, prim))
            return a->second;
    }

    if(prim.IsA<UsdSkelSkeleton>()) {
        _PrimToSkelDefinitionMap::accessor a;
        if(_cache->_skelDefinitionCache.insert(a, prim)) {
            a->second = UsdSkel_SkelDefinition::New(UsdSkelSkeleton(prim));
        }
        return a->second;
    }
    return nullptr;
}


UsdSkelSkeletonQuery
UsdSkel_CacheImpl::ReadScope::FindOrCreateSkelQuery(const UsdPrim& prim)
{
    TRACE_FUNCTION();

    {
        _PrimToSkelQueryMap::const_accessor a;
        if (_cache->_skelQueryCache.find(a, prim))
            return a->second;
    }

    if (auto skelDef = FindOrCreateSkelDefinition(prim)) {
        _PrimToSkelQueryMap::accessor a;
        if (_cache->_skelQueryCache.insert(a, prim)) {

            UsdSkelAnimQuery animQuery =
                FindOrCreateAnimQuery(
                    UsdSkelBindingAPI(prim).GetInheritedAnimationSource());

            a->second = UsdSkelSkeletonQuery(skelDef, animQuery);
        }
        return a->second;
    }
    return UsdSkelSkeletonQuery();
}


UsdSkelSkinningQuery
UsdSkel_CacheImpl::ReadScope::GetSkinningQuery(const UsdPrim& prim) const
{
    _PrimToSkinningQueryMap::const_accessor a;
    if(_cache->_primSkinningQueryCache.find(a, prim))
        return a->second;
    return UsdSkelSkinningQuery();
}


UsdSkelSkinningQuery
UsdSkel_CacheImpl::ReadScope::_FindOrCreateSkinningQuery(
    const UsdPrim& skinnedPrim,
    const SkinningQueryKey& key)
{
    UsdSkelSkeletonQuery skelQuery = FindOrCreateSkelQuery(key.skel);

    // TODO: Consider some form of deduplication.
    return UsdSkelSkinningQuery(
        skinnedPrim,
        skelQuery ? skelQuery.GetJointOrder() : VtTokenArray(),
        key.jointIndicesAttr, key.jointWeightsAttr,
        key.geomBindTransformAttr, key.jointsAttr);
}


namespace {


/// Create a string representing an indent.
std::string
_MakeIndent(size_t count, int indentSize=2)
{
    return std::string(count*indentSize, ' ');
}


} // namespace


bool
UsdSkel_CacheImpl::ReadScope::Populate(const UsdSkelRoot& root)
{
    TRACE_FUNCTION();

    TF_DEBUG(USDSKEL_CACHE).Msg("[UsdSkelCache] Populate map from <%s>\n",
                                root.GetPrim().GetPath().GetText());

    if(!root) {
        TF_CODING_ERROR("'root' is invalid.");
        return false;
    }

    std::vector<std::pair<SkinningQueryKey,UsdPrim> > stack(1);

    UsdPrimRange range =
        UsdPrimRange::PreAndPostVisit(root.GetPrim(),
                                      UsdPrimDefaultPredicate);
                                      // UsdPrimIsInstance);

    for (auto it = range.begin(); it != range.end(); ++it) {
        
        if (it.IsPostVisit()) {
            if (stack.size() > 0 && stack.back().second == *it) {
                stack.pop_back();
            }
            continue;
        }

        if (ARCH_UNLIKELY(!it->IsA<UsdGeomImageable>())) {
            TF_DEBUG(USDSKEL_CACHE).Msg(
                "[UsdSkelCache]: %sPruning traversal at <%s> "
                "(prim is not UsdGeomImageable)\n",
                _MakeIndent(stack.size()).c_str(), it->GetPath().GetText());

            it.PruneChildren();
            continue;
        }

        // TODO: Consider testing whether or not the API has been applied first.
        UsdSkelBindingAPI binding(*it);

        SkinningQueryKey key(stack.back().first);

        UsdSkelSkeleton skel;
        if (binding.GetSkeleton(&skel))
            key.skel = skel.GetPrim();

        if (UsdAttribute attr = binding.GetJointIndicesAttr())
            key.jointIndicesAttr = attr;

        if (UsdAttribute attr = binding.GetJointWeightsAttr())
            key.jointWeightsAttr = attr;
        
        if (UsdAttribute attr = binding.GetGeomBindTransformAttr())
            key.geomBindTransformAttr = attr;

        if (UsdAttribute attr = binding.GetJointsAttr())
            key.jointsAttr = attr;

        if (UsdSkelIsSkinnablePrim(*it) &&
            key.jointIndicesAttr && key.jointWeightsAttr) {

            _PrimToSkinningQueryMap::accessor a;
            if (_cache->_primSkinningQueryCache.insert(a, *it)) {
                a->second = _FindOrCreateSkinningQuery(*it, key);
            }

            TF_DEBUG(USDSKEL_CACHE).Msg(
                "[UsdSkelCache] %sAdded skinning query for prim <%s> "
                "(valid = %d).\n", _MakeIndent(stack.size()).c_str(),
                it->GetPath().GetText(), a->second.IsValid());
        }

        stack.emplace_back(key, *it);
    }
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
