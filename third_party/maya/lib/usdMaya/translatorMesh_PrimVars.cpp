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
#include "pxr/base/gf/transform.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/inherits.h"
#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdUtils/pipeline.h"

#include <maya/MVector.h>
#include <maya/MFloatArray.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>

static bool computeVertexIndices(MFnMesh &meshFn, MIntArray &valuesPerPolygon, MIntArray &valueIndexes, MIntArray &remapIndexes, TfToken interpolation)
{
    MIntArray meshIdxRemap;
    meshFn.getVertices(valuesPerPolygon, meshIdxRemap);
    int numFaceVertices = meshFn.numFaceVertices();
    valueIndexes.setLength(numFaceVertices);
    int vertexIndex;
    for (int i=0; i < numFaceVertices; i++) {
        if (interpolation == UsdGeomTokens->vertex) {
            vertexIndex=meshIdxRemap[i];
        } else {
            vertexIndex=i;
        }
        if (remapIndexes.length()>0) {
            valueIndexes.set(remapIndexes[vertexIndex],i);
        } else {
            valueIndexes.set(vertexIndex,i);
        }
    }
    return true;
}

static bool findCoincidentUVCoord(MFloatArray &uCoords, MFloatArray &vCoords, size_t uvCoordsArraySize, float newU, float newV, size_t *foundIdx)
{
    for (size_t i=0;i<uvCoordsArraySize;i++) {
        if (GfIsClose(uCoords[i], newU, 1e-9) && 
            GfIsClose(vCoords[i], newV, 1e-9)) {
            *foundIdx=i;
            return true;
        }
    }
    return false;
}

static void compressUVValues(VtArray<GfVec2f> uvValues, MFloatArray &uCoords, MFloatArray &vCoords, MIntArray &remapIndexes)
{
    uCoords.setLength(uvValues.size());
    vCoords.setLength(uvValues.size());
    remapIndexes.setLength(uvValues.size());
    size_t i=0, uvCoordsArraySize=0, foundIdx=0;
    TF_FOR_ALL(vec, uvValues) {
        if (findCoincidentUVCoord(uCoords, vCoords, uvCoordsArraySize, (*vec)[0], (*vec)[1], &foundIdx)) {
            remapIndexes[i]=foundIdx;
        } else {
            uCoords[uvCoordsArraySize] = (*vec)[0];
            vCoords[uvCoordsArraySize] = (*vec)[1];
            remapIndexes[i]=uvCoordsArraySize;
            uvCoordsArraySize++;
        }
        i++;
    }
    TF_VERIFY(i == uvValues.size());
    uCoords.setLength(uvCoordsArraySize);
    vCoords.setLength(uvCoordsArraySize);
}

/* static */
bool 
PxrUsdMayaTranslatorMesh::_AssignUVSetPrimvarToMesh( const UsdGeomPrimvar &primvar, MFnMesh &meshFn)
{
    TfToken name, interpolation;
    SdfValueTypeName typeName;
    int elementSize;
    primvar.GetDeclarationInfo(&name, &typeName, &interpolation, &elementSize);

    VtVec2fArray rawVal;
    if (primvar.ComputeFlattened(&rawVal, UsdTimeCode::Default())) {
        MFloatArray uCoords;
        MFloatArray vCoords;
        MIntArray remapIndexes;
        // Compress coincident UV points to preserve editability in maya
        compressUVValues(rawVal, uCoords, vCoords, remapIndexes);
        MString uvSetName(name.GetText());
        MStatus status;
        // We assume that the st UV set is the first UV set in maya called map1
        if (uvSetName=="st") {
            uvSetName="map1";
            status = MS::kSuccess;
        } else {
            status = meshFn.createUVSet(uvSetName);
        }
        if (status == MS::kSuccess) {
            status = meshFn.setUVs(uCoords, vCoords, &uvSetName );
            if (status == MS::kSuccess) {
                MIntArray valuesPerPolygon;
                MIntArray valueIndexes;
                if (computeVertexIndices(meshFn, valuesPerPolygon, valueIndexes, remapIndexes, interpolation)) {
                    status = meshFn.assignUVs(valuesPerPolygon, valueIndexes, &uvSetName);
                    if (status != MS::kSuccess) {
                        MGlobal::displayWarning(TfStringPrintf("Unable to assign UV set <%s> to mesh", name.GetText()).c_str());
                    } else {
                        return true;
                    }
                } else {
                    MGlobal::displayWarning(TfStringPrintf("Unable to computeVertexIndices on UV set <%s>", name.GetText()).c_str());
                }
            } else {
                MGlobal::displayWarning(TfStringPrintf("Unable to set UV data on <%s>", name.GetText()).c_str());
            }
        } else {
            MGlobal::displayWarning(TfStringPrintf("Unable to create UV <%s>", name.GetText()).c_str());
        }
    } else {
        MGlobal::displayWarning(TfStringPrintf("Primvar <%s> doesn't hold an array of Vec2f", name.GetText()).c_str());
    }

    return false;
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
