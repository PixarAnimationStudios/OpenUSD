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
#include "pxr/imaging/hdSt/basisCurvesComputations.h"
#include "pxr/imaging/hd/basisCurvesTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4i.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE


template <typename T> VtArray<T>
InterpolateVarying(size_t numVerts, VtIntArray const & vertexCounts, TfToken wrap,
    TfToken basis, VtArray<T> const & authoredValues)
{
    VtArray<T> outputValues(numVerts);

    size_t srcIndex = 0;
    size_t dstIndex = 0;

    if (wrap == HdTokens->periodic) {
        // XXX : Add support for periodic curves
        TF_WARN("Varying data is only supported for non-periodic curves.");
    }

    TF_FOR_ALL(itVertexCount, vertexCounts) {
        int nVerts = *itVertexCount;

        // Handling for the case of potentially incorrect vertex counts 
        if(nVerts < 1) {
            continue;
        }

        if(basis == HdTokens->catmullRom || basis == HdTokens->bSpline) {
            // For splines with a vstep of 1, we are doing linear interpolation 
            // between segments, so all we do here is duplicate the first and 
            // last outputValues. Since these are never acutally used during 
            // drawing, it would also work just to set the to 0.
            outputValues[dstIndex] = authoredValues[srcIndex];
            ++dstIndex;
            for (int i = 1; i < nVerts - 2; ++i){
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++dstIndex; ++srcIndex;
            }
            outputValues[dstIndex] = authoredValues[srcIndex];
            ++dstIndex;
            outputValues[dstIndex] = authoredValues[srcIndex];
            ++dstIndex; ++srcIndex;
        }
        else if (basis == HdTokens->bezier){
            // For bezier splines, we map the linear values to cubic values
            // the begin value gets mapped to the first two vertices and
            // the end value gets mapped to the last two vertices in a segment.
            // shaders can choose to access value[1] and value[2] when linearly
            // interpolating a value, which happens to match up with the
            // indexing to use for catmullRom and bSpline basis.
            int vStep = 3;
            outputValues[dstIndex] = authoredValues[srcIndex];
            ++dstIndex; // don't increment the srcIndex
            outputValues[dstIndex] = authoredValues[srcIndex];
            ++dstIndex; ++ srcIndex;

            // vstep - 1 control points will have an interpolated value
            for(int i = 2; i < nVerts - 2; i += vStep) {
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++ dstIndex; // don't increment the srcIndex
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++ dstIndex; // don't increment the srcIndex
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++ dstIndex; ++ srcIndex; 
            }
            outputValues[dstIndex] = authoredValues[srcIndex];
            ++dstIndex; // don't increment the srcIndex
            outputValues[dstIndex] = authoredValues[srcIndex];
            ++dstIndex; ++ srcIndex;
        }
        else {
            TF_WARN("Unsupported basis: '%s'", basis.GetText());
        }
    }
    TF_VERIFY(srcIndex == authoredValues.size());
    TF_VERIFY(dstIndex == numVerts);
    
    return outputValues;
}

HdSt_BasisCurvesIndexBuilderComputation::HdSt_BasisCurvesIndexBuilderComputation(
    HdBasisCurvesTopology *topology, bool forceLines)
    : _topology(topology),
      _forceLines(forceLines)
{
}

void
HdSt_BasisCurvesIndexBuilderComputation::AddBufferSpecs(
    HdBufferSpecVector *specs) const
{
    // index buffer
    if(!_forceLines && _topology->GetCurveType() == HdTokens->cubic) {
        specs->emplace_back(HdTokens->indices,
                            HdTupleType{HdTypeInt32Vec4, 1});
    }
    else {
        specs->emplace_back(HdTokens->indices,
                            HdTupleType{HdTypeInt32Vec2, 1});
    }

    // primitive index buffer (curve id per curve segment) is used only
    // when the basis curve has uniform primvars.
    // XXX: we currently create it even when the curve has no uniform primvars
    specs->emplace_back(HdTokens->primitiveParam,
                        HdTupleType{HdTypeInt32, 1});
}

HdSt_BasisCurvesIndexBuilderComputation::IndexAndPrimIndex
HdSt_BasisCurvesIndexBuilderComputation::_BuildLinesIndexArray()
{
    std::vector<GfVec2i> indices;
    std::vector<int> primIndices;
    VtArray<int> vertexCounts = _topology->GetCurveVertexCounts();

    int vertexIndex = 0;
    int curveIndex = 0;
    TF_FOR_ALL(itCounts, vertexCounts) {
        for(int i = 0; i < *itCounts; i+= 2) {
            indices.push_back(GfVec2i(vertexIndex, vertexIndex + 1));
            vertexIndex += 2;
            primIndices.push_back(curveIndex);
        }
        curveIndex++;
    }

    VtVec2iArray finalIndices(indices.size());
    VtIntArray const &curveIndices = _topology->GetCurveIndices();

    // If have topology has indices set, map the generated indices
    // with the given indices.
    if (curveIndices.empty())
    {
        std::copy(indices.begin(), indices.end(), finalIndices.begin());
    }
    else
    {
        size_t lineCount = indices.size();
        int maxIndex = curveIndices.size() - 1;

        for (size_t lineNum = 0; lineNum < lineCount; ++lineNum)
        {
            const GfVec2i &line = indices[lineNum];

            int i0 = std::min(line[0], maxIndex);
            int i1 = std::min(line[1], maxIndex);

            int v0 = curveIndices[i0];
            int v1 = curveIndices[i1];

            finalIndices[lineNum].Set(v0, v1);
        }
    }

    VtIntArray finalPrimIndices(primIndices.size());
    std::copy(  primIndices.begin(), 
                primIndices.end(), 
                finalPrimIndices.begin());

    return IndexAndPrimIndex(VtValue(finalIndices), VtValue(finalPrimIndices));
}

HdSt_BasisCurvesIndexBuilderComputation::IndexAndPrimIndex
HdSt_BasisCurvesIndexBuilderComputation::_BuildLineSegmentIndexArray()
{
    std::vector<GfVec2i> indices;
    std::vector<int> primIndices;
    VtArray<int> vertexCounts = _topology->GetCurveVertexCounts();
    bool wrap = _topology->GetCurveWrap() == HdTokens->periodic;
    int vertexIndex = 0;
    int curveIndex = 0;
    TF_FOR_ALL(itCounts, vertexCounts) {
        int v0 = vertexIndex;
        int v1;
        // Store first vert index incase we are wrapping
        int firstVert = v0;
        ++ vertexIndex;
        for(int i = 1;i < *itCounts; ++i) {
            v1 = vertexIndex;
            ++ vertexIndex;
            indices.push_back(GfVec2i(v0, v1));
            v0 = v1;
            primIndices.push_back(curveIndex);
        }
        if(wrap) {
            indices.push_back(GfVec2i(v0, firstVert));
            primIndices.push_back(curveIndex);
        }
        ++curveIndex;
    }

    VtVec2iArray finalIndices(indices.size());
    VtIntArray const &curveIndices = _topology->GetCurveIndices();

    // If have topology has indices set, map the generated indices
    // with the given indices.
    if (curveIndices.empty())
    {
        std::copy(indices.begin(), indices.end(), finalIndices.begin());
    }
    else
    {
        size_t lineCount = indices.size();
        int maxIndex = curveIndices.size() - 1;

        for (size_t lineNum = 0; lineNum < lineCount; ++lineNum)
        {
            const GfVec2i &line = indices[lineNum];

            int i0 = std::min(line[0], maxIndex);
            int i1 = std::min(line[1], maxIndex);

            int v0 = curveIndices[i0];
            int v1 = curveIndices[i1];

            finalIndices[lineNum].Set(v0, v1);
        }
    }

    VtIntArray finalPrimIndices(primIndices.size());
    std::copy(  primIndices.begin(),
                primIndices.end(),
                finalPrimIndices.begin());

    return IndexAndPrimIndex(VtValue(finalIndices), VtValue(finalPrimIndices));
}

HdSt_BasisCurvesIndexBuilderComputation::IndexAndPrimIndex
HdSt_BasisCurvesIndexBuilderComputation::_BuildCubicIndexArray()
{

    /* 
    Here's a diagram of what's happening in this code:

    For open (non periodic, wrap = false) curves:

      bezier (vStep = 3)
      0------1------2------3------4------5------6 (vertex index)
      [======= seg0 =======]
                           [======= seg1 =======]


      bspline / catmullRom (vStep = 1)
      0------1------2------3------4------5------6 (vertex index)
      [======= seg0 =======]
             [======= seg1 =======]
                    [======= seg2 =======]
                           [======= seg3 =======]


    For closed (periodic, wrap = true) curves:

       periodic bezier (vStep = 3)
       0------1------2------3------4------5------0 (vertex index)
       [======= seg0 =======]
                            [======= seg1 =======]


       periodic bspline / catmullRom (vStep = 1)
       0------1------2------3------4------5------0------1------2 (vertex index)
       [======= seg0 =======]
              [======= seg1 =======]
                     [======= seg2 =======]
                            [======= seg3 =======]
                                   [======= seg4 =======]
                                          [======= seg5 =======]
    */

    std::vector<GfVec4i> indices;
    std::vector<int> primIndices;

    VtArray<int> vertexCounts = _topology->GetCurveVertexCounts();
    bool wrap = _topology->GetCurveWrap() == HdTokens->periodic;
    int vStep;
    TfToken basis = _topology->GetCurveBasis();
    if(basis == HdTokens->bezier) {
        vStep = 3;
    }
    else {
        vStep = 1;
    }

    int vertexIndex = 0;
    int curveIndex = 0;
    TF_FOR_ALL(itCounts, vertexCounts) {
        int count = *itCounts;
        // The first segment always eats up 4 verts, not just vstep, so to
        // compensate, we break at count - 3.
        int numSegs;

        // If we're closing the curve, make sure that we have enough
        // segments to wrap all the way back to the beginning.
        if (wrap) {
            numSegs = count / vStep;
        } else {
            numSegs = ((count - 4) / vStep) + 1;
        }

        for(int i = 0;i < numSegs; ++i) {

            // Set up curve segments based on curve basis
            GfVec4i seg;
            int offset = i*vStep;
            for(int v = 0;v < 4; ++v) {
                // If there are not enough verts to round out the segment
                // just repeat the last vert.
                seg[v] = wrap 
                    ? vertexIndex + ((offset + v) % count)
                    : vertexIndex + std::min(offset + v, (count -1));
            }
            indices.push_back(seg);
            primIndices.push_back(curveIndex);
        }
        vertexIndex += count;
        curveIndex++;
    }

    VtVec4iArray finalIndices(indices.size());
    VtIntArray const &curveIndices = _topology->GetCurveIndices();

    // If have topology has indices set, map the generated indices
    // with the given indices.
    if (curveIndices.empty())
    {
        std::copy(indices.begin(), indices.end(), finalIndices.begin());
    }
    else
    {
        size_t lineCount = indices.size();
        int maxIndex = curveIndices.size() - 1;

        for (size_t lineNum = 0; lineNum < lineCount; ++lineNum)
        {
            const GfVec4i &line = indices[lineNum];

            int i0 = std::min(line[0], maxIndex);
            int i1 = std::min(line[1], maxIndex);
            int i2 = std::min(line[2], maxIndex);
            int i3 = std::min(line[3], maxIndex);

            int v0 = curveIndices[i0];
            int v1 = curveIndices[i1];
            int v2 = curveIndices[i2];
            int v3 = curveIndices[i3];

            finalIndices[lineNum].Set(v0, v1, v2, v3);
        }
    }

    VtIntArray finalPrimIndices(primIndices.size());
    std::copy(primIndices.begin(), primIndices.end(), finalPrimIndices.begin());    

    return IndexAndPrimIndex(VtValue(finalIndices), VtValue(finalPrimIndices));
}

bool
HdSt_BasisCurvesIndexBuilderComputation::Resolve()
{
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();

    IndexAndPrimIndex result;

    if(!_forceLines && _topology->GetCurveType() == HdTokens->cubic) {
        result = _BuildCubicIndexArray();
    } else {
        if (_topology->GetCurveWrap() == HdTokens->segmented) {
            result = _BuildLinesIndexArray();
        } else {
            result = _BuildLineSegmentIndexArray();
        }
    }

    _SetResult(HdBufferSourceSharedPtr(
                   new HdVtBufferSource(
                       HdTokens->indices,
                       VtValue(result._indices))));

    // the primitive param buffer is used only when the basis curve
    // has uniform primvars.
    // XXX: we currently create it even when the curve has no uniform primvars
    _primitiveParam.reset(new HdVtBufferSource(
                                HdTokens->primitiveParam,
                                VtValue(result._primIndices)));

    _SetResolved();
    return true;
}

bool
HdSt_BasisCurvesIndexBuilderComputation::_CheckValid() const
{
    return true;
}

bool
HdSt_BasisCurvesIndexBuilderComputation::HasChainedBuffer() const
{
    return true;
}

HdBufferSourceVector
HdSt_BasisCurvesIndexBuilderComputation::GetChainedBuffers() const
{
    return { _primitiveParam };
}

// -------------------------------------------------------------------------- //
// BasisCurves Widths Interpolater
// -------------------------------------------------------------------------- //

HdSt_BasisCurvesWidthsInterpolaterComputation::HdSt_BasisCurvesWidthsInterpolaterComputation(
    HdBasisCurvesTopology *topology,
    VtFloatArray authoredWidths)
 : _topology(topology)
 , _authoredWidths(authoredWidths)
{
}

void
HdSt_BasisCurvesWidthsInterpolaterComputation::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    specs->emplace_back(HdTokens->widths, HdTupleType{HdTypeFloat, 1});
}

bool
HdSt_BasisCurvesWidthsInterpolaterComputation::Resolve()
{
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    // We need to interpolate widths depending on the primvar type
    size_t numVerts = _topology->CalculateNeededNumberOfControlPoints();
    VtArray<float> widths(numVerts);
    size_t size = _authoredWidths.size();

    if(size <= 1) {
        // Uniform or missing data
        float width = size==0 ? 1.f : _authoredWidths[0];
        for(size_t i = 0; i < numVerts; ++ i) {
            widths[i] = width;
        }
    }
    else if(size == numVerts) {
        // Vertex data
        widths = _authoredWidths;
    }
    else if(size == _topology->CalculateNeededNumberOfVaryingControlPoints()) {
        // Varying data
        widths = InterpolateVarying<float>
                    (numVerts, _topology->GetCurveVertexCounts(), _topology->GetCurveWrap(),
                    _topology->GetCurveBasis(), _authoredWidths);
    }
    else {
        // Fallback
        for(size_t i = 0; i < numVerts; ++ i) {
            widths[i] = 1.0;
        }
        TF_WARN("Incorrect number of widths, using default 1.0 for rendering.");
    }

    _SetResult(HdBufferSourceSharedPtr(
                   new HdVtBufferSource(
                       HdTokens->widths,
                       VtValue(widths))));
    _SetResolved();
    return true;
}

bool
HdSt_BasisCurvesWidthsInterpolaterComputation::_CheckValid() const
{
    return true;
}

// -------------------------------------------------------------------------- //
// BasisCurves Normals Interpolater
// -------------------------------------------------------------------------- //

HdSt_BasisCurvesNormalsInterpolaterComputation::HdSt_BasisCurvesNormalsInterpolaterComputation(
    HdBasisCurvesTopology *topology,
    VtVec3fArray authoredNormals)
 : _topology(topology)
 , _authoredNormals(authoredNormals)
{
}

void
HdSt_BasisCurvesNormalsInterpolaterComputation::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    specs->emplace_back(HdTokens->normals, HdTupleType{HdTypeFloatVec3, 1});
}

bool
HdSt_BasisCurvesNormalsInterpolaterComputation::Resolve()
{
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();

    // We need to interpolate normals depending on the primvar type
    size_t numVerts = _topology->CalculateNeededNumberOfControlPoints();
    VtVec3fArray normals(numVerts);
    size_t size = _authoredNormals.size();

    if(size == 1) {
        // Uniform data
        GfVec3f normal = _authoredNormals[0];
        for(size_t i = 0; i < numVerts; ++ i) {
            normals[i] = normal;
        }
    }
    else if(size == numVerts) {
        // Vertex data
        normals = _authoredNormals;
    }
    else if(size == _topology->CalculateNeededNumberOfVaryingControlPoints()) {
        // Varying data
        normals = InterpolateVarying<GfVec3f>
                    (numVerts, _topology->GetCurveVertexCounts(), _topology->GetCurveWrap(),
                    _topology->GetCurveBasis(), _authoredNormals);
    }
    else {
        // Fallback
        GfVec3f normal(1.0,0.0,0.0);
        for(size_t i = 0; i < numVerts; ++ i) {
            normals[i] = normal;
        }
        TF_WARN("Incorrect number of normals, using default GfVec3f(1,0,0) for rendering.");
    }

    _SetResult(HdBufferSourceSharedPtr(new HdVtBufferSource(HdTokens->normals,
                                                            VtValue(normals))));
    _SetResolved();
    return true;
}

bool
HdSt_BasisCurvesNormalsInterpolaterComputation::_CheckValid() const
{
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

