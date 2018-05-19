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
#include "usdMaya/translatorNurbsPatch.h"

#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"
#include "usdMaya/translatorGprim.h"
#include "usdMaya/translatorMaterial.h"
#include "usdMaya/translatorUtil.h"

#include "pxr/usd/usdGeom/nurbsPatch.h"

#include <maya/MDoubleArray.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MObject.h>
#include <maya/MPointArray.h>
#include <maya/MTime.h>
#include <maya/MTimeArray.h>
#include <maya/MTrimBoundaryArray.h>

PXR_NAMESPACE_OPEN_SCOPE



/* static */
bool 
PxrUsdMayaTranslatorNurbsPatch::Read(
        const UsdGeomNurbsPatch& usdNurbsPatch,
        MObject parentNode,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context)
{
    if (!usdNurbsPatch) {
        return false;
    }

    const UsdPrim& prim = usdNurbsPatch.GetPrim();

    MStatus status;

    // Create the transform node for the patch.
    MObject mayaNode;
    if (!PxrUsdMayaTranslatorUtil::CreateTransformNode(prim,
                                                       parentNode,
                                                       args,
                                                       context,
                                                       &status,
                                                       &mayaNode)) {
        return false;
    }

    // Since we are "decollapsing", we will create a xform and a shape node for each USD prim
    std::string usdPrimName(prim.GetName().GetText());
    std::string shapeName(usdPrimName); shapeName += "Shape";
    std::string usdPrimPath(prim.GetPath().GetText());
    std::string shapePath(usdPrimPath);
    shapePath += "/";
    shapePath += shapeName;

    int numCVsInU, numCVsInV;
    int orderInU, orderInV;
    VtArray<double> knotsInU;
    VtArray<double> knotsInV;
    GfVec2d rangeInU, rangeInV;
    VtArray<GfVec3f> points;
    VtArray<double> weights;
    
    // NurbsPatch
    usdNurbsPatch.GetUVertexCountAttr().Get(&numCVsInU);
    usdNurbsPatch.GetVVertexCountAttr().Get(&numCVsInV);
    usdNurbsPatch.GetUOrderAttr().Get(&orderInU);
    usdNurbsPatch.GetVOrderAttr().Get(&orderInV);
    usdNurbsPatch.GetUKnotsAttr().Get(&knotsInU);
    usdNurbsPatch.GetVKnotsAttr().Get(&knotsInV);
    usdNurbsPatch.GetURangeAttr().Get(&rangeInU);
    usdNurbsPatch.GetVRangeAttr().Get(&rangeInV);
    usdNurbsPatch.GetPointWeightsAttr().Get(&weights);

    // Gather points.
    // If timeInterval is non-empty, pick the first available sample in the
    // timeInterval or default.
    UsdTimeCode pointsTimeSample=UsdTimeCode::EarliestTime();
    std::vector<double> pointsTimeSamples;
    size_t numTimeSamples = 0;
    if (!args.GetTimeInterval().IsEmpty()) {
        usdNurbsPatch.GetPointsAttr().GetTimeSamplesInInterval(
                args.GetTimeInterval(), &pointsTimeSamples);
        numTimeSamples = pointsTimeSamples.size();
        if (numTimeSamples>0) {
            pointsTimeSample = pointsTimeSamples[0];
        }
    }
    usdNurbsPatch.GetPointsAttr().Get(&points, pointsTimeSample);
    
    if (points.size() == 0) {
        MGlobal::displayError(
            TfStringPrintf("Points arrays is empty on NURBS <%s>. Skipping...", 
                            usdPrimPath.c_str()).c_str());
        return false; // invalid nurbs, so exit
    }
    
    if (points.size() != static_cast<size_t>((numCVsInU * numCVsInV))) {
        MString err = MString("CV array size not equal to UCount*VCount on NURBS: ") + 
                                MString(usdPrimPath.c_str());
        MGlobal::displayError(err);
        return false; // Bad CV data, so exit
    }

    // Maya stores the data where v varies the fastest (v,u order)
    // so we need to unpack the data differently u,v order
    // WE DIFFER FROM ALEMBIC READER, WE DON'T FLIP V
    
    bool rationalSurface = false, hasWeights = false;
    if (points.size()==weights.size()) hasWeights=true;
    int cvIndex=0;
    MPointArray mayaPoints;
    mayaPoints.setLength(numCVsInV*numCVsInU);
    for (int v = 0; v < numCVsInV; v++)
    {
        for (int u = 0; u < numCVsInU; u++)
        {
            int index = u * numCVsInV + v;
            if (hasWeights && GfIsClose(weights[cvIndex], 1.0, 1e-9)==false) {
                rationalSurface=true;
                mayaPoints.set( index, points[cvIndex][0], points[cvIndex][1], points[cvIndex][2], weights[cvIndex] );
            } else {
                mayaPoints.set( index, points[cvIndex][0], points[cvIndex][1], points[cvIndex][2] );
            }
            cvIndex++;
        }
    }

    double *knotsU=knotsInU.data();
    MDoubleArray mayaKnotsInU( &knotsU[1], knotsInU.size()-2);
    double *knotsV=knotsInV.data();
    MDoubleArray mayaKnotsInV( &knotsV[1], knotsInV.size()-2);

    MFnNurbsSurface::Form formInU = MFnNurbsSurface::kOpen;
    MFnNurbsSurface::Form formInV = MFnNurbsSurface::kOpen;
    TfToken form;
    usdNurbsPatch.GetUFormAttr().Get(&form);
    if (form == UsdGeomTokens->closed ) formInU = MFnNurbsSurface::kClosed;
    else if (form == UsdGeomTokens->periodic ) formInU = MFnNurbsSurface::kPeriodic;
    
    usdNurbsPatch.GetVFormAttr().Get(&form);
    if (form == UsdGeomTokens->closed ) formInV = MFnNurbsSurface::kClosed;
    else if (form == UsdGeomTokens->periodic ) formInV = MFnNurbsSurface::kPeriodic;

    // NOTE: In certain cases (i.e. linear cyilnder) Maya can't set the form
    // back to Closed when importing back an exported model. Seems a Maya bug
    
    // == Create NurbsSurface Shape Node
    MFnNurbsSurface surfaceFn;
      
    MObject surfaceObj = surfaceFn.create(mayaPoints,
                                        mayaKnotsInU,
                                        mayaKnotsInV,
                                        orderInU-1,
                                        orderInV-1,
                                        formInU,
                                        formInV,
                                        rationalSurface,
                                        mayaNode,
                                        &status);
    if (status != MS::kSuccess) {
        MString err = MString("Unable to create Maya Nurbs for USD NURBS: ") + MString(usdPrimPath.c_str());
        MGlobal::displayError(err);
        return false;
    }
    
    surfaceFn.setName(MString(shapeName.c_str()), false, &status);
    if (context) {
        context->RegisterNewMayaNode( shapePath, surfaceObj ); // used for undo/redo
    }

    // If a material is bound, create (or reuse if already present) and assign it
    // If no binding is present, assign the nurbs surface to the default shader
    const TfToken& shadingMode = args.GetShadingMode();  
    PxrUsdMayaTranslatorMaterial::AssignMaterial(
            shadingMode,
            usdNurbsPatch,
            surfaceObj,
            context);

    // NurbsSurface is a shape, so read Gprim properties
    PxrUsdMayaTranslatorGprim::Read(usdNurbsPatch, surfaceObj, context);

    // == Animate points ==
    //   Use blendShapeDeformer so that all the points for a frame are contained in a single node
    //   Almost identical code as used with MayaMeshReader.cpp
    //
    if (numTimeSamples > 0) {
        MObject surfaceAnimObj;

        MFnBlendShapeDeformer blendFn;
        MObject blendObj = blendFn.create(surfaceObj);
        if (context) {
            context->RegisterNewMayaNode(blendFn.name().asChar(), blendObj ); // used for undo/redo
        }
        
        for (unsigned int ti=0; ti < numTimeSamples; ++ti) {
            usdNurbsPatch.GetPointsAttr().Get(&points, pointsTimeSamples[ti]);

            cvIndex=0;
            for (int v = 0; v < numCVsInV; v++) {
                for (int u = 0; u < numCVsInU; u++) {
                    int index = u * numCVsInV + v;
                    mayaPoints.set( index, points[cvIndex][0], points[cvIndex][1], points[cvIndex][2] );
                    cvIndex++;
                }
            }

            // == Create NurbsSurface Shape Node
            MFnNurbsSurface surfaceFn;
            if ( surfaceAnimObj.isNull() ) {
                surfaceAnimObj = surfaceFn.create(mayaPoints,
                                        mayaKnotsInU,
                                        mayaKnotsInV,
                                        orderInU-1,
                                        orderInV-1,
                                        formInU,
                                        formInV,
                                        rationalSurface,
                                        mayaNode,
                                        &status);
                if (status != MS::kSuccess) {
                    continue;
                }
            }
            else {
                // Reuse the already created surface by copying it and then setting the points
                surfaceAnimObj = surfaceFn.copy(surfaceAnimObj, mayaNode, &status);
                surfaceFn.setCVs(mayaPoints);
            }
            blendFn.addTarget(surfaceObj, ti, surfaceAnimObj, 1.0);
            surfaceFn.setIntermediateObject(true);
            if (context) {
                context->RegisterNewMayaNode( surfaceFn.fullPathName().asChar(), surfaceAnimObj ); // used for undo/redo
            }
        }

        // Animate the weights so that surface0 has a weight of 1 at frame 0, etc.
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
                MObject animObj = animFn.create(plg, NULL, &status);
                animFn.addKeys(&timeArray, &valueArray);
                if (context) {
                    context->RegisterNewMayaNode(animFn.name().asChar(), animObj ); // used for undo/redo
                }
            }
        }
    }

    // Look for trim curves
    
    VtArray<int> trimNumCurves;
    VtArray<int> trimNumPos;
    VtArray<int> trimOrder;
    VtArray<double> trimKnot;
    VtArray<GfVec2d> trimRange;
    VtArray<GfVec3d> trimPoint;
    usdNurbsPatch.GetTrimCurveCountsAttr().Get(&trimNumCurves);    
    usdNurbsPatch.GetTrimCurveOrdersAttr().Get(&trimOrder);    
    usdNurbsPatch.GetTrimCurveVertexCountsAttr().Get(&trimNumPos);    
    usdNurbsPatch.GetTrimCurveKnotsAttr().Get(&trimKnot);    
    usdNurbsPatch.GetTrimCurveRangesAttr().Get(&trimRange);    
    usdNurbsPatch.GetTrimCurvePointsAttr().Get(&trimPoint);    

    int numLoops=trimNumCurves.size();
    if (numLoops == 0)
        return true;

    MTrimBoundaryArray trimBoundaryArray;
    MObjectArray deleteAfterTrim;

    int curCurve = 0;
    int curPos = 0;
    int curKnot = 0;

    for (int i = 0; i < numLoops; ++i) {
        MObjectArray trimLoop;

        int numCurves = trimNumCurves[i];
        for (int j = 0; j < numCurves; ++j, ++curCurve) {
            unsigned int degree = trimOrder[curCurve] - 1;
            int numVerts = trimNumPos[curCurve];
            int numKnots = numVerts + degree + 1;

            MPointArray cvs;
            cvs.setLength(numVerts);
            // WE DIFFER FROM ALEMBIC READER, WE DON'T FLIP V
            for (int k=0 ; k<numVerts; ++k, ++curPos) {
                double x = trimPoint[curPos][0];
                double y = trimPoint[curPos][1];
                double w = trimPoint[curPos][2];
                cvs.set(k, x, y, 0.0, w);
            }

            MDoubleArray dknots;
            dknots.setLength(numKnots - 2);
            ++curKnot;
            for (int k = 1; k < numKnots - 1; ++k, ++curKnot) {
                dknots.set(trimKnot[curKnot], k - 1);
            }
            ++curKnot;

            MFnNurbsCurve fnCurve;

            // when a 2D curve is created without a parent, the function
            // returns the transform node of the curve created.
            // The transform node as well as the curve node need to be
            // deleted once the trim is done, because after all this is not
            // the equivalent of "curveOnSurface" command
            MObject curve2D = fnCurve.create(cvs, dknots, degree,
                    MFnNurbsCurve::kOpen, true, true, MObject::kNullObj, &status);

            if (status == MS::kSuccess) {
                MFnTransform trans(curve2D, &status);
                if (status == MS::kSuccess) {
                    trimLoop.append(trans.child(0));
                    deleteAfterTrim.append(curve2D);
                }
            }
        }
        trimBoundaryArray.append(trimLoop);
    }

    // Now loop over trim boundary and determine maya regions
    // When we encounter a loop that is counterclockwise, it's an external
    // boundary. A trim region starts with an external boundary and can have
    // multiple internal boundaries.
    MTrimBoundaryArray oneRegion;
    // Add the first boundary since it has to be an outer one.
    // Then loop from the second one and when you encounter a new
    // outer boundary trim what has been collected so far
    oneRegion.append(trimBoundaryArray[0]);
    for (unsigned int i = 1; i < trimBoundaryArray.length(); i++) {
        MObject loopData = trimBoundaryArray.getMergedBoundary(i, &status);
        if (status != MS::kSuccess) continue;

        MFnNurbsCurve loop(loopData, &status);
        if (status != MS::kSuccess) continue;

        // Check whether this loop is an outer boundary.
        bool isOuterBoundary = false;

        double length  = loop.length();
        unsigned int segment = std::max(loop.numCVs(), 10);

        MPointArray curvePoints;
        curvePoints.setLength(segment);

        for (unsigned int j = 0; j < segment; j++) {
            double param = loop.findParamFromLength(length * j / segment);
            loop.getPointAtParam(param, curvePoints[j]);
        }

        // Find the right most curve point
        MPoint rightMostPoint = curvePoints[0];
        int rightMostIndex = 0;
        for (unsigned int j = 0; j < curvePoints.length(); j++) {
            if (rightMostPoint.x < curvePoints[j].x) {
                rightMostPoint = curvePoints[j];
                rightMostIndex = j;
            }
        }

        // Find the vertex just before and after the right most vertex
        int beforeIndex = (rightMostIndex == 0) ? curvePoints.length() - 1 : rightMostIndex - 1;
        int afterIndex  = (static_cast<size_t>(rightMostIndex) == curvePoints.length() - 1) ? 0 : rightMostIndex + 1;

        for (unsigned int j = 0; j < curvePoints.length(); j++) {
            if (fabs(curvePoints[beforeIndex].x - curvePoints[rightMostIndex].x) < 1e-5) {
                beforeIndex = (beforeIndex == 0) ? curvePoints.length() - 1 : beforeIndex - 1;
            }
        }

        for (unsigned int j = 0; j < curvePoints.length(); j++) {
                if (fabs(curvePoints[afterIndex].x - curvePoints[rightMostIndex].x) < 1e-5) {
                    afterIndex = (afterIndex == (int)(curvePoints.length()) - 1) ? 0 : afterIndex + 1;
                }
        }

        // failed. not a closed curve.
        if (fabs(curvePoints[afterIndex].x - curvePoints[rightMostIndex].x) < 1e-5 &&
                fabs(curvePoints[beforeIndex].x - curvePoints[rightMostIndex].x) < 1e-5) {
            continue;
        }

        if (beforeIndex < 0) beforeIndex += curvePoints.length();
        if (beforeIndex >= (int)(curvePoints.length())) beforeIndex = beforeIndex & curvePoints.length();
        if (afterIndex < 0) afterIndex += curvePoints.length();
        if (afterIndex >= (int)(curvePoints.length())) afterIndex = afterIndex & curvePoints.length();

        // Compute the cross product
        MVector vector1 = curvePoints[beforeIndex] - curvePoints[rightMostIndex];
        MVector vector2 = curvePoints[afterIndex]  - curvePoints[rightMostIndex];
        if ((vector1 ^ vector2).z < 0) {
            isOuterBoundary = true;
        }

        // Trim the NURBS surface. An outer boundary starts a new region.
        if (isOuterBoundary) {
            status = surfaceFn.trimWithBoundaries(oneRegion, false, 1e-3, 1e-5, true);
            if (status != MS::kSuccess) {
                MString err = MString("Trimming failed on NURBS: ") + MString(usdPrimPath.c_str());
                MGlobal::displayError(err);
            }
            oneRegion.clear();
        }
        oneRegion.append(trimBoundaryArray[i]);
    }

    if (oneRegion.length()>0) {
        status = surfaceFn.trimWithBoundaries(oneRegion, false, 1e-3, 1e-5, true);
    }
    if (status != MS::kSuccess) {
        MString err = MString("Trimming failed on NURBS: ") + MString(usdPrimPath.c_str());
        MGlobal::displayError(err);
    }
    // Deleted collected curves since they are not needed anymore
    unsigned int length = deleteAfterTrim.length();
    for (unsigned int l=0; l<length; l++) {
        MGlobal::deleteNode(deleteAfterTrim[l]);
    }
         
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

