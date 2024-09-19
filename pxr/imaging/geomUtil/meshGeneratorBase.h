//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GEOM_UTIL_MESH_GENERATOR_BASE_H
#define PXR_IMAGING_GEOM_UTIL_MESH_GENERATOR_BASE_H

#include "pxr/imaging/geomUtil/api.h"

#include "pxr/base/arch/math.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"

#include "pxr/pxr.h"

#include <iterator>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

class PxOsdMeshTopology;

/// This class provides common implementation for the different mesh generator
/// classes in GeomUtil.  As the mesh generators are entirely implemented as
/// static functions, this "base class" is more of a grouping and access control
/// mechanism than a base class in the polymorphic sense.
///
/// The mesh generator subclasses all follow a common pattern, providing static
/// methods for generating topology and point positions for their specific
/// geometric primitive.  The data produced by these classes is only guaranteed
/// to be suitable for imaging the described surface; it is only one of many
/// possible interpretations of the surface, and should not be relied upon for
/// any other use.  The generators may e.g. change the topology or ordering of
/// the produced data at any time.  In short: these utilities are meant only to
/// be used to produce a blob of semi-blind data, for feeding to an imager that
/// supports PxOsdMeshTopology.
///
/// The generators make use of templates and SFINAE to allow clients to pass any
/// output iterator that dereferences to either a GfVec3f or GfVec3d to their
/// GeneratePoints(...) method, and internally perform compile-time type erasure
/// in order to allow the implementations of their algorithms to be private
/// implementation detail, not defined in the headers.  Although it's expected
/// that clients will typically want their point data in VtVec3fArray, the
/// described implementation was chosen to minimize the chance that any
/// prospective client with unusual data management requirements would be unable
/// to make use of the generators, or would be forced to resort to a container
/// copy in order to do so.
///
/// The only public API on this class is a static GeneratePoints(...) template
/// method, intended to be added by subclasses to their GeneratePoints(...)
/// overload sets.  It serves as the "error case" for callers that attempt to
/// pass iterators of unsupported types to the GeneratePoints(...) methods each
/// subclass declares.  As all generator subclasses have this possibility and
/// the implementation requires SFINAE, it's implemented here and shared between
/// all subclasses.  However, it's important that subclasses explicitly include
/// a "using" statement for the fallback to be included in overload resolution.
///
class GeomUtilMeshGeneratorBase
{
private:

    // Delete the implicit default c'tor.  This class and its subclasses are
    // only for grouping; there's never any need to make instances.
    GeomUtilMeshGeneratorBase() = delete;

protected:

    // SFINAE helper types, for more compact and readable template shenanigans
    // in the subclasses.
    template<typename IterType>
    struct _IsGfVec3Iterator
    {
        using PointType = typename std::iterator_traits<IterType>::value_type;
        static constexpr bool value =
            std::is_same<PointType, GfVec3f>::value ||
            std::is_same<PointType, GfVec3d>::value;
    };

    template<typename IterType>
    struct _EnableIfGfVec3Iterator
        : public std::enable_if<_IsGfVec3Iterator<IterType>::value, void>
    {};

    template<typename IterType>
    struct _EnableIfNotGfVec3Iterator
        : public std::enable_if<!_IsGfVec3Iterator<IterType>::value, void>
    {};

    // Helper struct to provide iterator type erasure, allowing subclasses to
    // implement their GeneratePoints and GenerateNormals methods privately.
    //  Usage doesn't require any heap allocation or virtual dispatch/runtime
    // typing.  In addition to erasing the iterator type, this also provides a
    // convenient way to allow subclasses to offer GeneratePoints and
    // GenerateNormals methods that can apply an additional frame transform
    // without having to actually plumb that detail into the guts of their point
    // generator code.
    //
    // Note: Ensuring the interoperability of the PointType with the IterType
    // used at construction is the responsibility of the client.  It's typically
    // guaranteed by the client deriving PointType from IterType; see subclass
    // use for examples and how they guarantee IterType dereferences to a
    // supportable point type.
    template<typename PointType>
    struct _PointWriter
    {
        template<class IterType>
        _PointWriter(
            IterType& iter)
            : _writeFnPtr(&_PointWriter<PointType>::_WritePoint<IterType>)
            , _writeDirFnPtr(&_PointWriter<PointType>::_WriteDir<IterType>)
            , _untypedIterPtr(static_cast<void*>(&iter))
        {}

        template<class IterType>
        _PointWriter(
            IterType& iter,
            const GfMatrix4d* const framePtr)
            : _writeFnPtr(
                &_PointWriter<PointType>::_TransformAndWritePoint<IterType>)
            , _writeDirFnPtr(
                &_PointWriter<PointType>::_TransformAndWriteDir<IterType>)
            , _untypedIterPtr(static_cast<void*>(&iter))
            , _framePtr(framePtr)
        {}

        void Write(
            const PointType& pt) const
        {
            (this->*_writeFnPtr)(pt);
        }

        void WriteArc(
            const typename PointType::ScalarType scaleXY,
            const std::vector<std::array<
                typename PointType::ScalarType, 2>>& arcXY,
            const typename PointType::ScalarType arcZ) const
        {
            for (const auto& xy : arcXY) {
                Write(PointType(scaleXY * xy[0], scaleXY * xy[1], arcZ));
            }
        }

        void WriteDir(
            const PointType& dir) const
        {
            (this->*_writeDirFnPtr)(dir);
        }

        void WriteArcDir(
            const typename PointType::ScalarType scaleXY,
            const std::vector<std::array<
                typename PointType::ScalarType, 2>>& arcXY,
            const typename PointType::ScalarType arcZ) const
        {
            for (const auto& xy : arcXY) {
                WriteDir(PointType(scaleXY * xy[0], scaleXY * xy[1], arcZ));
            }
        }

    private:
        template<class IterType>
        void _WritePoint(
            const PointType& pt) const
        {
            IterType& iter = *static_cast<IterType*>(_untypedIterPtr);
            *iter = pt;
            ++iter;
        }

        template<class IterType>
        void _TransformAndWritePoint(
            const PointType& pt) const
        {
            IterType& iter = *static_cast<IterType*>(_untypedIterPtr);
            using OutType = typename std::remove_reference_t<decltype(*iter)>;
            *iter = static_cast<OutType>(_framePtr->Transform(pt));
            ++iter;
        }

        template<class IterType>
        void _WriteDir(
            const PointType& pt) const
        {
            IterType& iter = *static_cast<IterType*>(_untypedIterPtr);
            *iter = pt;
            ++iter;
        }

        template<class IterType>
        void _TransformAndWriteDir(
            const PointType& dir) const
        {
            IterType& iter = *static_cast<IterType*>(_untypedIterPtr);
            using OutType = typename std::remove_reference_t<decltype(*iter)>;
            *iter = static_cast<OutType>(_framePtr->TransformDir(dir));
            ++iter;
        }

        using _WriteFnPtr =
            void (_PointWriter<PointType>::*)(const PointType &) const;
        _WriteFnPtr _writeFnPtr;
        _WriteFnPtr _writeDirFnPtr;
        void* _untypedIterPtr;
        const GfMatrix4d* _framePtr;
    };

    // Common topology helper method.
    //
    // Several of the subclasses make use of a common topology, specifically "a
    // triangle fan around a 'bottom' point, some number of quad strips forming
    // rings with shared edges, and another triangle fan surrounding a 'top'
    // point."  The two triangle fans can be considered "caps" on a "tube" of
    // linked quad strips.  This triangle fans + quad strips topology also
    // describes the latitude/longitude topology of the globe, as another
    // example.
    //
    // Because we currently rely on downstream machinery to infer surface
    // normals from the topology, we sometimes want the "caps" to share their
    // edge-ring with the adjacent quad strip, and other times need that edge-
    // ring to not be shared between the "cap" and "body" surfaces.  The edges
    // are coincident in space but the surface is not continuous across that
    // edge.
    //
    // Subclasses specify the "cap" conditions they require to support the
    // surface-continuity condition described above, and other uses where a
    // "cap" is not needed (e.g. the point-end of a cone).
    //
    // Subclasses also specify whether the surface is closed or open.  This
    // is typically exposed via a sweep parameter, wherein a sweep of a multiple
    // of 2 * pi results in a "closed" surface.  The generated points and by
    // extension, the generated topology, differs for "open" and "closed"
    // surfaces.
    //
    enum _CapStyle {
        CapStyleNone,
        CapStyleSharedEdge,
        CapStyleSeparateEdge
    };

    static PxOsdMeshTopology _GenerateCappedQuadTopology(
        const size_t numRadial,
        const size_t numQuadStrips,
        const _CapStyle bottomCapStyle,
        const _CapStyle topCapStyle,
        const bool closedSweep);


    // Subclasses that use the topology helper method above generate one or more
    // circular arcs during point generation.  The number of radial points on 
    // each arc depends on the number of radial segments and whether the arc
    // is fully swept (i.e., a ring).
    static size_t _ComputeNumRadialPoints(
        const size_t numRadial,
        const bool closedSweep);

    // Subclasses that use the topology helper method above must generate points
    // forming circular arcs and this method will compute the total number of
    // points required for the topology generated using these same parameters.
    static size_t _ComputeNumCappedQuadTopologyPoints(
        const size_t numRadial,
        const size_t numQuadStrips,
        const _CapStyle bottomCapStyle,
        const _CapStyle topCapStyle,
        const bool closedSweep);

    // Subclasses can use this helper method to generate a unit circular arc
    // in the XY plane that can then be passed into _PointWriter::WriteArc to
    // write out the points of circular arcs using varying radii.
    template<typename ScalarType>
    static std::vector<std::array<ScalarType, 2>> _GenerateUnitArcXY(
        const size_t numRadial,
        const ScalarType sweepDegrees)
    {
        constexpr ScalarType twoPi = 2.0 * M_PI;
        const ScalarType sweepRadians = GfDegreesToRadians(sweepDegrees);
        const ScalarType sweep = GfClamp(sweepRadians, -twoPi, twoPi);
        const bool closedSweep = GfIsClose(GfAbs(sweep), twoPi, 1e-6);
        const size_t numPts = _ComputeNumRadialPoints(numRadial, closedSweep);

        // Construct a circular arc of unit radius in the XY plane.
        std::vector<std::array<ScalarType, 2>> result(numPts);
        for (size_t radIdx = 0; radIdx < numPts; ++radIdx) {
            // Longitude range: [0, sweep]
            const ScalarType longAngle =
                (ScalarType(radIdx) / ScalarType(numRadial)) * sweep;
            result[radIdx][0] = cos(longAngle);
            result[radIdx][1] = sin(longAngle);
        }
        return result;
    }

public:

    // This template provides a "fallback" for GeneratePoints(...) calls that
    // do not meet the SFINAE requirement that the given point-container-
    // iterator must dereference to a GfVec3f or GfVec3d.  This version
    // generates a helpful compile time assertion in such a scenario.  As noted
    // earlier, subclasses should explicitly add a "using" statement with
    // this method to include it in overload resolution.
    //
    template<typename PointIterType,
             typename Enabled =
                typename _EnableIfNotGfVec3Iterator<PointIterType>::type>
    static void GeneratePoints(
        PointIterType iter, ...)
    {
        static_assert(_IsGfVec3Iterator<PointIterType>::value,
            "This function only supports iterators to GfVec3f or GfVec3d "
            "objects.");
    }

    // This template provides a "fallback" for GenerateNormals(...) calls that
    // do not meet the SFINAE requirement that the given point-container-
    // iterator must dereference to a GfVec3f or GfVec3d.  This version
    // generates a helpful compile time assertion in such a scenario.  As noted
    // earlier, subclasses should explicitly add a "using" statement with
    // this method to include it in overload resolution.
    //
    template<typename PointIterType,
             typename Enabled =
                typename _EnableIfNotGfVec3Iterator<PointIterType>::type>
    static void GenerateNormals(
        PointIterType iter, ...)
    {
        static_assert(_IsGfVec3Iterator<PointIterType>::value,
            "This function only supports iterators to GfVec3f or GfVec3d "
            "objects.");
    }

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_GEOM_UTIL_MESH_GENERATOR_BASE_H
