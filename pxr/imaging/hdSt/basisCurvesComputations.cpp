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

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4i.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE


HdSt_BasisCurvesIndexBuilderComputation::HdSt_BasisCurvesIndexBuilderComputation(
    HdBasisCurvesTopology *topology, bool forceLines)
    : _topology(topology),
      _forceLines(forceLines)
{
}

void
HdSt_BasisCurvesIndexBuilderComputation::GetBufferSpecs(
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
    // Note: This is used only when the wrap mode is segmented and is similar to
    //       the GL_LINES primitive mode. Vertices (0,1) form a line, (2,3) form
    //       the next line and so on.
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

    // If have topology has indices set, map the generated indices
    // with the given indices.
    if (!_topology->HasIndices())
    {
        std::copy(indices.begin(), indices.end(), finalIndices.begin());
    }
    else
    {
        VtIntArray const &curveIndices = _topology->GetCurveIndices();
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
    // Note: This is similiar to the GL_LINE_STRIP and GL_LINE_LOOP primitive
    //       modes where each pair of adjacent vertices form a line.
    const TfToken basis = _topology->GetCurveBasis();
    const TfToken wrap = _topology->GetCurveWrap();
    // Skip first and last segments for catRom when not using pinned wrap mode
    // since the curve interpolates at all except the end points.
    const bool skipFirstAndLastSegs =
        (basis == HdTokens->catmullRom ||
         basis == HdTokens->centripetalCatmullRom) &&
         wrap != HdTokens->pinned;

    std::vector<GfVec2i> indices;
    // primIndices stores the curve index that generated each line segment.
    std::vector<int> primIndices;
    const VtArray<int> vertexCounts = _topology->GetCurveVertexCounts();
    const bool periodic = wrap == HdTokens->periodic;
    int vertexIndex = 0; // Index of next vertex to emit
    int curveIndex = 0;  // Index of next curve to emit
    // For each curve
    TF_FOR_ALL(itCounts, vertexCounts) {
        int v0 = vertexIndex;
        int v1;
        // Store first vert index incase we are wrapping
        const int firstVert = v0;
        ++ vertexIndex;
        for(int i = 1;i < *itCounts; ++i) {
            v1 = vertexIndex;
            ++ vertexIndex;
            if (!skipFirstAndLastSegs || (i > 1 && i < (*itCounts)-1)) {
                indices.push_back(GfVec2i(v0, v1));
                // Map this line segment back to the curve it came from
                primIndices.push_back(curveIndex);
            }
            v0 = v1;
        }
        if (periodic) {
            indices.push_back(GfVec2i(v0, firstVert));
            primIndices.push_back(curveIndex);
        }
        ++curveIndex;
    }

    VtVec2iArray finalIndices(indices.size());

    // If have topology has indices set, map the generated indices
    // with the given indices.
    if (!_topology->HasIndices())
    {
        std::copy(indices.begin(), indices.end(), finalIndices.begin());
    }
    else
    {
        VtIntArray const &curveIndices = _topology->GetCurveIndices();
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
    HD_TRACE_FUNCTION();

    /* 
    Here's a diagram of what's happening in this code:

    For open (non periodic) curves:

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

    For pinned (non periodic) curves:
      indices with a trailing quote (') are added

      bspline (vStep = 1)
      
      0'-----0'-----0------1------2------3------4------4'-----4' (vertex index)
      [======= seg0 =======]
             [======= seg1 =======]
                    [======= seg2 =======]
                           [======= seg3 =======]
                                  [======= seg4 =======]
                                         [======= seg5 =======]

      catmullRom (vStep = 1)
      0'-----0------1------2------3------4------4' (vertex index)
      [======= seg0 =======]
             [======= seg1 =======]
                    [======= seg2 =======]
                           [======= seg3 =======]

    For closed (periodic) curves:

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

    const VtArray<int> vertexCounts = _topology->GetCurveVertexCounts();
    const bool periodic = _topology->GetCurveWrap() == HdTokens->periodic;
    int vStep;
    TfToken basis = _topology->GetCurveBasis();
    if(basis == HdTokens->bezier) {
        vStep = 3;
    }
    else {
        vStep = 1;
    }

    // The "pinned" wrap mode is relevant only to bspline and catmull-rom
    // curves to make the interpolated curve begin and end at the first and last
    // control vertices respectively.
    // Instead of computing the phantom points using reflection such that
    // p[-1] = 2 * p[0] - p[1] and p[n] = 2 * p[n-1] - p[n-2],
    // we simply repeat the start and end points (once for catmull-rom and twice
    // for bspline) to generate additional segment(s) at the start and end of
    // each curve. This simplifies the implementation considerably by avoiding 
    // expansion of authored primvar data and factoring it when computing the 
    // topology index buffer.
    //
    const bool pinned = (_topology->GetCurveWrap() == HdTokens->pinned) &&
                        (vStep == 1);

    auto addPinnedSegment =
        [&](int startIndex, int curveIndex, int cvCount, bool start) {

            // Triplicate the start/end vertex for bspline and duplicate for
            // catmull-rom. This generates 2 segments each at the start and end
            // of each curve for bspline and 1 for catmull-rom curves.
            if (start) {
                const int v0 = startIndex;
                const int endIndex = v0 + cvCount - 1;
                const int v1 = std::min(v0 + 1, endIndex);
                const int v2 = std::min(v0 + 2, endIndex);

                if (basis == HdTokens->bspline) {
                    indices.push_back(GfVec4i(v0, v0, v0, v1));
                    primIndices.push_back(curveIndex);
                }

                indices.push_back(GfVec4i(v0, v0, v1, v2));
                primIndices.push_back(curveIndex);

            } else {
                const GfVec4i lastSeg = indices.back();
                indices.push_back(GfVec4i(lastSeg[1], lastSeg[2],
                                          lastSeg[3], lastSeg[3]));
                primIndices.push_back(curveIndex);

                if (basis == HdTokens->bspline) {
                    indices.push_back(GfVec4i(lastSeg[2], lastSeg[3],
                                              lastSeg[3], lastSeg[3]));
                    primIndices.push_back(curveIndex);
                }
            }
        };

    int vertexIndex = 0;
    int curveIndex = 0;
    for (const int &count : vertexCounts) {
        if (count < 2) {
            // Minimum vertex count is 2 for pinned curves and 4 otherwise.
            continue;
        }

        // If we're closing the curve, make sure that we have enough
        // segments to wrap all the way back to the beginning.
        // Note that the value calculated  does _not_ account for the additional
        // segments for pinned (non-periodic) curves.
        // (this should match the logic in
        //  HdBasisCurvesTopology::CalculateNeededNumberOfVaryingControlPoints)
        const int numSegs = periodic?
            std::max<int>(count / vStep, 1) :
            (std::max<int>((count - 4), 0) / vStep) + 1;

        if (pinned) {
            addPinnedSegment(vertexIndex, curveIndex, count, /*start =*/ true);
        }

        for (int i = 0;i < numSegs; ++i) {

            // Set up curve segments based on curve basis
            GfVec4i seg;
            int offset = i*vStep;
            for (int v = 0; v < 4; ++v) {
                // If there are not enough verts to round out the segment
                // just repeat the last vert.
                seg[v] = periodic 
                    ? vertexIndex + ((offset + v) % count)
                    : vertexIndex + std::min(offset + v, (count - 1));
            }
            indices.push_back(seg);
            primIndices.push_back(curveIndex);
        }

        if (pinned) {
            addPinnedSegment(vertexIndex, curveIndex, count, /*start =*/ false);
        }

        vertexIndex += count;
        curveIndex++;
    }

    VtVec4iArray finalIndices(indices.size());

    // If have topology has indices set, map the generated indices
    // with the given indices.
    if (!_topology->HasIndices())
    {
        std::copy(indices.begin(), indices.end(), finalIndices.begin());
    }
    else
    {
        VtIntArray const &curveIndices = _topology->GetCurveIndices();
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

    _SetResult(std::make_shared<HdVtBufferSource>(
                                HdTokens->indices, 
                                VtValue(result._indices)));

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

HdBufferSourceSharedPtrVector
HdSt_BasisCurvesIndexBuilderComputation::GetChainedBuffers() const
{
    return { _primitiveParam };
}


PXR_NAMESPACE_CLOSE_SCOPE

