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

#include "usdMaya/util.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usdUtils/pipeline.h"

#include <maya/MFloatArray.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MItMeshFaceVertex.h>


/* static */
bool
PxrUsdMayaTranslatorMesh::_AssignUVSetPrimvarToMesh(
        const UsdGeomPrimvar& primvar,
        MFnMesh& meshFn)
{
    const TfToken& primvarName = primvar.GetBaseName();

    // Get the raw data before applying any indexing.
    VtVec2fArray uvValues;
    if (not primvar.Get(&uvValues) or uvValues.empty()) {
        MGlobal::displayWarning(
            TfStringPrintf("Could not read UV values from primvar '%s' on mesh: %s",
                           primvarName.GetText(),
                           meshFn.fullPathName().asChar()).c_str());
        return false;
    }

    // This is the number of UV values assuming the primvar is NOT indexed.
    size_t numUVs = uvValues.size();

    VtIntArray assignmentIndices;
    int unauthoredValuesIndex = -1;
    if (primvar.GetIndices(&assignmentIndices)) {
        // The primvar IS indexed, so the indices array is what determines the
        // number of UV values.
        numUVs = assignmentIndices.size();
        unauthoredValuesIndex = primvar.GetUnauthoredValuesIndex();
    }

    // Go through the UV data and add the U and V values to separate
    // MFloatArrays, taking into consideration that indexed data may have been
    // authored sparsely. If the assignmentIndices array is empty then the data
    // is NOT indexed, in which case we can use it directly.
    // Note that with indexed data, the data is added to the arrays in ascending
    // component ID order according to the primvar's interpolation (ascending
    // face ID for uniform interpolation, ascending vertex ID for vertex
    // interpolation, etc.). This ordering may be different from the way the
    // values are ordered in the primvar. Because of this, we recycle the
    // assignmentIndices array as we go to store the new mapping from component
    // index to UV index.
    MFloatArray uCoords;
    MFloatArray vCoords;
    for (size_t i = 0; i < numUVs; ++i) {
        size_t uvIndex = i;

        if (i < assignmentIndices.size()) {
            // The data is indexed, so consult the indices array for the
            // correct index into the data.
            uvIndex = assignmentIndices[i];

            if (uvIndex == unauthoredValuesIndex) {
                // This component is unauthored, so just update the
                // mapping in assignmentIndices and then skip the value.
                // We don't actually use the value at the unassigned index.
                assignmentIndices[i] = -1;
                continue;
            }

            // We'll be appending a new value, so the current length of the
            // array gives us the new value's index.
            assignmentIndices[i] = uCoords.length();
        }

        uCoords.append(uvValues[uvIndex][0]);
        vCoords.append(uvValues[uvIndex][1]);
    }

    // uCoords and vCoords now store all of the values and any unassigned
    // components have had their indices set to -1, so update the unauthored
    // values index.
    unauthoredValuesIndex = -1;

    MStatus status;
    MString uvSetName(primvarName.GetText());
    if (primvarName == UsdUtilsGetPrimaryUVSetName()) {
        // We assume that the primary USD UV set maps to Maya's default 'map1'
        // set which always exists, so we shouldn't try to create it.
        uvSetName = "map1";
    } else {
        status = meshFn.createUVSet(uvSetName);
        if (status != MS::kSuccess) {
            MGlobal::displayWarning(
                TfStringPrintf("Unable to create UV set '%s' for mesh: %s",
                               uvSetName.asChar(),
                               meshFn.fullPathName().asChar()).c_str());
            return false;
        }
    }

    // Create UVs on the mesh from the values we collected out of the primvar.
    // We'll assign mesh components to these values below.
    status = meshFn.setUVs(uCoords, vCoords, &uvSetName);
    if (status != MS::kSuccess) {
        MGlobal::displayWarning(
            TfStringPrintf("Unable to set UV data on UV set '%s' for mesh: %s",
                           uvSetName.asChar(),
                           meshFn.fullPathName().asChar()).c_str());
        return false;
    }

    const TfToken& interpolation = primvar.GetInterpolation();

    // Iterate through the mesh's face vertices assigning UV values to them.
    MItMeshFaceVertex itFV(meshFn.object());
    unsigned int fvi = 0;
    for (itFV.reset(); not itFV.isDone(); itFV.next(), ++fvi) {
        int faceId = itFV.faceId();
        int vertexId = itFV.vertId();
        int faceVertexId = itFV.faceVertId();

        // Primvars with constant or uniform interpolation are not really
        // meaningful as UV sets, but we support them anyway.
        int uvId = 0;
        if (interpolation == UsdGeomTokens->constant) {
            uvId = 0;
        } else if (interpolation == UsdGeomTokens->uniform) {
            uvId = faceId;
        } else if (interpolation == UsdGeomTokens->vertex) {
            uvId = vertexId;
        } else if (interpolation == UsdGeomTokens->faceVarying) {
            uvId = fvi;
        }

        if (uvId < assignmentIndices.size()) {
            // The data is indexed, so consult the indices array for the
            // correct index into the data.
            uvId = assignmentIndices[uvId];

            if (uvId == unauthoredValuesIndex) {
                // This component had no authored value, so skip it.
                continue;
            }
        }

        status = meshFn.assignUV(faceId, faceVertexId, uvId, &uvSetName);
        if (status != MS::kSuccess) {
            MGlobal::displayWarning(
                TfStringPrintf("Could not assign UV value to UV set '%s' on mesh: %s",
                               uvSetName.asChar(),
                               meshFn.fullPathName().asChar()).c_str());
            return false;
        }
    }

    return true;
}

/* static */
bool 
PxrUsdMayaTranslatorMesh::_AssignColorSetPrimvarToMesh(const UsdGeomMesh &primSchema, const UsdGeomPrimvar &primvar, MFnMesh &meshFn, MIntArray &polygonCounts, MIntArray &polygonConnects, MFnMesh::MColorRepresentation colorRep)
{
    TfToken name, interpolation;
    SdfValueTypeName typeName;
    int elementSize;
    primvar.GetDeclarationInfo(&name, &typeName, &interpolation, &elementSize);
    
    // If the primvar is displayOpacity and is a float array
    // Check if displayColor is authored.
    // If not, rename displayOpacity to displayColor and create it on the mesh
    // This supports cases where the user created a single channel value for displayColor
    // XXX for now we are not able to fully recover authored RGBA displayColor
    if (name == "displayOpacity" and typeName == SdfValueTypeNames->FloatArray) {
        if (not PxrUsdMayaUtil::GetBoolCustomData(primSchema.GetDisplayColorPrimvar(), TfToken("Authored"), false)) {
            name = TfToken("displayColor");
        } else {
            return true;
        }
    }

    VtValue vtValue;
    if (primvar.Get(&vtValue, UsdTimeCode::Default())) {
        MColorArray colorArray;
        switch(colorRep) {
            case MFnMesh::kAlpha: {
                VtArray<float> floatData;
                if (vtValue.IsHolding<VtArray<float> >() and 
                    primvar.ComputeFlattened(&floatData)) 
                {    
                    colorArray.setLength( floatData.size() );
                    for (unsigned int i=0; i < floatData.size(); i++) { 
                        colorArray.set(i, 1.0, 1.0, 1.0, floatData[i]);
                    }
                } else {
                    MGlobal::displayWarning(TfStringPrintf("Single channel "
                        "colorset primvar <%s> doesn't hold an array of float", 
                        name.GetText()).c_str());
                }
            } break;
            case MFnMesh::kRGB: {
                VtArray<GfVec3f> RGBData;
                if (vtValue.IsHolding<VtArray<GfVec3f>>() and 
                    primvar.ComputeFlattened(&RGBData)) 
                {
                    bool convertToDisplay= (name == "displayColor");

                    colorArray.setLength( RGBData.size() );
                    GfVec3f color;
                    for (unsigned int i=0; i < RGBData.size(); i++) {
                        if (convertToDisplay) {
                            color = GfConvertLinearToDisplay(RGBData[i]);
                        } else {
                            color = RGBData[i];
                        }
                        colorArray.set(i, color[0], color[1], color[2], 1.0);
                    }
                } else {
                    MGlobal::displayWarning(TfStringPrintf("RGB colorset primvar <%s> doesn't hold an array of Vec3f", name.GetText()).c_str());
                }
            } break;
            case MFnMesh::kRGBA: {
                VtArray<GfVec4f> RGBData;
                if (vtValue.IsHolding<VtArray<GfVec4f>>() and 
                    primvar.ComputeFlattened(&RGBData)) {

                    colorArray.setLength( RGBData.size() );
                    GfVec4f color;
                    for (unsigned int i=0; i < RGBData.size(); i++) { 
                        color = GfConvertLinearToDisplay(RGBData[i]);
                        colorArray.set(i, color[0], color[1], color[2], color[3]);
                    }
                } else {
                    MGlobal::displayWarning(TfStringPrintf("RGBA colorset primvar <%s> doesn't hold an array of Vec4f", name.GetText()).c_str());
                }
            } break;
        }
        
        if (colorArray.length()) {
            MString colorSetName(name.GetText());
            // When the Clamped custom data is not present, we assume unclamped colorset
            bool clamped = PxrUsdMayaUtil::GetBoolCustomData(primvar.GetAttr(), TfToken("Clamped"), false);
            if (interpolation == UsdGeomTokens->uniform &&
                colorArray.length() == static_cast<size_t>(meshFn.numPolygons())) {
                MIntArray faceIds;
                faceIds.setLength( colorArray.length() );
                for (size_t i=0; i < faceIds.length(); i++) {
                    faceIds.set(i,i);
                }
                meshFn.createColorSet(colorSetName, NULL, clamped, colorRep);
                meshFn.setCurrentColorSetName(colorSetName);
                meshFn.setFaceColors(colorArray, faceIds, colorRep); // per-vertex colors
                return true;
            }
            if (interpolation == UsdGeomTokens->vertex &&
                colorArray.length() == static_cast<size_t>(meshFn.numVertices())) {
                MIntArray vertIds;
                vertIds.setLength( colorArray.length() );
                for (size_t i=0; i < vertIds.length(); i++) {
                    vertIds.set(i,i);
                }
                meshFn.createColorSet(colorSetName, NULL, clamped, colorRep);
                meshFn.setCurrentColorSetName(colorSetName);
                meshFn.setVertexColors(colorArray, vertIds, NULL, colorRep); // per-vertex colors
                return true;
            }
            if (interpolation == UsdGeomTokens->faceVarying &&
                colorArray.length() == static_cast<size_t>(meshFn.numFaceVertices())) {
                MIntArray faceIds;
                for (size_t i=0; i < polygonCounts.length(); i++) {
                    for (int j=0;j<polygonCounts[i];j++) {
                        faceIds.append(i);
                    }
                }
                if (faceIds.length() == static_cast<size_t>(meshFn.numFaceVertices())) {
                    meshFn.createColorSet(colorSetName, NULL, clamped, colorRep);
                    meshFn.setCurrentColorSetName(colorSetName);
                    meshFn.setFaceVertexColors(colorArray, faceIds, polygonConnects, NULL, colorRep); // per-vertex colors
                    return true;
                }
            }
        }
    } else {
        MGlobal::displayWarning(TfStringPrintf("Unable to get primvar data for colorset <%s>", name.GetText()).c_str());
    }
    return false;
}
