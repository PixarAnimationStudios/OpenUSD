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
#include "pxr/pxr.h"
#include "usdKatana/attrMap.h"
#include "usdKatana/readGprim.h"
#include "usdKatana/readXformable.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usdGeom/curves.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/pointBased.h"

#include "pxr/base/gf/gamma.h"

#include "vtKatana/array.h"

#include <FnLogging/FnLogging.h>

PXR_NAMESPACE_OPEN_SCOPE


FnLogSetup("PxrUsdKatanaReadGprim");

void
PxrUsdKatanaReadGprim(
        const UsdGeomGprim& gprim,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    PxrUsdKatanaReadXformable(gprim, data, attrs);
}


FnKat::Attribute
PxrUsdKatanaGeomGetDisplayColorAttr(
        const UsdGeomGprim& gprim,
        const PxrUsdKatanaUsdInPrivateData& data)
{
    // Eval color.
    VtArray<GfVec3f> color;
    if (!gprim.GetDisplayColorPrimvar().ComputeFlattened(
        &color, data.GetCurrentTime())) {
        return FnKat::Attribute();
    }

    if (color.size() < 1) {
        FnLogWarn("Size 0 displaycolor from "<< gprim.GetPrim().GetName());
        return FnKat::Attribute();
    }

    // Build Katana attribute.
    // XXX(USD): what about alpha->opacity? warn?
    FnKat::FloatBuilder colorBuilder(3);
    std::vector<float> colorVec;
    colorVec.resize(3);
    colorVec[0] = color[0][0];
    colorVec[1] = color[0][1];
    colorVec[2] = color[0][2];
    colorBuilder.set(colorVec);

    FnKat::GroupBuilder groupBuilder;
    groupBuilder.set("inputType", FnKat::StringAttribute("color3"));
    groupBuilder.set("scope", FnKat::StringAttribute("primitive"));
    groupBuilder.set("value", colorBuilder.build());
    return groupBuilder.build();
}

Foundry::Katana::Attribute
PxrUsdKatanaGeomGetWindingOrderAttr(
        const UsdGeomGprim& gprim,
        const PxrUsdKatanaUsdInPrivateData& data)
{
    // XXX(USD) From sgg_tidScene/tidSceneContext.cpp:
    //
    // NOTE: this logic may seem reversed, in that "leftHanded"
    // orientation would normally be clockwise. However, something
    // in Katana is backward, in that by default they apply a -1 scale
    // to Z for their lights, which is behavior assumed in their light
    // shaders. We disable this behavior, because our light shaders
    // don't expect that. This leads to a confusion of terminology
    // between what right vs. left, clockwise vs. counter-clockwise
    // means. This only affects the GL viewer, not render output.
    //
    TfToken orientation = UsdGeomTokens->rightHanded;
    gprim.GetOrientationAttr().Get(&orientation);

    if (orientation == UsdGeomTokens->leftHanded)
    {
        return FnKat::StringAttribute("counterclockwise");
    }
    else
    {
        return FnKat::StringAttribute("clockwise");
    }
}

namespace {

template <typename T_USD, typename T_ATTR> FnKat::Attribute
_ConvertGeomAttr(
    const UsdAttribute& usdAttr,
    const int tupleSize,
    const PxrUsdKatanaUsdInPrivateData& data)
{
    if (!usdAttr.HasValue())
    {
        return FnKat::Attribute();
    }

    const double currentTime = data.GetCurrentTime();
    const std::vector<double>& motionSampleTimes = data.GetMotionSampleTimes(usdAttr);

    // Flag to check if we discovered the topology is varying, in
    // which case we only output the sample at the curent frame.
    bool varyingTopology = false;

    const bool isMotionBackward = data.IsMotionBackward();

    std::vector<float> times;
    std::vector<VtArray<T_USD>> values;
    for (double relSampleTime : motionSampleTimes){
        double time = currentTime + relSampleTime;

        // Eval attr.
        VtArray<T_USD> attrArray;
        usdAttr.Get(&attrArray, time);
        
        if (!values.empty()){
            if (values.front().size() != attrArray.size()){
                times.clear();
                values.clear();
                varyingTopology = true;
                break;
	    }
        }
        times.push_back(isMotionBackward ? PxrUsdKatanaUtils::ReverseTimeSample(relSampleTime) : relSampleTime);
        values.push_back(attrArray);
    }

    // Varying topology was found, build for the current frame only.
    if (varyingTopology){
        VtArray<T_USD> attrArray;
        usdAttr.Get(&attrArray, currentTime);
        return VtKatanaMapOrCopy<T_USD>(attrArray);
    }
    else{
        if (isMotionBackward){
            std::reverse(times.begin(), times.end());
            std::reverse(values.begin(), values.end());
        }
        return VtKatanaMapOrCopy<T_USD>(times, values);
    }
}

} // anon namespace

FnKat::Attribute
PxrUsdKatanaGeomGetPAttr(
    const UsdGeomPointBased& points,
    const PxrUsdKatanaUsdInPrivateData& data)
{
    return _ConvertGeomAttr<GfVec3f, FnKat::FloatAttribute>(
            points.GetPointsAttr(), 3, data);
}

Foundry::Katana::Attribute
PxrUsdKatanaGeomGetNormalAttr(
    const UsdGeomPointBased& points,
    const PxrUsdKatanaUsdInPrivateData& data)
{
    return _ConvertGeomAttr<GfVec3f, FnKat::FloatAttribute>(
            points.GetNormalsAttr(), 3, data);
}

Foundry::Katana::Attribute
PxrUsdKatanaGeomGetVelocityAttr(
    const UsdGeomPointBased& points,
    const PxrUsdKatanaUsdInPrivateData& data)
{
    return _ConvertGeomAttr<GfVec3f, FnKat::FloatAttribute>(
            points.GetVelocitiesAttr(), 3, data);

}

PXR_NAMESPACE_CLOSE_SCOPE

