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
#include "usdKatana/readNurbsPatch.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usdGeom/nurbsPatch.h"

#include <FnLogging/FnLogging.h>

FnLogSetup("PxrUsdKatanaReadNurbsPatch");

static FnKat::IntAttribute
_GetUSizeAttr(
    const UsdGeomNurbsPatch &nurbsPatch,
    double currentTime)
{
    // (USD) UVertexCount --> (Katana) uSize
    int uVertexCount;
    nurbsPatch.GetUVertexCountAttr().Get(&uVertexCount, currentTime);
    return FnKat::IntAttribute(uVertexCount);
}

static FnKat::IntAttribute
_GetVSizeAttr(
    const UsdGeomNurbsPatch &nurbsPatch,
    double currentTime)
{
    // (USD) VVertexCount --> (Katana) vSize
    int vVertexCount;
    nurbsPatch.GetVVertexCountAttr().Get(&vVertexCount, currentTime);
    return FnKat::IntAttribute(vVertexCount);
}

/*
 * Convert the USD "form" attribute to int that Katana
 * understands for represening the nurbs patch's form.
 */
static int
_FormTokenToInt(TfToken &form)
{
    if (form == UsdGeomTokens->open) {
        return 0;
    }
    else if (form == UsdGeomTokens->closed) {
        return 1;
    }
    else if (form == UsdGeomTokens->periodic) {
        return 2;
    }
    
    return 0;
}

static FnKat::IntAttribute
_GetUClosedAttr(
    const UsdGeomNurbsPatch &nurbsPatch,
    double currentTime)
{
    // USD UForm --> (Katana) uClosed
    TfToken uForm;
    nurbsPatch.GetUFormAttr().Get(&uForm, currentTime);
    return FnKat::IntAttribute(_FormTokenToInt(uForm));
}

static FnKat::IntAttribute
_GetVClosedAttr(
    const UsdGeomNurbsPatch &nurbsPatch,
    double currentTime)
{
    // USD VForm --> (Katana) vClosed
    TfToken vForm;
    nurbsPatch.GetVFormAttr().Get(&vForm, currentTime);
    return FnKat::IntAttribute(_FormTokenToInt(vForm));
}

/*
 * Return a FloatAttribute for points. There are 4 floats per
 * point, where the first 3 floats are the point's position,
 * and the 4th float is the weight of the point.
 */
static FnKat::FloatAttribute
_GetPwAttr(
    const UsdGeomNurbsPatch &nurbsPatch,
    double currentTime,
    const std::vector<double>& motionSampleTimes,
    const bool isMotionBackward)
{
    UsdAttribute weightsAttr = nurbsPatch.GetPointWeightsAttr();
    UsdAttribute pointsAttr = nurbsPatch.GetPointsAttr();

    if (not pointsAttr)
    {
        return FnKat::FloatAttribute();
    }

    FnKat::FloatBuilder pwBuilder(/* tupleSize = */ 4);
    TF_FOR_ALL(iter, motionSampleTimes)
    {
        double relSampleTime = *iter;
        double time = currentTime + relSampleTime;

        // Eval points at this motion sample time
        VtVec3fArray ptArray;
        pointsAttr.Get(&ptArray, time);
        // Eval Weights at this motion sample time
        VtDoubleArray wtArray;
        weightsAttr.Get(&wtArray, currentTime);
        
        bool hasWeights = false;
        if (ptArray.size() == wtArray.size())
        {
            hasWeights = true;
        }
        else if (wtArray.size() > 0)
        {
            FnLogWarn("Nurbs Patch " 
                    << nurbsPatch.GetPath().GetText()
                    << " has mismatched weights array. Skipping.");
            return FnKat::FloatAttribute();
        }

        // set the points data in katana at the give motion sample time
        std::vector<float> &ptVec = pwBuilder.get(isMotionBackward ?
            PxrUsdKatanaUtils::ReverseTimeSample(relSampleTime) : relSampleTime);

        ptVec.resize(ptArray.size() * 4);

        size_t count = 0;
        for (size_t i = 0; i != ptArray.size(); ++i)
        {
            float weight = hasWeights ? wtArray[i] : 1.0f;
            ptVec[count++] = ptArray[i][0]*weight;
            ptVec[count++] = ptArray[i][1]*weight;
            ptVec[count++] = ptArray[i][2]*weight;
            // the 4th float is the weight of the point
            ptVec[count++] = weight ;
        }
    }

    return pwBuilder.build();
}

/*
 * Build the "geometry.u" or "geometry.v" group attributes.
 */
static FnKat::GroupAttribute
_BuildUOrVAttr(int order, GfVec2d& range, VtDoubleArray& knots)
{
    FnKat::GroupBuilder gb;

    gb.set("order", FnKat::IntAttribute(order));
    gb.set("min", FnKat::FloatAttribute(range[0]));
    gb.set("max", FnKat::FloatAttribute(range[1]));

    FnKat::FloatBuilder knotsBuilder(/* tuplesize */ 1);
    knotsBuilder.set(std::vector<float>(knots.begin(), knots.end()));
    gb.set("knots", knotsBuilder.build());

    return gb.build();
}

static FnKat::GroupAttribute
_GetUAttr(
    const UsdGeomNurbsPatch &nurbsPatch,
    double currentTime)
{
    int uOrder;
    nurbsPatch.GetUOrderAttr().Get(&uOrder, currentTime);

    GfVec2d uRange;
    nurbsPatch.GetURangeAttr().Get(&uRange, currentTime);

    VtDoubleArray uKnots;
    nurbsPatch.GetUKnotsAttr().Get(&uKnots, currentTime);

    return _BuildUOrVAttr(uOrder, uRange, uKnots);
}

static FnKat::GroupAttribute
_GetVAttr(
    const UsdGeomNurbsPatch &nurbsPatch,
    double currentTime)
{
    int vOrder;
    nurbsPatch.GetVOrderAttr().Get(&vOrder, currentTime);

    GfVec2d vRange;
    nurbsPatch.GetVRangeAttr().Get(&vRange, currentTime);

    VtDoubleArray vKnots;
    nurbsPatch.GetVKnotsAttr().Get(&vKnots, currentTime);

    return _BuildUOrVAttr(vOrder, vRange, vKnots);
}

static FnKat::GroupAttribute
_GetTrimCurvesAttr(
    const UsdGeomNurbsPatch &nurbsPatch,
    double currentTime)
{
    FnKat::GroupBuilder trimBuilder;

    // (USD) TrimCurveCounts --> (Katana) trim_ncurves
    VtIntArray curveCounts;
    nurbsPatch.GetTrimCurveCountsAttr().Get(&curveCounts, currentTime);
    FnKat::IntBuilder nCurvesBuilder;
    std::vector<int> nCurves(curveCounts.begin(), curveCounts.end());
    nCurvesBuilder.set(nCurves);
    trimBuilder.set("trim_ncurves", nCurvesBuilder.build());

    // (USD) TrimCurveOrder --> (Katana) trim_order
    VtIntArray curveOrders;
    nurbsPatch.GetTrimCurveOrdersAttr().Get(&curveOrders, currentTime);
    FnKat::IntBuilder orderBuilder;
    std::vector<int> order(curveOrders.begin(), curveOrders.end());
    orderBuilder.set(order);
    trimBuilder.set("trim_order", orderBuilder.build());

    // (USD) TrimCurveVertexCounts --> (Katana) trim_n
    VtIntArray vertexCounts;
    nurbsPatch.GetTrimCurveVertexCountsAttr().Get(&vertexCounts, currentTime);
    FnKat::IntBuilder nBuilder;
    std::vector<int> n(vertexCounts.begin(), vertexCounts.end());
    nBuilder.set(n);
    trimBuilder.set("trim_n", nBuilder.build());

    // (USD) TrimCurveRanges --> (Katana) trim_min and trim_max
    VtVec2dArray curveRanges;
    nurbsPatch.GetTrimCurveRangesAttr().Get(&curveRanges, currentTime);
    FnKat::FloatBuilder minBuilder;
    FnKat::FloatBuilder maxBuilder;
    std::vector<float> min(curveRanges.size());
    std::vector<float> max(curveRanges.size());
    for (size_t i = 0; i < curveRanges.size(); ++i) 
    {
        min[i] = curveRanges[i][0];
        max[i] = curveRanges[i][1];
    }
    minBuilder.set(min);
    maxBuilder.set(max);
    trimBuilder.set("trim_min", minBuilder.build());
    trimBuilder.set("trim_max", maxBuilder.build());

    // (USD) TrimCurveKnots --> (Katana) trim_knot
    VtDoubleArray curveKnots;
    nurbsPatch.GetTrimCurveKnotsAttr().Get(&curveKnots, currentTime);
    FnKat::FloatBuilder knotBuilder;
    std::vector<float> knot(curveKnots.begin(), curveKnots.end());
    knotBuilder.set(knot);
    trimBuilder.set("trim_knot", knotBuilder.build());

    // (USD) TrimCurveVertexpoints --> (Katana) trim_u, trim_v, and trim_w
    VtVec3dArray curvePoints;
    nurbsPatch.GetTrimCurvePointsAttr().Get(&curvePoints, currentTime);
    FnKat::FloatBuilder uBuilder;
    FnKat::FloatBuilder vBuilder;
    FnKat::FloatBuilder wBuilder;
    std::vector<float> u(curvePoints.size());
    std::vector<float> v(curvePoints.size());
    std::vector<float> w(curvePoints.size());
    for (size_t i = 0; i < curvePoints.size(); ++i)
    {
        u[i] = curvePoints[i][0];
        v[i] = curvePoints[i][1];
        w[i] = curvePoints[i][2];
    }
    uBuilder.set(u);
    vBuilder.set(v);
    wBuilder.set(w);
    trimBuilder.set("trim_u", uBuilder.build());
    trimBuilder.set("trim_v", vBuilder.build());
    trimBuilder.set("trim_w", wBuilder.build());
    
    return trimBuilder.build();
}

void
PxrUsdKatanaReadNurbsPatch(
        const UsdGeomNurbsPatch& nurbsPatch,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    const double currentTime = data.GetUsdInArgs()->GetCurrentTimeD();
    const std::vector<double>& motionSampleTimes = 
        data.GetMotionSampleTimes(UsdGeomPointBased(nurbsPatch).GetPointsAttr());

    //
    // Set all general attributes for a gprim type.
    //

    PxrUsdKatanaReadGprim(nurbsPatch, data, attrs);

    //
    // Set more specific Katana type.
    //

    attrs.set("type", FnKat::StringAttribute("nurbspatch"));
    
    //
    // Construct the 'geometry' attribute.
    //

    FnKat::GroupBuilder geometryBuilder;

    geometryBuilder.set("point.Pw", _GetPwAttr(
        nurbsPatch, currentTime, motionSampleTimes, data.GetUsdInArgs()->IsMotionBackward()));
    geometryBuilder.set("u", _GetUAttr(nurbsPatch, currentTime));
    geometryBuilder.set("v", _GetVAttr(nurbsPatch, currentTime));
    geometryBuilder.set("uSize", _GetUSizeAttr(nurbsPatch, currentTime));       
    geometryBuilder.set("vSize", _GetVSizeAttr(nurbsPatch, currentTime));
    geometryBuilder.set("uClosed", _GetUClosedAttr(nurbsPatch, currentTime)); 
    geometryBuilder.set("vClosed", _GetVClosedAttr(nurbsPatch, currentTime));
    geometryBuilder.set("trimCurves", _GetTrimCurvesAttr(nurbsPatch, currentTime));

    FnKat::GroupBuilder arbBuilder;

    FnKat::GroupAttribute primvarGroup = PxrUsdKatanaGeomGetPrimvarGroup(nurbsPatch, data);

    if (primvarGroup.isValid())
    {
        arbBuilder.update(primvarGroup);
    }

    geometryBuilder.set("arbitrary", arbBuilder.build());
    attrs.set("geometry", geometryBuilder.build());

    //
    // Set the 'windingOrder' viewer attribute.
    //

    attrs.set("viewer.default.drawOptions.windingOrder",
        PxrUsdKatanaGeomGetWindingOrderAttr(nurbsPatch, data));
}
