//
// Copyright 2020 benmalartre
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
#ifndef PXR_USD_PLUGIN_USD_ANIMX_CURVE_H
#define PXR_USD_PLUGIN_USD_ANIMX_CURVE_H

#include "pxr/pxr.h"
#include "pxr/usd/usdAnimX/api.h"
#include "pxr/usd/usdAnimX/animx.h"
#include "pxr/usd/usdAnimX/tokens.h"
#include "pxr/usd/usdAnimX/keyframe.h"
#include "pxr/usd/usdAnimX/desc.h"
#include <vector>
#include <set>

PXR_NAMESPACE_OPEN_SCOPE

class UsdAnimXCurve : public adsk::ICurve {
public:
    UsdAnimXCurve();
    UsdAnimXCurve(const UsdAnimXCurveDesc &desc);
    bool keyframe(double time, adsk::Keyframe &key) const override;
    bool keyframeAtIndex(int index, adsk::Keyframe &key) const override;
    bool first(adsk::Keyframe &key) const override;
    bool last(adsk::Keyframe &key) const override;
    adsk::InfinityType preInfinityType() const override;
    adsk::InfinityType postInfinityType() const override;
    bool isWeighted() const override;
    bool isStatic() const override;
    unsigned int keyframeCount() const override;
    bool isRotation() const;

    size_t findClosest(double time) const;
    void addKeyframe(const UsdAnimXKeyframe& key);
    void addKeyframe(double time, double value);
    void removeKeyframe(double time);
    void removeKeyframe(size_t index);
    void reindexKeys();

    std::set<double> computeSamples() const;
    double evaluate(double time) const;
    const std::string& getName() const;
    void setName(const std::string& name);
    ANIMX_API
    void setKeyframeAtIndex(size_t index, const adsk::Keyframe& key);

private:
    std::string                             _name;
    std::vector<UsdAnimXKeyframe>           _keyframes;
    adsk::CurveRotationInterpolationMethod  _rotationInterpolationMethod;
    adsk::InfinityType                      _preInfinityType;
    adsk::InfinityType                      _postInfinityType;
    bool                                    _weighted;
    bool                                    _static;
};

namespace { // anonymous namespace
static void _ResolveInfinityType(const TfToken &src, adsk::InfinityType *dst)
{
    if(src == UsdAnimXTokens->constant) {
        *dst = adsk::InfinityType::Constant;
    } else if(src == UsdAnimXTokens->cycle) {
        *dst = adsk::InfinityType::Cycle;
    } else if(src == UsdAnimXTokens->cycleRelative) {
        *dst = adsk::InfinityType::CycleRelative;
    } else if(src == UsdAnimXTokens->linear) {
        *dst = adsk::InfinityType::Linear;
    } else if(src == UsdAnimXTokens->oscillate) {
        *dst = adsk::InfinityType::Oscillate;
    }
}

static const TfToken& _ResolveInfinityType(const adsk::InfinityType& type)
{
    if(type == adsk::InfinityType::Constant) {
        return UsdAnimXTokens->constant;
    } else if(type == adsk::InfinityType::Cycle) {
        return UsdAnimXTokens->cycle;
    } else if(type == adsk::InfinityType::CycleRelative) {
        return UsdAnimXTokens->cycleRelative;
    } else if(type == adsk::InfinityType::Linear) {
        return UsdAnimXTokens->linear;
    } else if(type == adsk::InfinityType::Oscillate) {
        return UsdAnimXTokens->oscillate;
    }
}
} // end anonymous namespace

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PLUGIN_USD_ANIMX_CURVE_H