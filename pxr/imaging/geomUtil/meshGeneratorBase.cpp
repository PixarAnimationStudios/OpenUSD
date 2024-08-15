//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/meshGeneratorBase.h"

#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/vt/types.h"

PXR_NAMESPACE_OPEN_SCOPE


// static
PxOsdMeshTopology
GeomUtilMeshGeneratorBase::_GenerateCappedQuadTopology(
    const size_t numRadial,
    const size_t numQuadStrips,
    const GeomUtilMeshGeneratorBase::_CapStyle bottomCapStyle,
    const GeomUtilMeshGeneratorBase::_CapStyle topCapStyle,
    const bool closedSweep)
{
    if (numRadial < 3) {
        TF_CODING_ERROR("Invalid topology requested.");
        return PxOsdMeshTopology();
    }

    const size_t numTriStrips =
        (bottomCapStyle != CapStyleNone) + (topCapStyle != CapStyleNone);
    const size_t numTris = numTriStrips * numRadial;
    const size_t numQuads = numQuadStrips * numRadial;

    VtIntArray countsArray(numQuads + numTris);
    VtIntArray indicesArray((4 * numQuads) + (3 * numTris));

    // NOTE: When the surface is closed (sweep of 360 degrees), we ensure that
    //       the start and end points of each circular ring point are the same
    //       topologically.  This in turn means that the number of points on a
    //       (closed) ring is one less than an (open) arc.
    const size_t numRadialPts = _ComputeNumRadialPoints(numRadial, closedSweep);
    size_t ptIdx = 0;
    int* countsIt = countsArray.data();
    int* indicesIt = indicesArray.data();

    // Bottom triangle fan, if requested:
    if (bottomCapStyle != CapStyleNone) {
        size_t bottomPtIdx = ptIdx++;
        for (size_t radIdx = 0; radIdx < numRadial; ++radIdx) {
            *countsIt++ = 3;
            *indicesIt++ = ptIdx + ((radIdx + 1) % numRadialPts);
            *indicesIt++ = ptIdx + radIdx;
            *indicesIt++ = bottomPtIdx;
        }
        // Adjust the point index cursor if the edge isn't to be shared with the
        // following quad strip.
        if (bottomCapStyle == CapStyleSeparateEdge) {
            ptIdx += numRadialPts;
        }
    }

    // Middle quads:
    for (size_t stripIdx = 0; stripIdx < numQuadStrips; ++stripIdx) {
        for (size_t radIdx = 0; radIdx < numRadial; ++radIdx) {
            *countsIt++ = 4;
            *indicesIt++ = ptIdx + radIdx;
            *indicesIt++ = ptIdx + ((radIdx + 1) % numRadialPts);
            *indicesIt++ = ptIdx + ((radIdx + 1) % numRadialPts) + numRadialPts;
            *indicesIt++ = ptIdx + radIdx + numRadialPts;
        }
        ptIdx += numRadialPts;
    }

    // Top triangle fan, if requested:
    if (topCapStyle != CapStyleNone) {
        // Adjust the point index cursor if the edge isn't to be shared with the
        // preceeding quad strip.
        if (topCapStyle == CapStyleSeparateEdge) {
            ptIdx += numRadialPts;
        }
        size_t topPtIdx = ptIdx + numRadialPts;
        for (size_t radIdx = 0; radIdx < numRadial; ++radIdx) {
            *countsIt++ = 3;
            *indicesIt++ = ptIdx + radIdx;
            *indicesIt++ = ptIdx + ((radIdx + 1) % numRadialPts);
            *indicesIt++ = topPtIdx;
        }
    }

    return PxOsdMeshTopology(
        PxOsdOpenSubdivTokens->catmullClark,
        PxOsdOpenSubdivTokens->rightHanded,
        countsArray, indicesArray);
}

// static
size_t
GeomUtilMeshGeneratorBase::_ComputeNumRadialPoints(
    const size_t numRadial,
    const bool closedSweep)
{
    // For a ring, the first and last points are the same. For topological
    // correctness, do not regenerate the same point.
    return closedSweep? numRadial : numRadial + 1;
}

// static
size_t
GeomUtilMeshGeneratorBase::_ComputeNumCappedQuadTopologyPoints(
    const size_t numRadial,
    const size_t numQuadStrips,
    const _CapStyle bottomCapStyle,
    const _CapStyle topCapStyle,
    const bool closedSweep)
{
    const size_t numRadialPts = _ComputeNumRadialPoints(numRadial, closedSweep);

    size_t result = numRadialPts * (numQuadStrips + 1);

    if (bottomCapStyle != CapStyleNone) {
        // Add pole point.
        ++result;
        if (bottomCapStyle == CapStyleSeparateEdge) {
            // Add an extra set of radial points.
            result += numRadialPts;
        }
    }

    if (topCapStyle != CapStyleNone) {
        // Add pole point.
        ++result;
        if (topCapStyle == CapStyleSeparateEdge) {
            // Add an extra set of radial points.
            result += numRadialPts;
        }
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE