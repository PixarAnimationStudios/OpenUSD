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
#include "pxr/usd/usdSkel/skeletonQuery.h"

#include "pxr/base/tf/span.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdSkel/animQuery.h"
#include "pxr/usd/usdSkel/cacheImpl.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/topology.h"
#include "pxr/usd/usdSkel/utils.h"


PXR_NAMESPACE_OPEN_SCOPE


UsdSkelSkeletonQuery::UsdSkelSkeletonQuery(
    const UsdSkel_SkelDefinitionRefPtr& definition,
    const UsdSkelAnimQuery& animQuery)
    : _definition(definition), _animQuery(animQuery)
{
    if (definition && animQuery) {
        _animToSkelMapper = UsdSkelAnimMapper(animQuery.GetJointOrder(),
                                              definition->GetJointOrder());
    }
}


size_t
hash_value(const UsdSkelSkeletonQuery& query)
{
    size_t hash = hash_value(query._definition);
    boost::hash_combine(hash, query._animQuery);
    return hash;
}


bool
UsdSkelSkeletonQuery::_HasMappableAnim() const
{
    return _animQuery && !_animToSkelMapper.IsNull();
}


template <typename Matrix4>
bool
UsdSkelSkeletonQuery::ComputeJointLocalTransforms(VtArray<Matrix4>* xforms,
                                                  UsdTimeCode time,
                                                  bool atRest) const
{
    TRACE_FUNCTION();

    if (!xforms) {
        TF_CODING_ERROR("'xforms' pointer is null.");
        return false;
    }

    if (TF_VERIFY(IsValid(), "invalid skeleton query.")) {
        atRest = atRest || !_HasMappableAnim();
        return _ComputeJointLocalTransforms(xforms, time, atRest);
    }
    return false;
}


template USDSKEL_API bool
UsdSkelSkeletonQuery::ComputeJointLocalTransforms(VtArray<GfMatrix4d>*,
                                                  UsdTimeCode, bool) const;

template USDSKEL_API bool
UsdSkelSkeletonQuery::ComputeJointLocalTransforms(VtArray<GfMatrix4f>*,
                                                  UsdTimeCode, bool) const;


template <typename Matrix4>
bool
UsdSkelSkeletonQuery::_ComputeJointLocalTransforms(VtArray<Matrix4>* xforms,
                                                   UsdTimeCode time,
                                                   bool atRest) const
{
    if (atRest) {
        return _definition->GetJointLocalRestTransforms(xforms);
    }

    if (_animToSkelMapper.IsSparse()) {
        // Animation does not override all values;
        // Need to first fill in bind transforms.
        if (!_definition->GetJointLocalRestTransforms(xforms)) {
            TF_WARN("%s -- Failed computing local space transforms: "
                    "the the animation source (<%s>) is sparse, but the "
                    "'restTransforms' of the Skeleton are either unset, "
                    "or do not match the number of joints.",
                    GetSkeleton().GetPrim().GetPath().GetText(),
                    GetAnimQuery().GetPrim().GetPath().GetText());
            return false;
        }
    }

    VtArray<Matrix4> animXforms;
    if (_animQuery.ComputeJointLocalTransforms(&animXforms, time)) {
        return _animToSkelMapper.RemapTransforms(animXforms, xforms);
    } else {
        // Failed to compute anim xforms.
        // Fall back to our rest transforms.
        // These will have already been initialized above,
        // unless we have a non-sparse mapping.
        if (!_animToSkelMapper.IsSparse()) {
            return _definition->GetJointLocalRestTransforms(xforms);
        }
    }
    return true;
}


template <typename Matrix4>
bool
UsdSkelSkeletonQuery::ComputeJointSkelTransforms(VtArray<Matrix4>* xforms,
                                                 UsdTimeCode time,
                                                 bool atRest) const
{
    TRACE_FUNCTION();
    
    if (!xforms) {
        TF_CODING_ERROR("'xforms' pointer is null.");
        return false;
    }

    if (TF_VERIFY(IsValid(), "invalid skeleton query.")) {
        atRest = atRest || !_HasMappableAnim();
        return _ComputeJointSkelTransforms(xforms, time, atRest);
    }
    return false;
}


template USDSKEL_API bool
UsdSkelSkeletonQuery::ComputeJointSkelTransforms(VtArray<GfMatrix4d>*,
                                                 UsdTimeCode, bool) const;

template USDSKEL_API bool
UsdSkelSkeletonQuery::ComputeJointSkelTransforms(VtArray<GfMatrix4f>*,
                                                 UsdTimeCode, bool) const;


template <typename Matrix4>
bool
UsdSkelSkeletonQuery::_ComputeJointSkelTransforms(VtArray<Matrix4>* xforms,
                                                  UsdTimeCode time,
                                                  bool atRest) const
{
    if (atRest) {
        // This is cached on the definition.
        return _definition->GetJointSkelRestTransforms(xforms);
    }

    if (!xforms) {
        TF_CODING_ERROR("'xforms' is null");
        return false;
    }

    VtArray<Matrix4> localXforms;
    if (_ComputeJointLocalTransforms(&localXforms, time, atRest)) {
        const auto& topology = _definition->GetTopology();

        xforms->resize(topology.size());
        return UsdSkelConcatJointTransforms(topology, localXforms, *xforms);
    }
    return false;
}


namespace {

/// Compute `out = a * b`.
template <typename Matrix4>
void
_MultTransforms(TfSpan<const Matrix4> a,
                TfSpan<const Matrix4> b,
                TfSpan<Matrix4> out)
{
    TF_DEV_AXIOM(a.size() == b.size() && a.size() == out.size());

    for (ptrdiff_t i = 0; i < out.size(); ++i) {    
        out[i] = a[i] * b[i];
    }
}

} // namespace


template <typename Matrix4>
bool
UsdSkelSkeletonQuery::ComputeJointRestRelativeTransforms(
    VtArray<Matrix4>* xforms, UsdTimeCode time) const
{
    TRACE_FUNCTION();

    if (!xforms) {
        TF_CODING_ERROR("'xforms' pointer is null.");
        return false;
    }

    if (TF_VERIFY(IsValid(), "invalid skeleton query.")) {
        if (_HasMappableAnim()) {
            // jointLocalXf = restRelativeXf * restXf
            // restRelativeXf = jointLocalXf * inv(restXf)

            // Pull inverse rest transforms first
            // They are cached on the definition.
            VtArray<Matrix4> invRestXforms;
            if (_definition->GetJointLocalInverseRestTransforms(
                    &invRestXforms)) {

                VtArray<Matrix4> localXforms;
                if (_ComputeJointLocalTransforms(
                        &localXforms, time, /*atRest*/ false)) {
                    if (TF_VERIFY(localXforms.size() == invRestXforms.size())) {
                        
                        xforms->resize(localXforms.size());

                        _MultTransforms<Matrix4>(localXforms,
                                                 invRestXforms, *xforms);
                        return true;
                    }
                }
            } else {
                TF_WARN("%s -- Failed computing rest-relative transforms: "
                        "the 'restTransforms' of the Skeleton are either unset, "
                        "or do not have a matching number of joints.",
                        GetSkeleton().GetPrim().GetPath().GetText());
            }
        } else {
            // No bound animation, so rest relative transforms are identity.
            xforms->assign(GetTopology().size(), Matrix4(1));
            return true;
        }
    }
    return false;
}


template USDSKEL_API bool
UsdSkelSkeletonQuery::ComputeJointRestRelativeTransforms(
    VtArray<GfMatrix4d>*, UsdTimeCode) const;

template USDSKEL_API bool
UsdSkelSkeletonQuery::ComputeJointRestRelativeTransforms(
    VtArray<GfMatrix4f>*, UsdTimeCode) const;


template <typename Matrix4>
bool
UsdSkelSkeletonQuery::ComputeJointWorldTransforms(VtArray<Matrix4>* xforms,
                                                  UsdGeomXformCache* xfCache,
                                                  bool atRest) const
{
    TRACE_FUNCTION();

    if (!xforms) {
        TF_CODING_ERROR("'xforms' is null");
        return false;
    }

    if (!xfCache) {
        TF_CODING_ERROR("'xfCache' is null.");
        return false;
    }
    
    VtArray<Matrix4> localXforms;
    if (ComputeJointLocalTransforms(&localXforms, xfCache->GetTime(), atRest)) {
        const auto& topology = _definition->GetTopology();

        const Matrix4 rootXform(xfCache->GetLocalToWorldTransform(GetPrim()));

        xforms->resize(topology.size());
        return UsdSkelConcatJointTransforms(topology, localXforms,
                                            *xforms, &rootXform);
    }
    return false;
}


template USDSKEL_API bool
UsdSkelSkeletonQuery::ComputeJointWorldTransforms(
    VtArray<GfMatrix4d>*, UsdGeomXformCache*, bool) const;

template USDSKEL_API bool
UsdSkelSkeletonQuery::ComputeJointWorldTransforms(
    VtArray<GfMatrix4f>*, UsdGeomXformCache*, bool) const;


template <typename Matrix4>
bool
UsdSkelSkeletonQuery::ComputeSkinningTransforms(VtArray<Matrix4>* xforms,
                                                UsdTimeCode time) const
{
    TRACE_FUNCTION();

    if (!xforms) {
        TF_CODING_ERROR("'xforms' pointer is null.");
        return false;
    }

    if (TF_VERIFY(IsValid(), "invalid skeleton query.")) {
        return _ComputeSkinningTransforms(xforms, time);
    }
    return false;
}


template USDSKEL_API bool
UsdSkelSkeletonQuery::ComputeSkinningTransforms(VtArray<GfMatrix4d>*,
                                                UsdTimeCode) const;

template USDSKEL_API bool
UsdSkelSkeletonQuery::ComputeSkinningTransforms(VtArray<GfMatrix4f>*,
                                                UsdTimeCode) const;


template <typename Matrix4>
bool
UsdSkelSkeletonQuery::_ComputeSkinningTransforms(VtArray<Matrix4>* xforms,
                                                 UsdTimeCode time) const
{
    if (ComputeJointSkelTransforms(xforms, time)) {

        // XXX: Since this is a fairly frequent computation request,
        // skel-space inverse rest transforms are cached on-demand
        // on the definition
        
        VtArray<Matrix4> inverseBindXforms;
        if (!_definition->GetJointWorldInverseBindTransforms(
                &inverseBindXforms)) {
            TF_WARN("%s -- Failed fetching bind transforms. The "
                    "'bindTransforms' attribute may be unauthored, "
                    "or may not match the number of joints.",
                    GetSkeleton().GetPrim().GetPath().GetText());
            return false;
        }

        if (xforms->size() == inverseBindXforms.size()) {
            // xforms = inverseBindXforms * xforms
            _MultTransforms<Matrix4>(inverseBindXforms, *xforms, *xforms);
            return true;
        } else {
            TF_WARN("%s -- Size of computed joints transforms [%zu] does not "
                    "match the number of elements in the 'bindTransforms' "
                    "attr [%zu].", GetSkeleton().GetPrim().GetPath().GetText(),
                    xforms->size(), inverseBindXforms.size());
        }
    }
    return false;
}


template <typename Matrix4>
bool
UsdSkelSkeletonQuery::GetJointWorldBindTransforms(
    VtArray<Matrix4>* xforms) const
{
    if (TF_VERIFY(IsValid(), "invalid skeleton query.")) {
        return _definition->GetJointWorldBindTransforms(xforms);
    }
    return false;
}


template USDSKEL_API bool
UsdSkelSkeletonQuery::GetJointWorldBindTransforms(
    VtArray<GfMatrix4d>* xforms) const;

template USDSKEL_API bool
UsdSkelSkeletonQuery::GetJointWorldBindTransforms(
    VtArray<GfMatrix4f>* xforms) const;


UsdPrim
UsdSkelSkeletonQuery::GetPrim() const
{
    return GetSkeleton().GetPrim();
}


const UsdSkelSkeleton&
UsdSkelSkeletonQuery::GetSkeleton() const
{
    if (TF_VERIFY(IsValid(), "invalid skeleton query.")) {
        return _definition->GetSkeleton();
    }
    static UsdSkelSkeleton null;
    return null;
}


const UsdSkelAnimQuery&
UsdSkelSkeletonQuery::GetAnimQuery() const
{
    return _animQuery;
}


const UsdSkelTopology&
UsdSkelSkeletonQuery::GetTopology() const
{
    if (TF_VERIFY(IsValid(), "invalid skeleton query.")) {
        return _definition->GetTopology();
    }
    static const UsdSkelTopology null{};
    return null;
}


VtTokenArray
UsdSkelSkeletonQuery::GetJointOrder() const
{
    if (TF_VERIFY(IsValid(), "invalid skeleton query.")) {
        return _definition->GetJointOrder();
    }
    return VtTokenArray();
}


std::string
UsdSkelSkeletonQuery::GetDescription() const
{
    if (IsValid()) {
        return TfStringPrintf(
            "UsdSkelSkeletonQuery (skel = <%s>, anim = <%s>)",
            GetPrim().GetPath().GetText(),
            _animQuery.GetPrim().GetPath().GetText());
    }
    return "invalid UsdSkelSkeletonQuery";
}


PXR_NAMESPACE_CLOSE_SCOPE
