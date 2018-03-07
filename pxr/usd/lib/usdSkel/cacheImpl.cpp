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

#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usd/primRange.h"

#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usdGeom/xformable.h"

#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/debugCodes.h"
#include "pxr/usd/usdSkel/packedJointAnimation.h"
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
    _cache->_skinningQueryCache.clear();
}


// ------------------------------------------------------------
// UsdSkel_CacheImpl::ReadScope
// ------------------------------------------------------------


UsdSkel_CacheImpl::ReadScope::ReadScope(UsdSkel_CacheImpl* cache)
    : _cache(cache), _lock(cache->_mutex, /*write*/ false)
{}


UsdSkel_SkelDefinitionRefPtr
UsdSkel_CacheImpl::ReadScope::FindOrCreateSkelDefinition(const UsdPrim& prim)
{
    TRACE_FUNCTION();

    if(!prim || !prim.IsActive())
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


UsdSkel_AnimQueryImplRefPtr
UsdSkel_CacheImpl::ReadScope::FindOrCreateAnimQuery(const UsdPrim& prim)
{
    TRACE_FUNCTION();

    if(!prim || !prim.IsActive())
        return nullptr;

    if(prim.IsInstanceProxy())
        return FindOrCreateAnimQuery(prim.GetPrimInMaster());

    {
        _PrimToAnimMap::const_accessor a;
        if(_cache->_animQueryCache.find(a, prim))
            return a->second;
    }

    if(UsdSkel_AnimQueryImpl::IsAnimPrim(prim)) {
        _PrimToAnimMap::accessor a;
        if(_cache->_animQueryCache.insert(a, prim)) {
            a->second = UsdSkel_AnimQueryImpl::New(prim);
        }
        return a->second;
    }
    return nullptr;
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
    {
        _SkinningQueryMap::const_accessor a;
        if(_cache->_skinningQueryCache.find(a, key))
            return a->second;
    }
    
    _SkinningQueryMap::accessor a;
    if(_cache->_skinningQueryCache.insert(a, key)) {

        a->second = UsdSkelSkinningQuery(
            skinnedPrim,
            key.skelQuery ? key.skelQuery.GetJointOrder() : VtTokenArray(),
            key.jointIndicesAttr, key.jointWeightsAttr,
            key.geomBindTransformAttr,
            key.jointOrder ? &(*key.jointOrder) : nullptr);
    }
    return a->second;
}


UsdSkelSkeletonQuery
UsdSkel_CacheImpl::ReadScope::GetSkelQuery(const UsdPrim& prim) const
{
    _PrimToSkelQueryMap::const_accessor a;
    if(_cache->_skelQueryCache.find(a, prim))
        return a->second;
    return UsdSkelSkeletonQuery();
}


UsdSkelSkeletonQuery
UsdSkel_CacheImpl::ReadScope::GetInheritedSkelQuery(const UsdPrim& prim) const
{
    _PrimToSkelQueryMap::const_accessor a;
    for(UsdPrim p = prim; p; p = p.GetParent()) {
        if(_cache->_skelQueryCache.find(a, p)) {
            return a->second;
        }
        if(prim.IsA<UsdSkelRoot>()) {
            break;
        }
    }
    return UsdSkelSkeletonQuery();
}


UsdSkelSkeletonQuery
UsdSkel_CacheImpl::ReadScope::_FindOrCreateSkelQuery(
    const UsdPrim& instancePrim,
    const UsdPrim& skelPrim,
    const UsdSkelAnimQuery& animQuery)
{
    // TODO: We currently do not deduplicate skeleton queries,
    // but it may be worthwhile to do so.
    // Similarly, with many instances, it might be necessary to find
    // a way to share the anim->skel anim mappers.
    return UsdSkelSkeletonQuery(instancePrim,
                                FindOrCreateSkelDefinition(skelPrim),
                                animQuery);
}


namespace {


/// Return the a resolved prim for a target in \p targets.
UsdPrim
_GetFirstTarget(const UsdRelationship& rel, const SdfPathVector& targets)
{
    if(targets.size() > 0) {
        if(targets.size() > 1) {
            TF_WARN("%s -- relationship has more than one target. "
                    "Only the first will be used.",
                    rel.GetPath().GetText());
        }
        if(UsdPrim prim = rel.GetStage()->GetPrimAtPath(targets.front()))
            return prim;
        
        TF_WARN("%s -- Invalid target <%s>.",
                rel.GetPath().GetText(), targets.front().GetText());
    }
    return UsdPrim();
}


/// Create a string representing an indent.
std::string
_MakeIndent(size_t count, int indentSize=2)
{
    return std::string(count*indentSize, ' ');
}


} // namespace


void
UsdSkel_CacheImpl::ReadScope::_RecursivePopulate(const UsdPrim& prim,
                                                 SkinningQueryKey key,
                                                 UsdSkelAnimQuery animQuery,
                                                 size_t depth)
{
    if(!prim.IsA<UsdGeomImageable>()) {
        TF_DEBUG(USDSKEL_CACHE).Msg(
            "[UsdSkelCache]: %sPruning traversal at <%s> "
            "(prim types is not a UsdGeomImageable)\n",
            _MakeIndent(depth).c_str(), prim.GetPath().GetText());
        return;
    }
        
    TF_DEBUG(USDSKEL_CACHE).Msg("[UsdSkelCache]: %sVisiting <%s>\n",
                                _MakeIndent(depth).c_str(), 
                                prim.GetPath().GetText());

    UsdSkelBindingAPI binding(prim);

    if(UsdRelationship rel = binding.GetAnimationSourceRel()) {
        SdfPathVector targets;
        if(rel.GetForwardedTargets(&targets)) {
            animQuery = FindOrCreateAnimQuery(_GetFirstTarget(rel, targets));
        }
    }

    if(UsdRelationship rel = binding.GetSkeletonRel()) {
        SdfPathVector targets;
        if(rel.GetForwardedTargets(&targets)) {
            _PrimToSkelQueryMap::accessor a;
            if(_cache->_skelQueryCache.insert(a, prim)) {
                a->second = _FindOrCreateSkelQuery(
                    prim, _GetFirstTarget(rel, targets), animQuery);
            }
            key.skelQuery = a->second;

            TF_DEBUG(USDSKEL_CACHE).Msg(
                "[UsdSkelCache]: %sNew skeleton bound at <%s>: %s\n",
                _MakeIndent(depth).c_str(), prim.GetPath().GetText(),
                key.skelQuery.GetDescription().c_str());
        }
    }

    if(UsdAttribute attr = binding.GetJointIndicesAttr())
        key.jointIndicesAttr = attr;

    if(UsdAttribute attr = binding.GetJointWeightsAttr())
        key.jointWeightsAttr = attr;

    if(UsdAttribute attr = binding.GetGeomBindTransformAttr())
        key.geomBindTransformAttr = attr;

    if(UsdAttribute attr = binding.GetJointsAttr()) {
        VtTokenArray jointOrder;
        if(attr.Get(&jointOrder)) {
            key.jointOrder = jointOrder;
        }
    }

    if(prim.IsA<UsdGeomBoundable>() &&
       (key.jointIndicesAttr && key.jointWeightsAttr)) {

        _PrimToSkinningQueryMap::accessor a;
        if(_cache->_primSkinningQueryCache.insert(a, prim)) {
            a->second = _FindOrCreateSkinningQuery(prim, key);
        }

        TF_DEBUG(USDSKEL_CACHE).Msg(
            "[UsdSkelCache]: %sFound skinnable prim <%s> (valid? %d)\n",
            _MakeIndent(depth).c_str(), prim.GetPath().GetText(),
            a->second.IsValid());

        // Skinnable prims cannot be nested.
        return;
    }

    UsdPrim traversalPrim = !prim.IsInstance() ? prim : prim.GetMaster();
    for(const auto& child : prim.GetChildren()) {
        _RecursivePopulate(child, key, animQuery, depth+1);
    }
}


bool
UsdSkel_CacheImpl::ReadScope::Populate(const UsdSkelRoot& root)
{
    TRACE_FUNCTION();

    TF_DEBUG(USDSKEL_CACHE).Msg("[UsdSkelCache]: Populate map from <%s>\n",
                                root.GetPrim().GetPath().GetText());

    if(!root) {
        TF_CODING_ERROR("'root' is invalid.");
        return false;
    }

    _RecursivePopulate(root.GetPrim(), SkinningQueryKey(),
                       UsdSkelAnimQuery(), /*depth*/ 1);
    return true;
}


// ------------------------------------------------------------
// UsdSkel_CacheImpl::
// ------------------------------------------------------------


bool
UsdSkel_CacheImpl::_HashSkinningQueryKey::equal(const SkinningQueryKey& a,
                                                const SkinningQueryKey& b)
{
    return a.jointIndicesAttr == b.jointIndicesAttr &&
           a.jointWeightsAttr == b.jointWeightsAttr &&
           a.geomBindTransformAttr == b.geomBindTransformAttr &&
           a.skelQuery == b.skelQuery &&
           a.jointOrder == b.jointOrder;
}


size_t
UsdSkel_CacheImpl::_HashSkinningQueryKey::hash(const SkinningQueryKey& key)
{
    size_t hash = hash_value(key.jointIndicesAttr);
    boost::hash_combine(hash, key.jointWeightsAttr);
    boost::hash_combine(hash, key.geomBindTransformAttr);
    boost::hash_combine(hash, key.skelQuery);
    if(key.jointOrder) {
        boost::hash_combine(hash, *key.jointOrder);
    }
    return hash;
}


PXR_NAMESPACE_CLOSE_SCOPE
