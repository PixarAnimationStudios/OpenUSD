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
#include "usdMaya/MayaNurbsCurveWriter.h"

#include "pxr/usd/usdGeom/curves.h"
#include "pxr/usd/usdGeom/nurbsCurves.h"
#include "pxr/usd/usd/stage.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MPointArray.h>

MayaNurbsCurveWriter::MayaNurbsCurveWriter(MDagPath & iDag, UsdStageRefPtr stage, const JobExportArgs & iArgs) :
    MayaTransformWriter(iDag, stage, iArgs)
{
}


//virtual 
UsdPrim MayaNurbsCurveWriter::write(const UsdTimeCode &usdTime)
{
    // == Write
    UsdGeomNurbsCurves primSchema =
        UsdGeomNurbsCurves::Define(getUsdStage(), getUsdPath());
    TF_AXIOM(primSchema);
    UsdPrim prim = primSchema.GetPrim();
    TF_AXIOM(prim);

    // Write the attrs
    writeNurbsCurveAttrs(usdTime, primSchema);
    return prim;
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
    VtArray<double> curveKnots(mayaCurveKnots.length()); // all knots batched together
    for (unsigned int i=0; i < mayaCurveKnots.length(); i++) {
        curveKnots[i] = mayaCurveKnots[i];
    }

    // Gprim
    VtArray<GfVec3f> extent(2);
    UsdGeomCurves::ComputeExtent(points, curveWidths, &extent);
    primSchema.CreateExtentAttr().Set(extent, usdTime);

    // Curve
    primSchema.GetOrderAttr().Set(curveOrder);   // not animatable
    primSchema.GetCurveVertexCountsAttr().Set(curveVertexCounts); // not animatable
    primSchema.GetWidthsAttr().Set(curveWidths); // not animatable
    primSchema.GetKnotsAttr().Set(curveKnots);   // not animatable
    primSchema.GetRangesAttr().Set(ranges); // not animatable
    primSchema.GetPointsAttr().Set(points, usdTime); // CVs

    // TODO: Handle periodic and non-periodic cases

    return true;
}

bool
MayaNurbsCurveWriter::exportsGprims() const
{
    return true;
}
    
