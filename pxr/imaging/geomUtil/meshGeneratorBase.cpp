//
// Copyright 2022 Pixar
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

PXR_NAMESPACE_CLOSE_SCOPE