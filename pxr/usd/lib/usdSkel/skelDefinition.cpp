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
#include "pxr/usd/usdSkel/skelDefinition.h"

#include "pxr/base/arch/hints.h"

#include "pxr/usd/usdSkel/utils.h"


PXR_NAMESPACE_OPEN_SCOPE


namespace {


enum _Flags {
    _HaveBindPose = 1 << 0,
    _HaveRestPose = 1 << 1,
    _SkelRestXformsComputed = 1 << 2,
    _WorldInverseBindXformsComputed = 1 << 3,
    _LocalInverseRestXformsComputed = 1 << 4
};


} // namespace


UsdSkel_SkelDefinitionRefPtr
UsdSkel_SkelDefinition::New(const UsdSkelSkeleton& skel)
{
    if(skel) {
        UsdSkel_SkelDefinitionRefPtr def =
            TfCreateRefPtr(new UsdSkel_SkelDefinition);
        if(def->_Init(skel))
            return def;
    }
    return nullptr;
}


UsdSkel_SkelDefinition::UsdSkel_SkelDefinition()
    :  _flags(0)
{}


bool
UsdSkel_SkelDefinition::_Init(const UsdSkelSkeleton& skel)
{
    TRACE_FUNCTION();

    skel.GetJointsAttr().Get(&_jointOrder);

    _topology = UsdSkelTopology(_jointOrder);
    std::string reason;
    if (!_topology.Validate(&reason)) {
        TF_WARN("%s -- invalid topology: %s",
                skel.GetPrim().GetPath().GetText(), reason.c_str());
        return false;
    }

    skel.GetBindTransformsAttr().Get(&_jointWorldBindXforms);
    if (_jointWorldBindXforms.size() == _jointOrder.size()) {
        _flags = _flags|_HaveBindPose;
    } else {
        TF_WARN("%s -- size of 'bindTransforms' attr [%zu] does not "
                "match the number of joints in the 'joints' attr [%zu].",
                skel.GetPrim().GetPath().GetText(),
                _jointWorldBindXforms.size(), _jointOrder.size());
    }

    skel.GetRestTransformsAttr().Get(&_jointLocalRestXforms);
    if (_jointLocalRestXforms.size() == _jointOrder.size()) {
        _flags = _flags|_HaveRestPose;
    } else {
        TF_WARN("%s -- size of 'restTransforms' attr [%zu] does not "
                "match the number of joints in the 'joints' attr [%zu].",
                skel.GetPrim().GetPath().GetText(),
                _jointLocalRestXforms.size(), _jointOrder.size());
    }
    
    _skel = skel;
    return true;
}


bool
UsdSkel_SkelDefinition::GetJointLocalRestTransforms(VtMatrix4dArray* xforms)
{
    int flags = _flags;
    
    if (flags&_HaveRestPose) {

        if (!xforms) {
            TF_CODING_ERROR("'xforms' pointer is null.");
            return false;
        }

        *xforms = _jointLocalRestXforms;
        return true;
    }
    return false;
}


bool
UsdSkel_SkelDefinition::GetJointSkelRestTransforms(VtMatrix4dArray* xforms)
{
    int flags = _flags;
    if (flags&_HaveRestPose) {

        if (!xforms) {
            TF_CODING_ERROR("'xforms' pointer is null.");
            return false;
        }

        if (ARCH_UNLIKELY(!(flags&_SkelRestXformsComputed))) {

            _ComputeJointSkelRestTransforms();
        }
        *xforms = _jointSkelRestXforms;
        return true;
    }
    return false;
}


void
UsdSkel_SkelDefinition::_ComputeJointSkelRestTransforms()
{
    TRACE_FUNCTION();

    std::lock_guard<std::mutex> lock(_mutex);

    bool success =
        UsdSkelConcatJointTransforms(_topology, _jointLocalRestXforms,
                                     &_jointSkelRestXforms);
    // XXX: Topology was validated when the definition was constructed,
    /// so this should not have failed.
    TF_VERIFY(success);

    _flags = _flags|_SkelRestXformsComputed;
}


bool
UsdSkel_SkelDefinition::GetJointWorldBindTransforms(VtMatrix4dArray* xforms)
{
    int flags = _flags;
    
    if (flags&_HaveBindPose) {

        if (!xforms) {
            TF_CODING_ERROR("'xforms' pointer is null.");
            return false;
        }

        *xforms = _jointWorldBindXforms;
        return true;
    }
    return false;
}


bool
UsdSkel_SkelDefinition::GetJointWorldInverseBindTransforms(
    VtMatrix4dArray* xforms)
{
    int flags = _flags;
    if (flags&_HaveBindPose) {

        if (!xforms) {
            TF_CODING_ERROR("'xforms' pointer is null.");
            return false;
        }

        if (ARCH_UNLIKELY(!(flags&_WorldInverseBindXformsComputed))) {
            _ComputeJointWorldInverseBindTransforms();
        }
        *xforms = _jointWorldInverseBindXforms;
        return true;
    }
    return false;
}


void
UsdSkel_SkelDefinition::_ComputeJointWorldInverseBindTransforms()
{
    TRACE_FUNCTION();

    std::lock_guard<std::mutex> lock(_mutex);

    VtMatrix4dArray xforms(_jointWorldBindXforms);
    for(auto& xf : xforms) {
        xf = xf.GetInverse();
    }
    _jointWorldInverseBindXforms = xforms;

    _flags = _flags|_WorldInverseBindXformsComputed;
}


bool
UsdSkel_SkelDefinition::GetJointLocalInverseRestTransforms(
    VtMatrix4dArray* xforms)
{
    int flags = _flags;
    if (flags&_HaveRestPose) {
        if (!xforms) {
            TF_CODING_ERROR("'xforms' pointer is null.");
        }
        if (ARCH_UNLIKELY(!(flags&_LocalInverseRestXformsComputed))) {
            _ComputeJointLocalInverseRestTransforms();
        }
        *xforms = _jointLocalInverseRestXforms;
        return true;
    }
    return false;
}


void
UsdSkel_SkelDefinition::_ComputeJointLocalInverseRestTransforms()
{
    TRACE_FUNCTION();

    std::lock_guard<std::mutex> lock(_mutex);

    VtMatrix4dArray xforms(_jointLocalRestXforms);
    for (auto& xf : xforms) {
        xf = xf.GetInverse();
    }
    _jointLocalInverseRestXforms = xforms;

    _flags = _flags|_LocalInverseRestXformsComputed;
}


PXR_NAMESPACE_CLOSE_SCOPE
