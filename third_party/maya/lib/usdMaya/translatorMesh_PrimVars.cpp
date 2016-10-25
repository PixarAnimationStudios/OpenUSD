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
#include <maya/MIntArray.h>
#include <maya/MItMeshFaceVertex.h>


static
MIntArray
_GetMayaFaceVertexAssignmentIds(
        const MFnMesh& meshFn,
        const TfToken& interpolation,
        const VtIntArray& assignmentIndices,
        const int unauthoredValuesIndex)
{
    MIntArray valueIds(meshFn.numFaceVertices(), -1);

    MItMeshFaceVertex itFV(meshFn.object());
    unsigned int fvi = 0;
    for (itFV.reset(); not itFV.isDone(); itFV.next(), ++fvi) {
        int valueId = 0;
        if (interpolation == UsdGeomTokens->constant) {
            valueId = 0;
        } else if (interpolation == UsdGeomTokens->uniform) {
            valueId = itFV.faceId();
        } else if (interpolation == UsdGeomTokens->vertex) {
            valueId = itFV.vertId();
        } else if (interpolation == UsdGeomTokens->faceVarying) {
            valueId = fvi;
        }

        if (static_cast<size_t>(valueId) < assignmentIndices.size()) {
            // The data is indexed, so consult the indices array for the
            // correct index into the data.
            valueId = assignmentIndices[valueId];

            if (valueId == unauthoredValuesIndex) {
                // This component had no authored value, so leave it unassigned.
                continue;
            }
        }

        valueIds[fvi] = valueId;
    }

    return valueIds;
}

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
                           primvar.GetAttr().GetPrimPath().GetText()).c_str());
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
        int uvIndex = i;

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

    // The following two lines should have no effect on user-visible state but
    // prevent a Maya crash in MFnMesh.setUVs after creating a crease set.
    // XXX this workaround is needed pending a fix by Autodesk.
    MString currentSet = meshFn.currentUVSetName();
    meshFn.setCurrentUVSetName(currentSet);

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

    // Build an array of value assignments for each face vertex in the mesh.
    // Any assignments left as -1 will not be assigned a value.
    MIntArray uvIds = _GetMayaFaceVertexAssignmentIds(meshFn,
                                                      interpolation,
                                                      assignmentIndices,
                                                      unauthoredValuesIndex);

    MIntArray vertexCounts;
    MIntArray vertexList;
    status = meshFn.getVertices(vertexCounts, vertexList);
    if (status != MS::kSuccess) {
        MGlobal::displayWarning(
            TfStringPrintf("Could not get vertex counts for UV set '%s' on mesh: %s",
                           uvSetName.asChar(),
                           meshFn.fullPathName().asChar()).c_str());
        return false;
    }

    status = meshFn.assignUVs(vertexCounts, uvIds, &uvSetName);
    if (status != MS::kSuccess) {
        MGlobal::displayWarning(
            TfStringPrintf("Could not assign UV values to UV set '%s' on mesh: %s",
                           uvSetName.asChar(),
                           meshFn.fullPathName().asChar()).c_str());
        return false;
    }

    return true;
}

/* static */
bool
PxrUsdMayaTranslatorMesh::_AssignColorSetPrimvarToMesh(
        const UsdGeomMesh& primSchema,
        const UsdGeomPrimvar& primvar,
        MFnMesh& meshFn)
{
    const TfToken& primvarName = primvar.GetBaseName();
    const SdfValueTypeName& typeName = primvar.GetTypeName();

    MString colorSetName(primvarName.GetText());

    // If the primvar is displayOpacity and it is a FloatArray, check if
    // displayColor is authored. If not, we'll import this 'displayOpacity'
    // primvar as a 'displayColor' color set. This supports cases where the
    // user created a single channel value for displayColor.
    // Note that if BOTH displayColor and displayOpacity are authored, they will
    // be imported as separate color sets. We do not attempt to combine them
    // into a single color set.
    if (primvarName == PxrUsdMayaMeshColorSetTokens->DisplayOpacityColorSetName and
            typeName == SdfValueTypeNames->FloatArray) {
        if (not PxrUsdMayaUtil::GetBoolCustomData(primSchema.GetDisplayColorPrimvar(),
                                                  PxrUsdMayaMeshColorSetTokens->Authored,
                                                  false)) {
            colorSetName = PxrUsdMayaMeshColorSetTokens->DisplayColorColorSetName.GetText();
        }
    }

    // We'll need to convert colors from linear to display if this color set is
    // for display colors.
    const bool convertToDisplay =
        (colorSetName == PxrUsdMayaMeshColorSetTokens->DisplayColorColorSetName.GetText());

    // Get the raw data before applying any indexing. We'll only populate one
    // of these arrays based on the primvar's typeName, and we'll also set the
    // color representation so we know which array to use later.
    VtFloatArray alphaArray;
    VtVec3fArray rgbArray;
    VtVec4fArray rgbaArray;
    MFnMesh::MColorRepresentation colorRep;
    size_t numValues = 0;

    MStatus status = MS::kSuccess;

    if (typeName == SdfValueTypeNames->FloatArray) {
        colorRep = MFnMesh::kAlpha;
        if (not primvar.Get(&alphaArray) or alphaArray.empty()) {
            status = MS::kFailure;
        } else {
            numValues = alphaArray.size();
        }
    } else if (typeName == SdfValueTypeNames->Float3Array or
               typeName == SdfValueTypeNames->Color3fArray) {
        colorRep = MFnMesh::kRGB;
        if (not primvar.Get(&rgbArray) or rgbArray.empty()) {
            status = MS::kFailure;
        } else {
            numValues = rgbArray.size();
        }
    } else if (typeName == SdfValueTypeNames->Float4Array or
               typeName == SdfValueTypeNames->Color4fArray) {
        colorRep = MFnMesh::kRGBA;
        if (not primvar.Get(&rgbaArray) or rgbaArray.empty()) {
            status = MS::kFailure;
        } else {
            numValues = rgbaArray.size();
        }
    } else {
        MGlobal::displayWarning(
            TfStringPrintf("Unsupported color set primvar type '%s' for primvar '%s' on mesh: %s",
                           typeName.GetAsToken().GetText(),
                           primvarName.GetText(),
                           primvar.GetAttr().GetPrimPath().GetText()).c_str());
        return false;
    }

    if (status != MS::kSuccess or numValues == 0) {
        MGlobal::displayWarning(
            TfStringPrintf("Could not read color set values from primvar '%s' on mesh: %s",
                           primvarName.GetText(),
                           primvar.GetAttr().GetPrimPath().GetText()).c_str());
        return false;
    }

    VtIntArray assignmentIndices;
    int unauthoredValuesIndex = -1;
    if (primvar.GetIndices(&assignmentIndices)) {
        // The primvar IS indexed, so the indices array is what determines the
        // number of color values.
        numValues = assignmentIndices.size();
        unauthoredValuesIndex = primvar.GetUnauthoredValuesIndex();
    }

    // Go through the color data and translate the values into MColors in the
    // colorArray, taking into consideration that indexed data may have been
    // authored sparsely. If the assignmentIndices array is empty then the data
    // is NOT indexed.
    // Note that with indexed data, the data is added to the arrays in ascending
    // component ID order according to the primvar's interpolation (ascending
    // face ID for uniform interpolation, ascending vertex ID for vertex
    // interpolation, etc.). This ordering may be different from the way the
    // values are ordered in the primvar. Because of this, we recycle the
    // assignmentIndices array as we go to store the new mapping from component
    // index to color index.
    MColorArray colorArray;
    for (size_t i = 0; i < numValues; ++i) {
        int valueIndex = i;

        if (i < assignmentIndices.size()) {
            // The data is indexed, so consult the indices array for the
            // correct index into the data.
            valueIndex = assignmentIndices[i];

            if (valueIndex == unauthoredValuesIndex) {
                // This component is unauthored, so just update the
                // mapping in assignmentIndices and then skip the value.
                // We don't actually use the value at the unassigned index.
                assignmentIndices[i] = -1;
                continue;
            }

            // We'll be appending a new value, so the current length of the
            // array gives us the new value's index.
            assignmentIndices[i] = colorArray.length();
        }

        GfVec4f colorValue(1.0);

        switch(colorRep) {
            case MFnMesh::kAlpha:
                colorValue[3] = alphaArray[valueIndex];
                break;
            case MFnMesh::kRGB:
                colorValue[0] = rgbArray[valueIndex][0];
                colorValue[1] = rgbArray[valueIndex][1];
                colorValue[2] = rgbArray[valueIndex][2];
                break;
            case MFnMesh::kRGBA:
                colorValue[0] = rgbaArray[valueIndex][0];
                colorValue[1] = rgbaArray[valueIndex][1];
                colorValue[2] = rgbaArray[valueIndex][2];
                colorValue[3] = rgbaArray[valueIndex][3];
                break;
            default:
                break;
        }

        if (convertToDisplay) {
            colorValue = GfConvertLinearToDisplay(colorValue);
        }

        MColor mColor(colorValue[0], colorValue[1], colorValue[2], colorValue[3]);
        colorArray.append(mColor);
    }

    // colorArray now stores all of the values and any unassigned components
    // have had their indices set to -1, so update the unauthored values index.
    unauthoredValuesIndex = -1;

    // When the Clamped custom data is not present, we assume unclamped.
    const bool clamped =
        PxrUsdMayaUtil::GetBoolCustomData(primvar.GetAttr(),
                                          PxrUsdMayaMeshColorSetTokens->Clamped,
                                          false);

    status = meshFn.createColorSet(colorSetName, NULL, clamped, colorRep);
    if (status != MS::kSuccess) {
        MGlobal::displayWarning(
            TfStringPrintf("Unable to create color set '%s' for mesh: %s",
                           colorSetName.asChar(),
                           meshFn.fullPathName().asChar()).c_str());
        return false;
    }

    // Create colors on the mesh from the values we collected out of the
    // primvar. We'll assign mesh components to these values below.
    status = meshFn.setColors(colorArray, &colorSetName, colorRep);
    if (status != MS::kSuccess) {
        MGlobal::displayWarning(
            TfStringPrintf("Unable to set color data on color set '%s' for mesh: %s",
                           colorSetName.asChar(),
                           meshFn.fullPathName().asChar()).c_str());
        return false;
    }

    const TfToken& interpolation = primvar.GetInterpolation();

    // Build an array of value assignments for each face vertex in the mesh.
    // Any assignments left as -1 will not be assigned a value.
    MIntArray colorIds = _GetMayaFaceVertexAssignmentIds(meshFn,
                                                         interpolation,
                                                         assignmentIndices,
                                                         unauthoredValuesIndex);

    status = meshFn.assignColors(colorIds, &colorSetName);
    if (status != MS::kSuccess) {
        MGlobal::displayWarning(
            TfStringPrintf("Could not assign color values to color set '%s' on mesh: %s",
                           colorSetName.asChar(),
                           meshFn.fullPathName().asChar()).c_str());
        return false;
    }

    return true;
}
