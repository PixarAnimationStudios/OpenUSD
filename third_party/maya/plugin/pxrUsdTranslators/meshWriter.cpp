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
#include "pxrUsdTranslators/meshWriter.h"

#include "usdMaya/adaptor.h"
#include "usdMaya/meshUtil.h"
#include "usdMaya/primWriter.h"
#include "usdMaya/primWriterRegistry.h"
#include "usdMaya/util.h"
#include "usdMaya/writeUtil.h"
#include "usdMaya/writeJobContext.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usdUtils/pipeline.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MIntArray.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MUintArray.h>

#include <set>
#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


PXRUSDMAYA_REGISTER_WRITER(mesh, PxrUsdTranslators_MeshWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(mesh, UsdGeomMesh);


namespace {

void
_exportReferenceMesh(UsdGeomMesh& primSchema, MObject obj)
{
    MStatus status = MS::kSuccess;
    MFnDependencyNode dNode(obj, &status);
    if (!status) {
        return;
    }

    MPlug referencePlug = dNode.findPlug("referenceObject", &status);
    if (!status || referencePlug.isNull()) {
        return;
    }

    MPlugArray conns;
    referencePlug.connectedTo(conns, true, false);
    if (conns.length() == 0) {
        return;
    }

    MObject referenceObject = conns[0].node();
    if (!referenceObject.hasFn(MFn::kMesh)) {
        return;
    }

    MFnMesh referenceMesh(referenceObject, &status);
    if (!status) {
        return;
    }

    const float* mayaRawPoints = referenceMesh.getRawPoints(&status);
    const int numVertices = referenceMesh.numVertices();
    VtArray<GfVec3f> points(numVertices);
    for (int i = 0; i < numVertices; ++i) {
        const int floatIndex = i * 3;
        points[i].Set(mayaRawPoints[floatIndex],
                        mayaRawPoints[floatIndex + 1],
                        mayaRawPoints[floatIndex + 2]);
    }

    UsdGeomPrimvar primVar = primSchema.CreatePrimvar(
        UsdUtilsGetPrefName(),
        SdfValueTypeNames->Point3fArray,
        UsdGeomTokens->varying);

    if (!primVar) {
        return;
    }

    primVar.GetAttr().Set(VtValue(points));
}

} // anonymous namespace

const GfVec2f PxrUsdTranslators_MeshWriter::_DefaultUV = GfVec2f(0.f);

const GfVec3f PxrUsdTranslators_MeshWriter::_ShaderDefaultRGB = GfVec3f(0.5);
const float PxrUsdTranslators_MeshWriter::_ShaderDefaultAlpha = 0.0;

const GfVec3f PxrUsdTranslators_MeshWriter::_ColorSetDefaultRGB = GfVec3f(1.0);
const float PxrUsdTranslators_MeshWriter::_ColorSetDefaultAlpha = 1.0;
const GfVec4f PxrUsdTranslators_MeshWriter::_ColorSetDefaultRGBA = GfVec4f(
    PxrUsdTranslators_MeshWriter::_ColorSetDefaultRGB[0],
    PxrUsdTranslators_MeshWriter::_ColorSetDefaultRGB[1],
    PxrUsdTranslators_MeshWriter::_ColorSetDefaultRGB[2],
    PxrUsdTranslators_MeshWriter::_ColorSetDefaultAlpha);


PxrUsdTranslators_MeshWriter::PxrUsdTranslators_MeshWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath& usdPath,
        UsdMayaWriteJobContext& jobCtx) :
    UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    TF_AXIOM(GetDagPath().isValid());

    if (!isMeshValid()) {
        return;
    }

    // Get schema
    UsdGeomMesh primSchema = UsdGeomMesh::Define(GetUsdStage(), GetUsdPath());
    TF_AXIOM(primSchema);
    _usdPrim = primSchema.GetPrim();
    TF_AXIOM(_usdPrim);
}

/* virtual */
void
PxrUsdTranslators_MeshWriter::PostExport()
{
    _CleanupPrimvars();
}

/* virtual */
void
PxrUsdTranslators_MeshWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaPrimWriter::Write(usdTime);

    UsdGeomMesh primSchema(_usdPrim);
    writeMeshAttrs(usdTime, primSchema);
}

bool
PxrUsdTranslators_MeshWriter::writeMeshAttrs(
        const UsdTimeCode& usdTime,
        UsdGeomMesh& primSchema)
{
    MStatus status = MS::kSuccess;

    // Exporting reference object only once
    if (usdTime.IsDefault() && _GetExportArgs().exportReferenceObjects) {
        _exportReferenceMesh(primSchema, GetMayaObject());
    }

    // Write UsdSkel skeletal skinning data first, since this function will
    // determine whether we use the "input" or "final" mesh when exporting
    // mesh geometry. This should only be run once at default time.
    if (usdTime.IsDefault()) {
        _skelInputMesh = writeSkinningData(primSchema);
    }

    // This is the mesh that "lives" at the end of this dag node. We should
    // always pull user-editable "sidecar" data like color sets and tags from
    // this mesh.
    MFnMesh finalMesh(GetDagPath(), &status);
    if (!status) {
        TF_RUNTIME_ERROR(
            "Failed to get final mesh at DAG path: %s",
            GetDagPath().fullPathName().asChar());
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
        TF_RUNTIME_ERROR(
            "Failed to get geom mesh at DAG path: %s",
            GetDagPath().fullPathName().asChar());
        return false;
    }

    // Return if usdTime does not match if shape is animated.
    if (usdTime.IsDefault() == _IsMeshAnimated()) {
        // If the shape is animated (based on the check above), only export time
        // samples. If the shape is non-animated, only export at the default
        // time.
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
    _SetAttribute(primSchema.GetFaceVertexCountsAttr(), &faceVertexCounts, usdTime);
    _SetAttribute(primSchema.GetFaceVertexIndicesAttr(), &faceVertexIndices, usdTime);

    // Read subdiv scheme tagging. If not set, we default to defaultMeshScheme
    // flag (this is specified by the job args but defaults to catmullClark).
    TfToken sdScheme = UsdMayaMeshUtil::GetSubdivScheme(finalMesh);
    if (sdScheme.IsEmpty()) {
        sdScheme = _GetExportArgs().defaultMeshScheme;
    }
    primSchema.CreateSubdivisionSchemeAttr(VtValue(sdScheme), true);

    if (sdScheme == UsdGeomTokens->none) {
        // Polygonal mesh - export normals.
        bool emitNormals = true; // Default to emitting normals if no tagging.
        UsdMayaMeshUtil::GetEmitNormalsTag(finalMesh, &emitNormals);
        if (emitNormals) {
            VtArray<GfVec3f> meshNormals;
            TfToken normalInterp;

            if (UsdMayaMeshUtil::GetMeshNormals(
                    geomMesh,
                    &meshNormals,
                    &normalInterp)) {
                _SetAttribute(
                    primSchema.GetNormalsAttr(),
                    &meshNormals,
                    usdTime);
                primSchema.SetNormalsInterpolation(normalInterp);
            }
        }
    } else {
        // Subdivision surface - export subdiv-specific attributes.
        TfToken sdInterpBound = UsdMayaMeshUtil::GetSubdivInterpBoundary(
            finalMesh);
        if (!sdInterpBound.IsEmpty()) {
            _SetAttribute(primSchema.CreateInterpolateBoundaryAttr(),
                          sdInterpBound);
        }

        TfToken sdFVLinearInterpolation =
            UsdMayaMeshUtil::GetSubdivFVLinearInterpolation(finalMesh);
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
    if (_GetExportArgs().exportMeshUVs) {
        status = finalMesh.getUVSetNames(uvSetNames);
    }
    for (unsigned int i = 0; i < uvSetNames.length(); ++i) {
        VtArray<GfVec2f> uvValues;
        TfToken interpolation;
        VtArray<int> assignmentIndices;

        if (!_GetMeshUVSetData(
                finalMesh,
                uvSetNames[i],
                &uvValues,
                &interpolation,
                &assignmentIndices)) {
            continue;
        }

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
                         usdTime,
                         uvValues,
                         interpolation,
                         assignmentIndices);
    }

    // == Gather ColorSets
    std::vector<std::string> colorSetNames;
    if (_GetExportArgs().exportColorSets) {
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
    if (_GetExportArgs().exportDisplayColor || !colorSetNames.empty()) {
        UsdMayaUtil::GetLinearShaderColor(
            finalMesh,
            &shadersRGBData,
            &shadersAlphaData,
            &shadersInterpolation,
            &shadersAssignmentIndices);
    }

    for (const std::string& colorSetName: colorSetNames) {

        if (_excludeColorSets.count(colorSetName) > 0)
            continue;

        bool isDisplayColor = false;

        if (colorSetName == UsdMayaMeshColorSetTokens->DisplayColorColorSetName.GetString()) {
            if (!_GetExportArgs().exportDisplayColor) {
                continue;
            }
            isDisplayColor=true;
        }

        if (colorSetName == UsdMayaMeshColorSetTokens->DisplayOpacityColorSetName.GetString()) {
            TF_WARN("Mesh \"%s\" has a color set named \"%s\", "
                "which is a reserved Primvar name in USD. Skipping...",
                finalMesh.fullPathName().asChar(),
                UsdMayaMeshColorSetTokens->DisplayOpacityColorSetName
                    .GetText());
            continue;
        }

        VtArray<GfVec3f> RGBData;
        VtArray<float> AlphaData;
        TfToken interpolation;
        VtArray<int> assignmentIndices;
        MFnMesh::MColorRepresentation colorSetRep;
        bool clamped = false;

        if (!_GetMeshColorSetData(
                finalMesh,
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
            TF_WARN("Unable to retrieve colorSet data: %s on mesh: %s. "
                    "Skipping...",
                    colorSetName.c_str(), finalMesh.fullPathName().asChar());
            continue;
        }

        if (isDisplayColor) {
            // We tag the resulting displayColor/displayOpacity primvar as
            // authored to make sure we reconstruct the color set on import.
            _addDisplayPrimvars(
                primSchema,
                usdTime,
                colorSetRep,
                RGBData,
                AlphaData,
                interpolation,
                assignmentIndices,
                clamped,
                true);
        } else {
            const std::string sanitizedName = UsdMayaUtil::SanitizeColorSetName(colorSetName);
            // if our sanitized name is different than our current one and the
            // sanitized name already exists, it means 2 things are trying to
            // write to the same primvar.  warn and continue.
            if (colorSetName != sanitizedName &&
                    colorSetNamesSet.count(sanitizedName) > 0) {
                TF_WARN("Skipping colorSet '%s' as the colorSet '%s' exists as "
                        "well.",
                        colorSetName.c_str(), sanitizedName.c_str());
                continue;
            }

            TfToken colorSetNameToken = TfToken(sanitizedName);
            if (colorSetRep == MFnMesh::kAlpha) {
                _createAlphaPrimVar(primSchema,
                                    colorSetNameToken,
                                    usdTime,
                                    AlphaData,
                                    interpolation,
                                    assignmentIndices,
                                    clamped);
            } else if (colorSetRep == MFnMesh::kRGB) {
                _createRGBPrimVar(primSchema,
                                  colorSetNameToken,
                                  usdTime,
                                  RGBData,
                                  interpolation,
                                  assignmentIndices,
                                  clamped);
            } else if (colorSetRep == MFnMesh::kRGBA) {
                _createRGBAPrimVar(primSchema,
                                   colorSetNameToken,
                                   usdTime,
                                   RGBData,
                                   AlphaData,
                                   interpolation,
                                   assignmentIndices,
                                   clamped);
            }
        }
    }

    // _addDisplayPrimvars() will only author displayColor and displayOpacity
    // if no authored opinions exist, so the code below only has an effect if
    // we did NOT find a displayColor color set above.
    if (_GetExportArgs().exportDisplayColor) {
        // Using the shader default values (an alpha of zero, in particular)
        // results in Gprims rendering the same way in usdview as they do in
        // Maya (i.e. unassigned components are invisible).
        //
        // Since these colors come from the shaders and not a colorset, we are
        // not adding the clamp attribute as custom data. We also don't need to
        // reconstruct a color set from them on import since they originated
        // from the bound shader(s), so the authored flag is set to false.
        _addDisplayPrimvars(primSchema,
                            usdTime,
                            MFnMesh::kRGBA,
                            shadersRGBData,
                            shadersAlphaData,
                            shadersInterpolation,
                            shadersAssignmentIndices,
                            false,
                            false);
    }

    return true;
}

bool
PxrUsdTranslators_MeshWriter::isMeshValid()
{
    MStatus status = MS::kSuccess;

    // Sanity checks
    MFnMesh lMesh(GetDagPath(), &status);
    if (!status) {
        TF_RUNTIME_ERROR(
                "MFnMesh() failed for mesh at DAG path: %s",
                GetDagPath().fullPathName().asChar());
        return false;
    }

    unsigned int numVertices = lMesh.numVertices();
    unsigned int numPolygons = lMesh.numPolygons();
    if (numVertices < 3 && numVertices > 0)
    {
        TF_RUNTIME_ERROR(
                "%s is not a valid mesh, because it only has %u points,",
                lMesh.fullPathName().asChar(), numVertices);
    }
    if (numPolygons == 0)
    {
        TF_WARN("%s has no polygons.", lMesh.fullPathName().asChar());
    }
    return true;
}

/* virtual */
bool
PxrUsdTranslators_MeshWriter::ExportsGprims() const
{
    return true;
}

bool
PxrUsdTranslators_MeshWriter::_IsMeshAnimated() const
{
    // Note that _HasAnimCurves() as computed by UsdMayaTransformWriter is
    // whether the finalMesh is animated.
    return _skelInputMesh.isNull() ? _HasAnimCurves() : false;
}


PXR_NAMESPACE_CLOSE_SCOPE
