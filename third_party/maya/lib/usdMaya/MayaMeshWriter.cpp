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
#include "usdMaya/MayaMeshWriter.h"

#include "usdMaya/meshUtil.h"

#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usdUtils/pipeline.h"

#include <maya/MFloatVectorArray.h>
#include <maya/MUintArray.h>
#include <maya/MItMeshPolygon.h>

MayaMeshWriter::MayaMeshWriter(
        MDagPath & iDag, 
        UsdStageRefPtr stage, 
        const JobExportArgs & iArgs) :
    MayaTransformWriter(iDag, stage, iArgs)
{
}


//virtual 
UsdPrim MayaMeshWriter::write(const UsdTimeCode &usdTime)
{

    if ( !isMeshValid() ) {
        return UsdPrim();
    }

    // Get schema
    UsdGeomMesh primSchema = UsdGeomMesh::Define(getUsdStage(), getUsdPath());
    TF_AXIOM(primSchema);
    UsdPrim meshPrim = primSchema.GetPrim();
    TF_AXIOM(meshPrim);

    // Write the attrs
    writeMeshAttrs(usdTime, primSchema);
    return meshPrim;
}


// virtual
bool MayaMeshWriter::writeMeshAttrs(const UsdTimeCode &usdTime, UsdGeomMesh &primSchema)
{

    MStatus status = MS::kSuccess;

    // Write parent class attrs
    writeTransformAttrs(usdTime, primSchema);

    // Return if usdTime does not match if shape is animated
    if (usdTime.IsDefault() == isShapeAnimated() ) {
        // skip shape as the usdTime does not match if shape isAnimated value
        return true; 
    }

    MFnMesh lMesh(getDagPath(), &status);
    if (!status) {
        MGlobal::displayError(
            "MayaMeshWriter: MFnMesh() failed for mesh at dagPath: " +
            getDagPath().fullPathName());
        return false;
    }

    unsigned int numVertices = lMesh.numVertices();
    unsigned int numPolygons = lMesh.numPolygons();

    // Set mesh attrs ==========
    // Get points
    // TODO: Use memcpy()
    const float* mayaRawPoints = lMesh.getRawPoints(&status);
    VtArray<GfVec3f> points(numVertices);
    for (unsigned int i = 0; i < numVertices; i++) {
        unsigned int floatIndex = i*3;
        points[i].Set(mayaRawPoints[floatIndex],
                      mayaRawPoints[floatIndex+1],
                      mayaRawPoints[floatIndex+2]);
    }
    primSchema.GetPointsAttr().Set(points, usdTime); // ANIMATED

    // Compute the extent using the raw points
    VtArray<GfVec3f> extent(2);
    UsdGeomPointBased::ComputeExtent(points, &extent);
    primSchema.CreateExtentAttr().Set(extent, usdTime);

    // Get faceVertexIndices
    unsigned int numFaceVertices = lMesh.numFaceVertices(&status);
    VtArray<int>     faceVertexCounts(numPolygons);
    VtArray<int>     faceVertexIndices(numFaceVertices);
    MIntArray mayaFaceVertexIndices; // used in loop below
    unsigned int curFaceVertexIndex = 0;
    for (unsigned int i = 0; i < numPolygons; i++) {
        lMesh.getPolygonVertices(i, mayaFaceVertexIndices);
        faceVertexCounts[i] = mayaFaceVertexIndices.length();
        for (unsigned int j=0; j < mayaFaceVertexIndices.length(); j++) {
            faceVertexIndices[ curFaceVertexIndex ] = mayaFaceVertexIndices[j]; // push_back
            curFaceVertexIndex++;
        }
    }
    primSchema.GetFaceVertexCountsAttr().Set(faceVertexCounts);   // not animatable
    primSchema.GetFaceVertexIndicesAttr().Set(faceVertexIndices); // not animatable

    // Read usdSdScheme attribute. If not set, we default to defaultMeshScheme
    // flag that can be user defined and initialized to catmullClark
    TfToken sdScheme = PxrUsdMayaMeshUtil::getSubdivScheme(lMesh, getArgs().defaultMeshScheme);    
    primSchema.CreateSubdivisionSchemeAttr(VtValue(sdScheme), true);

    // Polygonal Mesh Case
    if (sdScheme==UsdGeomTokens->none) {
        // Support for standard USD bool and with Mojito bool tag
        TfToken normalInterp=PxrUsdMayaMeshUtil::getEmitNormals(lMesh, UsdGeomTokens->none);
        
        if (normalInterp==UsdGeomTokens->faceVarying) {
            // Get References to members of meshData object
            MFloatVectorArray normalArray;
            MFloatVectorArray vertexNormalArray;
 
            lMesh.getNormals(normalArray, MSpace::kObject);

            // Iterate through each face in the mesh.
            vertexNormalArray.setLength(lMesh.numFaceVertices());
            VtArray<GfVec3f> meshNormals(lMesh.numFaceVertices());
            size_t faceVertIdx = 0;
            for (MItMeshPolygon faceIter(getDagPath()); !faceIter.isDone(); faceIter.next()) {
                // Iterate through each face-vertex.
                for (size_t locVertIdx = 0; locVertIdx < faceIter.polygonVertexCount();
                        ++locVertIdx, ++faceVertIdx) {
                    int index=faceIter.normalIndex(locVertIdx);
                    for (int j=0;j<3;j++) {
                        meshNormals[faceVertIdx][j]=normalArray[index][j];
                    }
                }
            }
            primSchema.GetNormalsAttr().Set(meshNormals, usdTime);
            primSchema.SetNormalsInterpolation(normalInterp);
        }
    } else {
        TfToken sdInterpBound = PxrUsdMayaMeshUtil::getSubdivInterpBoundary(
            lMesh, UsdGeomTokens->edgeAndCorner);

        primSchema.CreateInterpolateBoundaryAttr(VtValue(sdInterpBound), true);
        
        TfToken sdFVInterpBound = PxrUsdMayaMeshUtil::getSubdivFVInterpBoundary(
            lMesh);

        primSchema.CreateFaceVaryingLinearInterpolationAttr(
            VtValue(sdFVInterpBound), true);

        assignSubDivTagsToUSDPrim( lMesh, primSchema);
    }

    // Holes - we treat InvisibleFaces as holes
    MUintArray mayaHoles = lMesh.getInvisibleFaces();
    if (mayaHoles.length() > 0) {
        VtArray<int> subdHoles(mayaHoles.length());
        for (unsigned int i=0; i < mayaHoles.length(); i++) {
            subdHoles[i] = mayaHoles[i];
        }
        // not animatable in Maya, so we'll set default only
        primSchema.GetHoleIndicesAttr().Set(subdHoles);
    }

    // == Write UVSets as Vec2f Primvars
    MStringArray uvSetNames;
    if (getArgs().exportMeshUVs) {
        status = lMesh.getUVSetNames(uvSetNames);
    }
    for (unsigned int i=0; i < uvSetNames.length(); i++) {
        // Initialize the VtArray to the max possible size (facevarying)
        VtArray<GfVec2f> uvValues(numFaceVertices);
        TfToken interpolation=TfToken();
        // Gather UV data and interpolation into a Vec2f VtArray and try to compress if possible
        if (_GetMeshUVSetData(lMesh, uvSetNames[i], &uvValues, &interpolation) == MS::kSuccess) {
        
            // XXX:bug 118447
            // We should be able to configure the UV map name that triggers this
            // behavior, and the name to which it exports.
            // The UV Set "map1" is renamed st. This is a Pixar/USD convention
            TfToken setName(uvSetNames[i].asChar());
            if (setName == "map1") setName=UsdUtilsGetPrimaryUVSetName();
       
            // Create the primvar and set the values
            UsdGeomPrimvar uvSet = 
                primSchema.CreatePrimvar(setName, SdfValueTypeNames->Float2Array, interpolation);
            uvSet.Set( uvValues ); // not animatable
        }
    }
    
    // == Gather ColorSets
    MStringArray colorSetNames;
    if (getArgs().exportColorSets) {
        status = lMesh.getColorSetNames(colorSetNames);
    }
    // shaderColor is used in our pipeline as displayColor.
    // shaderColor is used to fill faces where the colorset is not assigned
    MColorArray shaderColors;
    MObjectArray shaderObjs;
    
    VtArray<GfVec3f> shadersRGBData;
    TfToken shadersRGBInterp;
    VtArray<float> shadersAlphaData;
    TfToken shadersAlphaInterp;

    // If exportDisplayColor is set to true or we have color sets,
    // gather color & opacity from the shader including per face
    // assignment. Color set require this to initialize unauthored/unpainted faces 
    if (getArgs().exportDisplayColor or colorSetNames.length()>0) {
        PxrUsdMayaUtil::GetLinearShaderColor(lMesh, numPolygons, 
                &shadersRGBData, &shadersRGBInterp, 
                &shadersAlphaData, &shadersAlphaInterp);
    }

    for (unsigned int i=0; i < colorSetNames.length(); i++) {

        bool isDisplayColor=false;

        if (colorSetNames[i]=="displayColor") {
            if (not getArgs().exportDisplayColor)
                continue;
            isDisplayColor=true;
        }
        
        if (colorSetNames[i]=="displayOpacity") {
            MGlobal::displayWarning("displayOpacity on mesh:" + lMesh.fullPathName() + 
            " is a reserved PrimVar name in USD. Skipping...");
            continue;
        }

        VtArray<GfVec3f> RGBData;
        TfToken RGBInterp;
        VtArray<GfVec4f> RGBAData;
        TfToken RGBAInterp;
        VtArray<float> AlphaData;
        TfToken AlphaInterp;
        MFnMesh::MColorRepresentation colorSetRep;
        bool clamped=false;

        // If displayColor uses shaderValues for non authored areas
        // and allow RGB and Alpha to have different interpolation
        // For all other colorSets the non authored values are set 
        // to (1,1,1,1) and RGB and Alpha will have the same interplation
        // since they will be emitted as a Vec4f
        if (not _GetMeshColorSetData( lMesh, colorSetNames[i],
                                        isDisplayColor,
                                        shadersRGBData, shadersAlphaData,
                                        &RGBData, &RGBInterp,
                                        &RGBAData, &RGBAInterp,
                                        &AlphaData, &AlphaInterp,
                                        &colorSetRep, &clamped)) {
            MGlobal::displayWarning("Unable to retrieve colorSet data: " +
                    colorSetNames[i] + " on mesh: "+ lMesh.fullPathName() + ". Skipping...");
            continue;
        }

        if (isDisplayColor) {
            // We tag the resulting displayColor/displayOpacity primvar as
            // authored to make sure we reconstruct the colorset on import
            // The RGB is also convererted From DisplayToLinear
            
            
            _setDisplayPrimVar( primSchema, colorSetRep,
                                RGBData, RGBInterp,
                                AlphaData, AlphaInterp,
                                clamped, true);
        } else {
            TfToken colorSetNameToken = TfToken(
                    PxrUsdMayaUtil::SanitizeColorSetName(
                        std::string(colorSetNames[i].asChar())));
            if (colorSetRep == MFnMesh::kAlpha) {
                    _createAlphaPrimVar(primSchema, colorSetNameToken,
                        AlphaData, AlphaInterp, clamped);
            } else if (colorSetRep == MFnMesh::kRGB) {
                    _createRGBPrimVar(primSchema, colorSetNameToken,
                        RGBData, RGBInterp, clamped);
            } else if (colorSetRep == MFnMesh::kRGBA) {
                    _createRGBAPrimVar(primSchema, colorSetNameToken,
                        RGBAData, RGBAInterp, clamped);
            }
        }
    }
    // Set displayColor and displayOpacity only if they are NOT authored already
    // Since this primvar will come from the shader and not a colorset,
    // we are not adding the clamp attribute as custom data
    // If a displayColor/displayOpacity is added, it's not considered authored
    // we don't need to reconstruct this as a colorset since it orgininated
    // from bound shader[s], so the authored flag is set to false
    // Given that this RGB is for display, we do DisplayToLinear conversion
    if (getArgs().exportDisplayColor) {
        _setDisplayPrimVar( primSchema, MFnMesh::kRGBA,
                                shadersRGBData, shadersRGBInterp,
                                shadersAlphaData, shadersAlphaInterp,
                                false, false);
    }
    return true;
}

bool MayaMeshWriter::isMeshValid() 
{
    MStatus status = MS::kSuccess;

    // Sanity checks
    MFnMesh lMesh(getDagPath(), &status);
    if (!status) {
        MGlobal::displayError(
            "MayaMeshWriter: MFnMesh() failed for mesh at dagPath: " +
            getDagPath().fullPathName());
        return false;
    }

    unsigned int numVertices = lMesh.numVertices();
    unsigned int numPolygons = lMesh.numPolygons();
    if (numVertices < 3 && numVertices > 0)
    {
        MString err = lMesh.fullPathName() +
            " is not a valid mesh, because it only has ";
        err += numVertices;
        err += " points.";
        MGlobal::displayError(err);
    }
    if (numPolygons == 0)
    {
        MGlobal::displayWarning(lMesh.fullPathName() + " has no polygons.");
    }
    return true;
}

bool
MayaMeshWriter::exportsGprims() const
{
    return true;
}
    
