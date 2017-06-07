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
#include "usdMaya/translatorMesh.h"

#include "usdMaya/meshUtil.h"
#include "usdMaya/roundTripUtil.h"
#include "usdMaya/translatorGprim.h"
#include "usdMaya/translatorMaterial.h"
#include "usdMaya/translatorUtil.h"

#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvar.h"

#include <maya/MFnAnimCurve.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MPointArray.h>
#include <maya/MUintArray.h>

PXR_NAMESPACE_OPEN_SCOPE



/* static */
bool 
PxrUsdMayaTranslatorMesh::Create(
        const UsdGeomMesh& mesh,
        MObject parentNode,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context)
{
    if (!mesh) {
        return false;
    }

    const UsdPrim& prim = mesh.GetPrim();

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
    VtArray<GfVec3f> normals;
    VtArray<int>     faceVertexCounts;
    VtArray<int>     faceVertexIndices;
    
    UsdAttribute fvc = mesh.GetFaceVertexCountsAttr();
    if (fvc.ValueMightBeTimeVarying()){
        // at some point, it would be great, instead of failing, to create a usd/hydra proxy node
        // for the mesh, perhaps?  For now, better to give a more specific error
        MGlobal::displayError(
            TfStringPrintf("<%s> is a topologically varying Mesh (animated faceVertexCounts). Skipping...", 
                prim.GetPath().GetText()).c_str());
        return false;
    } else {
        // for any non-topo-varying mesh, sampling at zero will get us the right answer
        fvc.Get(&faceVertexCounts, 0);
    }

    UsdAttribute fvi = mesh.GetFaceVertexIndicesAttr();
    if (fvi.ValueMightBeTimeVarying()){
        // at some point, it would be great, instead of failing, to create a usd/hydra proxy node
        // for the mesh, perhaps?  For now, better to give a more specific error
        MGlobal::displayError(
            TfStringPrintf("<%s> is a topologically varying Mesh (animated faceVertexIndices). Skipping...", 
                prim.GetPath().GetText()).c_str());
        return false;
    } else {
        // for any non-topo-varying mesh, sampling at zero will get us the right answer
        fvi.Get(&faceVertexIndices, 0);
    }
        
    // Sanity Checks. If the vertex arrays are empty, skip this mesh
    if (faceVertexCounts.size() == 0 || faceVertexIndices.size() == 0) {
        MGlobal::displayError(
            TfStringPrintf("FaceVertex arrays are empty [Count:%zu Indices:%zu] on Mesh <%s>. Skipping...", 
                faceVertexCounts.size(), faceVertexIndices.size(), 
                prim.GetPath().GetText()).c_str());
        return false; // invalid mesh, so exit
    }

    // Gather points and normals
    // If args.GetReadAnimData() is TRUE,
    // pick the first avaiable sample or default
    UsdTimeCode pointsTimeSample=UsdTimeCode::EarliestTime();
    UsdTimeCode normalsTimeSample=UsdTimeCode::EarliestTime();
    std::vector<double> pointsTimeSamples;
    size_t pointsNumTimeSamples = 0;
    if (args.GetReadAnimData()) {
        PxrUsdMayaTranslatorUtil::GetTimeSamples(mesh.GetPointsAttr(), args,
                &pointsTimeSamples);
        pointsNumTimeSamples = pointsTimeSamples.size();
        if (pointsNumTimeSamples>0) {
            pointsTimeSample = pointsTimeSamples[0];
        }
    	std::vector<double> normalsTimeSamples;
        PxrUsdMayaTranslatorUtil::GetTimeSamples(mesh.GetNormalsAttr(), args,
                &normalsTimeSamples);
        if (normalsTimeSamples.size()) {
            normalsTimeSample = normalsTimeSamples[0];
        }
    }
    mesh.GetPointsAttr().Get(&points, pointsTimeSample);
    mesh.GetNormalsAttr().Get(&normals, normalsTimeSample);
    
    if (points.size() == 0) {
        MGlobal::displayError(
            TfStringPrintf("Points arrays is empty on Mesh <%s>. Skipping...", 
                prim.GetPath().GetText()).c_str());
        return false; // invalid mesh, so exit
    }


    // == Convert data
    size_t mayaNumVertices = points.size();
    MPointArray mayaPoints(mayaNumVertices);
    for (size_t i=0; i < mayaNumVertices; i++) {
        mayaPoints.set( i, points[i][0], points[i][1], points[i][2] );
    }

    MIntArray polygonCounts( faceVertexCounts.cdata(),  faceVertexCounts.size() );
    MIntArray polygonConnects( faceVertexIndices.cdata(), faceVertexIndices.size() );

    // == Create Mesh Shape Node
    MFnMesh meshFn;
    MObject meshObj = meshFn.create(mayaPoints.length(), 
                           polygonCounts.length(), 
                           mayaPoints, 
                           polygonCounts, 
                           polygonConnects,
                           mayaNodeTransformObj,
                           &status
                           );
                           
    if (status != MS::kSuccess) {
        return false;
    }

    // Since we are "decollapsing", we will create a xform and a shape node for each USD prim
    std::string usdPrimName(prim.GetName().GetText());
    std::string shapeName(usdPrimName); shapeName += "Shape";
    // Set mesh name and register
    meshFn.setName(MString(shapeName.c_str()), false, &status);
    if (context) {
        std::string usdPrimPath(prim.GetPath().GetText());
        std::string shapePath(usdPrimPath);
        shapePath += "/";
        shapePath += shapeName;
        context->RegisterNewMayaNode( shapePath, meshObj ); // used for undo/redo
    }

    // If a material is bound, create (or reuse if already present) and assign it
    // If no binding is present, assign the mesh to the default shader    
    const TfToken& shadingMode = args.GetShadingMode();
    PxrUsdMayaTranslatorMaterial::AssignMaterial(shadingMode, mesh, meshObj,
            context);
    
    // Mesh is a shape, so read Gprim properties
    PxrUsdMayaTranslatorGprim::Read(mesh, meshObj, context);

    // Set normals if supplied
    MIntArray normalsFaceIds;
    if (normals.size() == static_cast<size_t>(meshFn.numFaceVertices())) {

        for (size_t i=0; i < polygonCounts.length(); i++) {
            for (int j=0; j < polygonCounts[i]; j++) {
                normalsFaceIds.append(i);
            }
        }
        if (normalsFaceIds.length() == static_cast<size_t>(meshFn.numFaceVertices())) {
            MVectorArray mayaNormals(normals.size());
            for (size_t i=0; i < normals.size(); i++) {
                mayaNormals.set( MVector(normals[i][0], normals[i][1], normals[i][2]), i);
            }
            if (meshFn.setFaceVertexNormals(mayaNormals, normalsFaceIds, polygonConnects) != MS::kSuccess) {
            }
        }
     }

    // Determine if PolyMesh or SubdivMesh
    TfToken subdScheme = PxrUsdMayaMeshUtil::setSubdivScheme(mesh, meshFn, args.GetDefaultMeshScheme());

    // If we are dealing with polys, check if there are normals
    // If we are dealing with SubdivMesh, read additional attributes and SubdivMesh properties
    if (subdScheme == UsdGeomTokens->none) {
        if (normals.size() == static_cast<size_t>(meshFn.numFaceVertices())) {
            PxrUsdMayaMeshUtil::setEmitNormals(mesh, meshFn, UsdGeomTokens->none);
        }
    } else {
        PxrUsdMayaMeshUtil::setSubdivInterpBoundary(mesh, meshFn, UsdGeomTokens->edgeAndCorner);
        PxrUsdMayaMeshUtil::setSubdivFVLinearInterpolation(mesh, meshFn);
        _AssignSubDivTagsToMesh(mesh, meshObj, meshFn);
    }
 
    // Set Holes
    VtArray<int> holeIndices;
    mesh.GetHoleIndicesAttr().Get(&holeIndices);   // not animatable
    if ( holeIndices.size() != 0 ) {
        MUintArray mayaHoleIndices;
        mayaHoleIndices.setLength( holeIndices.size() );
        for (size_t i=0; i < holeIndices.size(); i++) {
            mayaHoleIndices[i] = holeIndices[i];
        }
        if (meshFn.setInvisibleFaces(mayaHoleIndices) == MS::kFailure) {
            MGlobal::displayError(TfStringPrintf("Unable to set Invisible Faces on <%s>", 
                            meshFn.fullPathName().asChar()).c_str());
        }
    }

    // GETTING PRIMVARS
    std::vector<UsdGeomPrimvar> primvars = mesh.GetPrimvars();
    TF_FOR_ALL(iter, primvars) {
        const UsdGeomPrimvar& primvar = *iter;
        const TfToken& name = primvar.GetBaseName();
        const SdfValueTypeName& typeName = primvar.GetTypeName();

        // If the primvar is called either displayColor or displayOpacity check
        // if it was really authored from the user.  It may not have been
        // authored by the user, for example if it was generated by shader
        // values and not an authored colorset/entity.
        // If it was not really authored, we skip the primvar.
        if (name == PxrUsdMayaMeshColorSetTokens->DisplayColorColorSetName || 
                name == PxrUsdMayaMeshColorSetTokens->DisplayOpacityColorSetName) {
            if (!PxrUsdMayaRoundTripUtil::IsAttributeUserAuthored(primvar)) {
                continue;
            }
        }

        // XXX: Maya stores UVs in MFloatArrays and color set data in MColors
        // which store floats, so we currently only import primvars holding
        // float-typed arrays. Should we still consider other precisions
        // (double, half, ...) and/or numeric types (int)?
        if (typeName == SdfValueTypeNames->Float2Array) {
            // We assume that Float2Array primvars are UV sets.
            if (!_AssignUVSetPrimvarToMesh(primvar, meshFn)) {
                MGlobal::displayWarning(
                    TfStringPrintf("Unable to retrieve and assign data for UV set <%s> on mesh <%s>", 
                                   name.GetText(),
                                   mesh.GetPrim().GetPath().GetText()).c_str());
            }
        } else if (typeName == SdfValueTypeNames->FloatArray   || 
                   typeName == SdfValueTypeNames->Float3Array  || 
                   typeName == SdfValueTypeNames->Color3fArray ||
                   typeName == SdfValueTypeNames->Float4Array  || 
                   typeName == SdfValueTypeNames->Color4fArray) {
            if (!_AssignColorSetPrimvarToMesh(mesh, primvar, meshFn)) {
                MGlobal::displayWarning(
                    TfStringPrintf("Unable to retrieve and assign data for color set <%s> on mesh <%s>",
                                   name.GetText(),
                                   mesh.GetPrim().GetPath().GetText()).c_str());
            }
        }
    }

    // We only vizualize the colorset by default if it is "displayColor".  
    MStringArray colorSetNames;
    if (meshFn.getColorSetNames(colorSetNames)==MS::kSuccess) {
        for (unsigned int i=0; i < colorSetNames.length(); i++) {
            const MString colorSetName = colorSetNames[i];
            if (std::string(colorSetName.asChar()) 
                    == PxrUsdMayaMeshColorSetTokens->DisplayColorColorSetName.GetString()) {
                MFnMesh::MColorRepresentation csRep=
                    meshFn.getColorRepresentation(colorSetName);
                if (csRep==MFnMesh::kRGB || csRep==MFnMesh::kRGBA) {

                    // both of these are needed to show the colorset.
                    MPlug plg=meshFn.findPlug("displayColors");
                    if ( !plg.isNull() ) {
                        plg.setBool(true);
                    }
                    meshFn.setCurrentColorSetName(colorSetName);
                }
                break;
            }
        }
    }
    
    // == Animate points ==
    //   Use blendShapeDeformer so that all the points for a frame are contained in a single node
    //
    if (pointsNumTimeSamples > 0) {
        MPointArray mayaPoints(mayaNumVertices);
        MObject meshAnimObj;

        MFnBlendShapeDeformer blendFn;
        MObject blendObj = blendFn.create(meshObj);
        if (context) {
            context->RegisterNewMayaNode( blendFn.name().asChar(), blendObj ); // used for undo/redo
        }

        for (unsigned int ti=0; ti < pointsNumTimeSamples; ++ti) {
             mesh.GetPointsAttr().Get(&points, pointsTimeSamples[ti]);

            for (unsigned int i=0; i < mayaNumVertices; i++) {
                mayaPoints.set( i, points[i][0], points[i][1], points[i][2] );
            }

            // == Create Mesh Shape Node
            MFnMesh meshFn;
            if ( meshAnimObj.isNull() ) {
                meshAnimObj = meshFn.create(mayaPoints.length(), 
                                       polygonCounts.length(), 
                                       mayaPoints, 
                                       polygonCounts, 
                                       polygonConnects,
                                       mayaNodeTransformObj,
                                       &status
                                       );
                if (status != MS::kSuccess) {
                    continue;
                }
            }
            else {
                // Reuse the already created mesh by copying it and then setting the points
                meshAnimObj = meshFn.copy(meshAnimObj, mayaNodeTransformObj, &status);
                meshFn.setPoints(mayaPoints);
            }

            // Set normals if supplied
            //
            // NOTE: This normal information is not propagated through the blendShapes, only the controlPoints.
            //
             mesh.GetNormalsAttr().Get(&normals, pointsTimeSamples[ti]);
             if (normals.size() == static_cast<size_t>(meshFn.numFaceVertices()) &&
                normalsFaceIds.length() == static_cast<size_t>(meshFn.numFaceVertices())) {

                MVectorArray mayaNormals(normals.size());
                for (size_t i=0; i < normals.size(); i++) {
                    mayaNormals.set( MVector(normals[i][0], normals[i][1], normals[i][2]), i);
                }
                if (meshFn.setFaceVertexNormals(mayaNormals, normalsFaceIds, polygonConnects) != MS::kSuccess) {
                }
             }

            // Add as target and set as an intermediate object
            blendFn.addTarget(meshObj, ti, meshAnimObj, 1.0);
            meshFn.setIntermediateObject(true);
            if (context) {
                context->RegisterNewMayaNode( meshFn.fullPathName().asChar(), meshAnimObj ); // used for undo/redo
            }
        }

        // Animate the weights so that mesh0 has a weight of 1 at frame 0, etc.
        MFnAnimCurve animFn;

        // Construct the time array to be used for all the keys
        MTimeArray timeArray;
        timeArray.setLength(pointsNumTimeSamples);
        for (unsigned int ti=0; ti < pointsNumTimeSamples; ++ti) {
            timeArray.set( MTime(pointsTimeSamples[ti]), ti);
        }

        // Key/Animate the weights
        MPlug plgAry = blendFn.findPlug( "weight" );
        if ( !plgAry.isNull() && plgAry.isArray() ) {
            for (unsigned int ti=0; ti < pointsNumTimeSamples; ++ti) {
                MPlug plg = plgAry.elementByLogicalIndex(ti, &status);
                MDoubleArray valueArray(pointsNumTimeSamples, 0.0);
                valueArray[ti] = 1.0; // Set the time value where this mesh's weight should be 1.0
                MObject animObj = animFn.create(plg, NULL, &status);
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

