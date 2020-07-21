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

    if (ARCH_UNLIKELY(!prim || !prim.IsActive()))
        return UsdSkelAnimQuery();

    if (prim.IsInstanceProxy())
        return FindOrCreateAnimQuery(prim.GetPrimInMaster());

    {
        _PrimToAnimMap::const_accessor a;
        if (_cache->_animQueryCache.find(a, prim))
            return UsdSkelAnimQuery(a->second);
    }

    if (UsdSkelIsSkelAnimationPrim(prim)) {
        _PrimToAnimMap::accessor a;
        if (_cache->_animQueryCache.insert(a, prim)) {
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

    if (ARCH_UNLIKELY(!prim || !prim.IsActive()))
        return nullptr;

    if (prim.IsInstanceProxy())
        return FindOrCreateSkelDefinition(prim.GetPrimInMaster());

    {
        _PrimToSkelDefinitionMap::const_accessor a;
        if (_cache->_skelDefinitionCache.find(a, prim))
            return a->second;
    }

    if (prim.IsA<UsdSkelSkeleton>()) {
        _PrimToSkelDefinitionMap::accessor a;
        if (_cache->_skelDefinitionCache.insert(a, prim)) {
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
    if (_cache->_primSkinningQueryCache.find(a, prim))
        return a->second;
    return UsdSkelSkinningQuery();
}


UsdSkelSkinningQuery
UsdSkel_CacheImpl::ReadScope::_FindOrCreateSkinningQuery(
    const UsdPrim& skinnedPrim,
    const _SkinningQueryKey& key)
{
    UsdSkelSkeletonQuery skelQuery = FindOrCreateSkelQuery(key.skel);
    const UsdSkelAnimQuery& animQuery = skelQuery.GetAnimQuery();

    // TODO: Consider some form of deduplication.
    return UsdSkelSkinningQuery(
        skinnedPrim,
        skelQuery ? skelQuery.GetJointOrder() : VtTokenArray(),
        animQuery ? animQuery.GetBlendShapeOrder() : VtTokenArray(),
        key.jointIndicesAttr, key.jointWeightsAttr,
        key.geomBindTransformAttr, key.jointsAttr,
        key.blendShapesAttr, key.blendShapeTargetsRel);
}


namespace {

/// Create a string representing an indent.
std::string
_MakeIndent(size_t count, int indentSize=2)
{
    return std::string(count*indentSize, ' ');
}

void
_DeprecatedBindingCheck(bool hasBindingAPI, const UsdProperty& prop)
{
    if (!hasBindingAPI) {
        TF_WARN("Found binding property <%s>, but the SkelBindingAPI was not "
                "applied on the owning prim. In the future, binding properties "
                "will be ignored unless the SkelBindingAPI is applied "
                "(see UsdSkelBindingAPI::Apply)", prop.GetPath().GetText());
    }
}

} // namespace


bool
UsdSkel_CacheImpl::ReadScope::Populate(const UsdSkelRoot& root)
{
    TRACE_FUNCTION();

    TF_DEBUG(USDSKEL_CACHE).Msg("[UsdSkelCache] Populate map from <%s>\n",
                                root.GetPrim().GetPath().GetText());

    if (!root) {
        TF_CODING_ERROR("'root' is invalid.");
        return false;
    }

    std::vector<std::pair<_SkinningQueryKey,UsdPrim> > stack(1);

    // TODO: Consider traversing instance proxies at this point.
    // But when doing so, must ensure that UsdSkelBakeSkinning, et.al.,
    // take instancing into account.
    const UsdPrimRange range =
        UsdPrimRange::PreAndPostVisit(root.GetPrim(),
                                      UsdPrimDefaultPredicate);
                                      // UsdTraverseInstanceProxies());

    for (auto it = range.begin(); it != range.end(); ++it) {
        
        if (it.IsPostVisit()) {
            if (stack.size() > 0 && stack.back().second == *it) {
                stack.pop_back();
            }
            continue;
        }

        if (ARCH_UNLIKELY(!it->IsA<UsdGeomImageable>())) {
            TF_DEBUG(USDSKEL_CACHE).Msg(
                "[UsdSkelCache]  %sPruning traversal at <%s> "
                "(prim is not UsdGeomImageable)\n",
                _MakeIndent(stack.size()).c_str(), it->GetPath().GetText());

            it.PruneChildren();
            continue;
        }

        // XXX: For backwards-compatibility, must potentially look for
        // UsdSkelBindingAPI properties, even if the API schema was not
        // applied to the prim.
        
        const bool hasBindingAPI = it->HasAPI<UsdSkelBindingAPI>();
        
        _SkinningQueryKey key(stack.back().first);

        const UsdSkelBindingAPI binding(*it);

        UsdSkelSkeleton skel;
        if (binding.GetSkeleton(&skel)) {
            key.skel = skel.GetPrim();
        }

        // XXX: When looking for binding properties, only include
        // properties that have an authored value. Properties with
        // no authored value are treated as if they do not exist.

        if (UsdAttribute attr = binding.GetJointIndicesAttr()) {
            if (attr.HasAuthoredValue()) {
                _DeprecatedBindingCheck(hasBindingAPI, attr);
                key.jointIndicesAttr = std::move(attr);
            }
        }

        if (UsdAttribute attr = binding.GetJointWeightsAttr()) {
            if (attr.HasAuthoredValue()) {
                _DeprecatedBindingCheck(hasBindingAPI, attr);
                key.jointWeightsAttr = std::move(attr);
            }
        }
        
        if (UsdAttribute attr = binding.GetGeomBindTransformAttr()) {
            if (attr.HasAuthoredValue()) {
                _DeprecatedBindingCheck(hasBindingAPI, attr);
                key.geomBindTransformAttr = std::move(attr);
            }
        }

        if (UsdAttribute attr = binding.GetJointsAttr()) {
            if (attr.HasAuthoredValue()) {
                _DeprecatedBindingCheck(hasBindingAPI, attr);
                key.jointsAttr = std::move(attr);
            }
        }

        const bool isSkinnable = UsdSkelIsSkinnablePrim(*it);

        // Unlike other binding properties above, skel:blendShapes and
        // skel:blendShapeTargets are *not* inherited, so we only check
        // for them on skinnable prims.
        if (isSkinnable) {
            if (UsdAttribute attr = binding.GetBlendShapesAttr()) {
                if (attr.HasAuthoredValue()) {
                    _DeprecatedBindingCheck(hasBindingAPI, attr);
                    key.blendShapesAttr = std::move(attr);
                }
            }

            if (UsdRelationship rel =
                binding.GetBlendShapeTargetsRel()) {
                if (rel.HasAuthoredTargets()) {
                    _DeprecatedBindingCheck(hasBindingAPI, rel);
                    key.blendShapeTargetsRel = std::move(rel);
                }
            }
        }

        if (isSkinnable) {
            // Append a skinning query using the resolved binding properties.

            _PrimToSkinningQueryMap::accessor a;
            if (_cache->_primSkinningQueryCache.insert(a, *it)) {
                a->second = _FindOrCreateSkinningQuery(*it, key);
            }

            TF_DEBUG(USDSKEL_CACHE).Msg(
                "[UsdSkelCache] %sAdded skinning query for prim <%s>\n",
                _MakeIndent(stack.size()).c_str(),
                it->GetPath().GetText());

            // TODO: How should nested skinnable primitives be handled?
            // Should we prune traversal at this point?
        }

        stack.emplace_back(key, *it);
    }
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
