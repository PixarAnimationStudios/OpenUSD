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
#ifndef PXR_IMAGING_GEOM_UTIL_CUBOID_MESH_GENERATOR_H
#define PXR_IMAGING_GEOM_UTIL_CUBOID_MESH_GENERATOR_H

#include "pxr/imaging/geomUtil/api.h"
#include "pxr/imaging/geomUtil/meshGeneratorBase.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class GfMatrix4d;
class PxOsdMeshTopology;

/// This class provides an implementation for generating topology and point
/// positions on a rectangular cuboid given the dimensions along the X, Y and Z
/// axes.  The generated cuboid is centered at the origin.
/// 
/// An optional transform may be provided to GeneratePoints to orient the
/// cuboid as necessary.
///
/// Usage:
/// \code{.cpp}
///
/// const size_t numPoints =
///     GeomUtilCuboidMeshGenerator::ComputeNumPoints();
/// const float l = 5, b = 4, h = 3;
///
/// MyPointContainer<GfVec3f> points(numPoints);
///
/// GeomUtilCuboidMeshGenerator::GeneratePoints(
///     points.begin(), l, b, h);
///
/// \endcode
///
class GeomUtilCuboidMeshGenerator final
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
        const ScalarType zLength,
        const GfMatrix4d* framePtr = nullptr)
    {
        using PointType =
            typename std::iterator_traits<PointIterType>::value_type;

        _GeneratePointsImpl(xLength, yLength, zLength,
            framePtr ? _PointWriter<PointType>(iter, framePtr)
                     : _PointWriter<PointType>(iter));
    }

    using GeomUtilMeshGeneratorBase::GeneratePoints;

private:
    
    template<typename PointType>
    static void _GeneratePointsImpl(
        const typename PointType::ScalarType xLength,
        const typename PointType::ScalarType yLength,
        const typename PointType::ScalarType zLength,
        const _PointWriter<PointType>& ptWriter);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_GEOM_UTIL_CUBOID_MESH_GENERATOR_H
