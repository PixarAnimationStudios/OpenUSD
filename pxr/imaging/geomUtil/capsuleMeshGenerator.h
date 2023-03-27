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
#ifndef PXR_IMAGING_GEOM_UTIL_CAPSULE_MESH_GENERATOR_H
#define PXR_IMAGING_GEOM_UTIL_CAPSULE_MESH_GENERATOR_H

#include "pxr/imaging/geomUtil/api.h"
#include "pxr/imaging/geomUtil/meshGeneratorBase.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class GfMatrix4d;
class PxOsdMeshTopology;

/// This class provides an implementation for generating topology and point
/// positions on a capsule.  The simplest form takes a radius and height and is
/// a cylinder capped by two hemispheres that is centered at the origin.  The 
/// generated capsule is made up of circular cross-sections in the XY plane.
/// Each cross-section has numRadial segments.  Successive cross-sections for
/// each of the hemispheres are generated at numCapAxial locations along the Z
/// and -Z axes respectively.  The height is aligned with the Z axis and
/// represents the height of just the cylindrical portion.
///
/// An optional transform may be provided to GeneratePoints to orient the
/// capsule as necessary (e.g., whose height is along the Y axis) .
///
/// An additional overload of GeneratePoints is provided to specify different
/// radii and heights for the bottom and top caps, as well as the sweep angle
/// for the capsule about the +Z axis.  When the sweep is less than 360 degrees,
/// the generated geometry is not closed.
///
/// Usage:
/// \code{.cpp}
///
/// const size_t numRadial = 4, numCapAxial = 4;
/// const size_t numPoints =
///     GeomUtilCapsuleMeshGenerator::ComputeNumPoints(numRadial, numCapAxial);
/// const float radius = 1, height = 2;
///
/// MyPointContainer<GfVec3f> points(numPoints);
///
/// GeomUtilCapsuleMeshGenerator::GeneratePoints(
///     points.begin(), numRadial, numCapAxial, radius, height);
///
/// \endcode
///
class GeomUtilCapsuleMeshGenerator final
    : public GeomUtilMeshGeneratorBase
{
public:
    static constexpr size_t minNumRadial = 3;
    static constexpr size_t minNumCapAxial = 1;

    GEOMUTIL_API
    static size_t ComputeNumPoints(
        const size_t numRadial,
        const size_t numCapAxial,
        const bool closedSweep = true);

    GEOMUTIL_API
    static PxOsdMeshTopology GenerateTopology(
        const size_t numRadial,
        const size_t numCapAxial,
        const bool closedSweep = true);

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GeneratePoints(
        PointIterType iter,
        const size_t numRadial,
        const size_t numCapAxial,
        const ScalarType radius,
        const ScalarType height,
        const GfMatrix4d* framePtr = nullptr)
    {
        constexpr ScalarType sweep = 360;

        GeneratePoints(iter, numRadial, numCapAxial,
                       /* bottomRadius =    */ radius,
                       /* topRadius    =    */ radius,
                       height,
                       /* bottomCapHeight = */ radius,
                       /* topCapHeight =    */ radius,
                        sweep, framePtr);
    }

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GeneratePoints(
        PointIterType iter,
        const size_t numRadial,
        const size_t numCapAxial,
        const ScalarType bottomRadius,
        const ScalarType topRadius,
        const ScalarType height,
        const ScalarType bottomCapHeight,
        const ScalarType topCapHeight,
        const ScalarType sweepDegrees,
        const GfMatrix4d* framePtr = nullptr)
    {
        using PointType =
            typename std::iterator_traits<PointIterType>::value_type;

        _GeneratePointsImpl(numRadial, numCapAxial, bottomRadius, topRadius,
            height, bottomCapHeight, topCapHeight, sweepDegrees,
            framePtr ? _PointWriter<PointType>(iter, framePtr)
                     : _PointWriter<PointType>(iter));
    }

    using GeomUtilMeshGeneratorBase::GeneratePoints;

private:
    template<typename PointType>
    static void _GeneratePointsImpl(
        const size_t numRadial,
        const size_t numCapAxial,
        const typename PointType::ScalarType bottomRadius,
        const typename PointType::ScalarType topRadius,
        const typename PointType::ScalarType height,
        const typename PointType::ScalarType bottomCapHeight,
        const typename PointType::ScalarType topCapHeight,
        const typename PointType::ScalarType sweep,
        const _PointWriter<PointType>& ptWriter);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_GEOM_UTIL_CAPSULE_MESH_GENERATOR_H
