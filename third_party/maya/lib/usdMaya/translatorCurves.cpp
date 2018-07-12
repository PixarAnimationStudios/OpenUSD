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
#include "usdMaya/translatorCurves.h"

#include "usdMaya/translatorUtil.h"

#include "pxr/usd/usdGeom/basisCurves.h"
#include "pxr/usd/usdGeom/nurbsCurves.h"

#include <maya/MDoubleArray.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MIntArray.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MTime.h>
#include <maya/MTimeArray.h>

PXR_NAMESPACE_OPEN_SCOPE



/* static */
bool
PxrUsdMayaTranslatorCurves::Create(
        const UsdGeomCurves& curves,
        MObject parentNode,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context)
{
    if (!curves) {
        return false;
    }

    const UsdPrim& prim = curves.GetPrim();

    MStatus status;

    // Create node (transform)
    MObject mayaNodeTransformObj;
    if (!PxrUsdMayaTranslatorUtil::CreateTransformNode(prim,
                                                          parentNode,
                                                          args,
                                                          context,
                                                          &status,
                                                          &mayaNodeTransformObj)) {
        return false;
    }

    VtArray<GfVec3f> points;
    VtArray<int>     curveOrder;
    VtArray<int>     curveVertexCounts;
    VtArray<float>   curveWidths;
    VtArray<GfVec2d> curveRanges;
    VtArray<double>  curveKnots;

    // LIMITATION:  xxx REVISIT xxx
    //   Non-animated Attrs
    //   Assuming that a number of these USD attributes are assumed to not be animated
    //   Some we may want to expose as animatable later.
    //
    curves.GetCurveVertexCountsAttr().Get(&curveVertexCounts); // not animatable

    // XXX:
    // Only supporting single curve for now.
    // Sanity Checks
    if (curveVertexCounts.size() == 0) {
        TF_RUNTIME_ERROR(
                "vertexCount array is empty on NurbsCurves <%s>. Skipping...",
                prim.GetPath().GetText());
        return false; // No verts for the curve, so exit
    } else if (curveVertexCounts.size() > 1) {
        TF_WARN("Multiple curves in <%s>. Only reading the first one...", 
                prim.GetPath().GetText());
    }

    int curveIndex = 0;
    curves.GetWidthsAttr().Get(&curveWidths); // not animatable

    // Gather points. If timeInterval is non-empty, pick the first available
    // sample in the timeInterval or default.
    UsdTimeCode pointsTimeSample=UsdTimeCode::EarliestTime();
    std::vector<double> pointsTimeSamples;
    size_t numTimeSamples = 0;
    if (!args.GetTimeInterval().IsEmpty()) {
        curves.GetPointsAttr().GetTimeSamplesInInterval(
                args.GetTimeInterval(), &pointsTimeSamples);
        numTimeSamples = pointsTimeSamples.size();
        if (numTimeSamples>0) {
            pointsTimeSample = pointsTimeSamples[0];
        }
    }
    curves.GetPointsAttr().Get(&points, pointsTimeSample);
    
    if (points.size() == 0) {
        TF_RUNTIME_ERROR(
                "points array is empty on NurbsCurves <%s>. Skipping...",
                prim.GetPath().GetText());
        return false; // invalid nurbscurves, so exit
    }

    if (UsdGeomNurbsCurves nurbsSchema = UsdGeomNurbsCurves(prim)) {
        nurbsSchema.GetOrderAttr().Get(&curveOrder);   // not animatable
        nurbsSchema.GetKnotsAttr().Get(&curveKnots);   // not animatable
        nurbsSchema.GetRangesAttr().Get(&curveRanges); // not animatable
    } else {

        // Handle basis curves originally modelled in Maya as nurbs.

        curveOrder.resize(1);
        UsdGeomBasisCurves basisSchema = UsdGeomBasisCurves(prim);
        TfToken typeToken;
        basisSchema.GetTypeAttr().Get(&typeToken);
        if (typeToken == UsdGeomTokens->linear) {
            curveOrder[0] = 2;
            curveKnots.resize(points.size());
            for (size_t i=0; i < curveKnots.size(); ++i) {
                curveKnots[i] = i;
            }
        } else {
            curveOrder[0] = 4;

            // Strip off extra end points; assuming this is non-periodic.
            VtArray<GfVec3f> tmpPts(points.size() - 2);
            std::copy(points.begin() + 1, points.end() - 1, tmpPts.begin());
            points.swap(tmpPts);

            // Cubic curves in Maya have numSpans + 2*3 - 1, and for geometry
            // that came in as basis curves, we have numCV's - 3 spans. See the
            // MFnNurbsCurve documentation for more details.
            curveKnots.resize(points.size() -3 + 5);
            int knotIdx = 0;
            for (size_t i=0; i < curveKnots.size(); ++i) {
                if (i < 3) {
                    curveKnots[i] = 0.0;
                } else {
                    if (i <= curveKnots.size() - 3) {
                        ++knotIdx;
                    }
                    curveKnots[i] = double(knotIdx);
                } 
            }
        }
    }

    // == Convert data
    size_t mayaNumVertices = points.size();
    MPointArray mayaPoints(mayaNumVertices);
    for (size_t i=0; i < mayaNumVertices; i++) {
        mayaPoints.set( i, points[i][0], points[i][1], points[i][2] );
    }

    double *knots=curveKnots.data();
    MDoubleArray mayaKnots( knots, curveKnots.size());

    int mayaDegree = curveOrder[curveIndex] - 1;

    MFnNurbsCurve::Form mayaCurveForm = MFnNurbsCurve::kOpen; // HARDCODED
    bool mayaCurveCreate2D = false;
    bool mayaCurveCreateRational = true;

    // == Create NurbsCurve Shape Node
    MFnNurbsCurve curveFn;
    MObject curveObj = curveFn.create(mayaPoints, 
                                     mayaKnots,
                                     mayaDegree,
                                     mayaCurveForm,
                                     mayaCurveCreate2D,
                                     mayaCurveCreateRational,
                                     mayaNodeTransformObj,
                                     &status
                                     );
     if (status != MS::kSuccess) {
         return false;
     }
    MString nodeName( prim.GetName().GetText() );
    nodeName += "Shape";
    curveFn.setName(nodeName, false, &status);

    std::string nodePath( prim.GetPath().GetText() );
    nodePath += "/";
    nodePath += nodeName.asChar();
    if (context) {
        context->RegisterNewMayaNode( nodePath, curveObj ); // used for undo/redo
    }

    // == Animate points ==
    //   Use blendShapeDeformer so that all the points for a frame are contained in a single node
    //   Almost identical code as used with MayaMeshReader.cpp
    //
    if (numTimeSamples > 0) {
        MPointArray mayaPoints(mayaNumVertices);
        MObject curveAnimObj;

        MFnBlendShapeDeformer blendFn;
        MObject blendObj = blendFn.create(curveObj);
        if (context) {
            context->RegisterNewMayaNode(blendFn.name().asChar(), blendObj ); // used for undo/redo
        }
        
        for (unsigned int ti=0; ti < numTimeSamples; ++ti) {
             curves.GetPointsAttr().Get(&points, pointsTimeSamples[ti]);

            for (unsigned int i=0; i < mayaNumVertices; i++) {
                mayaPoints.set( i, points[i][0], points[i][1], points[i][2] );
            }

            // == Create NurbsCurve Shape Node
            MFnNurbsCurve curveFn;
            if ( curveAnimObj.isNull() ) {
                curveAnimObj = curveFn.create(mayaPoints, 
                                     mayaKnots,
                                     mayaDegree,
                                     mayaCurveForm,
                                     mayaCurveCreate2D,
                                     mayaCurveCreateRational,
                                     mayaNodeTransformObj,
                                     &status
                                     );
                if (status != MS::kSuccess) {
                    continue;
                }
            }
            else {
                // Reuse the already created curve by copying it and then setting the points
                curveAnimObj = curveFn.copy(curveAnimObj, mayaNodeTransformObj, &status);
                curveFn.setCVs(mayaPoints);
            }
            blendFn.addTarget(curveObj, ti, curveAnimObj, 1.0);
            curveFn.setIntermediateObject(true);
            if (context) {
                context->RegisterNewMayaNode( curveFn.fullPathName().asChar(), curveAnimObj ); // used for undo/redo
            }
        }

        // Animate the weights so that curve0 has a weight of 1 at frame 0, etc.
        MFnAnimCurve animFn;

        // Construct the time array to be used for all the keys
        MTimeArray timeArray;
        timeArray.setLength(numTimeSamples);
        for (unsigned int ti=0; ti < numTimeSamples; ++ti) {
            timeArray.set( MTime(pointsTimeSamples[ti]), ti);
        }

        // Key/Animate the weights
        MPlug plgAry = blendFn.findPlug( "weight" );
        if ( !plgAry.isNull() && plgAry.isArray() ) {
            for (unsigned int ti=0; ti < numTimeSamples; ++ti) {
                MPlug plg = plgAry.elementByLogicalIndex(ti, &status);
                MDoubleArray valueArray(numTimeSamples, 0.0);
                valueArray[ti] = 1.0; // Set the time value where this curve's weight should be 1.0
                MObject animObj = animFn.create(plg, nullptr, &status);
                animFn.addKeys(&timeArray, &valueArray);
                if (context) {
                    context->RegisterNewMayaNode(animFn.name().asChar(), animObj ); // used for undo/redo
                }
            }
        }
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

