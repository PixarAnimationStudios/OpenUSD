//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/dashDotLinesComputations.h"
#include "pxr/imaging/hd/dashDotLinesTopology.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4i.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

HdSt_DashDotLinesIndexBuilderComputation::HdSt_DashDotLinesIndexBuilderComputation(
    HdDashDotLinesTopology *topology, bool forceLines)
    : _topology(topology),
      _forceLines(forceLines)
{
}

void
HdSt_DashDotLinesIndexBuilderComputation::GetBufferSpecs(
    HdBufferSpecVector *specs) const
{
    // index buffer
    if (_topology->NeedAdjacentPoints())
    {
        specs->emplace_back(HdTokens->indices,
            HdTupleType{ HdTypeInt32Vec3, 1 });
    }
    else {
        specs->emplace_back(HdTokens->indices,
                            HdTupleType{HdTypeInt32Vec2, 1});
    }

    // primitive index buffer (curve id per curve segment) is used only
    // when the dashdot lines has uniform primvars.
    // XXX: we currently create it even when the curve has no uniform primvars
    specs->emplace_back(HdTokens->primitiveParam,
                        HdTupleType{HdTypeInt32, 1});
}

HdSt_DashDotLinesIndexBuilderComputation::IndexAndPrimIndex
HdSt_DashDotLinesIndexBuilderComputation::_BuildLineSegmentIndexArray(bool needAdjInfo)
{
    // The indices when we don't need adjacent information.
    std::vector<GfVec2i> indices;
    // The indices when we need adjacent information.
    std::vector<GfVec3i> indicesAdj;
    // primIndices stores the curve index that generated each line segment.
    std::vector<int> primIndices;
    const VtArray<int> vertexCounts = _topology->GetCurveVertexCounts();
    int vertexIndex = 0; // Index of next vertex to emit
    int curveIndex = 0;  // Index of next curve to emit
    // For each curve
    TF_FOR_ALL(itCounts, vertexCounts) {
        if (needAdjInfo)
        {
            int maxVertexIndex = vertexIndex + *itCounts - 1;
            for (int i = vertexIndex; i < maxVertexIndex; ++i) {
                // For each line segment, it will be converted to one quad. So 
                // we add two triangle indices.
                int currentSegmentStart = i * 4;
                indicesAdj.push_back(GfVec3i(currentSegmentStart, currentSegmentStart + 1,
                    currentSegmentStart + 2));
                indicesAdj.push_back(GfVec3i(currentSegmentStart + 2, currentSegmentStart + 1,
                    currentSegmentStart + 3));
            }
            vertexIndex = maxVertexIndex;
        }
        else
        {
            // The first vertex of the segment.
            int v0 = vertexIndex;
            // The second vertex of the segment.
            int v1;
            // Store first vert index incase we are wrapping
            const int firstVert = v0;
            ++vertexIndex;
            for (int i = 1; i < *itCounts; ++i) {
                v1 = vertexIndex;
                ++vertexIndex;
                indices.push_back(GfVec2i(v0, v1));
                // Map this line segment back to the curve it came from
                primIndices.push_back(curveIndex);
                v0 = v1;
            }
            ++curveIndex;
        }
    }

    VtVec2iArray finalIndices(indices.size());
    VtVec3iArray finalIndicesAdj(indicesAdj.size());

    // If have topology has indices set, map the generated indices
    // with the given indices.
    if (!_topology->HasIndices())
    {
        if (needAdjInfo)
            std::copy(indicesAdj.begin(), indicesAdj.end(), finalIndicesAdj.begin());
        else
            std::copy(indices.begin(), indices.end(), finalIndices.begin());
    }
    else
    {
        if (needAdjInfo)
        {
            TF_CODING_ERROR("Indices is set for styled BasisCurve");
        }
        else
        {
            VtIntArray const& curveIndices = _topology->GetCurveIndices();
            size_t lineCount = needAdjInfo ? indicesAdj.size() : indices.size();
            int maxIndex = curveIndices.size() - 1;

            for (size_t lineNum = 0; lineNum < lineCount; ++lineNum)
            {
                const GfVec2i& line = indices[lineNum];

                int i0 = std::min(line[0], maxIndex);
                int i1 = std::min(line[1], maxIndex);

                int v0 = curveIndices[i0];
                int v1 = curveIndices[i1];

                finalIndices[lineNum].Set(v0, v1);
            }
        }
    }

    VtIntArray finalPrimIndices(primIndices.size());
    std::copy(  primIndices.begin(),
                primIndices.end(),
                finalPrimIndices.begin());

    return needAdjInfo ? IndexAndPrimIndex(VtValue(finalIndicesAdj), VtValue(finalPrimIndices)) :
        IndexAndPrimIndex(VtValue(finalIndices), VtValue(finalPrimIndices));
}

bool
HdSt_DashDotLinesIndexBuilderComputation::Resolve()
{
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();

    IndexAndPrimIndex result;

    result = _BuildLineSegmentIndexArray(_topology->NeedAdjacentPoints());

    _SetResult(std::make_shared<HdVtBufferSource>(
                                HdTokens->indices, 
                                VtValue(result._indices)));

    // the primitive param buffer is used only when the dashdot lines
    // has uniform primvars.
    // XXX: we currently create it even when the curve has no uniform primvars
    _primitiveParam.reset(new HdVtBufferSource(
                                HdTokens->primitiveParam,
                                VtValue(result._primIndices)));

    _SetResolved();
    return true;
}

bool
HdSt_DashDotLinesIndexBuilderComputation::_CheckValid() const
{
    return true;
}

bool
HdSt_DashDotLinesIndexBuilderComputation::HasChainedBuffer() const
{
    return true;
}

HdBufferSourceSharedPtrVector
HdSt_DashDotLinesIndexBuilderComputation::GetChainedBuffers() const
{
    return { _primitiveParam };
}


PXR_NAMESPACE_CLOSE_SCOPE

