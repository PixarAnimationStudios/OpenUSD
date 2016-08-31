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
#include "usdKatana/attrMap.h"
#include "usdKatana/readGprim.h"
#include "usdKatana/readXformable.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usdGeom/curves.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/pointBased.h"

#include "pxr/base/gf/gamma.h"

#include <FnLogging/FnLogging.h>

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
PxrUsdKatanaGeomGetPAttr(
    const UsdGeomPointBased& points,
    const PxrUsdKatanaUsdInPrivateData& data)
{
    UsdAttribute pointsAttr = points.GetPointsAttr();
    if (not pointsAttr)
    {
        return FnKat::Attribute();
    }

    const double currentTime = data.GetUsdInArgs()->GetCurrentTimeD();
    const std::vector<double>& motionSampleTimes = data.GetMotionSampleTimes(pointsAttr);

    // Flag to check if we discovered the topology is varying, in
    // which case we only output the sample at the curent frame.
    bool varyingTopology = false;

    // Used to compare value sizes to identify varying topology.
    int arraySize = -1;

    const bool isMotionBackward = data.GetUsdInArgs()->IsMotionBackward();

    FnKat::FloatBuilder attrBuilder(/* tupleSize = */ 3);
    TF_FOR_ALL(iter, motionSampleTimes)
    {
        double relSampleTime = *iter;
        double time = currentTime + relSampleTime;

        // Eval points.
        VtVec3fArray ptArray;
        pointsAttr.Get(&ptArray, time);

        if (arraySize == -1) {
            arraySize = ptArray.size();
        } else if ( ptArray.size() != static_cast<size_t>(arraySize) ) {
            // Topology has changed. Don't create this or subsequent samples.
            varyingTopology = true;
            break;
        }

        std::vector<float> &ptVec = attrBuilder.get(isMotionBackward ?
            PxrUsdKatanaUtils::ReverseTimeSample(relSampleTime) : relSampleTime);

        PxrUsdKatanaUtils::ConvertArrayToVector(ptArray, &ptVec);
    }

    // Varying topology was found, build for the current frame only.
    if (varyingTopology)
    {
        FnKat::FloatBuilder defaultBuilder(/* tupleSize = */ 3);
        VtVec3fArray ptArray;

        pointsAttr.Get(&ptArray, currentTime);
        std::vector<float> &ptVec = defaultBuilder.get(0);
        PxrUsdKatanaUtils::ConvertArrayToVector(ptArray, &ptVec);
        
        return defaultBuilder.build();
    }

    return attrBuilder.build();
}

FnKat::Attribute
PxrUsdKatanaGeomGetDisplayColorAttr(
        const UsdGeomGprim& gprim,
        const PxrUsdKatanaUsdInPrivateData& data)
{
    // Eval color.
    VtArray<GfVec3f> color;
    if (not gprim.GetDisplayColorPrimvar().ComputeFlattened(
            &color, data.GetUsdInArgs()->GetCurrentTimeD())) {
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

FnKat::Attribute
PxrUsdKatanaGeomGetPrimvarGroup(
        const UsdGeomGprim& gprim,
        const PxrUsdKatanaUsdInPrivateData& data)
{
    // Usd primvars -> Primvar attributes
    FnKat::GroupBuilder gdBuilder;

    std::vector<UsdGeomPrimvar> primvarAttrs = gprim.GetPrimvars();
    TF_FOR_ALL(primvar, primvarAttrs) {
        TfToken          name, interpolation;
        SdfValueTypeName typeName;
        int              elementSize;

        primvar->GetDeclarationInfo(&name, &typeName, 
                                    &interpolation, &elementSize);

        // Name: this will eventually want to be GetBaseName() to strip
        // off prefixes
        std::string gdName = name;

        // Convert interpolation -> scope
        FnKat::StringAttribute scopeAttr;
        const bool isCurve = gprim.GetPrim().IsA<UsdGeomCurves>();
        if (isCurve && interpolation == UsdGeomTokens->vertex) {
            // it's a curve, so "vertex" == "vertex"
            scopeAttr = FnKat::StringAttribute("vertex");
        } else {
            scopeAttr = FnKat::StringAttribute(
                (interpolation == UsdGeomTokens->faceVarying)? "vertex" :
                (interpolation == UsdGeomTokens->varying)    ? "point" :
                (interpolation == UsdGeomTokens->vertex)     ? "point" /*see below*/ :
                (interpolation == UsdGeomTokens->uniform)    ? "face" :
                "primitive" );
        }

        // Resolve the value
        VtValue vtValue;
        if (not primvar->ComputeFlattened(
                &vtValue, data.GetUsdInArgs()->GetCurrentTimeD()))
        {
            continue;
        }

        // Convert value to the required Katana attributes to describe it.
        FnKat::Attribute valueAttr, inputTypeAttr, elementSizeAttr;
        PxrUsdKatanaUtils::ConvertVtValueToKatCustomGeomAttr(
            vtValue, elementSize, typeName.GetRole(),
            &valueAttr, &inputTypeAttr, &elementSizeAttr);

        // Bundle them into a group attribute
        FnKat::GroupBuilder attrBuilder;
        attrBuilder.set("scope", scopeAttr);
        attrBuilder.set("inputType", inputTypeAttr);
        if (elementSizeAttr.isValid()) {
            attrBuilder.set("elementSize", elementSizeAttr);
        }
        attrBuilder.set("value", valueAttr);
        // Note that 'varying' vs 'vertex' require special handling, as in
        // Katana they are both expressed as 'point' scope above. To get
        // 'vertex' interpolation we must set an additional
        // 'interpolationType' attribute.  So we will flag that here.
        const bool vertexInterpolationType = 
            (interpolation == UsdGeomTokens->vertex);
        if (vertexInterpolationType) {
            attrBuilder.set("interpolationType",
                FnKat::StringAttribute("subdiv"));
        }
        gdBuilder.set(gdName, attrBuilder.build());
    }

    return gdBuilder.build();
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

Foundry::Katana::Attribute
PxrUsdKatanaGeomGetNormalAttr(
    const UsdGeomPointBased& points,
    const PxrUsdKatanaUsdInPrivateData& data)
{
    UsdAttribute normalsAttr = points.GetNormalsAttr();
    if (not normalsAttr)
    {
        return FnKat::Attribute();
    }

    const double currentTime = data.GetUsdInArgs()->GetCurrentTimeD();
    const std::vector<double>& motionSampleTimes =
            data.GetMotionSampleTimes(normalsAttr);

    const bool isMotionBackward = data.GetUsdInArgs()->IsMotionBackward();

    FnKat::FloatBuilder attrBuilder(/* tupleSize = */ 3);
    TF_FOR_ALL(iter, motionSampleTimes)
    {
        double relSampleTime = *iter;
        double time = currentTime + relSampleTime;

        // Retrieve normals at time.
        VtVec3fArray normalsArray;
        normalsAttr.Get(&normalsArray, time);

        std::vector<float> &normalsVec = attrBuilder.get(isMotionBackward ?
            PxrUsdKatanaUtils::ReverseTimeSample(relSampleTime) : relSampleTime);

        PxrUsdKatanaUtils::ConvertArrayToVector(normalsArray, &normalsVec);
    }

    return attrBuilder.build();
}
