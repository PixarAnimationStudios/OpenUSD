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

#include "usdMaya/util.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/transform.h"
#include "pxr/usd/usdGeom/mesh.h"

#include <maya/MColor.h>
#include <maya/MColorArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshFaceVertex.h>


bool
MayaMeshWriter::_GetMeshUVSetData(
        const MFnMesh& mesh,
        const MString& uvSetName,
        VtArray<GfVec2f>* uvArray,
        TfToken* interpolation,
        VtArray<int>* assignmentIndices)
{
    MStatus status;

    // Sanity check first to make sure this UV set even has assigned values
    // before we attempt to do anything with the data.
    MIntArray uvCounts, uvIds;
    status = mesh.getAssignedUVs(uvCounts, uvIds, &uvSetName);
    if (status != MS::kSuccess) {
        return false;
    }
    if (uvCounts.length() == 0 || uvIds.length() == 0) {
        return false;
    }

    // We'll populate the assignment indices for every face vertex, but we'll
    // only push values into the data if the face vertex has a value. All face
    // vertices are initially unassigned/unauthored.
    const unsigned int numFaceVertices = mesh.numFaceVertices(&status);
    uvArray->clear();
    assignmentIndices->assign((size_t)numFaceVertices, -1);
    *interpolation = UsdGeomTokens->faceVarying;

    MItMeshFaceVertex itFV(mesh.object());
    unsigned int fvi = 0;
    for (itFV.reset(); !itFV.isDone(); itFV.next(), ++fvi) {
        if (!itFV.hasUVs(uvSetName)) {
            // No UVs for this faceVertex, so leave it unassigned.
            continue;
        }

        float2 uv;
        itFV.getUV(uv, &uvSetName);

        GfVec2f value(uv[0], uv[1]);
        uvArray->push_back(value);
        (*assignmentIndices)[fvi] = uvArray->size() - 1;
    }

    PxrUsdMayaUtil::MergeEquivalentIndexedValues(uvArray,
                                                 assignmentIndices);
    PxrUsdMayaUtil::CompressFaceVaryingPrimvarIndices(mesh,
                                                      interpolation,
                                                      assignmentIndices);

    return true;
}

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
    if (!colorSetRGBData || !colorSetAlphaData || !colorSetAssignmentIndices) {
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

    // We first combine the separate color and alpha arrays into one GfVec4f
    // array.
    VtArray<GfVec4f> colorsWithAlphasData(numValues);
    for (size_t i = 0; i < numValues; ++i) {
        const GfVec3f color = (*colorSetRGBData)[i];
        const float alpha = (*colorSetAlphaData)[i];

        colorsWithAlphasData[i][0] = color[0];
        colorsWithAlphasData[i][1] = color[1];
        colorsWithAlphasData[i][2] = color[2];
        colorsWithAlphasData[i][3] = alpha;
    }

    VtArray<int> mergedIndices(*colorSetAssignmentIndices);
    PxrUsdMayaUtil::MergeEquivalentIndexedValues(&colorsWithAlphasData,
                                                 &mergedIndices);

    // If we reduced the number of values by merging, copy the results back,
    // separating the values back out into colors and alphas.
    const size_t newSize = colorsWithAlphasData.size();
    if (newSize < numValues) {
        colorSetRGBData->resize(newSize);
        colorSetAlphaData->resize(newSize);

        for (size_t i = 0; i < newSize; ++i) {
            const GfVec4f colorWithAlpha = colorsWithAlphasData[i];

            (*colorSetRGBData)[i][0] = colorWithAlpha[0];
            (*colorSetRGBData)[i][1] = colorWithAlpha[1];
            (*colorSetRGBData)[i][2] = colorWithAlpha[2];
            (*colorSetAlphaData)[i] = colorWithAlpha[3];
        }
        (*colorSetAssignmentIndices) = mergedIndices;
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
        const MString& colorSet,
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
    for (itFV.reset(); !itFV.isDone(); itFV.next(), ++fvi) {
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
            } else if (faceIndex >= 0 && 
                static_cast<size_t>(faceIndex) < shadersAssignmentIndices.size()) {

                int tmpIndex = shadersAssignmentIndices[faceIndex];
                if (tmpIndex >= 0 && 
                    static_cast<size_t>(tmpIndex) < shadersRGBData.size()) {
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
            } else if (faceIndex >= 0 && 
                static_cast<size_t>(faceIndex) < shadersAssignmentIndices.size()) {
                int tmpIndex = shadersAssignmentIndices[faceIndex];
                if (tmpIndex >= 0 && 
                    static_cast<size_t>(tmpIndex) < shadersAlphaData.size()) {
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

            if (useShaderColorFallback              || 
                    (*colorSetRep == MFnMesh::kRGB) || 
                    (*colorSetRep == MFnMesh::kRGBA)) {
                rgbValue = _LinearColorFromColorSet(colorSetData[fvi],
                                                    convertDisplayColorToLinear);
            }
            if (useShaderAlphaFallback                || 
                    (*colorSetRep == MFnMesh::kAlpha) || 
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
    PxrUsdMayaUtil::CompressFaceVaryingPrimvarIndices(mesh,
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
    if (numValues == 1 && interp == UsdGeomTokens->constant) {
        interp = TfToken();
    }

    UsdGeomPrimvar primVar =
        primSchema.CreatePrimvar(name,
                                 SdfValueTypeNames->FloatArray,
                                 interp);

    primVar.Set(data);

    if (!assignmentIndices.empty()) {
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
    if (numValues == 1 && interp == UsdGeomTokens->constant) {
        interp = TfToken();
    }

    UsdGeomPrimvar primVar =
        primSchema.CreatePrimvar(name,
                                 SdfValueTypeNames->Color3fArray,
                                 interp);

    primVar.Set(data);

    if (!assignmentIndices.empty()) {
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
    if (numValues == 0 || numValues != alphaData.size()) {
        return false;
    }

    TfToken interp = interpolation;
    if (numValues == 1 && interp == UsdGeomTokens->constant) {
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

    if (!assignmentIndices.empty()) {
        primVar.SetIndices(assignmentIndices);
        if (unassignedValueIndex != primVar.GetUnauthoredValuesIndex()) {
           primVar.SetUnauthoredValuesIndex(unassignedValueIndex);
        }
    }

    SetPVCustomData(primVar.GetAttr(), clamped);

    return true;
}

bool MayaMeshWriter::_createUVPrimVar(
        UsdGeomGprim &primSchema,
        const TfToken& name,
        const VtArray<GfVec2f>& data,
        const TfToken& interpolation,
        const VtArray<int>& assignmentIndices,
        const int unassignedValueIndex)
{
    unsigned int numValues = data.size();
    if (numValues == 0) {
        return false;
    }

    TfToken interp = interpolation;
    if (numValues == 1 && interp == UsdGeomTokens->constant) {
        interp = TfToken();
    }

    UsdGeomPrimvar primVar =
        primSchema.CreatePrimvar(name,
                                 SdfValueTypeNames->Float2Array,
                                 interp);

    primVar.Set(data);

    if (!assignmentIndices.empty()) {
        primVar.SetIndices(assignmentIndices);
        if (unassignedValueIndex != primVar.GetUnauthoredValuesIndex()) {
           primVar.SetUnauthoredValuesIndex(unassignedValueIndex);
        }
    }

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
    if (!colorAttr.HasAuthoredValueOpinion() && !RGBData.empty()) {
        UsdGeomPrimvar displayColor = primSchema.GetDisplayColorPrimvar();
        if (interpolation != displayColor.GetInterpolation()) {
            displayColor.SetInterpolation(interpolation);
        }
        displayColor.Set(RGBData);
        if (!assignmentIndices.empty()) {
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
    if (!alphaAttr.HasAuthoredValueOpinion() && !AlphaData.empty()) {
        // we consider a single alpha value that is 1.0 to be the "default"
        // value.  We only want to write values that are not the "default".
        bool hasDefaultAlpha = AlphaData.size() == 1 && GfIsClose(AlphaData[0], 1.0, 1e-9);
        if (!hasDefaultAlpha) {
            UsdGeomPrimvar displayOpacity = primSchema.GetDisplayOpacityPrimvar();
            if (interpolation != displayOpacity.GetInterpolation()) {
                displayOpacity.SetInterpolation(interpolation);
            }
            displayOpacity.Set(AlphaData);
            if (!assignmentIndices.empty()) {
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
