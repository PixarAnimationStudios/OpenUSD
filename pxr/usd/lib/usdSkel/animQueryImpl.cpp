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
#include "pxr/usd/usdSkel/animQueryImpl.h"

#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/usd/usdSkel/packedJointAnimation.h"
#include "pxr/usd/usdSkel/utils.h"


PXR_NAMESPACE_OPEN_SCOPE


// --------------------------------------------------
// UsdSkel_PackedJointAnimationQuery
// --------------------------------------------------


/// Animation query implementation for UsdSkelPackedJointAnimation primitives.
class UsdSkel_PackedJointAnimationQueryImpl : public UsdSkel_AnimQueryImpl
{
public:
    UsdSkel_PackedJointAnimationQueryImpl(
        const UsdSkelPackedJointAnimation& anim);

    virtual ~UsdSkel_PackedJointAnimationQueryImpl() {}

    virtual UsdPrim GetPrim() const override { return _anim.GetPrim(); }
    
    virtual bool ComputeJointLocalTransforms(VtMatrix4dArray* xforms,
                                             UsdTimeCode time) const override;

    virtual bool
    GetJointTransformTimeSamples(const GfInterval& interval,
                                 std::vector<double>* times) const override;

    virtual bool JointTransformsMightBeTimeVarying() const override;

    virtual bool
    ComputeRootTransform(GfMatrix4d* xform,
                         UsdGeomXformCache* xfCache) const override;


private:
    UsdSkelPackedJointAnimation _anim;
    UsdAttributeQuery _translations, _rotations, _scales;
};


UsdSkel_PackedJointAnimationQueryImpl::UsdSkel_PackedJointAnimationQueryImpl(
    const UsdSkelPackedJointAnimation& anim)
    : _anim(anim),
      _translations(anim.GetTranslationsAttr()),
      _rotations(anim.GetRotationsAttr()),
      _scales(anim.GetScalesAttr())
{
    if(TF_VERIFY(anim)) {
        anim.GetJointOrder(&_jointOrder);
    }
}


bool
UsdSkel_PackedJointAnimationQueryImpl::ComputeJointLocalTransforms(
    VtMatrix4dArray* xforms,
    UsdTimeCode time) const
{
    TRACE_FUNCTION();

    VtVec3fArray translations;
    VtQuatfArray rotations;
    VtVec3hArray scales;

    if(_translations.Get(&translations, time) &&
       _rotations.Get(&rotations, time) &&
       _scales.Get(&scales, time)) {

        if(UsdSkelMakeTransforms(translations, rotations, scales, xforms))
            return true;

        TF_WARN("%s -- failed composing transforms from components.",
                _anim.GetPrim().GetPath().GetText());
    }
    return false;
}


bool
UsdSkel_PackedJointAnimationQueryImpl::GetJointTransformTimeSamples(
    const GfInterval& interval,
    std::vector<double>* times) const
{
    return UsdAttribute::GetUnionedTimeSamplesInInterval(
        {_translations.GetAttribute(),
         _rotations.GetAttribute(),
         _scales.GetAttribute()}, interval, times);
}


bool
UsdSkel_PackedJointAnimationQueryImpl::JointTransformsMightBeTimeVarying() const
{
    return _translations.ValueMightBeTimeVarying() ||
           _rotations.ValueMightBeTimeVarying() ||
           _scales.ValueMightBeTimeVarying();
}


bool
UsdSkel_PackedJointAnimationQueryImpl::ComputeRootTransform(
    GfMatrix4d* xform,
    UsdGeomXformCache* xfCache) const
{
    if(TF_VERIFY(_anim, "PackedJointAnimation schema object is invalid.")) {
        if(!xform) {
            TF_CODING_ERROR("'xform' pointer is null.");
            return false;
        }
        if(!xfCache) {
            TF_CODING_ERROR("'xfCache' pointer is null.");
            return false;
        }

        *xform = xfCache->GetLocalToWorldTransform(_anim.GetPrim());
        return true;
    }
    return false;
}


// --------------------------------------------------
// UsdSkel_AnimQuery
// --------------------------------------------------


UsdSkel_AnimQueryImplRefPtr
UsdSkel_AnimQueryImpl::New(const UsdPrim& prim)
{
    if(prim.IsA<UsdSkelPackedJointAnimation>()) {
        return TfCreateRefPtr(
            new UsdSkel_PackedJointAnimationQueryImpl(
                UsdSkelPackedJointAnimation(prim)));
    }
    return nullptr;
}


bool
UsdSkel_AnimQueryImpl::IsAnimPrim(const UsdPrim& prim)
{
    return prim.IsA<UsdSkelPackedJointAnimation>();
}


PXR_NAMESPACE_CLOSE_SCOPE
