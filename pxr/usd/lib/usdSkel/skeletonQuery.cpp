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
    if(definition && animQuery) {
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


bool
UsdSkelSkeletonQuery::ComputeAnimTransform(GfMatrix4d* xform,
                                           UsdTimeCode time) const
{
    TRACE_FUNCTION();

    if(TF_VERIFY(IsValid(), "invalid skeleton query.")) {
        if(_animQuery) {
            return _animQuery.ComputeTransform(xform, time);
        }

        if(!xform) {
            TF_CODING_ERROR("'xform' pointer is null.");
            return false;
        }

        // No anim; fallback to using identity.
        xform->SetIdentity();
        return true;
    }
    return false;
}


bool
UsdSkelSkeletonQuery::ComputeJointLocalTransforms(VtMatrix4dArray* xforms,
                                                  UsdTimeCode time,
                                                  bool atRest) const
{
    TRACE_FUNCTION();

    if(!xforms) {
        TF_CODING_ERROR("'xforms' pointer is null.");
        return false;
    }

    if(TF_VERIFY(IsValid(), "invalid skeleton query.")) {
        atRest = atRest || !_HasMappableAnim();
        return _ComputeJointLocalTransforms(xforms, time, atRest);
    }
    return false;
}


bool
UsdSkelSkeletonQuery::_ComputeJointLocalTransforms(VtMatrix4dArray* xforms,
                                                   UsdTimeCode time,
                                                   bool atRest) const
{
    if(atRest) {
        *xforms = _definition->GetJointLocalRestTransforms();
        return true;
    }

    if(_animToSkelMapper.IsSparse()) {
        // Animation does not override all values;
        // Need to first fill in rest transforms.
        *xforms = _definition->GetJointLocalRestTransforms();
    }

    VtMatrix4dArray animXforms;
    if(_animQuery.ComputeJointLocalTransforms(&animXforms, time)) {
        return _animToSkelMapper.Remap(animXforms, xforms);
    }
    return false;
}


bool
UsdSkelSkeletonQuery::ComputeJointSkelTransforms(VtMatrix4dArray* xforms,
                                                 UsdTimeCode time,
                                                 bool atRest) const
{
    TRACE_FUNCTION();
    
    if(!xforms) {
        TF_CODING_ERROR("'xforms' pointer is null.");
        return false;
    }

    if(TF_VERIFY(IsValid(), "invalid skeleton query.")) {
        atRest = atRest || !_HasMappableAnim();
        return _ComputeJointSkelTransforms(xforms, time, atRest);
    }
    return false;
}


bool
UsdSkelSkeletonQuery::_ComputeJointSkelTransforms(VtMatrix4dArray* xforms,
                                                  UsdTimeCode time,
                                                  bool atRest) const
{
    if(atRest) {
        // This is cached on the definition.
        *xforms = _definition->GetJointSkelRestTransforms();
        return true;
    }

    VtMatrix4dArray localXforms;
    if(_ComputeJointLocalTransforms(&localXforms, time, atRest)) {
        const auto& topology = _definition->GetTopology();
        return UsdSkelConcatJointTransforms(topology, localXforms, xforms);
    }
    return false;
}


bool
UsdSkelSkeletonQuery::ComputeSkinningTransforms(VtMatrix4dArray* xforms,
                                                UsdTimeCode time) const
{
    TRACE_FUNCTION();

    if(!xforms) {
        TF_CODING_ERROR("'xforms' pointer is null.");
        return false;
    }

    if(TF_VERIFY(IsValid(), "invalid skeleton query.")) {
        return _ComputeSkinningTransforms(xforms, time);
    }
    return false;
}


namespace {

/// Compute xforms = premult*xforms.
void
_PreMultXforms(const VtMatrix4dArray& premult, VtMatrix4dArray* xforms)
{
    TRACE_FUNCTION();

    const GfMatrix4d* premultData = premult.cdata();
    GfMatrix4d* xformsData = xforms->data();
    for(size_t i = 0; i < xforms->size(); ++i) {
        xformsData[i] = premultData[i] * xformsData[i];
    }
}

} // namespace
        

bool
UsdSkelSkeletonQuery::_ComputeSkinningTransforms(VtMatrix4dArray* xforms,
                                                 UsdTimeCode time) const
{
    if(!_animQuery) {
        // All transforms would be identity.
        xforms->assign(_definition->GetTopology().GetNumJoints(),
                       GfMatrix4d(1));
        return true;
    }

    if(_ComputeJointSkelTransforms(xforms, time, /*rest*/ false)) {

        // XXX: Since this is a fairly frequent computation request,
        // skel-space inverse rest transforms are cached on-demand
        // on the definition

        const VtMatrix4dArray& restInverseXforms = 
            _definition->GetJointSkelInverseRestTransforms();

        if(xforms->size() == restInverseXforms.size()) {
            // xforms = restInverseXforms * xforms
            _PreMultXforms(restInverseXforms, xforms);
            return true;
        } else {
            TF_WARN("Xforms.size() [%zu] != restInverseXforms.size() [%zu].",
                    xforms->size(), restInverseXforms.size());
        }
    }
    return false;
}


const UsdSkelSkeleton&
UsdSkelSkeletonQuery::GetSkeleton() const
{
    if(TF_VERIFY(IsValid(), "invalid skeleton query.")) {
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
    if(TF_VERIFY(IsValid(), "invalid skeleton query.")) {
        return _definition->GetTopology();
    }
    static const UsdSkelTopology null;
    return null;
}


VtTokenArray
UsdSkelSkeletonQuery::GetJointOrder() const
{
    if(TF_VERIFY(IsValid(), "invalid skeleton query.")) {
        return _definition->GetJointOrder();
    }
    return VtTokenArray();
}


std::string
UsdSkelSkeletonQuery::GetDescription() const
{
    if(IsValid()) {
        return TfStringPrintf(
            "UsdSkelSkeletonQuery (skel = <%s>, anim = <%s>)",
            _definition->GetSkeleton().GetPrim().GetPath().GetText(),
            _animQuery.GetPrim().GetPath().GetText());
    }
    return "invalid UsdSkelSkeletonQuery";
}


PXR_NAMESPACE_CLOSE_SCOPE
