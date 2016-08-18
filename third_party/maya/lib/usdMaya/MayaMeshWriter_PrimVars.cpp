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

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/transform.h"
#include "pxr/base/gf/gamma.h"

#include "pxr/usd/usdGeom/mesh.h"

#include <maya/MVector.h>
#include <maya/MColorArray.h>
#include <maya/MFloatArray.h>
#include <maya/MPlugArray.h>
#include <maya/MUintArray.h>
#include <maya/MColor.h>
#include <maya/MPoint.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshFaceVertex.h>

// This function tries to compress facevarying data to uniform, vertex or constant
// Loop over all elements and invalidate any of the 3 conditions
// Copy uniform and vertex values into a temporary array
// At the end resize to 1 or copy those temporary array into the resulting data
template <class VAL>
static void compressFVPrimvar(MFnMesh& m, VtArray<VAL> *data, TfToken *interpolation)
{
    std::vector<bool> valueOnVertex;
    std::vector<bool> valueOnFace;
    VtArray<VAL> vertexValue;
    VtArray<VAL> faceValue;
    
    bool isConstant=true;
    bool isUniform=true;
    bool isVertex=true;
    int numVertices = m.numVertices();
    int numPolygons = m.numPolygons();
    valueOnVertex.resize(numVertices, false);
    valueOnFace.resize(numPolygons, false);
    vertexValue.resize(numVertices);
    faceValue.resize(numPolygons);
    
    MItMeshFaceVertex itFV( m.object() );
    int fvi=0;
    for( itFV.reset(); !itFV.isDone(); itFV.next() ) {
        int faceId=itFV.faceId();
        int vertId=itFV.vertId();
        // Validate if constant by checking against the first element
        if (isConstant && fvi>0) {
            if (not GfIsClose((*data)[0], (*data)[fvi], 1e-9)) {
                isConstant=false;
            }
        }
        // Validate if uniform by storing the first value on a given face
        // and the if already stored, check if being a different value
        // on the same face
        if (isUniform) {
            if (valueOnFace[faceId]) {
                if (not GfIsClose(faceValue[faceId], 
                                            (*data)[fvi], 1e-9)) {
                    isUniform=false;
                }
            } else {
                valueOnFace[faceId]=true;
                faceValue[faceId]=(*data)[fvi];
            }
        }
        // Validate if vertex by storing the first value on a given vertex
        // and the if already stored, check if being a different value
        // on the same vertex
        if (isVertex) {
            if (valueOnVertex[vertId]) {
                if (not GfIsClose(vertexValue[vertId], 
                                            (*data)[fvi], 1e-9)) {
                    isVertex=false;
                }
            } else {
                valueOnVertex[vertId]=true;
                vertexValue[vertId]=(*data)[fvi];
            }
        }
        fvi++;
    }
    
    if (isConstant) {
        data->resize(1);
        *interpolation=UsdGeomTokens->constant;
    } else if (isUniform) {
        *data=faceValue;
        *interpolation=UsdGeomTokens->uniform;
    } else if(isVertex) {
        *data=vertexValue;
        *interpolation=UsdGeomTokens->vertex;
    } else {
        *interpolation=UsdGeomTokens->faceVarying;
    }
}

static inline
GfVec3f
_LinearColorFromColorSet(
        const MColor& mayaColor,
        bool isDisplayColor)
{
    // we assume all color sets except displayColor are in linear space.
    // if we got a color from colorSetData and we're a displayColor, we
    // need to convert it to linear.
    GfVec3f c(mayaColor[0], mayaColor[1], mayaColor[2]);
    if (isDisplayColor) {
        return GfConvertDisplayToLinear(c);
    }
    return c;
}

// Collect values from the colorset
// If gathering for displayColor, set the unpainted values to
// the underlying shaders values, else set to 1,1,1,1
// Values are gathered per "facevertex" but then the data
// is compressed to constant, uniform and vertex if possible
// RGB and Alpha data are compressed independently
// RGBA data is compressed as a single Vec4f array
// NOTE: We could only fill RGB and Alpha and then
// do a merge, compress and unmerge if RGBA is needed
// but for code simplicity we always fill the 3 arrays
bool MayaMeshWriter::_GetMeshColorSetData(
            MFnMesh& m,
            MString colorSet,
            bool isDisplayColor,
            const VtArray<GfVec3f>& shadersRGBData,
            const VtArray<float>& shadersAlphaData,
            VtArray<GfVec3f> *RGBData, TfToken *RGBInterp,
            VtArray<GfVec4f> *RGBAData, TfToken *RGBAInterp,
            VtArray<float> *AlphaData, TfToken *AlphaInterp,
            MFnMesh::MColorRepresentation *colorSetRep,
            bool *clamped)
{
    // If there are no colors, return immediately as failure
    if (m.numColors(colorSet)==0) {
        return false;
    }
    
    // Get ColorSet representation and clamping
    *colorSetRep = m.getColorRepresentation(colorSet);
    *clamped = m.isColorClamped(colorSet);
            
    MColorArray colorSetData;
    const MColor unsetColor(-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX);
    if (m.getFaceVertexColors (colorSetData, &colorSet, &unsetColor)
        == MS::kFailure) {
        return false;
    }

    // Resize the returning containers with FaceVertex amounts
	RGBData->resize(colorSetData.length());
	AlphaData->resize(colorSetData.length());
	RGBAData->resize(colorSetData.length());

    // Loop over every face vertex to populate colorArray
    MItMeshFaceVertex itFV( m.object() );
    int fvi=0;
    for( itFV.reset(); !itFV.isDone(); itFV.next() ) {
        // Initialize RGBData and AlphaData to (1,1,1,1) and then
        // if isDisplayColor from  shader values (constant or uniform)
        GfVec3f RGBValue=GfVec3f(1.0,1.0,1.0);
        float AlphaValue=1;

        // NOTE, shadersRGBData is already linear
        if (isDisplayColor && shadersRGBData.size() == static_cast<size_t>(m.numPolygons())) {
            RGBValue=shadersRGBData[itFV.faceId()];
        } else if (isDisplayColor && shadersRGBData.size()==1) {
            RGBValue=shadersRGBData[0];
        }
        if (isDisplayColor && shadersAlphaData.size() == static_cast<size_t>(m.numPolygons())) {
            AlphaValue=shadersAlphaData[itFV.faceId()];
        } else if (isDisplayColor && shadersAlphaData.size()==1) {
            AlphaValue=shadersAlphaData[0];
        }
        // Assign retrieved color set values
        // unauthored color set values are ==unsetColor
        // for those we use the previously initialized values
        if (colorSetData[fvi]!=unsetColor) {
            if ((*colorSetRep) == MFnMesh::kAlpha) {
                AlphaValue=colorSetData[fvi][3];
            } else if ((*colorSetRep) == MFnMesh::kRGB) {
                RGBValue=_LinearColorFromColorSet(colorSetData[fvi], isDisplayColor);
                AlphaValue=1;
            } else if ((*colorSetRep) == MFnMesh::kRGBA) {
                RGBValue=_LinearColorFromColorSet(colorSetData[fvi], isDisplayColor);
                AlphaValue=colorSetData[fvi][3];
            }
        }
        (*RGBData)[fvi]=RGBValue;
        (*AlphaData)[fvi]=AlphaValue;
        (*RGBAData)[fvi]=GfVec4f(RGBValue[0], RGBValue[1],
                                RGBValue[2], AlphaValue);
        fvi++;
    }
    compressFVPrimvar(m, RGBData, RGBInterp);
    compressFVPrimvar(m, AlphaData, AlphaInterp);
    compressFVPrimvar(m, RGBAData, RGBAInterp);
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
    UsdGeomGprim &primSchema, const TfToken name,
    const VtArray<float>& data, TfToken interpolation,
    bool clamped)
{
    unsigned int numValues=data.size();
    if (numValues==0) return false;
    TfToken interp=interpolation;
    if (numValues==1 && interp==UsdGeomTokens->constant) {
        interp=TfToken();
    }
	UsdGeomPrimvar colorSet = primSchema.CreatePrimvar(name, 
                                             SdfValueTypeNames->FloatArray, 
                                             interp );
	colorSet.Set(data);
    SetPVCustomData(colorSet.GetAttr(), clamped);
    return true;
}

bool MayaMeshWriter::_createRGBPrimVar(
    UsdGeomGprim &primSchema, const TfToken name,
    const VtArray<GfVec3f>& data, TfToken interpolation,
    bool clamped)
{
    unsigned int numValues=data.size();
    if (numValues==0) return false;
    TfToken interp=interpolation;
    if (numValues==1 && interp==UsdGeomTokens->constant) {
        interp=TfToken();
    }
	UsdGeomPrimvar colorSet = primSchema.CreatePrimvar(name, 
                                             SdfValueTypeNames->Color3fArray, 
                                             interp );
	colorSet.Set(data);
    SetPVCustomData(colorSet.GetAttr(), clamped);
    return true;
}

bool MayaMeshWriter::_createRGBAPrimVar(
    UsdGeomGprim &primSchema, const TfToken name,
    const VtArray<GfVec4f>& RGBAData, TfToken RGBAInterp,
    bool clamped)
{
    unsigned int numValues=RGBAData.size();
    if (numValues==0) return false;
    TfToken interp=RGBAInterp;
    if (numValues==1 && interp==UsdGeomTokens->constant) {
        interp=TfToken();
    }
	UsdGeomPrimvar colorSet = primSchema.CreatePrimvar(name, 
                                             SdfValueTypeNames->Float4Array, 
                                             interp );
	colorSet.Set(RGBAData);
    SetPVCustomData(colorSet.GetAttr(), clamped);
    return true;
}

bool MayaMeshWriter::_setDisplayPrimVar(
    UsdGeomGprim &primSchema,
    MFnMesh::MColorRepresentation colorRep,
    VtArray<GfVec3f> RGBData, TfToken RGBInterp,
    VtArray<float> AlphaData, TfToken AlphaInterp,
    bool clamped, bool authored)
{

    UsdAttribute colorAttr = primSchema.GetDisplayColorAttr();
    if (not colorAttr.HasAuthoredValueOpinion() and not RGBData.empty()) {
        UsdGeomPrimvar displayColor = primSchema.GetDisplayColorPrimvar();
        if (RGBInterp != displayColor.GetInterpolation())
            displayColor.SetInterpolation(RGBInterp);
        displayColor.Set(RGBData);
        bool authRGB=authored;
        if (colorRep == MFnMesh::kAlpha) { authRGB=false; }
        if (authRGB) {
            colorAttr.SetCustomDataByKey(TfToken("Authored"), VtValue(authRGB));
            SetPVCustomData(colorAttr, clamped);
        }
    }
    
    UsdAttribute alphaAttr = primSchema.GetDisplayOpacityAttr();

    // if we already have an authored value, don't try to write a new one.
    if (not alphaAttr.HasAuthoredValueOpinion()) {

        if (not AlphaData.empty()) {
            // we consider a single alpha value that is 1.0 to be the "default"
            // value.  We only want to write values that are not the "default".
            bool hasDefaultAlpha = AlphaData.size() == 1 and GfIsClose(AlphaData[0], 1.0, 1e-9);
            if (not hasDefaultAlpha) {
                UsdGeomPrimvar displayOpacity = primSchema.GetDisplayOpacityPrimvar();
                if (AlphaInterp != displayOpacity.GetInterpolation())
                    displayOpacity.SetInterpolation(AlphaInterp);
                displayOpacity.Set(AlphaData);
                bool authAlpha=authored;
                if (colorRep == MFnMesh::kRGB) { authAlpha=false; }
                if (authAlpha) {
                    alphaAttr.SetCustomDataByKey(TfToken("Authored"), VtValue(authAlpha));
                    SetPVCustomData(alphaAttr, clamped);
                }
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
