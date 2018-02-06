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
    :  _computeFlags(0)
{}


bool
UsdSkel_SkelDefinition::_Init(const UsdSkelSkeleton& skel)
{
    TRACE_FUNCTION();

    skel.GetJointsAttr().Get(&_jointOrder);
    skel.GetRestTransformsAttr().Get(&_jointLocalRestXforms);

    if(_jointLocalRestXforms.size() == _jointOrder.size()) {

        _topology = UsdSkelTopology(_jointOrder);

        std::string reason;
        if(_topology.Validate(&reason)) {
            _skel = skel;
            return true;
        } else {
            TF_WARN("%s -- invalid topology: %s",
                    skel.GetPrim().GetPath().GetText(), reason.c_str());
        }
    } else {
        TF_WARN("%s -- size of 'restTransforms' attr [%zu] does not "
                "match the number of joints in the 'joints' rel [%zu].",
                skel.GetPrim().GetPath().GetText(),
                _jointLocalRestXforms.size(), _jointOrder.size());
    }
    return false;
}


namespace {


enum _ComputeFlags {
    _SkelXformsComputed = 0x1,
    _SkelInverseXformsComputed = 0x2
};


} // namespace


const VtMatrix4dArray&
UsdSkel_SkelDefinition::GetJointLocalRestTransforms() const
{
    return _jointLocalRestXforms;
}


const VtMatrix4dArray&
UsdSkel_SkelDefinition::GetJointSkelRestTransforms()
{
    int flags = _computeFlags;
    if(ARCH_UNLIKELY(!(flags&_SkelXformsComputed))) {
        _ComputeJointSkelRestTransforms();
    }
    return _jointSkelRestXforms;
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

    _computeFlags = _computeFlags|_SkelXformsComputed;
}


const VtMatrix4dArray&
UsdSkel_SkelDefinition::GetJointSkelInverseRestTransforms()
{
    int flags = _computeFlags;
    if(ARCH_UNLIKELY(!(flags&_SkelInverseXformsComputed))) {
        _ComputeJointSkelInverseRestTransforms();
    }
    return _jointSkelInverseRestXforms;
}


void
UsdSkel_SkelDefinition::_ComputeJointSkelInverseRestTransforms()
{
    TRACE_FUNCTION();

    // XXX: querying skel-space xforms may require a lock.
    // Be careful not to lock early.
    VtMatrix4dArray xforms = GetJointSkelRestTransforms();

    std::lock_guard<std::mutex> lock(_mutex);
    for(auto& xf : xforms) {
        xf = xf.GetInverse();
    }
    _jointSkelInverseRestXforms = xforms;

    _computeFlags = _computeFlags|_SkelInverseXformsComputed;
}


PXR_NAMESPACE_CLOSE_SCOPE
