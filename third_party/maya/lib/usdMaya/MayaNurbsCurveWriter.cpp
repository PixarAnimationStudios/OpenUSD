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
#include "usdMaya/MayaNurbsCurveWriter.h"

#include "usdMaya/adaptor.h"

#include "pxr/usd/usdGeom/curves.h"
#include "pxr/usd/usdGeom/nurbsCurves.h"
#include "pxr/usd/usd/stage.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MPointArray.h>

#include <numeric>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(MFn::kNurbsCurve, UsdGeomNurbsCurves);

MayaNurbsCurveWriter::MayaNurbsCurveWriter(const MDagPath & iDag,
                                           const SdfPath& uPath,
                                           bool instanceSource,
                                           usdWriteJobCtx& jobCtx) :
    MayaTransformWriter(iDag, uPath, instanceSource, jobCtx)
{
    UsdGeomNurbsCurves primSchema =
        UsdGeomNurbsCurves::Define(getUsdStage(), getUsdPath());
    TF_AXIOM(primSchema);
    mUsdPrim = primSchema.GetPrim();
    TF_AXIOM(mUsdPrim);
}


//virtual 
void MayaNurbsCurveWriter::write(const UsdTimeCode &usdTime)
{
    // == Write
    UsdGeomNurbsCurves primSchema(mUsdPrim);

    // Write the attrs
    writeNurbsCurveAttrs(usdTime, primSchema);
}

// virtual
bool MayaNurbsCurveWriter::writeNurbsCurveAttrs(const UsdTimeCode &usdTime, UsdGeomNurbsCurves &primSchema)
{
    MStatus status = MS::kSuccess;

    // Write parent class attrs
    writeTransformAttrs(usdTime, primSchema);

    // Return if usdTime does not match if shape is animated
    if (usdTime.IsDefault() == isShapeAnimated() ) {
        // skip shape as the usdTime does not match if shape isAnimated value
        return true; 
    }

    MFnDependencyNode fnDepNode(getDagPath().node(), &status);
    MString name = fnDepNode.name();

    MFnNurbsCurve curveFn(getDagPath(), &status);
    if (!status) {
        MGlobal::displayError(
            "MayaNurbsCurveWriter: MFnNurbsCurve() failed for curve at dagPath: " +
            getDagPath().fullPathName());
        return false;
    }
    
    // How to repeat the end knots.
    bool wrap = false;
    MFnNurbsCurve::Form form(curveFn.form());
    if (form == MFnNurbsCurve::kClosed ||
        form == MFnNurbsCurve::kPeriodic){
        wrap = true;
    }

    // Get curve attrs ======
    unsigned int numCurves = 1; // Assuming only 1 curve for now
    VtArray<int> curveOrder(numCurves);
    VtArray<int> curveVertexCounts(numCurves);
    VtArray<float> curveWidths(numCurves);
    VtArray<GfVec2d> ranges(numCurves);

    curveOrder[0] = curveFn.degree()+1;
    curveVertexCounts[0] = curveFn.numCVs();
    TF_AXIOM(curveOrder[0] <= curveVertexCounts[0] );
    curveWidths[0] = 1.0; // TODO: Retrieve from custom attr

    double mayaKnotDomainMin;
    double mayaKnotDomainMax;
    status = curveFn.getKnotDomain(mayaKnotDomainMin, mayaKnotDomainMax);
    TF_AXIOM(status == MS::kSuccess);
    ranges[0][0] = mayaKnotDomainMin;
    ranges[0][1] = mayaKnotDomainMax;

    MPointArray mayaCurveCVs;
    status = curveFn.getCVs(mayaCurveCVs, MSpace::kObject);
    TF_AXIOM(status == MS::kSuccess);
    VtArray<GfVec3f> points(mayaCurveCVs.length()); // all CVs batched together
    for (unsigned int i=0; i < mayaCurveCVs.length(); i++) {
        points[i].Set(mayaCurveCVs[i].x, mayaCurveCVs[i].y, mayaCurveCVs[i].z);
    }

    MDoubleArray mayaCurveKnots;
    status = curveFn.getKnots(mayaCurveKnots);
    TF_AXIOM(status == MS::kSuccess);
    VtArray<double> curveKnots(mayaCurveKnots.length() + 2); // all knots batched together
    for (unsigned int i = 0; i < mayaCurveKnots.length(); i++) {
        curveKnots[i + 1] = mayaCurveKnots[i];
    }
    if (wrap) {
        curveKnots[0] = curveKnots[1] - (curveKnots[curveKnots.size() - 2] -
                                         curveKnots[curveKnots.size() - 3]);
        curveKnots[curveKnots.size() - 1] =
            curveKnots[curveKnots.size() - 2] + (curveKnots[2] - curveKnots[1]);
    } else {
        curveKnots[0] = curveKnots[1];
        curveKnots[curveKnots.size() - 1] = curveKnots[curveKnots.size() - 2];
    }

    // Gprim
    VtArray<GfVec3f> extent(2);
    UsdGeomCurves::ComputeExtent(points, curveWidths, &extent);
    _SetAttribute(primSchema.CreateExtentAttr(), &extent, usdTime);

    // find the number of segments: (vertexCount - order + 1) per curve
    // varying interpolation is number of segments + number of curves
    size_t accumulatedVertexCount =
        std::accumulate(curveVertexCounts.begin(), curveVertexCounts.end(), 0);
    size_t accumulatedOrder =
        std::accumulate(curveOrder.begin(), curveOrder.end(), 0);
    size_t expectedSegmentCount =
        accumulatedVertexCount - accumulatedOrder + curveVertexCounts.size();
    size_t expectedVaryingSize =
        expectedSegmentCount + curveVertexCounts.size();

    if (curveWidths.size() == 1)
        primSchema.SetWidthsInterpolation(UsdGeomTokens->constant);
    else if (curveWidths.size() == points.size())
        primSchema.SetWidthsInterpolation(UsdGeomTokens->vertex);
    else if (curveWidths.size() == curveVertexCounts.size())
        primSchema.SetWidthsInterpolation(UsdGeomTokens->uniform);
    else if (curveWidths.size() == expectedVaryingSize)
        primSchema.SetWidthsInterpolation(UsdGeomTokens->varying);
    else {
        MGlobal::displayWarning(
            "MayaNurbsCurveWriter: MFnNurbsCurve() has unsupported width size "
            "for standard interpolation metadata: " +
            getDagPath().fullPathName());
    }

    // Curve
    // not animatable
    _SetAttribute(primSchema.GetOrderAttr(), curveOrder); 
    _SetAttribute(primSchema.GetCurveVertexCountsAttr(), &curveVertexCounts); 
    _SetAttribute(primSchema.GetWidthsAttr(), &curveWidths);

    _SetAttribute(primSchema.GetKnotsAttr(), &curveKnots); // not animatable
    _SetAttribute(primSchema.GetRangesAttr(), &ranges); // not animatable
    _SetAttribute(primSchema.GetPointsAttr(), &points, usdTime); // CVs

    // TODO: Handle periodic and non-periodic cases

    return true;
}

bool
MayaNurbsCurveWriter::exportsGprims() const
{
    return true;
}
    

PXR_NAMESPACE_CLOSE_SCOPE

