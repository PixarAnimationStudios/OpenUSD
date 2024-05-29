//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/cuboidMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/vt/types.h"

PXR_NAMESPACE_OPEN_SCOPE


// static
size_t
GeomUtilCuboidMeshGenerator::ComputeNumPoints()
{
    return 8;
}

// static
PxOsdMeshTopology
GeomUtilCuboidMeshGenerator::GenerateTopology()
{
    // These never vary, we might as well share a single copy via VtArray.
    static const VtIntArray countsArray{ 4, 4, 4, 4, 4, 4 };
    static const VtIntArray indicesArray{ 0, 1, 2, 3,
                                          4, 5, 6, 7,
                                          0, 6, 5, 1,
                                          4, 7, 3, 2,
                                          0, 3, 7, 6,
                                          4, 2, 1, 5 };
    return PxOsdMeshTopology(
        PxOsdOpenSubdivTokens->bilinear,
        PxOsdOpenSubdivTokens->rightHanded,
        countsArray, indicesArray);
}

// static
template<typename PointType>
void
GeomUtilCuboidMeshGenerator::_GeneratePointsImpl(
    const typename PointType::ScalarType xLength,
    const typename PointType::ScalarType yLength,
    const typename PointType::ScalarType zLength,
    const _PointWriter<PointType>& ptWriter)
{
    using ScalarType = typename PointType::ScalarType;

    const ScalarType x = 0.5 * xLength;
    const ScalarType y = 0.5 * yLength;
    const ScalarType z = 0.5 * zLength;

    ptWriter.Write(PointType( x,  y,  z));
    ptWriter.Write(PointType(-x,  y,  z));
    ptWriter.Write(PointType(-x, -y,  z));
    ptWriter.Write(PointType( x, -y,  z));
    ptWriter.Write(PointType(-x, -y, -z));
    ptWriter.Write(PointType(-x,  y, -z));
    ptWriter.Write(PointType( x,  y, -z));
    ptWriter.Write(PointType( x, -y, -z));
}

// Force-instantiate _GeneratePointsImpl for the supported point types.  Only
// these instantiations will ever be needed due to the SFINAE machinery on the
// calling method template (the public GeneratePoints, in the header).
template GEOMUTIL_API void GeomUtilCuboidMeshGenerator::_GeneratePointsImpl(
    const float, const float, const float,
    const GeomUtilCuboidMeshGenerator::_PointWriter<GfVec3f>&);

template GEOMUTIL_API void GeomUtilCuboidMeshGenerator::_GeneratePointsImpl(
    const double, const double, const double,
    const GeomUtilCuboidMeshGenerator::_PointWriter<GfVec3d>&);


PXR_NAMESPACE_CLOSE_SCOPE