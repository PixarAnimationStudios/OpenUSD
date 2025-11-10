//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/dashDotLinesTopology.h"
#include "pxr/imaging/hdSt/dashDotLinesComputations.h"

#include "pxr/imaging/hd/vtBufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE



// static
HdSt_DashDotLinesTopologySharedPtr
HdSt_DashDotLinesTopology::New(const HdDashDotLinesTopology &src)
{
    return HdSt_DashDotLinesTopologySharedPtr(new HdSt_DashDotLinesTopology(src));
}

// explicit
HdSt_DashDotLinesTopology::HdSt_DashDotLinesTopology(const HdDashDotLinesTopology& src)
 : HdDashDotLinesTopology(src)
{
}


HdSt_DashDotLinesTopology::~HdSt_DashDotLinesTopology()
{
}

HdBufferSourceSharedPtr
HdSt_DashDotLinesTopology::GetPointsIndexBuilderComputation()
{
    // This is simple enough to return the result right away, instead of
    // using a computed buffer source.
    const VtIntArray& vertexCounts = GetCurveVertexCounts();
    int numVerts = std::accumulate(vertexCounts.begin(), vertexCounts.end(), 0);

    VtIntArray finalIndices(numVerts);
    const VtIntArray& curveIndices = GetCurveIndices();
    if (curveIndices.empty()) {
        std::iota(finalIndices.begin(), finalIndices.end(), 0);
    } else {
        if(numVerts >= curveIndices.size())
            std::copy(curveIndices.begin(), curveIndices.end(), finalIndices.begin());
        else
            std::copy(curveIndices.begin(), curveIndices.begin() + numVerts - 1, finalIndices.begin());
    }

    // Note: The primitive param buffer isn't bound.
    return std::make_shared<HdVtBufferSource>(
        HdTokens->indices, VtValue(finalIndices));
}

HdBufferSourceSharedPtr
HdSt_DashDotLinesTopology::GetIndexBuilderComputation(bool forceLines)
{
    return std::make_shared<HdSt_DashDotLinesIndexBuilderComputation>(
        this, forceLines);
}

PXR_NAMESPACE_CLOSE_SCOPE

