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
#include "usdMaya/MayaMeshWriter.h"

#include "usdMaya/meshUtil.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usdUtils/pipeline.h"

#include <maya/MFloatVectorArray.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MUintArray.h>

PXR_NAMESPACE_OPEN_SCOPE



const GfVec2f MayaMeshWriter::_DefaultUV = GfVec2f(-1.0e30);

const GfVec3f MayaMeshWriter::_ShaderDefaultRGB = GfVec3f(0.5);
const float MayaMeshWriter::_ShaderDefaultAlpha = 0.0;

const GfVec3f MayaMeshWriter::_ColorSetDefaultRGB = GfVec3f(1.0);
const float MayaMeshWriter::_ColorSetDefaultAlpha = 1.0;


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

static
bool
_GetMeshNormals(
        const MFnMesh& mesh,
        VtArray<GfVec3f>* normalsArray,
        TfToken* interpolation)
{
    MStatus status;

    // Sanity check first to make sure we can get this mesh's normals.
    int numNormals = mesh.numNormals(&status);
    if (status != MS::kSuccess || numNormals == 0) {
        return false;
    }

    // using itFV.getNormal() does not always give us the right answer, so
    // instead, we have to use itFV.normalId() and use that to index into the
    // normals.
    MFloatVectorArray mayaNormals;
    if (mesh.getNormals(mayaNormals) != MS::kSuccess) {
        return false;
    }

    const unsigned int numFaceVertices = mesh.numFaceVertices(&status);
    if (status != MS::kSuccess) {
        return false;
    }

    normalsArray->resize(numFaceVertices);
    *interpolation = UsdGeomTokens->faceVarying;

    MItMeshFaceVertex itFV(mesh.object());
    unsigned int fvi = 0;
    for (itFV.reset(); !itFV.isDone(); itFV.next(), ++fvi) {
        int normalId = itFV.normalId();
        if (normalId < 0 || static_cast<size_t>(normalId) >= mayaNormals.length()) {
            return false;
        }

        MVector normal = mayaNormals[normalId];
        (*normalsArray)[fvi][0] = normal[0];
        (*normalsArray)[fvi][1] = normal[1];
        (*normalsArray)[fvi][2] = normal[2];
    }

    return true;
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
    if (sdScheme == UsdGeomTokens->none) {
        // Support for standard USD bool and Mojito bool tags.
        if (PxrUsdMayaMeshUtil::getEmitNormals(lMesh, sdScheme)) {
            VtArray<GfVec3f> meshNormals;
            TfToken normalInterp;

            if (_GetMeshNormals(lMesh, &meshNormals, &normalInterp)) {
                primSchema.GetNormalsAttr().Set(meshNormals, usdTime);
                primSchema.SetNormalsInterpolation(normalInterp);
            }
        }
    } else {
        TfToken sdInterpBound = PxrUsdMayaMeshUtil::getSubdivInterpBoundary(
            lMesh, UsdGeomTokens->edgeAndCorner);

        primSchema.CreateInterpolateBoundaryAttr(VtValue(sdInterpBound), true);
        
        TfToken sdFVLinearInterpolation =
            PxrUsdMayaMeshUtil::getSubdivFVLinearInterpolation(lMesh);

        if (!sdFVLinearInterpolation.IsEmpty()) {
            primSchema.CreateFaceVaryingLinearInterpolationAttr(
                VtValue(sdFVLinearInterpolation), true);
        }

        assignSubDivTagsToUSDPrim(lMesh, primSchema);
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
    for (unsigned int i = 0; i < uvSetNames.length(); ++i) {
        VtArray<GfVec2f> uvValues;
        TfToken interpolation;
        VtArray<int> assignmentIndices;

        if (!_GetMeshUVSetData(lMesh,
                                  uvSetNames[i],
                                  &uvValues,
                                  &interpolation,
                                  &assignmentIndices)) {
            continue;
        }

        int unassignedValueIndex = -1;
        PxrUsdMayaUtil::AddUnassignedUVIfNeeded(&uvValues,
                                                &assignmentIndices,
                                                &unassignedValueIndex,
                                                _DefaultUV);

        // XXX:bug 118447
        // We should be able to configure the UV map name that triggers this
        // behavior, and the name to which it exports.
        // The UV Set "map1" is renamed st. This is a Pixar/USD convention.
        TfToken setName(uvSetNames[i].asChar());
        if (setName == "map1") {
            setName = UsdUtilsGetPrimaryUVSetName();
        }

        _createUVPrimVar(primSchema,
                         setName,
                         uvValues,
                         interpolation,
                         assignmentIndices,
                         unassignedValueIndex);
    }

    // == Gather ColorSets
    MStringArray colorSetNames;
    if (getArgs().exportColorSets) {
        status = lMesh.getColorSetNames(colorSetNames);
    }

    VtArray<GfVec3f> shadersRGBData;
    VtArray<float> shadersAlphaData;
    TfToken shadersInterpolation;
    VtArray<int> shadersAssignmentIndices;

    // If we're exporting displayColor or we have color sets, gather colors and
    // opacities from the shaders assigned to the mesh and/or its faces.
    // If we find a displayColor color set, the shader colors and opacities
    // will be used to fill in unauthored/unpainted faces in the color set.
    if (getArgs().exportDisplayColor || colorSetNames.length() > 0) {
        PxrUsdMayaUtil::GetLinearShaderColor(lMesh,
                                             &shadersRGBData,
                                             &shadersAlphaData,
                                             &shadersInterpolation,
                                             &shadersAssignmentIndices);
    }

    for (unsigned int i=0; i < colorSetNames.length(); ++i) {

        bool isDisplayColor = false;

        if (colorSetNames[i] == PxrUsdMayaMeshColorSetTokens->DisplayColorColorSetName.GetText()) {
            if (!getArgs().exportDisplayColor) {
                continue;
            }
            isDisplayColor=true;
        }
        
        if (colorSetNames[i] == PxrUsdMayaMeshColorSetTokens->DisplayOpacityColorSetName.GetText()) {
            MGlobal::displayWarning("Mesh \"" + lMesh.fullPathName() +
                "\" has a color set named \"" +
                MString(PxrUsdMayaMeshColorSetTokens->DisplayOpacityColorSetName.GetText()) +
                "\" which is a reserved Primvar name in USD. Skipping...");
            continue;
        }

        VtArray<GfVec3f> RGBData;
        VtArray<float> AlphaData;
        TfToken interpolation;
        VtArray<int> assignmentIndices;
        int unassignedValueIndex = -1;
        MFnMesh::MColorRepresentation colorSetRep;
        bool clamped = false;

        if (!_GetMeshColorSetData(lMesh,
                                     colorSetNames[i],
                                     isDisplayColor,
                                     shadersRGBData,
                                     shadersAlphaData,
                                     shadersAssignmentIndices,
                                     &RGBData,
                                     &AlphaData,
                                     &interpolation,
                                     &assignmentIndices,
                                     &colorSetRep,
                                     &clamped)) {
            MGlobal::displayWarning("Unable to retrieve colorSet data: " +
                colorSetNames[i] + " on mesh: " + lMesh.fullPathName() +
                ". Skipping...");
            continue;
        }

        PxrUsdMayaUtil::AddUnassignedColorAndAlphaIfNeeded(
            &RGBData,
            &AlphaData,
            &assignmentIndices,
            &unassignedValueIndex,
            _ColorSetDefaultRGB,
            _ColorSetDefaultAlpha);

        if (isDisplayColor) {
            // We tag the resulting displayColor/displayOpacity primvar as
            // authored to make sure we reconstruct the color set on import.
            _addDisplayPrimvars(primSchema,
                                colorSetRep,
                                RGBData,
                                AlphaData,
                                interpolation,
                                assignmentIndices,
                                unassignedValueIndex,
                                clamped,
                                true);
        } else {
            TfToken colorSetNameToken = TfToken(
                PxrUsdMayaUtil::SanitizeColorSetName(
                    std::string(colorSetNames[i].asChar())));
            if (colorSetRep == MFnMesh::kAlpha) {
                _createAlphaPrimVar(primSchema,
                                    colorSetNameToken,
                                    AlphaData,
                                    interpolation,
                                    assignmentIndices,
                                    unassignedValueIndex,
                                    clamped);
            } else if (colorSetRep == MFnMesh::kRGB) {
                _createRGBPrimVar(primSchema,
                                  colorSetNameToken,
                                  RGBData,
                                  interpolation,
                                  assignmentIndices,
                                  unassignedValueIndex,
                                  clamped);
            } else if (colorSetRep == MFnMesh::kRGBA) {
                _createRGBAPrimVar(primSchema,
                                   colorSetNameToken,
                                   RGBData,
                                   AlphaData,
                                   interpolation,
                                   assignmentIndices,
                                   unassignedValueIndex,
                                   clamped);
            }
        }
    }

    // _addDisplayPrimvars() will only author displayColor and displayOpacity
    // if no authored opinions exist, so the code below only has an effect if
    // we did NOT find a displayColor color set above.
    if (getArgs().exportDisplayColor) {
        // Using the shader default values (an alpha of zero, in particular)
        // results in Gprims rendering the same way in usdview as they do in
        // Maya (i.e. unassigned components are invisible).
        int unassignedValueIndex = -1;
        PxrUsdMayaUtil::AddUnassignedColorAndAlphaIfNeeded(
                &shadersRGBData,
                &shadersAlphaData,
                &shadersAssignmentIndices,
                &unassignedValueIndex,
                _ShaderDefaultRGB,
                _ShaderDefaultAlpha);

        // Since these colors come from the shaders and not a colorset, we are
        // not adding the clamp attribute as custom data. We also don't need to
        // reconstruct a color set from them on import since they originated
        // from the bound shader(s), so the authored flag is set to false.
        _addDisplayPrimvars(primSchema,
                            MFnMesh::kRGBA,
                            shadersRGBData,
                            shadersAlphaData,
                            shadersInterpolation,
                            shadersAssignmentIndices,
                            unassignedValueIndex,
                            false,
                            false);
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
    

PXR_NAMESPACE_CLOSE_SCOPE

