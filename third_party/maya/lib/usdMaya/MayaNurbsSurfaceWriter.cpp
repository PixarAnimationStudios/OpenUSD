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

#include "usdMaya/MayaNurbsSurfaceWriter.h"

#include "pxr/usd/usdGeom/nurbsPatch.h"
#include "pxr/usd/usdGeom/nurbsCurves.h"
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdUtils/pipeline.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MTrimBoundaryArray.h>
#include <maya/MPointArray.h>

MayaNurbsSurfaceWriter::MayaNurbsSurfaceWriter(
        MDagPath & iDag, 
        UsdStageRefPtr stage, 
        const JobExportArgs & iArgs) :
    MayaTransformWriter(iDag, stage, iArgs)
{
}

static void
_FixNormalizedKnotRange(
    VtArray <double> & knots,
    const unsigned int numKnots,
    const unsigned int degree,
    const double startVal,
    const double endVal)
{
    // ensure we've produced valid knot ranges; the data coming
    // from Maya is fine but sometimes rounding errors in the normalization
    // cause problems.  So we change the knots on the boundaries
    // (whether single or multiple) to match the u/v range.
    double changeVal;

    if (startVal < knots[degree]) {
        changeVal = knots[degree];
        for (unsigned int i = 0; i <= degree; ++i) {
            if (knots[i] == changeVal) {
                knots[i] = startVal;
            }
        }
    }
    if (endVal > knots[numKnots - (degree + 1)]) {
        changeVal = knots[numKnots - (degree + 1)];
        for (unsigned int i = numKnots - (degree + 1); i < numKnots; ++i) {
            if (knots[i] == changeVal) {
                knots[i] = endVal;
            }
        }
    }
}

//virtual 
UsdPrim MayaNurbsSurfaceWriter::write(const UsdTimeCode &usdTimeCode)
{
    // == Write
    UsdGeomNurbsPatch primSchema =
        UsdGeomNurbsPatch::Define(getUsdStage(), getUsdPath());
    TF_AXIOM(primSchema);
    UsdPrim prim = primSchema.GetPrim();
    TF_AXIOM(prim);

    // Write the attrs
    writeNurbsSurfaceAttrs(usdTimeCode, primSchema);
    return prim;
}

// virtual
bool MayaNurbsSurfaceWriter::writeNurbsSurfaceAttrs(
    const UsdTimeCode &usdTimeCode,
    UsdGeomNurbsPatch &primSchema)
{
    MStatus status = MS::kSuccess;

    // Write parent class attrs
    writeTransformAttrs(usdTimeCode, primSchema);

    // Return if usdTimeCode does not match if shape is animated
    if (usdTimeCode.IsDefault() == isShapeAnimated() ) {
        // skip shape as the usdTimeCode does not match if shape isAnimated value
        return true; 
    }

    MFnNurbsSurface nurbs(getDagPath(), &status);
    if (!status) {
        MGlobal::displayError(
            "MayaNurbsSurfaceWriter: MFnNurbsSurface() failed for surface at dagPath: " +
            getDagPath().fullPathName());
        return false;
    }
    
    // Gather GPrim DisplayColor/DisplayOpacity
    // We use the same code used for gathering shader data on a mesh
    // but we pass 0 for the numfaces argument since there is no per face
    // shader assignment possible.
    if (getArgs().exportDisplayColor) {
        VtArray<GfVec3f> RGBData;
        VtArray<float> AlphaData;
        TfToken interpolation;
        VtArray<int> assignmentIndices;
        if (PxrUsdMayaUtil::GetLinearShaderColor(nurbs,
                                                 &RGBData,
                                                 &AlphaData,
                                                 &interpolation,
                                                 &assignmentIndices)) {
            if (RGBData.size()>0) {
                UsdGeomPrimvar dispColor = primSchema.GetDisplayColorPrimvar();
                if (interpolation != dispColor.GetInterpolation()) {
                    dispColor.SetInterpolation(interpolation);
                }
                dispColor.Set(RGBData);
                if (not assignmentIndices.empty()) {
                    dispColor.SetIndices(assignmentIndices);
                }
            }
            if (AlphaData.size() > 0 && 
                GfIsClose(AlphaData[0], 1.0, 1e-9)==false) {
                UsdGeomPrimvar dispOpacity = primSchema.GetDisplayOpacityPrimvar();
                if (interpolation != dispOpacity.GetInterpolation()) {
                    dispOpacity.SetInterpolation(interpolation);
                }
                dispOpacity.Set(AlphaData);
                if (not assignmentIndices.empty()) {
                    dispOpacity.SetIndices(assignmentIndices);
                }
            }
        }
    }

    unsigned int numKnotsInU = nurbs.numKnotsInU();
    unsigned int numKnotsInV = nurbs.numKnotsInV();
    if (numKnotsInU < 2 || numKnotsInV < 2) {
        MGlobal::displayError(
            "MFnNurbsSurface() has degenerate knot vectors. Skippping..." );
        return false;        
    }

    MDoubleArray knotsInU;
    nurbs.getKnotsInU(knotsInU);
    MDoubleArray knotsInV;
    nurbs.getKnotsInV(knotsInV);
    
    // determine range
    double startU, endU, startV, endV;
    nurbs.getKnotDomain(startU, endU, startV, endV);

    // Offset and scale to normalize knots from 0 to 1
    double uOffset=0.0;
    double vOffset=0.0;
    double uScale = 1.0;
    double vScale = 1.0;

    if (getArgs().normalizeNurbs) {
        if (endU>startU && endV>startV) {
            uOffset = startU;
            vOffset = startV;
            uScale = 1.0 / (endU - startU);
            vScale = 1.0 / (endV - startV);
            startU = 0; startV = 0;
            endU = 1; endV = 1;
        }
    }

    GfVec2d uRange, vRange;
    uRange[0]=startU; uRange[1]=endU;
    vRange[0]=startV; vRange[1]=endV;
    
    // pad the start and end with a knot on each side, since thats what
    // most apps, like Houdini and Renderman want these two extra knots
    VtArray<double> sampKnotsInU(numKnotsInU+2);
    VtArray<double> sampKnotsInV(numKnotsInV+2);

    for (unsigned int i = 0; i < numKnotsInU; i++) {
        sampKnotsInU[i+1]=(double)((knotsInU[i]-uOffset)*uScale);
    }
    
    for (unsigned int i = 0; i < numKnotsInV; i++) {
        sampKnotsInV[i+1]=(double)((knotsInV[i]-vOffset)*vScale);
    }

    if (getArgs().normalizeNurbs) {
        _FixNormalizedKnotRange(sampKnotsInU, numKnotsInU+2, nurbs.degreeU(), startU, endU);
        _FixNormalizedKnotRange(sampKnotsInV, numKnotsInV+2, nurbs.degreeV(), startV, endV);
    }


    sampKnotsInU[0] = (2.0 * sampKnotsInU[1] - sampKnotsInU[2]);
    sampKnotsInU[numKnotsInU+1] = (2.0 * sampKnotsInU[numKnotsInU] - 
                                        sampKnotsInU[numKnotsInU-1]);
    sampKnotsInV[0] = (2.0 * sampKnotsInV[1] - sampKnotsInV[2]);
    sampKnotsInV[numKnotsInV+1] = (2.0 * sampKnotsInV[numKnotsInV] - 
                                        sampKnotsInV[numKnotsInV-1]);

    MPointArray cvArray;
    nurbs.getCVs(cvArray, MSpace::kObject);
    unsigned int numCVs = cvArray.length();
    int numCVsInU = nurbs.numCVsInU();
    int numCVsInV = nurbs.numCVsInV();

    VtArray<GfVec3f> sampPos(numCVs);
    VtArray<double> sampPosWeights(numCVs);
    bool setWeights = false;

    // Create st vec2f vertex primvar
    // NOTE: We currently support PxUsdExportJobArgsTokens->Uniform
    // So no need to do a check in its value yet
    VtArray<GfVec2f> stValues;
    if (getArgs().exportNurbsExplicitUV) {
        stValues.resize(numCVsInU*numCVsInV);
    }

    // Maya stores the data where v varies the fastest (v,u order)
    // so we need to pack the data differently u,v order
    // WE DIFFER FROM ALEMBIC WRITER, WE DON'T FLIP V
    int cvIndex=0;
    for (int v = 0; v < numCVsInV; v++) {
        for (int u = 0; u < numCVsInU; u++) {
            int index = u * numCVsInV + v;
            
            // Extract CV location and weight
            sampPos[cvIndex].Set(cvArray[index].x, cvArray[index].y, cvArray[index].z);
            sampPosWeights[cvIndex] = cvArray[index].w;
            if (GfIsClose(cvArray[index].w, 1.0, 1e-9)==false) {
                setWeights = true;
            }
            
            // Compute uniform ST values if stValues can hold it
            // No need to check for nurbsTexCoordParam yet since we only
            // support uniform in the code
            if (stValues.size() > static_cast<size_t>(cvIndex)) {
                float sValue = static_cast<float>(u)/static_cast<float>(numCVsInU-1);
                float tValue = static_cast<float>(v)/static_cast<float>(numCVsInV-1);
                stValues[cvIndex] = GfVec2f(sValue, tValue);
            }
            
            cvIndex++;
        }
    }
    
    // Set Gprim Attributes
    // Compute the extent using the CVs.
    VtArray<GfVec3f> extent(2);
    UsdGeomPointBased::ComputeExtent(sampPos, &extent);
    primSchema.CreateExtentAttr().Set(extent, usdTimeCode);

    // Set NurbsPatch attributes
    primSchema.GetUVertexCountAttr().Set(numCVsInU);
    primSchema.GetVVertexCountAttr().Set(numCVsInV);
    primSchema.GetUOrderAttr().Set(nurbs.degreeU() + 1);
    primSchema.GetVOrderAttr().Set(nurbs.degreeV() + 1);
    primSchema.GetUKnotsAttr().Set(sampKnotsInU);
    primSchema.GetVKnotsAttr().Set(sampKnotsInV);
    primSchema.GetURangeAttr().Set(uRange);
    primSchema.GetVRangeAttr().Set(vRange);
    primSchema.GetPointsAttr().Set(sampPos, usdTimeCode);
    if (setWeights) {
        primSchema.GetPointWeightsAttr().Set(sampPosWeights);
    }

    // If stValues vector has vertex data, create and assign st
    if (stValues.size() == static_cast<size_t>(numCVsInU * numCVsInV)) {
        UsdGeomPrimvar uvSet = 
            primSchema.CreatePrimvar(UsdUtilsGetPrimaryUVSetName(),
            SdfValueTypeNames->Float2Array, 
            UsdGeomTokens->vertex);                                                                      
        uvSet.Set( stValues );
    }

    // Set Form
    switch (nurbs.formInU()) {
        case MFnNurbsSurface::kClosed:
            primSchema.GetUFormAttr().Set(UsdGeomTokens->closed);
        break;
        case MFnNurbsSurface::kPeriodic:
            primSchema.GetUFormAttr().Set(UsdGeomTokens->periodic);
        break;
        default:
            primSchema.GetUFormAttr().Set(UsdGeomTokens->open);        
    }
    switch (nurbs.formInV()) {
        case MFnNurbsSurface::kClosed:
            primSchema.GetVFormAttr().Set(UsdGeomTokens->closed);
        break;
        case MFnNurbsSurface::kPeriodic:
            primSchema.GetVFormAttr().Set(UsdGeomTokens->periodic);
        break;
        default:
            primSchema.GetVFormAttr().Set(UsdGeomTokens->open);        
    }

    // If not trimmed surface, you are done
    // ONLY TRIM CURVE CODE BEYOND THIS POINT
    if (!nurbs.isTrimmedSurface()) {
        return true;
    }

    unsigned int numRegions = nurbs.numRegions();

    // each boundary is a curvegroup, it can have multiple trim curve segments

    // A Maya's trimmed NURBS surface has multiple regions.
    // Inside a region, there are multiple boundaries.
    // There are one CCW outer boundary and optional CW inner boundaries.
    // Each boundary is a closed boundary and consists of multiple curves.
    // NOTE: Maya regions are flattened but thanks for the curve ordering
    // we can reconstruct them at read time back into Maya
    // USD has the same semantic as RenderMan.
    // RenderMan's doc says: "The curves of a loop connect
    //   in head-to-tail fashion and must be explicitly closed. "

    // A Maya boundary is equivalent to an USD/RenderMan loop
    VtArray<int> trimNumCurves;
    VtArray<int> trimNumPos;
    VtArray<int> trimOrder;
    VtArray<double> trimKnot;
    VtArray<GfVec2d> trimRange;
    VtArray<GfVec3d> trimPoint;

    int numLoops = 0;
    for (unsigned int i = 0; i < numRegions; i++)
    {
        MTrimBoundaryArray result;

        // if the 3rd argument is set to be true, return the 2D curve
        nurbs.getTrimBoundaries(result, i, true);
        unsigned int numBoundaries = result.length();
                
        for (unsigned int j = 0; j < numBoundaries; j++)
        {
            /*
            WE DON'T NEED THIS BUT IT'S HERE FOR POSSIBLE FUTURE USE
            switch(fn.boundaryType(i,j)) {
                case MFnNurbsSurface::kInner: break;
                case MFnNurbsSurface::kOuter: break;
                case MFnNurbsSurface::kSegment: break;
                case MFnNurbsSurface::kClosedSegment: break;
                default: break;
            }
            */
            
            MObjectArray boundary = result[j];
            unsigned int numTrimCurve = boundary.length();
            trimNumCurves.push_back(numTrimCurve);
            numLoops++;
            for (unsigned int k = 0; k < numTrimCurve; k++)
            {
                MObject curveObj = boundary[k];
                if (curveObj.hasFn(MFn::kNurbsCurve))
                {
                    MFnNurbsCurve mFnCurve(curveObj);

                    int numCVs = mFnCurve.numCVs();
                    trimNumPos.push_back(numCVs);
                    trimOrder.push_back(mFnCurve.degree()+1);

                    double start, end;
                    mFnCurve.getKnotDomain(start, end);
                    GfVec2d range;
                    range[0]=start;
                    range[1]=end;
                    trimRange.push_back(range);

                    MPointArray cvArray;
                    mFnCurve.getCVs(cvArray);
                    // WE DIFFER FROM ALEMBIC WRITER, WE DON'T FLIP V
                    for (int l = 0; l < numCVs; l++)
                    {
                        GfVec3d point;                        

                        point[0]=(double)((cvArray[l].x-uOffset)*uScale);
                        point[1]=(double)((cvArray[l].y-vOffset)*vScale);
                        point[2]=cvArray[l].w;
                        trimPoint.push_back(point);
                    }

                    MDoubleArray knot;
                    mFnCurve.getKnots(knot);
                    unsigned int numKnots = knot.length();

                    // push_back a dummy value, we will set it below
                    std::size_t totalNumKnots = trimKnot.size();
                    trimKnot.push_back(0.0);
                    for (unsigned int l = 0; l < numKnots; l++)
                    {
                        trimKnot.push_back(knot[l]);
                    }

                    // for a knot sequence with multiple end knots, duplicate
                    // the existing first and last knots once more.
                    // for a knot sequence with uniform end knots, create their
                    // the new knots offset at an interval equal to the existing
                    // first and last knot intervals
                    double k1 = trimKnot[totalNumKnots+1];
                    double k2 = trimKnot[totalNumKnots+2];
                    double klast_1 = trimKnot[trimKnot.size()-1];
                    double klast_2 = trimKnot[trimKnot.size()-2];
                    trimKnot[totalNumKnots] = 2.0 * k1 - k2;
                    trimKnot.push_back(2.0 * klast_1 - klast_2);
                }
            } // for k
        } // for j
    } // for i

    primSchema.GetTrimCurveCountsAttr().Set(trimNumCurves);    
    primSchema.GetTrimCurveOrdersAttr().Set(trimOrder);    
    primSchema.GetTrimCurveVertexCountsAttr().Set(trimNumPos);    
    primSchema.GetTrimCurveKnotsAttr().Set(trimKnot);    
    primSchema.GetTrimCurveRangesAttr().Set(trimRange);    
    primSchema.GetTrimCurvePointsAttr().Set(trimPoint);    

    // NO NON TRIM CODE HERE SINCE WE RETURN EARLIER IF NOT TRIMMED
    return true;
}

bool
MayaNurbsSurfaceWriter::exportsGprims() const
{
    return true;
}
    
