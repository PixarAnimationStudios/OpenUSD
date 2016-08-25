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

#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/transform.h"
#include "pxr/usd/usdGeom/mesh.h"

#include <maya/MColor.h>
#include <maya/MColorArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshFaceVertex.h>

#include <unordered_map>


template <class T>
struct ValueHash
{
    std::size_t operator() (const T& value) const {
        return hash_value(value);
    }
};

template <class T>
struct ValuesEqual
{
    bool operator() (const T& a, const T& b) const {
        return GfIsClose(a, b, 1e-9);
    }
};

// This function condenses distinct indices that point to the same color values
// (the combination of RGB AND Alpha) to all point to the same index for that
// value. This will potentially shrink the data arrays.
static
void
_MergeEquivalentColorSetValues(
        VtArray<GfVec3f>* colorSetRGBData,
        VtArray<float>* colorSetAlphaData,
        VtArray<int>* colorSetAssignmentIndices)
{
    if (not colorSetRGBData or not colorSetAlphaData or not colorSetAssignmentIndices) {
        return;
    }

    const size_t numValues = colorSetRGBData->size();
    if (numValues == 0) {
        return;
    }

    if (colorSetAlphaData->size() != numValues) {
        TF_CODING_ERROR("Unequal sizes for color (%zu) and alpha (%zu)",
                        colorSetRGBData->size(), colorSetAlphaData->size());
    }

    // We maintain a map of values (color AND alpha together) to those values'
    // indices in our unique value arrays (color and alpha separate).
    std::unordered_map<GfVec4f, size_t, ValueHash<GfVec4f>, ValuesEqual<GfVec4f> > valuesSet;
    VtArray<GfVec3f> uniqueColors;
    VtArray<float> uniqueAlphas;
    VtArray<int> uniqueIndices;

    for (size_t i = 0; i < colorSetAssignmentIndices->size(); ++i) {
        int index = (*colorSetAssignmentIndices)[i];

        if (index < 0 or index >= numValues) {
            // This is an unassigned or otherwise unknown index, so just keep it.
            uniqueIndices.push_back(index);
            continue;
        }

        const GfVec3f color = (*colorSetRGBData)[index];
        const float alpha = (*colorSetAlphaData)[index];
        const GfVec4f value(color[0], color[1], color[2], alpha);

        int uniqueIndex = -1;

        auto inserted = valuesSet.insert(
            std::pair<GfVec4f, size_t>(value,
                                       uniqueColors.size()));
        if (inserted.second) {
            // This is a new value, so add it to the arrays.
            uniqueColors.push_back(GfVec3f(value[0], value[1], value[2]));
            uniqueAlphas.push_back(value[3]);
            uniqueIndex = uniqueColors.size() - 1;
        } else {
            // This is an existing value, so re-use the original's index.
            uniqueIndex = inserted.first->second;
        }

        uniqueIndices.push_back(uniqueIndex);
    }

    // If we reduced the number of values by merging, copy the results back.
    if (uniqueColors.size() < numValues) {
        (*colorSetRGBData) = uniqueColors;
        (*colorSetAlphaData) = uniqueAlphas;
        (*colorSetAssignmentIndices) = uniqueIndices;
    }
}

// This function tries to compress faceVarying primvar indices to uniform,
// vertex, or constant interpolation if possible. This will potentially shrink
// the indices array and will update the interpolation if any compression was
// possible.
static
void
_CompressFaceVaryingPrimvarIndices(
        const MFnMesh& mesh,
        TfToken *interpolation,
        VtArray<int>* assignmentIndices)
{
    if (not interpolation or
            not assignmentIndices or
            assignmentIndices->size() == 0) {
        return;
    }

    int numPolygons = mesh.numPolygons();
    VtArray<int> uniformAssignments;
    uniformAssignments.assign((size_t)numPolygons, -2);

    int numVertices = mesh.numVertices();
    VtArray<int> vertexAssignments;
    vertexAssignments.assign((size_t)numVertices, -2);

    // We assume that the data is constant/uniform/vertex until we can
    // prove otherwise that two components have differing values.
    bool isConstant = true;
    bool isUniform = true;
    bool isVertex = true;

    MItMeshFaceVertex itFV(mesh.object());
    unsigned int fvi = 0;
    for (itFV.reset(); not itFV.isDone(); itFV.next(), ++fvi) {
        int faceIndex = itFV.faceId();
        int vertexIndex = itFV.vertId();

        int assignedIndex = (*assignmentIndices)[fvi];

        if (isConstant) {
            if (assignedIndex != (*assignmentIndices)[0]) {
                isConstant = false;
            }
        }

        if (isUniform) {
            if (uniformAssignments[faceIndex] < -1) {
                // No value for this face yet, so store one.
                uniformAssignments[faceIndex] = assignedIndex;
            } else if (assignedIndex != uniformAssignments[faceIndex]) {
                isUniform = false;
            }
        }

        if (isVertex) {
            if (vertexAssignments[vertexIndex] < -1) {
                // No value for this vertex yet, so store one.
                vertexAssignments[vertexIndex] = assignedIndex;
            } else if (assignedIndex != vertexAssignments[vertexIndex]) {
                isVertex = false;
            }
        }

        if (not isConstant and not isUniform and not isVertex) {
            // No compression will be possible, so stop trying.
            break;
        }
    }

    if (isConstant) {
        assignmentIndices->resize(1);
        *interpolation = UsdGeomTokens->constant;
    } else if (isUniform) {
        *assignmentIndices = uniformAssignments;
        *interpolation = UsdGeomTokens->uniform;
    } else if(isVertex) {
        *assignmentIndices = vertexAssignments;
        *interpolation = UsdGeomTokens->vertex;
    } else {
        *interpolation = UsdGeomTokens->faceVarying;
    }
}

static inline
GfVec3f
_LinearColorFromColorSet(
        const MColor& mayaColor,
        bool shouldConvertToLinear)
{
    // we assume all color sets except displayColor are in linear space.
    // if we got a color from colorSetData and we're a displayColor, we
    // need to convert it to linear.
    GfVec3f c(mayaColor[0], mayaColor[1], mayaColor[2]);
    if (shouldConvertToLinear) {
        return GfConvertDisplayToLinear(c);
    }
    return c;
}

/// Collect values from the color set named \p colorSet.
/// If \p isDisplayColor is true and this color set represents displayColor,
/// the unauthored/unpainted values in the color set will be filled in using
/// the shader values in \p shadersRGBData and \p shadersAlphaData if available.
/// Values are gathered per face vertex, but then the data is compressed to
/// vertex, uniform, or constant interpolation if possible.
/// Unauthored/unpainted values will be given the index -1.
bool MayaMeshWriter::_GetMeshColorSetData(
        MFnMesh& mesh,
        MString colorSet,
        bool isDisplayColor,
        const VtArray<GfVec3f>& shadersRGBData,
        const VtArray<float>& shadersAlphaData,
        const VtArray<int>& shadersAssignmentIndices,
        VtArray<GfVec3f>* colorSetRGBData,
        VtArray<float>* colorSetAlphaData,
        TfToken* interpolation,
        VtArray<int>* colorSetAssignmentIndices,
        MFnMesh::MColorRepresentation* colorSetRep,
        bool* clamped)
{
    // If there are no colors, return immediately as failure.
    if (mesh.numColors(colorSet) == 0) {
        return false;
    }

    MColorArray colorSetData;
    const MColor unsetColor(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);
    if (mesh.getFaceVertexColors(colorSetData, &colorSet, &unsetColor)
            == MS::kFailure) {
        return false;
    }

    if (colorSetData.length() == 0) {
        return false;
    }

    // Get the color set representation and clamping.
    *colorSetRep = mesh.getColorRepresentation(colorSet);
    *clamped = mesh.isColorClamped(colorSet);

    // We'll populate the assignment indices for every face vertex, but we'll
    // only push values into the data if the face vertex has a value. All face
    // vertices are initially unassigned/unauthored.
    colorSetRGBData->clear();
    colorSetAlphaData->clear();
    colorSetAssignmentIndices->assign((size_t)colorSetData.length(), -1);
    *interpolation = UsdGeomTokens->faceVarying;

    // Loop over every face vertex to populate the value arrays.
    MItMeshFaceVertex itFV(mesh.object());
    unsigned int fvi = 0;
    for (itFV.reset(); not itFV.isDone(); itFV.next(), ++fvi) {
        // If this is a displayColor color set, we may need to fallback on the
        // bound shader colors/alphas for this face in some cases. In
        // particular, if the color set is alpha-only, we fallback on the
        // shader values for the color. If the color set is RGB-only, we
        // fallback on the shader values for alpha only. If there's no authored
        // color for this face vertex, we use both the color AND alpha values
        // from the shader.
        bool useShaderColorFallback = false;
        bool useShaderAlphaFallback = false;
        if (isDisplayColor) {
            if (colorSetData[fvi] == unsetColor) {
                useShaderColorFallback = true;
                useShaderAlphaFallback = true;
            } else if (*colorSetRep == MFnMesh::kAlpha) {
                // The color set does not provide color, so fallback on shaders.
                useShaderColorFallback = true;
            } else if (*colorSetRep == MFnMesh::kRGB) {
                // The color set does not provide alpha, so fallback on shaders.
                useShaderAlphaFallback = true;
            }
        }

        // If we're exporting displayColor and we use the value from the color
        // set, we need to convert it to linear.
        bool convertDisplayColorToLinear = isDisplayColor;

        // Shader values for the mesh could be constant
        // (shadersAssignmentIndices is empty) or uniform.
        int faceIndex = itFV.faceId();
        if (useShaderColorFallback) {
            // There was no color value in the color set to use, so we use the
            // shader color, or the default color if there is no shader color.
            // This color will already be in linear space, so don't convert it
            // again.
            convertDisplayColorToLinear = false;

            int valueIndex = -1;
            if (shadersAssignmentIndices.empty()) {
                if (shadersRGBData.size() == 1) {
                    valueIndex = 0;
                }
            } else if (faceIndex >= 0 and faceIndex < shadersAssignmentIndices.size()) {
                int tmpIndex = shadersAssignmentIndices[faceIndex];
                if (tmpIndex >= 0 and tmpIndex < shadersRGBData.size()) {
                    valueIndex = tmpIndex;
                }
            }
            if (valueIndex >= 0) {
                colorSetData[fvi][0] = shadersRGBData[valueIndex][0];
                colorSetData[fvi][1] = shadersRGBData[valueIndex][1];
                colorSetData[fvi][2] = shadersRGBData[valueIndex][2];
            } else {
                // No shader color to fallback on. Use the default shader color.
                colorSetData[fvi][0] = _ShaderDefaultRGB[0];
                colorSetData[fvi][1] = _ShaderDefaultRGB[1];
                colorSetData[fvi][2] = _ShaderDefaultRGB[2];
            }
        }
        if (useShaderAlphaFallback) {
            int valueIndex = -1;
            if (shadersAssignmentIndices.empty()) {
                if (shadersAlphaData.size() == 1) {
                    valueIndex = 0;
                }
            } else if (faceIndex >= 0 and faceIndex < shadersAssignmentIndices.size()) {
                int tmpIndex = shadersAssignmentIndices[faceIndex];
                if (tmpIndex >= 0 and tmpIndex < shadersAlphaData.size()) {
                    valueIndex = tmpIndex;
                }
            }
            if (valueIndex >= 0) {
                colorSetData[fvi][3] = shadersAlphaData[valueIndex];
            } else {
                // No shader alpha to fallback on. Use the default shader alpha.
                colorSetData[fvi][3] = _ShaderDefaultAlpha;
            }
        }

        // If we have a color/alpha value, add it to the data to be returned.
        if (colorSetData[fvi] != unsetColor) {
            GfVec3f rgbValue = _ColorSetDefaultRGB;
            float alphaValue = _ColorSetDefaultAlpha;

            if (useShaderColorFallback or
                    (*colorSetRep == MFnMesh::kRGB) or
                    (*colorSetRep == MFnMesh::kRGBA)) {
                rgbValue = _LinearColorFromColorSet(colorSetData[fvi],
                                                    convertDisplayColorToLinear);
            }
            if (useShaderAlphaFallback or
                    (*colorSetRep == MFnMesh::kAlpha) or
                    (*colorSetRep == MFnMesh::kRGBA)) {
                alphaValue = colorSetData[fvi][3];
            }

            colorSetRGBData->push_back(rgbValue);
            colorSetAlphaData->push_back(alphaValue);
            (*colorSetAssignmentIndices)[fvi] = colorSetRGBData->size() - 1;
        }
    }

    _MergeEquivalentColorSetValues(colorSetRGBData,
                                   colorSetAlphaData,
                                   colorSetAssignmentIndices);
    _CompressFaceVaryingPrimvarIndices(mesh,
                                       interpolation,
                                       colorSetAssignmentIndices);

    return true;
}

// We assumed that primvars in USD are always unclamped so we add the
// clamped custom data ONLY when clamping is set to true in the colorset
static void SetPVCustomData(UsdAttribute obj, bool clamped)
{
    if (clamped) {
        obj.SetCustomDataByKey(TfToken("Clamped"), VtValue(clamped));
    }
}


bool MayaMeshWriter::_createAlphaPrimVar(
        UsdGeomGprim &primSchema,
        const TfToken& name,
        const VtArray<float>& data,
        const TfToken& interpolation,
        const VtArray<int>& assignmentIndices,
        const int unassignedValueIndex,
        bool clamped)
{
    unsigned int numValues = data.size();
    if (numValues == 0) {
        return false;
    }

    TfToken interp = interpolation;
    if (numValues == 1 and interp == UsdGeomTokens->constant) {
        interp = TfToken();
    }

    UsdGeomPrimvar primVar =
        primSchema.CreatePrimvar(name,
                                 SdfValueTypeNames->FloatArray,
                                 interp);

    primVar.Set(data);

    if (not assignmentIndices.empty()) {
        primVar.SetIndices(assignmentIndices);
        if (unassignedValueIndex != primVar.GetUnauthoredValuesIndex()) {
           primVar.SetUnauthoredValuesIndex(unassignedValueIndex);
        }
    }

    SetPVCustomData(primVar.GetAttr(), clamped);

    return true;
}

bool MayaMeshWriter::_createRGBPrimVar(
        UsdGeomGprim &primSchema,
        const TfToken& name,
        const VtArray<GfVec3f>& data,
        const TfToken& interpolation,
        const VtArray<int>& assignmentIndices,
        const int unassignedValueIndex,
        bool clamped)
{
    unsigned int numValues = data.size();
    if (numValues == 0) {
        return false;
    }

    TfToken interp = interpolation;
    if (numValues == 1 and interp == UsdGeomTokens->constant) {
        interp = TfToken();
    }

    UsdGeomPrimvar primVar =
        primSchema.CreatePrimvar(name,
                                 SdfValueTypeNames->Color3fArray,
                                 interp);

    primVar.Set(data);

    if (not assignmentIndices.empty()) {
        primVar.SetIndices(assignmentIndices);
        if (unassignedValueIndex != primVar.GetUnauthoredValuesIndex()) {
           primVar.SetUnauthoredValuesIndex(unassignedValueIndex);
        }
    }

    SetPVCustomData(primVar.GetAttr(), clamped);

    return true;
}

bool MayaMeshWriter::_createRGBAPrimVar(
        UsdGeomGprim &primSchema,
        const TfToken& name,
        const VtArray<GfVec3f>& rgbData,
        const VtArray<float>& alphaData,
        const TfToken& interpolation,
        const VtArray<int>& assignmentIndices,
        const int unassignedValueIndex,
        bool clamped)
{
    unsigned int numValues = rgbData.size();
    if (numValues == 0 or numValues != alphaData.size()) {
        return false;
    }

    TfToken interp = interpolation;
    if (numValues == 1 and interp == UsdGeomTokens->constant) {
        interp = TfToken();
    }

    UsdGeomPrimvar primVar =
        primSchema.CreatePrimvar(name,
                                 SdfValueTypeNames->Color4fArray,
                                 interp);

    VtArray<GfVec4f> rgbaData(numValues);
    for (size_t i = 0; i < rgbaData.size(); ++i) {
        rgbaData[i] = GfVec4f(rgbData[i][0], rgbData[i][1], rgbData[i][2],
                              alphaData[i]);
    }

    primVar.Set(rgbaData);

    if (not assignmentIndices.empty()) {
        primVar.SetIndices(assignmentIndices);
        if (unassignedValueIndex != primVar.GetUnauthoredValuesIndex()) {
           primVar.SetUnauthoredValuesIndex(unassignedValueIndex);
        }
    }

    SetPVCustomData(primVar.GetAttr(), clamped);

    return true;
}

bool MayaMeshWriter::_addDisplayPrimvars(
        UsdGeomGprim &primSchema,
        const MFnMesh::MColorRepresentation colorRep,
        const VtArray<GfVec3f>& RGBData,
        const VtArray<float>& AlphaData,
        const TfToken& interpolation,
        const VtArray<int>& assignmentIndices,
        const int unassignedValueIndex,
        const bool clamped,
        const bool authored)
{
    // If we already have an authored value, don't try to write a new one.
    UsdAttribute colorAttr = primSchema.GetDisplayColorAttr();
    if (not colorAttr.HasAuthoredValueOpinion() and not RGBData.empty()) {
        UsdGeomPrimvar displayColor = primSchema.GetDisplayColorPrimvar();
        if (interpolation != displayColor.GetInterpolation()) {
            displayColor.SetInterpolation(interpolation);
        }
        displayColor.Set(RGBData);
        if (not assignmentIndices.empty()) {
            displayColor.SetIndices(assignmentIndices);
            if (unassignedValueIndex != displayColor.GetUnauthoredValuesIndex()) {
               displayColor.SetUnauthoredValuesIndex(unassignedValueIndex);
            }
        }
        bool authRGB = authored;
        if (colorRep == MFnMesh::kAlpha) {
            authRGB = false;
        }
        if (authRGB) {
            colorAttr.SetCustomDataByKey(TfToken("Authored"), VtValue(authRGB));
            SetPVCustomData(colorAttr, clamped);
        }
    }

    UsdAttribute alphaAttr = primSchema.GetDisplayOpacityAttr();
    if (not alphaAttr.HasAuthoredValueOpinion() and not AlphaData.empty()) {
        // we consider a single alpha value that is 1.0 to be the "default"
        // value.  We only want to write values that are not the "default".
        bool hasDefaultAlpha = AlphaData.size() == 1 and GfIsClose(AlphaData[0], 1.0, 1e-9);
        if (not hasDefaultAlpha) {
            UsdGeomPrimvar displayOpacity = primSchema.GetDisplayOpacityPrimvar();
            if (interpolation != displayOpacity.GetInterpolation()) {
                displayOpacity.SetInterpolation(interpolation);
            }
            displayOpacity.Set(AlphaData);
            if (not assignmentIndices.empty()) {
                displayOpacity.SetIndices(assignmentIndices);
                if (unassignedValueIndex != displayOpacity.GetUnauthoredValuesIndex()) {
                   displayOpacity.SetUnauthoredValuesIndex(unassignedValueIndex);
                }
            }
            bool authAlpha = authored;
            if (colorRep == MFnMesh::kRGB) {
                authAlpha = false;
            }
            if (authAlpha) {
                alphaAttr.SetCustomDataByKey(TfToken("Authored"), VtValue(authAlpha));
                SetPVCustomData(alphaAttr, clamped);
            }
        }
    }

    return true;
}

inline bool _IsClose(double a, double b)
{
    // This matches the results produced by numpy.allclose
    static const double aTol(1.0e-8);
    static const double rTol(1.0e-5);
    return fabs(a - b) < (aTol + rTol * fabs(b));
}


inline bool _HasZeros(const MIntArray& v)
{
    for (size_t i = 0; i < v.length(); i++) {
        if (v[i] == 0)
            return true;
    }

    return false;
}

inline void
_CopyUVs(const MFloatArray& uArray, const MFloatArray& vArray,
         VtArray<GfVec2f> *uvArray)
{
    uvArray->clear();
    uvArray->resize(uArray.length());
    for (size_t i = 0; i < uvArray->size(); i++) {
        (*uvArray)[i] = GfVec2f(uArray[i], vArray[i]);
    }
}

/* static */
MStatus
MayaMeshWriter::_FullUVsFromSparse(
    const MFnMesh& m,
    const MIntArray& uvCounts, const MIntArray& uvIds,
    const MFloatArray& uArray, const MFloatArray& vArray,
    VtArray<GfVec2f> *uvArray
)
{
    MIntArray faceVertexCounts, faceVertexIndices;
    MStatus status = m.getVertices(faceVertexCounts, faceVertexIndices);
    if (!status)
        return status;

    // Construct a cumulative index array. Each element in this array
    // is the starting index into the vertex index and uv index arrays.
    // We use it later to map face indices to uv indices.
    //
    std::vector<size_t> cumIndices(faceVertexCounts.length());
    size_t cumIndex = 0;
    for (size_t i = 0; i < cumIndices.size(); i++) {
        cumIndices[i] = cumIndex;
        cumIndex += faceVertexCounts[i];
    }

    // Our "full" u and v arrays will each have the same number of elements as
    // faceVertexIndices. Make new arrays, and fill them with a very large
    // negative value (very large positive values are poisonous to Mari). The
    // idea is that texture look-ups on faces with no uvs will trigger wrap
    // behavior, which can be set to "black", if necessary.
    //
    const int numUVs = faceVertexIndices.length();
    MFloatArray uArrayFull(numUVs, -1.0e30);
    MFloatArray vArrayFull(numUVs, -1.0e30);

    // Now poke in the u and v values that actually exist
    // k assumes values in the range [0, uvIds.length())
    //
    int k = 0;
    for (size_t i = 0; i < uvCounts.length(); i++) {
        if (uvCounts[i] == 0)
            continue;

        const int cumIndex = cumIndices[i];
        for (int j = cumIndex; j < cumIndex + uvCounts[i]; j++, k++) {
            const int uvId = uvIds[k];
            uArrayFull[j] = uArray[uvId];
            vArrayFull[j] = vArray[uvId];
        }
    }

    if (k == 0) {
        // No uvs assigned at all ... clear the result
        uvArray->clear();
        return MS::kFailure;
    } else {
        _CopyUVs(uArrayFull, vArrayFull, uvArray);
        return MS::kSuccess;
    }
}

static inline bool _AllSame(const MFloatArray& v)
{
    const float& firstVal = v[0];
    for (size_t i = 1; i < v.length(); i++) {
        if (!_IsClose(v[i], firstVal))
            return false;
    }

    return true;
}

/* static */
MStatus
MayaMeshWriter::_CompressUVs(const MFnMesh& m,
                             const MIntArray& uvIds,
                             const MFloatArray& uArray, const MFloatArray& vArray,
                             VtArray<GfVec2f> *uvArray,
                             TfToken *interpolation)
{
    MIntArray faceVertexCounts, faceVertexIndices;
    MStatus status = m.getVertices(faceVertexCounts, faceVertexIndices);
    if (!status)
        return status;

    // All uvs are natively stored and accessed as "faceVarying" in Maya.
    // But we'd like to save space when possible, so we examine the u and
    // values to see if they can't be represented as "vertex" or "constant" instead.
    //
    // Our strategy is to visit all vertices of all faces, some of which might
    // be the same physical vertex. We look up the u and v values for each
    // vertex, and check to see if they are the same as they were for the last
    // visit to that vertex. If they are not, we know the uvs can't be "vertex",
    // and thus not "constant" either.
    //
    // Even if the uv set turns out to be "faceVarying" after all, we have to
    // fill in the face-varying arrays, because we can't assume that "uArray"
    // and "vArray" have as many values as there are vertices (we may still
    // have uv sharing).
    //
    // Note that the Maya API guarantees that vertex indices are always in the
    // range [0, numVertices). This algorithm depends on that being the case.
    //
    MFloatArray uArrayFV(faceVertexIndices.length());
    MFloatArray vArrayFV(faceVertexIndices.length());

    MFloatArray uArrayVertex(m.numVertices(), 0);
    MFloatArray vArrayVertex(m.numVertices(), 0);
    std::vector<bool> visited(m.numVertices(), false);

    // Start off with "vertex" -- we may decide it's
    // "faceVarying" in the middle of the loop.
    //
    *interpolation = UsdGeomTokens->vertex;

    int k = 0;
    for (size_t i = 0; i < faceVertexCounts.length(); i++) {
        for (int j = 0; j < faceVertexCounts[i]; j++, k++) {
            const int vertexIndex = faceVertexIndices[k];
            const float u = uArray[uvIds[k]];
            const float v = vArray[uvIds[k]];
            uArrayFV[k] = u;
            vArrayFV[k] = v;
            if (*interpolation == UsdGeomTokens->vertex) {
                if (visited[vertexIndex]) {
                    // We've been here before -- check to see if
                    // the u and v are the same
                    if (!_IsClose(uArrayVertex[vertexIndex], u) ||
                        !_IsClose(vArrayVertex[vertexIndex], v)) {
                        // Alas, it's not "vertex". Switch the detail
                        // to "faceVarying" and clear the arrays. Henceforth,
                        // only fill uArrayFV and vArrayFV.
                        //
                        *interpolation = UsdGeomTokens->faceVarying;
                        uArrayVertex.clear();
                        vArrayVertex.clear();
                    }
                } else {
                    // Never been here .. mark visited, and
                    // store u and v.
                    visited[vertexIndex] = true;
                    uArrayVertex[vertexIndex] = u;
                    vArrayVertex[vertexIndex] = v;
                }
            }
        }
    }

    if (*interpolation == UsdGeomTokens->vertex) {
        // Check to see if all the (u, v) values are the same. If they are, we can
        // declare the detail ""constant"", and fill in just one value.
        //
        if (_AllSame(uArrayVertex) && _AllSame(vArrayVertex)) {
            *interpolation = UsdGeomTokens->constant;
            uvArray->clear();
            uvArray->push_back(GfVec2f(uArrayVertex[0], vArrayVertex[0]));
        } else {
            // Nope, still "vertex"
            _CopyUVs(uArrayVertex, vArrayVertex, uvArray);
        }
    } else {
        // "faceVarying"
        _CopyUVs(uArrayFV, vArrayFV, uvArray);
    }

    return MS::kSuccess;
}



MStatus
MayaMeshWriter::_GetMeshUVSetData(MFnMesh& m, MString uvSetName,
                                  VtArray<GfVec2f> *uvArray,
                                  TfToken *interpolation)
{
    MStatus status;

    MIntArray uvCounts, uvIds;
    status = m.getAssignedUVs(uvCounts, uvIds, &uvSetName);
    if (!status)
        return status;


    MFloatArray uArray, vArray;
    status = m.getUVs(uArray, vArray, &uvSetName);
    if (!status)
        return status;

    // Sanity check the data before we attempt to do anything with it.
    if (uvCounts.length() == 0 or uvIds.length() == 0 or
        uArray.length() == 0 or vArray.length() == 0) {
        return MS::kFailure;
    }

    // Check for zeros in "uvCounts" -- if there are any,
    // the uvs are sparse.
    const bool isSparse = _HasZeros(uvCounts);

    if (!isSparse) {
        return _CompressUVs(m, uvIds, uArray, vArray,
                            uvArray, interpolation);
    } else {
        *interpolation = UsdGeomTokens->faceVarying;
        return _FullUVsFromSparse(m, uvCounts, uvIds, uArray, vArray,
                                  uvArray);
    }

}
