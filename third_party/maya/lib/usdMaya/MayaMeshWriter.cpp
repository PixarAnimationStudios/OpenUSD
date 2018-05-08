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

#include <maya/MUintArray.h>

PXR_NAMESPACE_OPEN_SCOPE



const GfVec2f MayaMeshWriter::_DefaultUV = GfVec2f(-1.0e30);

const GfVec3f MayaMeshWriter::_ShaderDefaultRGB = GfVec3f(0.5);
const float MayaMeshWriter::_ShaderDefaultAlpha = 0.0;

const GfVec3f MayaMeshWriter::_ColorSetDefaultRGB = GfVec3f(1.0);
const float MayaMeshWriter::_ColorSetDefaultAlpha = 1.0;


MayaMeshWriter::MayaMeshWriter(
        const MDagPath & iDag,
        const SdfPath& uPath,
        bool instanceSource,
        usdWriteJobCtx& jobCtx) :
    MayaTransformWriter(iDag, uPath, instanceSource, jobCtx)
{
    if ( !isMeshValid() ) {
        return;
    }

    // Get schema
    UsdGeomMesh primSchema = UsdGeomMesh::Define(getUsdStage(), getUsdPath());
    TF_AXIOM(primSchema);
    mUsdPrim = primSchema.GetPrim();
    TF_AXIOM(mUsdPrim);
}

//virtual 
void MayaMeshWriter::write(const UsdTimeCode &usdTime)
{
    UsdGeomMesh primSchema(mUsdPrim);
    // Write the attrs
    writeMeshAttrs(usdTime, primSchema);
}

// virtual
bool MayaMeshWriter::writeMeshAttrs(const UsdTimeCode &usdTime, UsdGeomMesh &primSchema)
{
    MStatus status = MS::kSuccess;

    // Write parent class attrs
    writeTransformAttrs(usdTime, primSchema);

    // Write UsdSkel skeletal skinning data first, since this function will
    // determine whether we use the "input" or "final" mesh when exporting
    // mesh geometry. This should only be run once at default time.
    if (usdTime.IsDefault()) {
        _skelInputMesh = writeSkinningData(primSchema);
    }

    // This is the mesh that "lives" at the end of this dag node. We should
    // always pull user-editable "sidecar" data like color sets and tags from
    // this mesh.
    MFnMesh finalMesh(getDagPath(), &status);
    if (!status) {
        MGlobal::displayError(
            "Failed to get final mesh at dagPath: " +
            getDagPath().fullPathName());
        return false;
    }

    // If exporting skinning, then geomMesh and finalMesh will be different
    // meshes. The general rule is to use geomMesh only for geometric data such
    // as vertices, faces, normals, but use finalMesh for UVs, color sets,
    // and user-defined tagging (e.g. subdiv tags).
    MObject geomMeshObj = _skelInputMesh.isNull() ?
            finalMesh.object() : _skelInputMesh;
    MFnMesh geomMesh(geomMeshObj, &status);
    if (!status) {
        MGlobal::displayError(
            "Failed to get geom mesh at dagPath: " +
            getDagPath().fullPathName());
        return false;
    }

    // Return if usdTime does not match if shape is animated.
    // XXX In theory you could have an animated input mesh before the
    // skinCluster is applied but we don't support that right now.
    // Note that isShapeAnimated() as computed by MayaTransformWriter is
    // whether the finalMesh is animated.
    bool isAnimated = _skelInputMesh.isNull() ? isShapeAnimated() : false;
    if (usdTime.IsDefault() == isAnimated) {
        // skip shape as the usdTime does not match if shape isAnimated value
        return true; 
    }

    unsigned int numVertices = geomMesh.numVertices();
    unsigned int numPolygons = geomMesh.numPolygons();

    // Set mesh attrs ==========
    // Get points
    // TODO: Use memcpy()
    const float* mayaRawPoints = geomMesh.getRawPoints(&status);
    VtArray<GfVec3f> points(numVertices);
    for (unsigned int i = 0; i < numVertices; i++) {
        unsigned int floatIndex = i*3;
        points[i].Set(mayaRawPoints[floatIndex],
                      mayaRawPoints[floatIndex+1],
                      mayaRawPoints[floatIndex+2]);
    }

    VtArray<GfVec3f> extent(2);
    // Compute the extent using the raw points
    UsdGeomPointBased::ComputeExtent(points, &extent);

    _SetAttribute(primSchema.GetPointsAttr(), &points, usdTime);
    _SetAttribute(primSchema.CreateExtentAttr(), &extent, usdTime);

    // Get faceVertexIndices
    unsigned int numFaceVertices = geomMesh.numFaceVertices(&status);
    VtArray<int>     faceVertexCounts(numPolygons);
    VtArray<int>     faceVertexIndices(numFaceVertices);
    MIntArray mayaFaceVertexIndices; // used in loop below
    unsigned int curFaceVertexIndex = 0;
    for (unsigned int i = 0; i < numPolygons; i++) {
        geomMesh.getPolygonVertices(i, mayaFaceVertexIndices);
        faceVertexCounts[i] = mayaFaceVertexIndices.length();
        for (unsigned int j=0; j < mayaFaceVertexIndices.length(); j++) {
            faceVertexIndices[ curFaceVertexIndex ] = mayaFaceVertexIndices[j]; // push_back
            curFaceVertexIndex++;
        }
    }
    _SetAttribute(primSchema.GetFaceVertexCountsAttr(), &faceVertexCounts);
    _SetAttribute(primSchema.GetFaceVertexIndicesAttr(), &faceVertexIndices);

    // Read usdSdScheme attribute. If not set, we default to defaultMeshScheme
    // flag that can be user defined and initialized to catmullClark
    TfToken sdScheme = PxrUsdMayaMeshUtil::getSubdivScheme(finalMesh,
            getArgs().defaultMeshScheme);
    primSchema.CreateSubdivisionSchemeAttr(VtValue(sdScheme), true);

    if (sdScheme == UsdGeomTokens->none) {
        // Polygonal Mesh Case
        if (PxrUsdMayaMeshUtil::getEmitNormals(finalMesh, sdScheme)) {
            VtArray<GfVec3f> meshNormals;
            TfToken normalInterp;

            if (PxrUsdMayaMeshUtil::GetMeshNormals(geomMesh,
                                                   &meshNormals,
                                                   &normalInterp)) {
                _SetAttribute(primSchema.GetNormalsAttr(), &meshNormals, 
                              usdTime);
                primSchema.SetNormalsInterpolation(normalInterp);
            }
        }
    } else {
        TfToken sdInterpBound = PxrUsdMayaMeshUtil::getSubdivInterpBoundary(
            finalMesh, UsdGeomTokens->edgeAndCorner);

        _SetAttribute(primSchema.CreateInterpolateBoundaryAttr(VtValue(), true), 
                      sdInterpBound);
        
        TfToken sdFVLinearInterpolation =
            PxrUsdMayaMeshUtil::getSubdivFVLinearInterpolation(finalMesh);

        if (!sdFVLinearInterpolation.IsEmpty()) {
            _SetAttribute(primSchema.CreateFaceVaryingLinearInterpolationAttr(),
                          sdFVLinearInterpolation);
        }

        assignSubDivTagsToUSDPrim(finalMesh, primSchema);
    }

    // Holes - we treat InvisibleFaces as holes
    MUintArray mayaHoles = finalMesh.getInvisibleFaces();
    if (mayaHoles.length() > 0) {
        VtArray<int> subdHoles(mayaHoles.length());
        for (unsigned int i=0; i < mayaHoles.length(); i++) {
            subdHoles[i] = mayaHoles[i];
        }
        // not animatable in Maya, so we'll set default only
        _SetAttribute(primSchema.GetHoleIndicesAttr(), &subdHoles);
    }

    // == Write UVSets as Vec2f Primvars
    MStringArray uvSetNames;
    if (getArgs().exportMeshUVs) {
        status = finalMesh.getUVSetNames(uvSetNames);
    }
    for (unsigned int i = 0; i < uvSetNames.length(); ++i) {
        VtArray<GfVec2f> uvValues;
        TfToken interpolation;
        VtArray<int> assignmentIndices;

        if (!_GetMeshUVSetData(finalMesh,
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
    std::vector<std::string> colorSetNames;
    if (getArgs().exportColorSets) {
        MStringArray mayaColorSetNames;
        status = finalMesh.getColorSetNames(mayaColorSetNames);
        colorSetNames.reserve(mayaColorSetNames.length());
        for (unsigned int i = 0; i < mayaColorSetNames.length(); i++) {
            colorSetNames.emplace_back(mayaColorSetNames[i].asChar());
        }
    }

    std::set<std::string> colorSetNamesSet(colorSetNames.begin(), colorSetNames.end());

    VtArray<GfVec3f> shadersRGBData;
    VtArray<float> shadersAlphaData;
    TfToken shadersInterpolation;
    VtArray<int> shadersAssignmentIndices;

    // If we're exporting displayColor or we have color sets, gather colors and
    // opacities from the shaders assigned to the mesh and/or its faces.
    // If we find a displayColor color set, the shader colors and opacities
    // will be used to fill in unauthored/unpainted faces in the color set.
    if (getArgs().exportDisplayColor || colorSetNames.size() > 0) {
        PxrUsdMayaUtil::GetLinearShaderColor(finalMesh,
                                             &shadersRGBData,
                                             &shadersAlphaData,
                                             &shadersInterpolation,
                                             &shadersAssignmentIndices);
    }

    for (const std::string& colorSetName: colorSetNames) {
        bool isDisplayColor = false;

        if (colorSetName == PxrUsdMayaMeshColorSetTokens->DisplayColorColorSetName.GetString()) {
            if (!getArgs().exportDisplayColor) {
                continue;
            }
            isDisplayColor=true;
        }
        
        if (colorSetName == PxrUsdMayaMeshColorSetTokens->DisplayOpacityColorSetName.GetString()) {
            MGlobal::displayWarning("Mesh \"" + finalMesh.fullPathName() +
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

        if (!_GetMeshColorSetData(finalMesh,
                                     MString(colorSetName.c_str()),
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
            const std::string warning = TfStringPrintf(
                    "Unable to retrieve colorSet data: %s on mesh: %s.  Skipping...",
                    colorSetName.c_str(), finalMesh.fullPathName().asChar());
            MGlobal::displayWarning(MString(warning.c_str()));
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
            const std::string sanitizedName = PxrUsdMayaUtil::SanitizeColorSetName(colorSetName);
            // if our sanitized name is different than our current one and the
            // sanitized name already exists, it means 2 things are trying to
            // write to the same primvar.  warn and continue.
            if (colorSetName != sanitizedName
                    && colorSetNamesSet.count(sanitizedName) > 0) {
                const std::string warning = TfStringPrintf(
                        "Skipping colorSet '%s' as the colorSet '%s' exists as well.",
                        colorSetName.c_str(), sanitizedName.c_str());
                MGlobal::displayWarning(MString(warning.c_str()));
                continue;
            }

            TfToken colorSetNameToken = TfToken(sanitizedName);
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

