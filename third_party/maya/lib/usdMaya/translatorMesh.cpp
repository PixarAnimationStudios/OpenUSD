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
#include "usdMaya/translatorMesh.h"

#include "usdMaya/meshUtil.h"
#include "usdMaya/pointBasedDeformerNode.h"
#include "usdMaya/readUtil.h"
#include "usdMaya/roundTripUtil.h"
#include "usdMaya/stageNode.h"
#include "usdMaya/translatorGprim.h"
#include "usdMaya/translatorMaterial.h"
#include "usdMaya/translatorUtil.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/tokens.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvar.h"

#include <maya/MDGModifier.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnGeometryFilter.h>
#include <maya/MFnSet.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MUintArray.h>

#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


static
bool
_SetupPointBasedDeformerForMayaNode(
        MObject& mayaObj,
        const UsdPrim& prim,
        UsdMayaPrimReaderContext* context)
{
    // We try to get the USD stage node from the context's registry, so if we
    // don't have a reader context, we can't continue.
    if (!context) {
        return false;
    }

    MObject stageNode =
        context->GetMayaNode(
            SdfPath(UsdMayaStageNodeTokens->MayaTypeName.GetString()),
            false);
    if (stageNode.isNull()) {
        return false;
    }

    // Get the output time plug and node for Maya's global time object.
    MPlug timePlug = UsdMayaUtil::GetMayaTimePlug();
    if (timePlug.isNull()) {
        return false;
    }

    MStatus status;
    MObject timeNode = timePlug.node(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Clear the selection list so that the deformer command doesn't try to add
    // anything to the new deformer's set. We'll do that manually afterwards.
    status = MGlobal::clearSelectionList();
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Create the point based deformer node for this prim.
    const std::string pointBasedDeformerNodeName =
        TfStringPrintf("usdPointBasedDeformerNode%s",
                       TfStringReplace(prim.GetPath().GetString(),
                                       SdfPathTokens->childDelimiter.GetString(),
                                       "_").c_str());

    const std::string deformerCmd = TfStringPrintf(
        "from maya import cmds; cmds.deformer(name=\'%s\', type=\'%s\')[0]",
        pointBasedDeformerNodeName.c_str(),
        UsdMayaPointBasedDeformerNodeTokens->MayaTypeName.GetText());
    MString newPointBasedDeformerName;
    status = MGlobal::executePythonCommand(deformerCmd.c_str(),
                                           newPointBasedDeformerName);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Get the newly created point based deformer node.
    MObject pointBasedDeformerNode;
    status = UsdMayaUtil::GetMObjectByName(newPointBasedDeformerName.asChar(),
                                              pointBasedDeformerNode);
    CHECK_MSTATUS_AND_RETURN(status, false);

    context->RegisterNewMayaNode(newPointBasedDeformerName.asChar(),
                                 pointBasedDeformerNode);

    MFnDependencyNode depNodeFn(pointBasedDeformerNode, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MDGModifier dgMod;

    // Set the prim path on the deformer node.
    MPlug primPathPlug =
        depNodeFn.findPlug(UsdMayaPointBasedDeformerNode::primPathAttr,
                           true,
                           &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    status = dgMod.newPlugValueString(primPathPlug, prim.GetPath().GetText());
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Connect the stage node's stage output to the deformer node.
    status = dgMod.connect(stageNode,
                           UsdMayaStageNode::outUsdStageAttr,
                           pointBasedDeformerNode,
                           UsdMayaPointBasedDeformerNode::inUsdStageAttr);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Connect the global Maya time to the deformer node.
    status = dgMod.connect(timeNode,
                           timePlug.attribute(),
                           pointBasedDeformerNode,
                           UsdMayaPointBasedDeformerNode::timeAttr);
    CHECK_MSTATUS_AND_RETURN(status, false);

    status = dgMod.doIt();
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Add the Maya object to the point based deformer node's set.
    const MFnGeometryFilter geomFilterFn(pointBasedDeformerNode, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MObject deformerSet = geomFilterFn.deformerSet(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MFnSet setFn(deformerSet, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    status = setFn.addMember(mayaObj);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // When we created the point based deformer, Maya will have automatically
    // created a tweak deformer and put it *before* the point based deformer in
    // the deformer chain. We don't want that, since any component edits made
    // interactively in Maya will appear to have no effect since they'll be
    // overridden by the point based deformer. Instead, we want the tweak to go
    // *after* the point based deformer. To do this, we need to dig for the
    // name of the tweak deformer node that Maya created to be able to pass it
    // to the reorderDeformers command.
    const MFnDagNode dagNodeFn(mayaObj, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // XXX: This seems to be the "most sane" way of finding the tweak deformer
    // node's name...
    const std::string findTweakCmd = TfStringPrintf(
        "from maya import cmds; [x for x in cmds.listHistory(\'%s\') if cmds.nodeType(x) == \'tweak\'][0]",
        dagNodeFn.fullPathName().asChar());

    MString tweakDeformerNodeName;
    status = MGlobal::executePythonCommand(findTweakCmd.c_str(),
                                           tweakDeformerNodeName);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Do the reordering.
    const std::string reorderDeformersCmd = TfStringPrintf(
        "from maya import cmds; cmds.reorderDeformers(\'%s\', \'%s\', \'%s\')",
        tweakDeformerNodeName.asChar(),
        newPointBasedDeformerName.asChar(),
        dagNodeFn.fullPathName().asChar());
    status = MGlobal::executePythonCommand(reorderDeformersCmd.c_str());
    CHECK_MSTATUS_AND_RETURN(status, false);

    return true;
}

/* static */
bool
UsdMayaTranslatorMesh::Create(
        const UsdGeomMesh& mesh,
        MObject parentNode,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext* context)
{
    if (!mesh) {
        return false;
    }

    const UsdPrim& prim = mesh.GetPrim();

    MStatus status;

    // Create node (transform)
    MObject mayaNodeTransformObj;
    if (!UsdMayaTranslatorUtil::CreateTransformNode(prim,
                                                       parentNode,
                                                       args,
                                                       context,
                                                       &status,
                                                       &mayaNodeTransformObj)) {
        return false;
    }

    VtVec3fArray points;
    VtVec3fArray normals;
    VtIntArray faceVertexCounts;
    VtIntArray faceVertexIndices;

    const UsdAttribute fvc = mesh.GetFaceVertexCountsAttr();
    if (fvc.ValueMightBeTimeVarying()){
        // at some point, it would be great, instead of failing, to create a usd/hydra proxy node
        // for the mesh, perhaps?  For now, better to give a more specific error
        TF_RUNTIME_ERROR(
                "<%s> is a topologically varying Mesh (has animated "
                "faceVertexCounts), which isn't currently supported. "
                "Skipping...",
                prim.GetPath().GetText());
        return false;
    } else {
        fvc.Get(&faceVertexCounts, UsdTimeCode::EarliestTime());
    }

    const UsdAttribute fvi = mesh.GetFaceVertexIndicesAttr();
    if (fvi.ValueMightBeTimeVarying()){
        // at some point, it would be great, instead of failing, to create a usd/hydra proxy node
        // for the mesh, perhaps?  For now, better to give a more specific error
        TF_RUNTIME_ERROR(
                "<%s> is a topologically varying Mesh (has animated "
                "faceVertexIndices), which isn't currently supported. "
                "Skipping...",
                prim.GetPath().GetText());
        return false;
    } else {
        fvi.Get(&faceVertexIndices, UsdTimeCode::EarliestTime());
    }

    // Sanity Checks. If the vertex arrays are empty, skip this mesh
    if (faceVertexCounts.empty() || faceVertexIndices.empty()) {
        TF_RUNTIME_ERROR(
                "faceVertexCounts or faceVertexIndices array is empty "
                "[count: %zu, indices:%zu] on Mesh <%s>. Skipping...",
                faceVertexCounts.size(), faceVertexIndices.size(),
                prim.GetPath().GetText());
        return false; // invalid mesh, so exit
    }

    // Gather points and normals
    // If timeInterval is non-empty, pick the first available sample in the
    // timeInterval or default.
    UsdTimeCode pointsTimeSample = UsdTimeCode::EarliestTime();
    UsdTimeCode normalsTimeSample = UsdTimeCode::EarliestTime();
    std::vector<double> pointsTimeSamples;
    size_t pointsNumTimeSamples = 0u;
    if (!args.GetTimeInterval().IsEmpty()) {
        mesh.GetPointsAttr().GetTimeSamplesInInterval(args.GetTimeInterval(),
                                                      &pointsTimeSamples);
        if (!pointsTimeSamples.empty()) {
            pointsNumTimeSamples = pointsTimeSamples.size();
            pointsTimeSample = pointsTimeSamples.front();
        }

        std::vector<double> normalsTimeSamples;
        mesh.GetNormalsAttr().GetTimeSamplesInInterval(args.GetTimeInterval(),
                                                       &normalsTimeSamples);
        if (!normalsTimeSamples.empty()) {
            normalsTimeSample = normalsTimeSamples.front();
        }
    }

    mesh.GetPointsAttr().Get(&points, pointsTimeSample);
    mesh.GetNormalsAttr().Get(&normals, normalsTimeSample);

    if (points.empty()) {
        TF_RUNTIME_ERROR("points array is empty on Mesh <%s>. Skipping...",
                         prim.GetPath().GetText());
        return false; // invalid mesh, so exit
    }

    std::string reason;
    if (!UsdGeomMesh::ValidateTopology(faceVertexIndices,
                                       faceVertexCounts,
                                       points.size(),
                                       &reason)) {
        TF_RUNTIME_ERROR("Skipping Mesh <%s> with invalid topology: %s",
                         prim.GetPath().GetText(), reason.c_str());
        return false;
    }


    // == Convert data
    const size_t mayaNumVertices = points.size();
    MPointArray mayaPoints(mayaNumVertices);
    for (size_t i = 0u; i < mayaNumVertices; ++i) {
        mayaPoints.set(i, points[i][0], points[i][1], points[i][2]);
    }

    MIntArray polygonCounts(faceVertexCounts.cdata(), faceVertexCounts.size());
    MIntArray polygonConnects(faceVertexIndices.cdata(), faceVertexIndices.size());

    // == Create Mesh Shape Node
    MFnMesh meshFn;
    MObject meshObj = meshFn.create(mayaPoints.length(),
                                    polygonCounts.length(),
                                    mayaPoints,
                                    polygonCounts,
                                    polygonConnects,
                                    mayaNodeTransformObj,
                                    &status);
    if (status != MS::kSuccess) {
        return false;
    }

    // Since we are "decollapsing", we will create a xform and a shape node for each USD prim
    const std::string usdPrimName = prim.GetName().GetString();
    const std::string shapeName = TfStringPrintf("%sShape",
                                                 usdPrimName.c_str());

    // Set mesh name and register
    meshFn.setName(MString(shapeName.c_str()), false, &status);
    if (context) {
        const SdfPath shapePath =
            prim.GetPath().AppendChild(TfToken(shapeName));
        context->RegisterNewMayaNode(shapePath.GetString(), meshObj); // used for undo/redo
    }

    // If a material is bound, create (or reuse if already present) and assign it
    // If no binding is present, assign the mesh to the default shader
    const TfToken& shadingMode = args.GetShadingMode();
    UsdMayaTranslatorMaterial::AssignMaterial(shadingMode,
                                                 mesh,
                                                 meshObj,
                                                 context);

    // Mesh is a shape, so read Gprim properties
    UsdMayaTranslatorGprim::Read(mesh, meshObj, context);

    // Set normals if supplied
    MIntArray normalsFaceIds;
    if (normals.size() == static_cast<size_t>(meshFn.numFaceVertices())) {
        for (size_t i = 0u; i < polygonCounts.length(); ++i) {
            for (int j = 0; j < polygonCounts[i]; ++j) {
                normalsFaceIds.append(i);
            }
        }

        if (normalsFaceIds.length() == static_cast<size_t>(meshFn.numFaceVertices())) {
            MVectorArray mayaNormals(normals.size());
            for (size_t i = 0u; i < normals.size(); ++i) {
                mayaNormals.set(MVector(normals[i][0u],
                                        normals[i][1u],
                                        normals[i][2u]),
                                i);
            }

            meshFn.setFaceVertexNormals(mayaNormals,
                                        normalsFaceIds,
                                        polygonConnects);
        }
     }

    // Copy UsdGeomMesh schema attrs into Maya if they're authored.
    UsdMayaReadUtil::ReadSchemaAttributesFromPrim<UsdGeomMesh>(
        prim,
        meshFn.object(),
        {
            UsdGeomTokens->subdivisionScheme,
            UsdGeomTokens->interpolateBoundary,
            UsdGeomTokens->faceVaryingLinearInterpolation
        });

    // If we are dealing with polys, check if there are normals and set the
    // internal emit-normals tag so that the normals will round-trip.
    // If we are dealing with a subdiv, read additional subdiv tags.
    TfToken subdScheme;
    if (mesh.GetSubdivisionSchemeAttr().Get(&subdScheme) &&
            subdScheme == UsdGeomTokens->none) {
        if (normals.size() == static_cast<size_t>(meshFn.numFaceVertices()) &&
                mesh.GetNormalsInterpolation() == UsdGeomTokens->faceVarying) {
            UsdMayaMeshUtil::SetEmitNormalsTag(meshFn, true);
        }
    } else {
        _AssignSubDivTagsToMesh(mesh, meshObj, meshFn);
    }

    // Set Holes
    VtIntArray holeIndices;
    mesh.GetHoleIndicesAttr().Get(&holeIndices); // not animatable
    if (!holeIndices.empty()) {
        MUintArray mayaHoleIndices;
        mayaHoleIndices.setLength(holeIndices.size());
        for (size_t i = 0u; i < holeIndices.size(); ++i) {
            mayaHoleIndices[i] = holeIndices[i];
        }

        if (meshFn.setInvisibleFaces(mayaHoleIndices) == MS::kFailure) {
            TF_RUNTIME_ERROR("Unable to set Invisible Faces on <%s>",
                             meshFn.fullPathName().asChar());
        }
    }

    // GETTING PRIMVARS
    const std::vector<UsdGeomPrimvar> primvars = mesh.GetPrimvars();
    TF_FOR_ALL(iter, primvars) {
        const UsdGeomPrimvar& primvar = *iter;
        const TfToken name = primvar.GetBaseName();
        const TfToken fullName = primvar.GetPrimvarName();
        const SdfValueTypeName typeName = primvar.GetTypeName();
        const TfToken& interpolation = primvar.GetInterpolation();


        // Exclude primvars using the full primvar name without "primvars:".
        // This applies to all primvars; we don't care if it's a color set, a
        // UV set, etc.
        if (args.GetExcludePrimvarNames().count(fullName) != 0) {
            continue;
        }

        // If the primvar is called either displayColor or displayOpacity check
        // if it was really authored from the user.  It may not have been
        // authored by the user, for example if it was generated by shader
        // values and not an authored colorset/entity.
        // If it was not really authored, we skip the primvar.
        if (name == UsdMayaMeshColorSetTokens->DisplayColorColorSetName ||
                name == UsdMayaMeshColorSetTokens->DisplayOpacityColorSetName) {
            if (!UsdMayaRoundTripUtil::IsAttributeUserAuthored(primvar)) {
                continue;
            }
        }

        // XXX: Maya stores UVs in MFloatArrays and color set data in MColors
        // which store floats, so we currently only import primvars holding
        // float-typed arrays. Should we still consider other precisions
        // (double, half, ...) and/or numeric types (int)?
        if(typeName == SdfValueTypeNames->TexCoord2fArray ||
                (UsdMayaReadUtil::ReadFloat2AsUV() &&
                 typeName == SdfValueTypeNames->Float2Array)) {
            // Looks for TexCoord2fArray types for UV sets first
            // Otherwise, if env variable for reading Float2
            // as uv sets is turned on, we assume that Float2Array primvars
            // are UV sets.
            if (!_AssignUVSetPrimvarToMesh(primvar, meshFn)) {
                TF_WARN("Unable to retrieve and assign data for UV set <%s> on "
                        "mesh <%s>",
                        name.GetText(),
                        mesh.GetPrim().GetPath().GetText());
            }
        } else if (typeName == SdfValueTypeNames->FloatArray   ||
                   typeName == SdfValueTypeNames->Float3Array  ||
                   typeName == SdfValueTypeNames->Color3fArray ||
                   typeName == SdfValueTypeNames->Float4Array  ||
                   typeName == SdfValueTypeNames->Color4fArray) {
            if (!_AssignColorSetPrimvarToMesh(mesh, primvar, meshFn)) {
                TF_WARN("Unable to retrieve and assign data for color set <%s> "
                        "on mesh <%s>",
                        name.GetText(),
                        mesh.GetPrim().GetPath().GetText());
            }
        // 
        // constant primvars get added as attributes on mesh
        //
        } else if (interpolation == UsdGeomTokens->constant){
            if (!_AssignConstantPrimvarToMesh(primvar, meshFn)) {
                TF_WARN("Unable to assign constant primvars as attributes, <%s> for mesh <%s>",
                        name.GetText(),
                        mesh.GetPrim().GetPath().GetText());
            }
        }

    }

    // We only vizualize the colorset by default if it is "displayColor".
    MStringArray colorSetNames;
    if (meshFn.getColorSetNames(colorSetNames) == MS::kSuccess) {
        for (unsigned int i = 0u; i < colorSetNames.length(); ++i) {
            const MString colorSetName = colorSetNames[i];
            if (std::string(colorSetName.asChar())
                    == UsdMayaMeshColorSetTokens->DisplayColorColorSetName.GetString()) {
                const MFnMesh::MColorRepresentation csRep =
                    meshFn.getColorRepresentation(colorSetName);
                if (csRep == MFnMesh::kRGB || csRep == MFnMesh::kRGBA) {
                    // both of these are needed to show the colorset.
                    MPlug plg = meshFn.findPlug("displayColors");
                    if (!plg.isNull()) {
                        plg.setBool(true);
                    }
                    meshFn.setCurrentColorSetName(colorSetName);
                }
                break;
            }
        }
    }

    // Code below this point is for handling deforming meshes, so if we don't
    // have time samples to deal with, we're done.
    if (pointsNumTimeSamples == 0u) {
        return true;
    }

    // If we're using the imported USD as an animation cache, try to setup the
    // point based deformer for this prim. If that fails, we'll fallback on
    // creating a blend shape deformer.
    if (args.GetUseAsAnimationCache() &&
            _SetupPointBasedDeformerForMayaNode(meshObj, prim, context)) {
        return true;
    }

    // Use blendShapeDeformer so that all the points for a frame are contained
    // in a single node.
    //
    MPointArray mayaAnimPoints(mayaNumVertices);
    MObject meshAnimObj;

    MFnBlendShapeDeformer blendFn;
    MObject blendObj = blendFn.create(meshObj);
    if (context) {
        context->RegisterNewMayaNode(blendFn.name().asChar(), blendObj); // used for undo/redo
    }

    for (unsigned int ti = 0u; ti < pointsNumTimeSamples; ++ti) {
        mesh.GetPointsAttr().Get(&points, pointsTimeSamples[ti]);

        for (unsigned int i = 0u; i < mayaNumVertices; ++i) {
            mayaAnimPoints.set(i, points[i][0], points[i][1], points[i][2]);
        }

        // == Create Mesh Shape Node
        MFnMesh meshFn;
        if (meshAnimObj.isNull()) {
            meshAnimObj = meshFn.create(mayaAnimPoints.length(),
                                        polygonCounts.length(),
                                        mayaAnimPoints,
                                        polygonCounts,
                                        polygonConnects,
                                        mayaNodeTransformObj,
                                        &status);
            if (status != MS::kSuccess) {
                continue;
            }
        }
        else {
            // Reuse the already created mesh by copying it and then setting the points
            meshAnimObj = meshFn.copy(meshAnimObj, mayaNodeTransformObj, &status);
            meshFn.setPoints(mayaAnimPoints);
        }

        // Set normals if supplied
        //
        // NOTE: This normal information is not propagated through the blendShapes, only the controlPoints.
        //
        mesh.GetNormalsAttr().Get(&normals, pointsTimeSamples[ti]);
        if (normals.size() == static_cast<size_t>(meshFn.numFaceVertices()) &&
                normalsFaceIds.length() == static_cast<size_t>(meshFn.numFaceVertices())) {
            MVectorArray mayaNormals(normals.size());
            for (size_t i = 0; i < normals.size(); ++i) {
                mayaNormals.set(MVector(normals[i][0u],
                                        normals[i][1u],
                                        normals[i][2u]),
                                i);
            }

            meshFn.setFaceVertexNormals(mayaNormals,
                                        normalsFaceIds,
                                        polygonConnects);
        }

        // Add as target and set as an intermediate object. We do *not*
        // register the mesh object for undo/redo, since it will be handled
        // automatically by deleting the blend shape deformer object.
        blendFn.addTarget(meshObj, ti, meshAnimObj, 1.0);
        meshFn.setIntermediateObject(true);
    }

    // Animate the weights so that mesh0 has a weight of 1 at frame 0, etc.
    MFnAnimCurve animFn;

    // Construct the time array to be used for all the keys
    MTimeArray timeArray;
    timeArray.setLength(pointsNumTimeSamples);
    for (unsigned int ti = 0u; ti < pointsNumTimeSamples; ++ti) {
        timeArray.set(MTime(pointsTimeSamples[ti]), ti);
    }

    // Key/Animate the weights
    MPlug plgAry = blendFn.findPlug("weight");
    if (!plgAry.isNull() && plgAry.isArray()) {
        for (unsigned int ti = 0u; ti < pointsNumTimeSamples; ++ti) {
            MPlug plg = plgAry.elementByLogicalIndex(ti, &status);
            MDoubleArray valueArray(pointsNumTimeSamples, 0.0);
            valueArray[ti] = 1.0; // Set the time value where this mesh's weight should be 1.0
            MObject animObj = animFn.create(plg, nullptr, &status);
            animFn.addKeys(&timeArray, &valueArray);
            // We do *not* register the anim curve object for undo/redo,
            // since it will be handled automatically by deleting the blend
            // shape deformer object.
        }
    }

    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
