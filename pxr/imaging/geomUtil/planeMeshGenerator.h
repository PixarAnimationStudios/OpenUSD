//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GEOM_UTIL_PLANE_MESH_GENERATOR_H
#define PXR_IMAGING_GEOM_UTIL_PLANE_MESH_GENERATOR_H

#include "pxr/imaging/geomUtil/api.h"
#include "pxr/imaging/geomUtil/meshGeneratorBase.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class GfMatrix4d;
class PxOsdMeshTopology;

/// This class provides an implementation for generating topology and point
/// positions on a rectangular Plane given the dimensions along the X and Y
/// axes.  The generated Plane is centered at the origin.
/// 
/// An optional transform may be provided to GeneratePoints to orient the
/// Plane as necessary.
///
/// Usage:
/// \code{.cpp}
///
/// const size_t numPoints =
///     GeomUtilPlaneMeshGenerator::ComputeNumPoints();
/// const float w = 5, l = 4;
///
/// MyPointContainer<GfVec3f> points(numPoints);
///
/// GeomUtilPlaneMeshGenerator::GeneratePoints(
///     points.begin(), w, l);
///
/// \endcode
///
class GeomUtilPlaneMeshGenerator final
    : public GeomUtilMeshGeneratorBase
{
public:
    GEOMUTIL_API
    static size_t ComputeNumPoints();

    GEOMUTIL_API
    static PxOsdMeshTopology GenerateTopology();

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GeneratePoints(
        PointIterType iter,
        const ScalarType xLength,
        const ScalarType yLength,
        const GfMatrix4d* framePtr = nullptr)
    {
        using PointType =
            typename std::iterator_traits<PointIterType>::value_type;

        _GeneratePointsImpl(xLength, yLength,
            framePtr ? _PointWriter<PointType>(iter, framePtr)
                     : _PointWriter<PointType>(iter));
    }

    using GeomUtilMeshGeneratorBase::GeneratePoints;

private:
    
    template<typename PointType>
    static void _GeneratePointsImpl(
        const typename PointType::ScalarType xLength,
        const typename PointType::ScalarType yLength,
        const _PointWriter<PointType>& ptWriter);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_GEOM_UTIL_PLANE_MESH_GENERATOR_H
