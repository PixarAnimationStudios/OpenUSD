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

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/attribute.h"

#include "pxr/usd/usdGeom/xformable.h"

#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/utils.h"


PXR_NAMESPACE_OPEN_SCOPE


// --------------------------------------------------
// UsdSkel_SkelAnimationQueryImpl
// --------------------------------------------------


/// Animation query implementation for UsdSkelAnimation primitives.
class UsdSkel_SkelAnimationQueryImpl : public UsdSkel_AnimQueryImpl
{
public:
    UsdSkel_SkelAnimationQueryImpl(const UsdSkelAnimation& anim);

    virtual ~UsdSkel_SkelAnimationQueryImpl() {}

    virtual UsdPrim GetPrim() const override { return _anim.GetPrim(); }
    
    bool ComputeJointLocalTransforms(VtMatrix4dArray* xforms,
                                     UsdTimeCode time) const override
         { return _ComputeJointLocalTransforms(xforms, time); }

    bool ComputeJointLocalTransforms(VtMatrix4fArray* xforms,
                                     UsdTimeCode time) const override
         { return _ComputeJointLocalTransforms(xforms, time); }

    bool ComputeJointLocalTransformComponents(
             VtVec3fArray* translations,
             VtQuatfArray* rotations,
             VtVec3hArray* scales,
             UsdTimeCode time) const override;

    bool ComputeBlendShapeWeights(VtFloatArray* weights,
                                  UsdTimeCode time) const override;

    bool GetJointTransformTimeSamples(
             const GfInterval& interval,
             std::vector<double>* times) const override;

    bool GetJointTransformAttributes(
             std::vector<UsdAttribute>* attrs) const override;
    
    bool JointTransformsMightBeTimeVarying() const override;

    bool GetBlendShapeWeightTimeSamples(
             const GfInterval& interval,
             std::vector<double>* times) const override;

    bool BlendShapeWeightsMightBeTimeVarying() const override;

private:
    template <typename Matrix4>
    bool _ComputeJointLocalTransforms(VtArray<Matrix4>* xforms,
                                      UsdTimeCode time) const;

private:
    UsdSkelAnimation _anim;
    UsdAttributeQuery _translations, _rotations, _scales, _blendShapeWeights;
};


UsdSkel_SkelAnimationQueryImpl::UsdSkel_SkelAnimationQueryImpl(
    const UsdSkelAnimation& anim)
    : _anim(anim),
      _translations(anim.GetTranslationsAttr()),
      _rotations(anim.GetRotationsAttr()),
      _scales(anim.GetScalesAttr()),
      _blendShapeWeights(anim.GetBlendShapeWeightsAttr())
{
    if (TF_VERIFY(anim)) {
        anim.GetJointsAttr().Get(&_jointOrder);
        anim.GetBlendShapesAttr().Get(&_blendShapeOrder);
    }
}


template <typename Matrix4>
bool
UsdSkel_SkelAnimationQueryImpl::_ComputeJointLocalTransforms(
    VtArray<Matrix4>* xforms,
    UsdTimeCode time) const
{
    TRACE_FUNCTION();

    if (!xforms) {
        TF_CODING_ERROR("'xforms' is null");
        return false;
    }

    VtVec3fArray translations;
    VtQuatfArray rotations;
    VtVec3hArray scales;

    if (ComputeJointLocalTransformComponents(&translations, &rotations,
                                            &scales, time)) {

        xforms->resize(translations.size());
        if (UsdSkelMakeTransforms(translations, rotations,
                                  scales, *xforms)) {

            if (xforms->size() == _jointOrder.size()) {
                return true;
            } else if (xforms->empty()) {
                // If all transform components were empty, that could mean:
                // - the attributes were never authored
                // - the attributes were blocked
                // - the attributes were authored with empty arrays
                //   (possibly intentionally)
                    
                // In many of these cases, we should expect the animation
                // to be silently ignored, so throw no warning.
                return false;
            }
            TF_WARN("%s -- size of transform component arrays [%zu] "
                    "!= joint order size [%zu].",
                    _anim.GetPrim().GetPath().GetText(),
                    xforms->size(), _jointOrder.size());
        } else {
            TF_WARN("%s -- failed composing transforms from components.",
                    _anim.GetPrim().GetPath().GetText());
        }
    }
    return false;
}


bool
UsdSkel_SkelAnimationQueryImpl::ComputeJointLocalTransformComponents(
    VtVec3fArray* translations,
    VtQuatfArray* rotations,
    VtVec3hArray* scales,
    UsdTimeCode time) const
{
    TRACE_FUNCTION();

    return _translations.Get(translations, time) &&
           _rotations.Get(rotations, time) &&
           _scales.Get(scales, time);
}


bool
UsdSkel_SkelAnimationQueryImpl::GetJointTransformTimeSamples(
    const GfInterval& interval,
    std::vector<double>* times) const
{
    return UsdAttribute::GetUnionedTimeSamplesInInterval(
        {_translations.GetAttribute(),
         _rotations.GetAttribute(),
         _scales.GetAttribute()}, interval, times);
}

bool
UsdSkel_SkelAnimationQueryImpl::GetJointTransformAttributes(
    std::vector<UsdAttribute>* attrs) const
{
    attrs->push_back(_translations.GetAttribute());
    attrs->push_back(_rotations.GetAttribute());
    attrs->push_back(_scales.GetAttribute());
    return true;
}


bool
UsdSkel_SkelAnimationQueryImpl::JointTransformsMightBeTimeVarying() const
{
    return _translations.ValueMightBeTimeVarying() ||
           _rotations.ValueMightBeTimeVarying() ||
           _scales.ValueMightBeTimeVarying();
}


bool
UsdSkel_SkelAnimationQueryImpl::ComputeBlendShapeWeights(
    VtFloatArray* weights,
    UsdTimeCode time) const
{
    if (TF_VERIFY(_anim, "PackedJointAnimation schema object is invalid.")) {
        return _blendShapeWeights.Get(weights, time);
    }
    return false;
}


bool
UsdSkel_SkelAnimationQueryImpl::GetBlendShapeWeightTimeSamples(
    const GfInterval& interval,
    std::vector<double>* times) const
{
    return _blendShapeWeights.GetTimeSamplesInInterval(interval, times);
}


bool
UsdSkel_SkelAnimationQueryImpl::BlendShapeWeightsMightBeTimeVarying() const
{
    return _blendShapeWeights.ValueMightBeTimeVarying();
}


// --------------------------------------------------
// UsdSkel_AnimQueryImpl
// --------------------------------------------------


UsdSkel_AnimQueryImplRefPtr
UsdSkel_AnimQueryImpl::New(const UsdPrim& prim)
{
    if (prim.IsA<UsdSkelAnimation>()) {
        return TfCreateRefPtr(new UsdSkel_SkelAnimationQueryImpl(
                                  UsdSkelAnimation(prim)));
    }
    return nullptr;
}


PXR_NAMESPACE_CLOSE_SCOPE
