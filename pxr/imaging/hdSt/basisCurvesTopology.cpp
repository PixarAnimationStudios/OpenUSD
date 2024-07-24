//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/basisCurvesTopology.h"
#include "pxr/imaging/hdSt/basisCurvesComputations.h"

#include "pxr/imaging/hd/vtBufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE



// static
HdSt_BasisCurvesTopologySharedPtr
HdSt_BasisCurvesTopology::New(const HdBasisCurvesTopology &src)
{
    return HdSt_BasisCurvesTopologySharedPtr(new HdSt_BasisCurvesTopology(src));
}

// explicit
HdSt_BasisCurvesTopology::HdSt_BasisCurvesTopology(const HdBasisCurvesTopology& src)
 : HdBasisCurvesTopology(src)
{
}


HdSt_BasisCurvesTopology::~HdSt_BasisCurvesTopology()
{
}

HdBufferSourceSharedPtr
HdSt_BasisCurvesTopology::GetPointsIndexBuilderComputation()
{
    // This is simple enough to return the result right away, instead of
    // using a computed buffer source.
    const VtIntArray& vertexCounts = GetCurveVertexCounts();
    int numVerts = 0;
    for (int count : vertexCounts) {
        numVerts += count;
    }

    VtIntArray finalIndices(numVerts);
    const VtIntArray& curveIndices = GetCurveIndices();
    if (curveIndices.empty()) {
        for (int i=0; i<numVerts; ++i) {
            finalIndices[i] = i;
        }
    } else {
        for (int i=0; i<numVerts; ++i) {
            finalIndices[i] = curveIndices[i];
        }
    }

    // Note: The primitive param buffer isn't bound.
    return std::make_shared<HdVtBufferSource>(
        HdTokens->indices, VtValue(finalIndices));
}

HdBufferSourceSharedPtr
HdSt_BasisCurvesTopology::GetIndexBuilderComputation(bool forceLines)
{
    return std::make_shared<HdSt_BasisCurvesIndexBuilderComputation>(
        this, forceLines);
}

PXR_NAMESPACE_CLOSE_SCOPE

