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
    // Matrix4dArray computations
    _SkelRestXforms4dComputed = 1 << 2,
    _WorldInverseBindXforms4dComputed = 1 << 3,
    _LocalInverseRestXforms4dComputed = 1 << 4,
    // Matrix4fArray computations
    _SkelRestXforms4fComputed = 1 << 5,
    _WorldInverseBindXforms4fComputed = 1 << 6,
    _LocalInverseRestXforms4fComputed = 1 << 7,
};


template <typename Matrix4>
void
_InvertTransforms(const VtArray<Matrix4>& xforms,
                  VtArray<Matrix4>* inverseXforms)
{
    inverseXforms->resize(xforms.size());
    Matrix4* dst = inverseXforms->data();
    for (size_t i = 0; i < xforms.size(); ++i) {
        dst[i] = xforms[i].GetInverse();
    }
}


void
_Convert4dXformsTo4f(const VtMatrix4dArray& matrix4dArray,
                     VtMatrix4fArray* matrix4fArray)
{
    matrix4fArray->resize(matrix4dArray.size());
    GfMatrix4f* dst = matrix4fArray->data();    
    for (size_t i = 0; i < matrix4dArray.size(); ++i) {
        dst[i] = GfMatrix4f(matrix4dArray[i]);
    }
}


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


template <>
VtMatrix4dArray&
UsdSkel_SkelDefinition::_XformHolder::Get<GfMatrix4d>()
{ return xforms4d; }

template <>
const VtMatrix4dArray&
UsdSkel_SkelDefinition::_XformHolder::Get<GfMatrix4d>() const
{ return xforms4d; }

template <>
VtMatrix4fArray&
UsdSkel_SkelDefinition::_XformHolder::Get<GfMatrix4f>()
{ return xforms4f; }

template <>
const VtMatrix4fArray&
UsdSkel_SkelDefinition::_XformHolder::Get<GfMatrix4f>() const
{ return xforms4f; }


template <>
bool
UsdSkel_SkelDefinition::GetJointLocalRestTransforms(VtMatrix4dArray* xforms)
{
    const int flags = _flags;
    if (flags&_HaveRestPose) {

        if (!xforms) {
            TF_CODING_ERROR("'xforms' pointer is null.");
            return false;
        }

        // double-precision rest xforms are pre-computed.
        *xforms = _jointLocalRestXforms;
        return true;
    }
    return false;
}


template <>
bool
UsdSkel_SkelDefinition::GetJointLocalRestTransforms(VtMatrix4fArray* xforms)
{
    if (!xforms) {
        TF_CODING_ERROR("'xforms' pointer is null.");
        return false;
    }

    // float-precision uses uncached conversion from double-precision.
    VtMatrix4dArray xforms4d;
    if (GetJointLocalRestTransforms(&xforms4d)) {
        _Convert4dXformsTo4f(xforms4d, xforms);
        return true;
    }
    return false;
}


template <int ComputeFlag, typename Matrix4>
bool
UsdSkel_SkelDefinition::_GetJointSkelRestTransforms(VtArray<Matrix4>* xforms)
{
    const int flags = _flags;
    if (flags&_HaveRestPose) {

        if (!xforms) {
            TF_CODING_ERROR("'xforms' pointer is null.");
            return false;
        }

        if (ARCH_UNLIKELY(!(flags&ComputeFlag))) {
            if (!_ComputeJointSkelRestTransforms<ComputeFlag,Matrix4>()) {
                return false;
            }
        }
        *xforms = _jointSkelRestXforms.Get<Matrix4>();
        return true;
    }
    return false;
}


template <>
bool
UsdSkel_SkelDefinition::GetJointSkelRestTransforms(VtMatrix4dArray* xforms)
{
    return _GetJointSkelRestTransforms<_SkelRestXforms4dComputed>(xforms);
}


template <>
bool
UsdSkel_SkelDefinition::GetJointSkelRestTransforms(VtMatrix4fArray* xforms)
{
    return _GetJointSkelRestTransforms<_SkelRestXforms4fComputed>(xforms);
}


template <int ComputeFlag, typename Matrix4>
bool
UsdSkel_SkelDefinition::_ComputeJointSkelRestTransforms()
{
    TRACE_FUNCTION();

    VtArray<Matrix4> jointLocalRestXforms;
    if (TF_VERIFY(GetJointLocalRestTransforms(&jointLocalRestXforms))) {

        std::lock_guard<std::mutex> lock(_mutex);
        
        VtArray<Matrix4>& skelXforms = _jointSkelRestXforms.Get<Matrix4>();
        skelXforms.resize(_topology.size());

        const bool success =
            UsdSkelConcatJointTransforms(_topology, jointLocalRestXforms,
                                         skelXforms);

        // XXX: Topology was validated when the definition was constructed,
        /// so this should not have failed.
        TF_VERIFY(success);

        _flags = _flags|ComputeFlag;

        return true;
    }
    return false;
}


template <>
bool
UsdSkel_SkelDefinition::GetJointWorldBindTransforms(VtMatrix4dArray* xforms)
{
    const int flags = _flags;
    if (flags&_HaveBindPose) {

        if (!xforms) {
            TF_CODING_ERROR("'xforms' pointer is null.");
            return false;
        }

        // double-precision bind xforms are pre-computed.
        *xforms = _jointWorldBindXforms;
        return true;
    }
    return false;
}


template <>
bool
UsdSkel_SkelDefinition::GetJointWorldBindTransforms(VtMatrix4fArray* xforms)
{
    if (!xforms) {
        TF_CODING_ERROR("'xforms' pointer is null.");
        return false;
    }

    // float-precision uses uncached conversion from double-precision.
    VtMatrix4dArray xforms4d;
    if (GetJointWorldBindTransforms(&xforms4d)) {
        _Convert4dXformsTo4f(xforms4d, xforms);
        return true;
    }
    return false;
}


template <int ComputeFlag, typename Matrix4>
bool
UsdSkel_SkelDefinition::_GetJointWorldInverseBindTransforms(
    VtArray<Matrix4>* xforms)
{
    const int flags = _flags;
    if (flags&_HaveBindPose) {

        if (!xforms) {
            TF_CODING_ERROR("'xforms' pointer is null.");
            return false;
        }

        if (ARCH_UNLIKELY(!(flags&ComputeFlag))) {
            if (!_ComputeJointWorldInverseBindTransforms<
                    ComputeFlag,Matrix4>()) {
                return false;
            }
        }
        *xforms = _jointWorldInverseBindXforms.Get<Matrix4>();
        return true;
    }
    return false;
}


template <>
bool
UsdSkel_SkelDefinition::GetJointWorldInverseBindTransforms(
    VtMatrix4dArray* xforms)
{
    return _GetJointWorldInverseBindTransforms<
        _WorldInverseBindXforms4dComputed>(xforms);
}


template <>
bool
UsdSkel_SkelDefinition::GetJointWorldInverseBindTransforms(
    VtMatrix4fArray* xforms)
{
    return _GetJointWorldInverseBindTransforms<
        _WorldInverseBindXforms4fComputed>(xforms);
}


template <int ComputeFlag, typename Matrix4>
bool
UsdSkel_SkelDefinition::_ComputeJointWorldInverseBindTransforms()
{
    TRACE_FUNCTION();

    VtArray<Matrix4> jointWorldBindXforms;
    if (TF_VERIFY(GetJointWorldBindTransforms(&jointWorldBindXforms))) {

        std::lock_guard<std::mutex> lock(_mutex);

        _InvertTransforms<Matrix4>(
            jointWorldBindXforms,
            &_jointWorldInverseBindXforms.Get<Matrix4>());

        _flags = _flags|ComputeFlag;
        return true;
    }
    return false;
}


template <int ComputeFlag, typename Matrix4>
bool
UsdSkel_SkelDefinition::_GetJointLocalInverseRestTransforms(
    VtArray<Matrix4>* xforms)
{
    const int flags = _flags;
    if (flags&_HaveRestPose) {

        if (!xforms) {
            TF_CODING_ERROR("'xforms' pointer is null.");
            return false;
        }

        if (ARCH_UNLIKELY(!(flags&ComputeFlag))) {
            if (!_ComputeJointLocalInverseRestTransforms<
                    ComputeFlag,Matrix4>()) {
                return false;
            }
        }
        *xforms = _jointLocalInverseRestXforms.Get<Matrix4>();
        return true;
    }
    return false;
}


template <>
bool
UsdSkel_SkelDefinition::GetJointLocalInverseRestTransforms(
    VtMatrix4dArray* xforms)
{
    return _GetJointLocalInverseRestTransforms<
        _LocalInverseRestXforms4dComputed>(xforms);
    return false;
}


template <>
bool
UsdSkel_SkelDefinition::GetJointLocalInverseRestTransforms(
    VtMatrix4fArray* xforms)
{
    return _GetJointLocalInverseRestTransforms<
        _LocalInverseRestXforms4fComputed>(xforms);
    return false;
}


template <int ComputeFlag, typename Matrix4>
bool
UsdSkel_SkelDefinition::_ComputeJointLocalInverseRestTransforms()
{
    TRACE_FUNCTION();

    VtArray<Matrix4> jointLocalRestXforms;
    if (TF_VERIFY(GetJointLocalRestTransforms(&jointLocalRestXforms))) {

        std::lock_guard<std::mutex> lock(_mutex);

        _InvertTransforms<Matrix4>(
            jointLocalRestXforms,
            &_jointLocalInverseRestXforms.Get<Matrix4>());

        _flags = _flags|ComputeFlag;

        return true;
    }
    return false;
}


PXR_NAMESPACE_CLOSE_SCOPE
