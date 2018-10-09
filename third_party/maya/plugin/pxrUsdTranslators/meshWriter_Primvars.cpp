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
#include "pxrUsdTranslators/meshWriter.h"

#include "usdMaya/colorSpace.h"
#include "usdMaya/roundTripUtil.h"
#include "usdMaya/util.h"
#include "usdMaya/writeUtil.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/transform.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/usd/usdGeom/mesh.h"

#include <maya/MColor.h>
#include <maya/MColorArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MItMeshVertex.h>

PXR_NAMESPACE_OPEN_SCOPE

bool
PxrUsdTranslators_MeshWriter::_GetMeshUVSetData(
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

    // using itFV.getUV() does not always give us the right answer, so
    // instead, we have to use itFV.getUVIndex() and use that to index into the
    // UV set.
    MFloatArray uArray;
    MFloatArray vArray;
    mesh.getUVs(uArray, vArray, &uvSetName);
    if (uArray.length() != vArray.length()) {
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

        int uvIndex;
        itFV.getUVIndex(uvIndex, &uvSetName);
        if (uvIndex < 0 || static_cast<size_t>(uvIndex) >= uArray.length()) {
            return false;
        }

        GfVec2f value(uArray[uvIndex], vArray[uvIndex]);
        uvArray->push_back(value);
        (*assignmentIndices)[fvi] = uvArray->size() - 1;
    }

    UsdMayaUtil::MergeEquivalentIndexedValues(uvArray,
                                                 assignmentIndices);
    UsdMayaUtil::CompressFaceVaryingPrimvarIndices(mesh,
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
    UsdMayaUtil::MergeEquivalentIndexedValues(&colorsWithAlphasData,
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
        return UsdMayaColorSpace::ConvertMayaToLinear(c);
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
bool PxrUsdTranslators_MeshWriter::_GetMeshColorSetData(
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
    UsdMayaUtil::CompressFaceVaryingPrimvarIndices(mesh,
                                                      interpolation,
                                                      colorSetAssignmentIndices);

    return true;
}

bool PxrUsdTranslators_MeshWriter::_createAlphaPrimVar(
        UsdGeomGprim &primSchema,
        const TfToken& name,
        const UsdTimeCode& usdTime,
        const VtArray<float>& data,
        const TfToken& interpolation,
        const VtArray<int>& assignmentIndices,
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
    _SetPrimvar(
            primVar,
            assignmentIndices,
            VtValue(data),
            VtValue(PxrUsdTranslators_MeshWriter::_ColorSetDefaultAlpha),
            usdTime);

    if (clamped) {
        UsdMayaRoundTripUtil::MarkPrimvarAsClamped(primVar);
    }

    return true;
}

bool PxrUsdTranslators_MeshWriter::_createRGBPrimVar(
        UsdGeomGprim &primSchema,
        const TfToken& name,
        const UsdTimeCode& usdTime,
        const VtArray<GfVec3f>& data,
        const TfToken& interpolation,
        const VtArray<int>& assignmentIndices,
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
    _SetPrimvar(
            primVar,
            assignmentIndices,
            VtValue(data),
            VtValue(PxrUsdTranslators_MeshWriter::_ColorSetDefaultRGB),
            usdTime);

    if (clamped) {
        UsdMayaRoundTripUtil::MarkPrimvarAsClamped(primVar);
    }

    return true;
}

bool PxrUsdTranslators_MeshWriter::_createRGBAPrimVar(
        UsdGeomGprim &primSchema,
        const TfToken& name,
        const UsdTimeCode& usdTime,
        const VtArray<GfVec3f>& rgbData,
        const VtArray<float>& alphaData,
        const TfToken& interpolation,
        const VtArray<int>& assignmentIndices,
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

    _SetPrimvar(
            primVar,
            assignmentIndices,
            VtValue(rgbaData),
            VtValue(PxrUsdTranslators_MeshWriter::_ColorSetDefaultRGBA),
            usdTime);

    if (clamped) {
        UsdMayaRoundTripUtil::MarkPrimvarAsClamped(primVar);
    }

    return true;
}

bool PxrUsdTranslators_MeshWriter::_createUVPrimVar(
        UsdGeomGprim &primSchema,
        const TfToken& name,
        const UsdTimeCode& usdTime,
        const VtArray<GfVec2f>& data,
        const TfToken& interpolation,
        const VtArray<int>& assignmentIndices)
{
    unsigned int numValues = data.size();
    if (numValues == 0) {
        return false;
    }

    TfToken interp = interpolation;
    if (numValues == 1 && interp == UsdGeomTokens->constant) {
        interp = TfToken();
    }

    SdfValueTypeName uvValueType = (UsdMayaWriteUtil::WriteUVAsFloat2())?
        (SdfValueTypeNames->Float2Array) : (SdfValueTypeNames->TexCoord2fArray); 
    UsdGeomPrimvar primVar = 
        primSchema.CreatePrimvar(name, uvValueType, interp);
    _SetPrimvar(
            primVar,
            assignmentIndices,
            VtValue(data),
            VtValue(PxrUsdTranslators_MeshWriter::_DefaultUV),
            usdTime);

    return true;
}

bool PxrUsdTranslators_MeshWriter::_addDisplayPrimvars(
        UsdGeomGprim &primSchema,
        const UsdTimeCode& usdTime,
        const MFnMesh::MColorRepresentation colorRep,
        const VtArray<GfVec3f>& RGBData,
        const VtArray<float>& AlphaData,
        const TfToken& interpolation,
        const VtArray<int>& assignmentIndices,
        const bool clamped,
        const bool authored)
{
    // We are appending the default value to the primvar in the post export function
    // so if the dataset is empty and the assignment indices are not, we still
    // have to set an empty array.
    // If we already have an authored value, don't try to write a new one.
    UsdAttribute colorAttr = primSchema.GetDisplayColorAttr();
    if (!colorAttr.HasAuthoredValueOpinion() && (!RGBData.empty() || !assignmentIndices.empty())) {
        UsdGeomPrimvar displayColor = primSchema.CreateDisplayColorPrimvar();
        if (interpolation != displayColor.GetInterpolation()) {
            displayColor.SetInterpolation(interpolation);
        }

        _SetPrimvar(
                displayColor,
                assignmentIndices,
                VtValue(RGBData),
                VtValue(PxrUsdTranslators_MeshWriter::_ShaderDefaultRGB),
                usdTime);

        bool authRGB = authored;
        if (colorRep == MFnMesh::kAlpha) {
            authRGB = false;
        }
        if (authRGB) {
            if (clamped) {
                UsdMayaRoundTripUtil::MarkPrimvarAsClamped(displayColor);
            }
        }
        else {
            UsdMayaRoundTripUtil::MarkAttributeAsMayaGenerated(colorAttr);
        }
    }

    UsdAttribute alphaAttr = primSchema.GetDisplayOpacityAttr();
    if (!alphaAttr.HasAuthoredValueOpinion() && (!AlphaData.empty() || !assignmentIndices.empty())) {
        // we consider a single alpha value that is 1.0 to be the "default"
        // value.  We only want to write values that are not the "default".
        bool hasDefaultAlpha = AlphaData.size() == 1 && GfIsClose(AlphaData[0], 1.0, 1e-9);
        if (!hasDefaultAlpha) {
            UsdGeomPrimvar displayOpacity = primSchema.CreateDisplayOpacityPrimvar();
            if (interpolation != displayOpacity.GetInterpolation()) {
                displayOpacity.SetInterpolation(interpolation);
            }

            _SetPrimvar(
                    displayOpacity,
                    assignmentIndices,
                    VtValue(AlphaData),
                    VtValue(PxrUsdTranslators_MeshWriter::_ShaderDefaultAlpha),
                    usdTime);

            bool authAlpha = authored;
            if (colorRep == MFnMesh::kRGB) {
                authAlpha = false;
            }
            if (authAlpha) {
                if (clamped) {
                    UsdMayaRoundTripUtil::MarkPrimvarAsClamped(displayOpacity);
                }
            }
            else {
                UsdMayaRoundTripUtil::MarkAttributeAsMayaGenerated(alphaAttr);
            }
        }
    }

    return true;
}

namespace {

static VtIntArray
_ShiftIndices(const VtIntArray& array, int shift)
{
    VtIntArray output(array.size());
    for (size_t i = 0; i < array.size(); ++i) {
        output[i] = std::max(0, array[i] + shift);
    }
    return output;
}

template <typename T>
static
VtValue
_PushFirstValue(VtArray<T> arr, const T& value)
{
    arr.resize(arr.size() + 1);
    std::move_backward(arr.begin(), arr.end() - 1, arr.end());
    arr[0] = value;
    return VtValue(arr);
}

static
VtValue
_PushFirstValue(const VtValue& arr, const VtValue& defaultValue)
{
    if (arr.IsHolding<VtArray<float>>() &&
            defaultValue.IsHolding<float>()) {
        return _PushFirstValue(
                arr.UncheckedGet<VtArray<float>>(),
                defaultValue.UncheckedGet<float>());
    }
    else if (arr.IsHolding<VtArray<GfVec2f>>() &&
            defaultValue.IsHolding<GfVec2f>()) {
        return _PushFirstValue(
                arr.UncheckedGet<VtArray<GfVec2f>>(),
                defaultValue.UncheckedGet<GfVec2f>());
    }
    else if (arr.IsHolding<VtArray<GfVec3f>>() &&
            defaultValue.IsHolding<GfVec3f>()) {
        return _PushFirstValue(
                arr.UncheckedGet<VtArray<GfVec3f>>(),
                defaultValue.UncheckedGet<GfVec3f>());
    }
    else if (arr.IsHolding<VtArray<GfVec4f>>() &&
            defaultValue.IsHolding<GfVec4f>()) {
        return _PushFirstValue(
                arr.UncheckedGet<VtArray<GfVec4f>>(),
                defaultValue.UncheckedGet<GfVec4f>());
    }

    TF_CODING_ERROR("Unsupported type");
    return VtValue();
};

template <typename T>
static
VtValue
_PopFirstValue(VtArray<T> arr)
{
    std::move(arr.begin() + 1, arr.end(), arr.begin());
    arr.pop_back();
    return VtValue(arr);
}

static
VtValue
_PopFirstValue(const VtValue& arr)
{
    if (arr.IsHolding<VtArray<float>>()) {
        return _PopFirstValue(arr.UncheckedGet<VtArray<float>>());
    }
    else if (arr.IsHolding<VtArray<GfVec2f>>()) {
        return _PopFirstValue(arr.UncheckedGet<VtArray<GfVec2f>>());
    }
    else if (arr.IsHolding<VtArray<GfVec3f>>()) {
        return _PopFirstValue(arr.UncheckedGet<VtArray<GfVec3f>>());
    }
    else if (arr.IsHolding<VtArray<GfVec4f>>()) {
        return _PopFirstValue(arr.UncheckedGet<VtArray<GfVec4f>>());
    }

    TF_CODING_ERROR("Unsupported type");
    return VtValue();
};

static
bool
_ContainsUnauthoredValues(const VtIntArray& indices)
{
    return std::any_of(
            indices.begin(),
            indices.end(),
            [](const int& i) { return i < 0; });
}

} // anonymous namespace

void
PxrUsdTranslators_MeshWriter::_SetPrimvar(
        const UsdGeomPrimvar& primvar,
        const VtIntArray& indices,
        const VtValue& values,
        const VtValue& defaultValue,
        const UsdTimeCode& usdTime)
{
    // Simple case of non-indexed primvars.
    if (indices.empty()) {
        _SetAttribute(primvar.GetAttr(), values, usdTime);
        return;
    }

    // The mesh writer writes primvars only at default time or at time samples,
    // but never both. We depend on that fact here to do different things
    // depending on whether you ever export the default-time data or not.
    if (usdTime.IsDefault()) {
        // If we are only exporting the default values, then we know
        // definitively whether we need to pad the values array with the
        // unassigned value or not.
        if (_ContainsUnauthoredValues(indices)) {
            primvar.SetUnauthoredValuesIndex(0);

            const VtValue paddedValues = _PushFirstValue(values, defaultValue);
            if (!paddedValues.IsEmpty()) {
                _SetAttribute(primvar.GetAttr(), paddedValues, usdTime);
                _SetAttribute(
                        primvar.CreateIndicesAttr(),
                        _ShiftIndices(indices, 1),
                        usdTime);
            }
            else {
                TF_CODING_ERROR("Unable to pad values array for <%s>",
                        primvar.GetAttr().GetPath().GetText());
            }
        }
        else {
            _SetAttribute(primvar.GetAttr(), values, usdTime);
            _SetAttribute(primvar.CreateIndicesAttr(), indices, usdTime);
        }
    }
    else {
        // If we are exporting animation, then we don't know definitively
        // whether we need to set the unauthoredValuesIndex.
        // In order to keep the primvar valid throughout the entire export
        // process, _always_ pad the values array with the unassigned value,
        // then go back and clean it up during the post-export.
        if (primvar.GetUnauthoredValuesIndex() != 0 &&
                _ContainsUnauthoredValues(indices)) {
            primvar.SetUnauthoredValuesIndex(0);
        }

        const VtValue paddedValues = _PushFirstValue(values, defaultValue);
        if (!paddedValues.IsEmpty()) {
            _SetAttribute(primvar.GetAttr(), paddedValues, usdTime);
            _SetAttribute(
                    primvar.CreateIndicesAttr(),
                    _ShiftIndices(indices, 1),
                    usdTime);
        }
        else {
            TF_CODING_ERROR("Unable to pad values array for <%s>",
                    primvar.GetAttr().GetPath().GetText());
        }
    }
}

void
PxrUsdTranslators_MeshWriter::_CleanupPrimvars()
{
    if (!_IsMeshAnimated()) {
        // Based on how _SetPrimvar() works, the cleanup phase doesn't apply to
        // non-animated meshes.
        return;
    }

    // On animated meshes, we forced an extra value (the "unassigned" or
    // "unauthored" value) into index 0 of any indexed primvar's values array.
    // If the indexed primvar doesn't need the unassigned value (because all
    // of the indices are assigned), then we can remove the unassigned value
    // and shift all the indices down.
    const UsdGeomMesh primSchema(GetUsdPrim());
    for (const UsdGeomPrimvar& primvar: primSchema.GetPrimvars()) {
        if (!primvar) {
            continue;
        }

        // Cleanup phase applies only to indexed primvars.
        // Unindexed primvars were written directly without modification.
        if (!primvar.IsIndexed()) {
            continue;
        }

        // If the unauthoredValueIndex is 0, that means we purposefully set it
        // to indicate that at least one time sample has unauthored values.
        const int unauthoredValueIndex = primvar.GetUnauthoredValuesIndex();
        if (unauthoredValueIndex == 0) {
            continue;
        }

        // If the unauthoredValueIndex wasn't 0 above, it must be -1 (the
        // fallback value in USD).
        TF_AXIOM(unauthoredValueIndex == -1);

        // Since the unauthoredValueIndex is -1, we never explicitly set it,
        // meaning that none of the samples contain an unassigned value.
        // Since we authored the unassigned value as index 0 in each primvar,
        // we can eliminate it now from all time samples.
        if (const UsdAttribute attr = primvar.GetAttr()) {
            VtValue val;
            if (attr.Get(&val, UsdTimeCode::Default())) {
                const VtValue newVal = _PopFirstValue(val);
                if (!newVal.IsEmpty()) {
                    attr.Set(newVal, UsdTimeCode::Default());
                }
            }
            std::vector<double> timeSamples;
            if (attr.GetTimeSamples(&timeSamples)) {
                for (const double& t : timeSamples) {
                    if (attr.Get(&val, t)) {
                        const VtValue newVal = _PopFirstValue(val);
                        if (!newVal.IsEmpty()) {
                            attr.Set(newVal, t);
                        }
                    }
                }
            }
        }

        // We then need to shift all the indices down one to account for index
        // 0 being eliminated.
        if (const UsdAttribute attr = primvar.GetIndicesAttr()) {
            VtIntArray val;
            if (attr.Get(&val, UsdTimeCode::Default())) {
                attr.Set(_ShiftIndices(val, -1), UsdTimeCode::Default());
            }
            std::vector<double> timeSamples;
            if (attr.GetTimeSamples(&timeSamples)) {
                for (const double& t : timeSamples) {
                    if (attr.Get(&val, t)) {
                        attr.Set(_ShiftIndices(val, -1), t);
                    }
                }
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

