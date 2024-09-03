//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/planeMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/vt/types.h"

PXR_NAMESPACE_OPEN_SCOPE


// static
size_t
GeomUtilPlaneMeshGenerator::ComputeNumPoints()
{
    return 4;
}

// static
PxOsdMeshTopology
GeomUtilPlaneMeshGenerator::GenerateTopology()
{
    // These never vary, we might as well share a single copy via VtArray.
    static const VtIntArray countsArray{ 4 };
    static const VtIntArray indicesArray{ 0, 1, 2, 3 };
    return PxOsdMeshTopology(
        PxOsdOpenSubdivTokens->bilinear,
        PxOsdOpenSubdivTokens->rightHanded,
        countsArray, indicesArray);
}

// static
template<typename PointType>
void
GeomUtilPlaneMeshGenerator::_GeneratePointsImpl(
    const typename PointType::ScalarType xLength,
    const typename PointType::ScalarType yLength,
    const _PointWriter<PointType>& ptWriter)
{
    using ScalarType = typename PointType::ScalarType;

    const ScalarType x = 0.5 * xLength;
    const ScalarType y = 0.5 * yLength;

    ptWriter.Write(PointType( x,  y,  0));
    ptWriter.Write(PointType(-x,  y,  0));
    ptWriter.Write(PointType(-x, -y,  0));
    ptWriter.Write(PointType( x, -y,  0));
}

// Force-instantiate _GeneratePointsImpl for the supported point types.  Only
// these instantiations will ever be needed due to the SFINAE machinery on the
// calling method template (the public GeneratePoints, in the header).
template GEOMUTIL_API void GeomUtilPlaneMeshGenerator::_GeneratePointsImpl(
    const float, const float,
    const GeomUtilPlaneMeshGenerator::_PointWriter<GfVec3f>&);

template GEOMUTIL_API void GeomUtilPlaneMeshGenerator::_GeneratePointsImpl(
    const double, const double,
    const GeomUtilPlaneMeshGenerator::_PointWriter<GfVec3d>&);


PXR_NAMESPACE_CLOSE_SCOPE
