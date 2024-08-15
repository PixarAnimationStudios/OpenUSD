//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdsi/implicitSurfaceSceneIndex.h"

#include "pxr/imaging/geomUtil/capsuleMeshGenerator.h"
#include "pxr/imaging/geomUtil/coneMeshGenerator.h"
#include "pxr/imaging/geomUtil/cuboidMeshGenerator.h"
#include "pxr/imaging/geomUtil/cylinderMeshGenerator.h"
#include "pxr/imaging/geomUtil/planeMeshGenerator.h"
#include "pxr/imaging/geomUtil/sphereMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/imaging/hd/capsuleSchema.h"
#include "pxr/imaging/hd/coneSchema.h"
#include "pxr/imaging/hd/cubeSchema.h"
#include "pxr/imaging/hd/cylinderSchema.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/dependencySchema.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/planeSchema.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sphereSchema.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdsiImplicitSurfaceSceneIndexTokens,
     HDSI_IMPLICIT_SURFACE_SCENE_INDEX_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((xAxis, "X"))
    ((yAxis, "Y"))
    ((zAxis, "Z"))

    (implicitToMesh)
    (implicitToXform)
);

namespace
{

GfMatrix4d
_GetBasis(TfToken const &axis)
{
    GfVec4d u, v, spine;
    if (axis == _tokens->xAxis) {
        u = GfVec4d::YAxis();
        v = GfVec4d::ZAxis();
        spine = GfVec4d::XAxis();
    } else if (axis == _tokens->yAxis) {
        u = GfVec4d::ZAxis();
        v = GfVec4d::XAxis();
        spine = GfVec4d::YAxis();
    } else { // (axis == _tokens->zAxis)
        u = GfVec4d::XAxis();
        v = GfVec4d::YAxis();
        spine = GfVec4d::ZAxis();
    }

    GfMatrix4d basis;
    basis.SetRow(0, u);
    basis.SetRow(1, v);
    basis.SetRow(2, spine);
    basis.SetRow(3, GfVec4d::WAxis());

    return basis;
}

using Time = HdSampledDataSource::Time;

template<typename S>
HdContainerDataSourceHandle
_ComputePointsDependenciesDataSource(const SdfPath &primPath)
{
    HdPathDataSourceHandle const dependedOnPrimPathDataSource =
        HdRetainedTypedSampledDataSource<SdfPath>::New(
            primPath);
    static HdLocatorDataSourceHandle const dependedOnLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            S::GetDefaultLocator());
    static HdLocatorDataSourceHandle const affectedLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdPrimvarsSchema::GetPointsLocator().Append(
                HdPrimvarSchemaTokens->primvarValue));

    return
        HdRetainedContainerDataSource::New(
            _tokens->implicitToMesh,
            HdDependencySchema::Builder()
                .SetDependedOnPrimPath(dependedOnPrimPathDataSource)
                .SetDependedOnDataSourceLocator(dependedOnLocatorDataSource)
                .SetAffectedDataSourceLocator(affectedLocatorDataSource)
                .Build());
}

template<typename S>
HdContainerDataSourceHandle
_ComputeMatrixDependenciesDataSource(const SdfPath &primPath)
{
    HdPathDataSourceHandle const dependedOnPrimPathDataSource =
        HdRetainedTypedSampledDataSource<SdfPath>::New(
            primPath);
    static HdLocatorDataSourceHandle const dependedOnLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            S::GetDefaultLocator());
    static HdLocatorDataSourceHandle const affectedLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdXformSchema::GetDefaultLocator()
                .Append(HdXformSchemaTokens->matrix));

    return
        HdRetainedContainerDataSource::New(
            _tokens->implicitToXform,
            HdDependencySchema::Builder()
                .SetDependedOnPrimPath(dependedOnPrimPathDataSource)
                .SetDependedOnDataSourceLocator(dependedOnLocatorDataSource)
                .SetAffectedDataSourceLocator(affectedLocatorDataSource)
                .Build());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Cube

namespace _CubeToMesh
{

HdContainerDataSourceHandle
_ComputeMeshDataSource()
{
    static const PxOsdMeshTopology topology = 
        GeomUtilCuboidMeshGenerator::GenerateTopology();

    return
        HdMeshSchema::Builder()
            .SetTopology(
                HdMeshTopologySchema::Builder()
                    .SetFaceVertexCounts(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexCounts()))
                    .SetFaceVertexIndices(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexIndices()))
                    .SetOrientation(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            HdMeshTopologySchemaTokens->rightHanded))
                    .Build())
            .SetSubdivisionScheme(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    topology.GetScheme()))
            .SetDoubleSided(
                HdRetainedTypedSampledDataSource<bool>::New(false))
            .Build();
}

class _PointsDataSource : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtVec3fArray GetTypedValue(const Time shutterOffset) override {
        const size_t numPoints =
            GeomUtilCuboidMeshGenerator::ComputeNumPoints();
        VtVec3fArray points(numPoints);

        const double size = _GetSize(shutterOffset);

        GeomUtilCuboidMeshGenerator::GeneratePoints(
            points.begin(),
            size, size, size
        );
        
        return points;
    }

    bool GetContributingSampleTimesForInterval(
                            const Time startTime,
                            const Time endTime,
                            std::vector<Time> * const outSampleTimes) override {
        if (HdDoubleDataSourceHandle const s = _GetSizeSource()) {
            return s->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
        }
        return false;
    }

private:
    _PointsDataSource(const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    HdDoubleDataSourceHandle _GetSizeSource() const {
        static const HdDataSourceLocator sizeLocator(
            HdCubeSchemaTokens->cube, HdCubeSchemaTokens->size);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    double _GetSize(const Time shutterOffset) const {
        if (HdDoubleDataSourceHandle const s = _GetSizeSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 1.0;
    }

    HdContainerDataSourceHandle _primDataSource;
};

HdContainerDataSourceHandle
_ComputePointsPrimvarDataSource(
    const HdContainerDataSourceHandle &primDataSource)
{
    static HdTokenDataSourceHandle const roleDataSource =
        HdPrimvarSchema::BuildRoleDataSource(
            HdPrimvarSchemaTokens->point);
    static HdTokenDataSourceHandle const interpolationDataSource =
        HdPrimvarSchema::BuildInterpolationDataSource(
            HdPrimvarSchemaTokens->vertex);

    return
        HdPrimvarSchema::Builder()
            .SetRole(roleDataSource)
            .SetInterpolation(interpolationDataSource)
            .SetPrimvarValue(_PointsDataSource::New(primDataSource))
            .Build();
}

HdContainerDataSourceHandle
_ComputePrimvarsDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    return
        HdRetainedContainerDataSource::New(
            HdPrimvarsSchemaTokens->points,
                    _ComputePointsPrimvarDataSource(primDataSource));
}

HdContainerDataSourceHandle
_ComputePrimDataSource(
    const SdfPath &primPath,
    const HdContainerDataSourceHandle &primDataSource)
{
    static HdDataSourceBaseHandle const cubeDataSource =
        HdBlockDataSource::New();
    static HdDataSourceBaseHandle const meshDataSource =
        _ComputeMeshDataSource();
    HdDataSourceBaseHandle const primvarsDataSource =
        _ComputePrimvarsDataSource(primDataSource);
    HdDataSourceBaseHandle const dependenciesDataSource =
        _ComputePointsDependenciesDataSource<HdCubeSchema>(primPath);

    HdContainerDataSourceHandle sources[] = {
        HdRetainedContainerDataSource::New(
            HdCubeSchema::GetSchemaToken(), cubeDataSource,
            HdMeshSchema::GetSchemaToken(), meshDataSource,
            HdPrimvarsSchema::GetSchemaToken(), primvarsDataSource,
            HdDependenciesSchema::GetSchemaToken(), dependenciesDataSource),
        primDataSource
    };

    return HdOverlayContainerDataSource::New(TfArraySize(sources), sources);
}

} // namespace _CubeToMesh

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Cone

namespace _ConeToMesh
{

static constexpr size_t numRadial = 10;

HdContainerDataSourceHandle
_ComputeMeshDataSource()
{
    static const PxOsdMeshTopology topology =
        GeomUtilConeMeshGenerator::GenerateTopology(numRadial);

    return
        HdMeshSchema::Builder()
            .SetTopology(
                HdMeshTopologySchema::Builder()
                    .SetFaceVertexCounts(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexCounts()))
                    .SetFaceVertexIndices(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexIndices()))
                    .SetOrientation(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            HdMeshTopologySchemaTokens->rightHanded))
                    .Build())
            .SetSubdivisionScheme(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    topology.GetScheme()))
            .SetDoubleSided(
                HdRetainedTypedSampledDataSource<bool>::New(false))
            .Build();
}

class _PointsDataSource : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtVec3fArray GetTypedValue(const Time shutterOffset) override {
       
        const GfMatrix4d basis = _GetBasis(_GetAxis(shutterOffset));
        const size_t numPoints =
            GeomUtilConeMeshGenerator::ComputeNumPoints(numRadial);
        VtVec3fArray points(numPoints);

        GeomUtilConeMeshGenerator::GeneratePoints(
            points.begin(),
            numRadial,
            _GetRadius(shutterOffset),
            _GetHeight(shutterOffset),
            &basis
        );
        
        return points;
    }

    bool GetContributingSampleTimesForInterval(
                            const Time startTime,
                            const Time endTime,
                            std::vector<Time> * const outSampleTimes) override {
        HdSampledDataSourceHandle sources[] = {
            _GetHeightSource(), _GetRadiusSource(), _GetAxisSource() };
        return HdGetMergedContributingSampleTimesForInterval(
            TfArraySize(sources), sources, startTime, endTime, outSampleTimes);
    }

private:
    _PointsDataSource(const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    HdDoubleDataSourceHandle _GetHeightSource() const {
        static const HdDataSourceLocator sizeLocator =
            HdConeSchema::GetDefaultLocator().Append(
                HdConeSchemaTokens->height);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    double _GetHeight(const Time shutterOffset) const {
        if (HdDoubleDataSourceHandle const s = _GetHeightSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 1.0;
    }

    HdDoubleDataSourceHandle _GetRadiusSource() const {
        static const HdDataSourceLocator locator =
            HdConeSchema::GetDefaultLocator().Append(
                HdConeSchemaTokens->radius);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, locator));
    }

    double _GetRadius(const Time shutterOffset) const {
        if (HdDoubleDataSourceHandle const s = _GetRadiusSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 1.0;
    }

    HdTokenDataSourceHandle _GetAxisSource() const {
        static const HdDataSourceLocator locator =
            HdConeSchema::GetDefaultLocator().Append(
                HdConeSchemaTokens->axis);
        return HdTokenDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, locator));
    }

    TfToken _GetAxis(const Time shutterOffset) const {
        if (HdTokenDataSourceHandle const s = _GetAxisSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return HdConeSchemaTokens->X;
    }

    HdContainerDataSourceHandle _primDataSource;
};

HdContainerDataSourceHandle
_ComputePointsPrimvarDataSource(
    const HdContainerDataSourceHandle &primDataSource)
{
    static HdTokenDataSourceHandle const roleDataSource =
        HdPrimvarSchema::BuildRoleDataSource(
            HdPrimvarSchemaTokens->point);
    static HdTokenDataSourceHandle const interpolationDataSource =
        HdPrimvarSchema::BuildInterpolationDataSource(
            HdPrimvarSchemaTokens->vertex);

    return
        HdPrimvarSchema::Builder()
            .SetRole(roleDataSource)
            .SetInterpolation(interpolationDataSource)
            .SetPrimvarValue(_PointsDataSource::New(primDataSource))
            .Build();
}

HdContainerDataSourceHandle
_ComputePrimvarsDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    return
        HdRetainedContainerDataSource::New(
            HdPrimvarsSchemaTokens->points,
                    _ComputePointsPrimvarDataSource(primDataSource));
}

HdContainerDataSourceHandle
_ComputePrimDataSource(
    const SdfPath &primPath,
    const HdContainerDataSourceHandle &primDataSource)
{
    static HdDataSourceBaseHandle const coneDataSource =
        HdBlockDataSource::New();
    static HdDataSourceBaseHandle const meshDataSource =
        _ComputeMeshDataSource();
    HdDataSourceBaseHandle const primvarsDataSource =
        _ComputePrimvarsDataSource(primDataSource);
    HdDataSourceBaseHandle const dependenciesDataSource =
        _ComputePointsDependenciesDataSource<HdConeSchema>(primPath);

    HdContainerDataSourceHandle sources[] = {
        HdRetainedContainerDataSource::New(
            HdConeSchema::GetSchemaToken(), coneDataSource,
            HdMeshSchema::GetSchemaToken(), meshDataSource,
            HdPrimvarsSchema::GetSchemaToken(), primvarsDataSource,
            HdDependenciesSchema::GetSchemaToken(), dependenciesDataSource),
        primDataSource
    };

    return HdOverlayContainerDataSource::New(TfArraySize(sources), sources);
}

} // namespace _ConeToMesh

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Cylinder

namespace _CylinderToMesh
{

static constexpr size_t numRadial = 10;

HdContainerDataSourceHandle
_ComputeMeshDataSource()
{
    static const PxOsdMeshTopology topology =
        GeomUtilCylinderMeshGenerator::GenerateTopology(numRadial);

    return
        HdMeshSchema::Builder()
            .SetTopology(
                HdMeshTopologySchema::Builder()
                    .SetFaceVertexCounts(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexCounts()))
                    .SetFaceVertexIndices(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexIndices()))
                    .SetOrientation(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            HdMeshTopologySchemaTokens->rightHanded))
                    .Build())
            .SetSubdivisionScheme(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    topology.GetScheme()))
            .SetDoubleSided(
                HdRetainedTypedSampledDataSource<bool>::New(false))
            .Build();
}

class _PointsDataSource : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtVec3fArray GetTypedValue(const Time shutterOffset) override {
        const GfMatrix4d basis = _GetBasis(_GetAxis(shutterOffset));
        const size_t numPoints =
            GeomUtilCylinderMeshGenerator::ComputeNumPoints(numRadial);
        VtVec3fArray points(numPoints);

        GeomUtilCylinderMeshGenerator::GeneratePoints(
            points.begin(),
            numRadial,
            _GetRadiusBottom(shutterOffset),
            _GetRadiusTop(shutterOffset),
            _GetHeight(shutterOffset),
            &basis
        );

        return points;
    }

    bool GetContributingSampleTimesForInterval(
                            const Time startTime,
                            const Time endTime,
                            std::vector<Time> * const outSampleTimes) override
    {
        // Note contributing sources also include radius source for backward
        // compatibility with cylinder schema with just 1 radius.
        HdSampledDataSourceHandle sources[] = {
            _GetHeightSource(), _GetRadiusSource(), _GetRadiusBottomSource(), 
            _GetRadiusTopSource(), _GetAxisSource() };
        return HdGetMergedContributingSampleTimesForInterval(
            TfArraySize(sources), sources, startTime, endTime, outSampleTimes);
    }

private:
    _PointsDataSource(const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    HdDoubleDataSourceHandle _GetHeightSource() const
    {
        static const HdDataSourceLocator sizeLocator(
            HdCylinderSchema::GetSchemaToken(), 
            HdCylinderSchemaTokens->height);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    };

    double _GetHeight(const Time shutterOffset) const {
        if (HdDoubleDataSourceHandle const s = _GetHeightSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 2.0;
    }

    // Deprecated
    HdDoubleDataSourceHandle _GetRadiusSource() const
    {
        static const HdDataSourceLocator sizeLocator(
            HdCylinderSchema::GetSchemaToken(),
            HdCylinderSchemaTokens->radius);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    HdDoubleDataSourceHandle _GetRadiusBottomSource() const
    {
        static const HdDataSourceLocator sizeLocator(
            HdCylinderSchema::GetSchemaToken(),
            HdCylinderSchemaTokens->radiusBottom);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    double _GetRadiusBottom(const Time shutterOffset) const 
    {
        if (HdDoubleDataSourceHandle const s = _GetRadiusBottomSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        // Fallback to old cylinder schema - deprecated
        if (HdDoubleDataSourceHandle const s = _GetRadiusSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 1.0;
    }

    HdDoubleDataSourceHandle _GetRadiusTopSource() const 
    {
        static const HdDataSourceLocator sizeLocator(
            HdCylinderSchema::GetSchemaToken(),
            HdCylinderSchemaTokens->radiusTop);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    double _GetRadiusTop(const Time shutterOffset) const 
    {
        if (HdDoubleDataSourceHandle const s = _GetRadiusTopSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        // Fallback to old cylinder schema - deprecated
        if (HdDoubleDataSourceHandle const s = _GetRadiusSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 1.0;
    }

    HdTokenDataSourceHandle _GetAxisSource() const
    {
        static const HdDataSourceLocator sizeLocator(
            HdCylinderSchema::GetSchemaToken(), 
            HdCylinderSchemaTokens->axis);
        return HdTokenDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    TfToken _GetAxis(const Time shutterOffset) const {
        if (HdTokenDataSourceHandle const s = _GetAxisSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return HdCylinderSchemaTokens->Z;
    }

    HdContainerDataSourceHandle _primDataSource;
};

HdContainerDataSourceHandle
_ComputePointsPrimvarDataSource(
    const HdContainerDataSourceHandle &primDataSource)
{
    static HdTokenDataSourceHandle const roleDataSource =
        HdPrimvarSchema::BuildRoleDataSource(
            HdPrimvarSchemaTokens->point);
    static HdTokenDataSourceHandle const interpolationDataSource =
        HdPrimvarSchema::BuildInterpolationDataSource(
            HdPrimvarSchemaTokens->vertex);

    return
        HdPrimvarSchema::Builder()
            .SetRole(roleDataSource)
            .SetInterpolation(interpolationDataSource)
            .SetPrimvarValue(_PointsDataSource::New(primDataSource))
            .Build();
}

HdContainerDataSourceHandle
_ComputePrimvarsDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    return
        HdRetainedContainerDataSource::New(
            HdPrimvarsSchemaTokens->points,
                    _ComputePointsPrimvarDataSource(primDataSource));
}

HdContainerDataSourceHandle
_ComputePrimDataSource(
    const SdfPath &primPath,
    const HdContainerDataSourceHandle &primDataSource)
{
    static HdDataSourceBaseHandle const cylinderDataSource =
        HdBlockDataSource::New();
    static HdDataSourceBaseHandle const meshDataSource =
        _ComputeMeshDataSource();
    HdDataSourceBaseHandle const primvarsDataSource =
        _ComputePrimvarsDataSource(primDataSource);
    HdDataSourceBaseHandle const dependenciesDataSource =
        _ComputePointsDependenciesDataSource<HdCylinderSchema>(primPath);

    HdContainerDataSourceHandle sources[] = {
        HdRetainedContainerDataSource::New(
            HdCylinderSchema::GetSchemaToken(), cylinderDataSource,
            HdMeshSchemaTokens->mesh, meshDataSource,
            HdPrimvarsSchemaTokens->primvars, primvarsDataSource,
            HdDependenciesSchemaTokens->__dependencies, dependenciesDataSource),
        primDataSource
    };

    return HdOverlayContainerDataSource::New(TfArraySize(sources), sources);
}

} // namespace _CylinderToMesh

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Sphere

namespace _SphereToMesh
{

static constexpr size_t numRadial = 10;
static constexpr size_t numAxial  = 10;

HdContainerDataSourceHandle
_ComputeMeshDataSource()
{
    static const PxOsdMeshTopology topology =
        GeomUtilSphereMeshGenerator::GenerateTopology(numRadial, numAxial);

    return
        HdMeshSchema::Builder()
            .SetTopology(
                HdMeshTopologySchema::Builder()
                    .SetFaceVertexCounts(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexCounts()))
                    .SetFaceVertexIndices(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexIndices()))
                    .SetOrientation(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            HdMeshTopologySchemaTokens->rightHanded))
                    .Build())
            .SetSubdivisionScheme(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    topology.GetScheme()))
            .SetDoubleSided(
                HdRetainedTypedSampledDataSource<bool>::New(false))
            .Build();
}

class _PointsDataSource : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtVec3fArray GetTypedValue(const Time shutterOffset) override {

        const size_t numPoints =
            GeomUtilSphereMeshGenerator::ComputeNumPoints(numRadial, numAxial);
        VtVec3fArray points(numPoints);

        GeomUtilSphereMeshGenerator::GeneratePoints(
            points.begin(),
            numRadial,
            numAxial,
            _GetRadius(shutterOffset)
        );

        return points;
    }

    bool GetContributingSampleTimesForInterval(
                            const Time startTime,
                            const Time endTime,
                            std::vector<Time> * const outSampleTimes) override {
        if (HdDoubleDataSourceHandle const s = _GetRadiusSource()) {
            return s->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
        }
        return false;
    }

private:
    _PointsDataSource(const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    HdDoubleDataSourceHandle _GetRadiusSource() const {
        static const HdDataSourceLocator radiusLocator(
            HdSphereSchemaTokens->sphere, HdSphereSchemaTokens->radius);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, radiusLocator));
    }

    double _GetRadius(const Time shutterOffset) const {
        if (HdDoubleDataSourceHandle const s = _GetRadiusSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 1.0;
    }

    HdContainerDataSourceHandle _primDataSource;
};

HdContainerDataSourceHandle
_ComputePointsPrimvarDataSource(
    const HdContainerDataSourceHandle &primDataSource)
{
    static HdTokenDataSourceHandle const roleDataSource =
        HdPrimvarSchema::BuildRoleDataSource(
            HdPrimvarSchemaTokens->point);
    static HdTokenDataSourceHandle const interpolationDataSource =
        HdPrimvarSchema::BuildInterpolationDataSource(
            HdPrimvarSchemaTokens->vertex);

    return
        HdPrimvarSchema::Builder()
            .SetRole(roleDataSource)
            .SetInterpolation(interpolationDataSource)
            .SetPrimvarValue(_PointsDataSource::New(primDataSource))
            .Build();
}

HdContainerDataSourceHandle
_ComputePrimvarsDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    return
        HdRetainedContainerDataSource::New(
            HdPrimvarsSchemaTokens->points,
                    _ComputePointsPrimvarDataSource(primDataSource));
}

HdContainerDataSourceHandle
_ComputePrimDataSource(
    const SdfPath &primPath,
    const HdContainerDataSourceHandle &primDataSource)
{
    static HdDataSourceBaseHandle const sphereDataSource =
        HdBlockDataSource::New();
    static HdDataSourceBaseHandle const meshDataSource =
        _ComputeMeshDataSource();
    HdDataSourceBaseHandle const primvarsDataSource =
        _ComputePrimvarsDataSource(primDataSource);
    HdDataSourceBaseHandle const dependenciesDataSource =
        _ComputePointsDependenciesDataSource<HdSphereSchema>(primPath);

    HdContainerDataSourceHandle sources[] = {
        HdRetainedContainerDataSource::New(
            HdSphereSchemaTokens->sphere, sphereDataSource,
            HdMeshSchemaTokens->mesh, meshDataSource,
            HdPrimvarsSchemaTokens->primvars, primvarsDataSource,
            HdDependenciesSchemaTokens->__dependencies, dependenciesDataSource),
        primDataSource
    };
    
    return HdOverlayContainerDataSource::New(TfArraySize(sources), sources);
}

} // namespace _SphereToMesh

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Capsule

namespace _CapsuleToMesh
{

// Number of radial segments (about the spine axis)
static constexpr size_t numRadial = 10;
// Number of axial divisions for each hemispherical cap (along the spine axis)
static constexpr size_t numCapAxial = 4;

HdContainerDataSourceHandle
_ComputeMeshDataSource()
{
    static const PxOsdMeshTopology topology =
        GeomUtilCapsuleMeshGenerator::GenerateTopology(numRadial, numCapAxial);

    return
        HdMeshSchema::Builder()
            .SetTopology(
                HdMeshTopologySchema::Builder()
                    .SetFaceVertexCounts(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexCounts()))
                    .SetFaceVertexIndices(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexIndices()))
                    .SetOrientation(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            HdMeshTopologySchemaTokens->rightHanded))
                    .Build())
            .SetSubdivisionScheme(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    topology.GetScheme()))
            .SetDoubleSided(
                HdRetainedTypedSampledDataSource<bool>::New(false))
            .Build();
}

class _PointsDataSource : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtVec3fArray GetTypedValue(const Time shutterOffset) override {

        const GfMatrix4d basis = _GetBasis(_GetAxis(shutterOffset));
        const size_t numPoints = GeomUtilCapsuleMeshGenerator::ComputeNumPoints(
            numRadial, numCapAxial);

        VtVec3fArray points(numPoints);
        GeomUtilCapsuleMeshGenerator::GeneratePoints(
            points.begin(),
            numRadial,
            numCapAxial,
            _GetRadiusBottom(shutterOffset),
            _GetRadiusTop(shutterOffset),
            _GetHeight(shutterOffset),
            &basis
        );

        return points;
    }

    bool GetContributingSampleTimesForInterval(
                            const Time startTime,
                            const Time endTime,
                            std::vector<Time> * const outSampleTimes) override
    {
        // Note contributing sources also include radius source for backward
        // compatibility with cylinder schema with just 1 radius.
        HdSampledDataSourceHandle sources[] = {
            _GetHeightSource(), _GetRadiusSource(), _GetRadiusBottomSource(),
            _GetRadiusTopSource(), _GetAxisSource() };
        return HdGetMergedContributingSampleTimesForInterval(
            TfArraySize(sources), sources, startTime, endTime, outSampleTimes);
    }

private:
    _PointsDataSource(const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    HdDoubleDataSourceHandle _GetHeightSource() const
    {
        static const HdDataSourceLocator sizeLocator(
            HdCapsuleSchema::GetSchemaToken(),
            HdCapsuleSchemaTokens->height);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    double _GetHeight(const Time shutterOffset) const
    {
        if (HdDoubleDataSourceHandle const s = _GetHeightSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 1.0;
    }

    HdDoubleDataSourceHandle _GetRadiusSource() const
    {
        static const HdDataSourceLocator sizeLocator(
            HdCapsuleSchema::GetSchemaToken(), HdCapsuleSchemaTokens->radius);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    HdDoubleDataSourceHandle _GetRadiusBottomSource() const
    {
        static const HdDataSourceLocator sizeLocator(
            HdCapsuleSchema::GetSchemaToken(), 
            HdCapsuleSchemaTokens->radiusBottom);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    double _GetRadiusBottom(const Time shutterOffset) const
    {
        if (HdDoubleDataSourceHandle const s = _GetRadiusBottomSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        // Fallback to old cylinder schema - deprecated
        if (HdDoubleDataSourceHandle const s = _GetRadiusSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 0.5;
    }

    HdDoubleDataSourceHandle _GetRadiusTopSource() const
    {
        static const HdDataSourceLocator sizeLocator(
            HdCapsuleSchema::GetSchemaToken(),
            HdCapsuleSchemaTokens->radiusTop);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    double _GetRadiusTop(const Time shutterOffset) const
    {
        if (HdDoubleDataSourceHandle const s = _GetRadiusTopSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        // Fallback to old cylinder schema - deprecated
        if (HdDoubleDataSourceHandle const s = _GetRadiusSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 0.5;
    }

    HdTokenDataSourceHandle _GetAxisSource() const
    {
        static const HdDataSourceLocator sizeLocator(
            HdCapsuleSchema::GetSchemaToken(),
            HdCapsuleSchemaTokens->axis);
        return HdTokenDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    TfToken _GetAxis(const Time shutterOffset) const {
        if (HdTokenDataSourceHandle const s = _GetAxisSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return HdCapsuleSchemaTokens->Z;
    }

    HdContainerDataSourceHandle _primDataSource;
};

HdContainerDataSourceHandle
_ComputePointsPrimvarDataSource(
    const HdContainerDataSourceHandle &primDataSource)
{
    static HdTokenDataSourceHandle const roleDataSource =
        HdPrimvarSchema::BuildRoleDataSource(
            HdPrimvarSchemaTokens->point);
    static HdTokenDataSourceHandle const interpolationDataSource =
        HdPrimvarSchema::BuildInterpolationDataSource(
            HdPrimvarSchemaTokens->vertex);

    return
        HdPrimvarSchema::Builder()
            .SetRole(roleDataSource)
            .SetInterpolation(interpolationDataSource)
            .SetPrimvarValue(_PointsDataSource::New(primDataSource))
            .Build();
}

HdContainerDataSourceHandle
_ComputePrimvarsDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    return
        HdRetainedContainerDataSource::New(
            HdPrimvarsSchemaTokens->points,
                    _ComputePointsPrimvarDataSource(primDataSource));
}

HdContainerDataSourceHandle
_ComputePrimDataSource(
    const SdfPath &primPath,
    const HdContainerDataSourceHandle &primDataSource)
{
    static HdDataSourceBaseHandle const capsuleDataSource =
        HdBlockDataSource::New();
    static HdDataSourceBaseHandle const meshDataSource =
        _ComputeMeshDataSource();
    HdDataSourceBaseHandle const primvarsDataSource =
        _ComputePrimvarsDataSource(primDataSource);
    HdDataSourceBaseHandle const dependenciesDataSource =
        _ComputePointsDependenciesDataSource<HdCapsuleSchema>(primPath);

    HdContainerDataSourceHandle sources[] = {
        HdRetainedContainerDataSource::New(
            HdCapsuleSchema::GetSchemaToken(), capsuleDataSource,
            HdMeshSchemaTokens->mesh, meshDataSource,
            HdPrimvarsSchemaTokens->primvars, primvarsDataSource,
            HdDependenciesSchemaTokens->__dependencies, dependenciesDataSource),
        primDataSource
    };

    return HdOverlayContainerDataSource::New(TfArraySize(sources), sources);
}

} // namespace _CapsuleToMesh

namespace _CylinderToTransformedCylinder
{

class _MatrixDataSource : public HdMatrixDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MatrixDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    GfMatrix4d GetTypedValue(const Time shutterOffset) override {
        return _GetAdjustmentMatrix(shutterOffset) * _GetMatrix(shutterOffset);
    }

    bool GetContributingSampleTimesForInterval(
                            const Time startTime,
                            const Time endTime,
                            std::vector<Time> * const outSampleTimes) override {
        HdSampledDataSourceHandle sources[] = {
            _GetMatrixSource(), _GetAxisSource() };
        return HdGetMergedContributingSampleTimesForInterval(
            TfArraySize(sources), sources, startTime, endTime, outSampleTimes);
    }

private:
    _MatrixDataSource(const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    HdMatrixDataSourceHandle _GetMatrixSource() const {
        return HdXformSchema::GetFromParent(_primDataSource).GetMatrix();
    }

    GfMatrix4d _GetMatrix(const Time shutterOffset) const {
        if (HdMatrixDataSourceHandle const src = _GetMatrixSource()) {
            return src->GetTypedValue(shutterOffset);
        }
        return GfMatrix4d(1.0);
    }

    HdTokenDataSourceHandle _GetAxisSource() const {
        static const HdDataSourceLocator locator(
            HdCylinderSchema::GetSchemaToken(), 
            HdCylinderSchemaTokens->axis);
        return HdTokenDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, locator));
    }

    TfToken _GetAxis(const Time shutterOffset) const {
        if (HdTokenDataSourceHandle const src = _GetAxisSource()) {
            return src->GetTypedValue(shutterOffset);
        }
        return TfToken();
    }
    
    const GfMatrix4d &_GetAdjustmentMatrix(const Time shutterOffset) const {
        const TfToken axis = _GetAxis(shutterOffset);
        if (axis == HdConeSchemaTokens->X) {
            static GfMatrix4d result(  0.0,  1.0,  0.0,  0.0,
                                       0.0,  0.0,  1.0,  0.0,
                                       1.0,  0.0,  0.0,  0.0,
                                       0.0,  0.0,  0.0,  1.0);
            return result;
        }
        if (axis == HdConeSchemaTokens->Y) {
            static GfMatrix4d result(  0.0,  0.0,  1.0,  0.0,
                                       1.0,  0.0,  0.0,  0.0,
                                       0.0,  1.0,  0.0,  0.0,
                                       0.0,  0.0,  0.0,  1.0);
            return result;
        }
        {
            static GfMatrix4d result(1.0);
            return result;
        }
    }

    HdContainerDataSourceHandle _primDataSource;
};

HdContainerDataSourceHandle
_ComputePrimDataSource(
    const SdfPath &primPath,
    const HdContainerDataSourceHandle &primDataSource)
{
    HdContainerDataSourceHandle xformSrc =
        HdXformSchema::Builder()
            .SetMatrix(_MatrixDataSource::New(primDataSource))
            .Build();

    HdDataSourceBaseHandle const dependenciesDataSource =
        _ComputeMatrixDependenciesDataSource<HdCylinderSchema>(primPath);

    HdContainerDataSourceHandle sources[] = {
        HdRetainedContainerDataSource::New(
            HdXformSchemaTokens->xform, std::move(xformSrc),
            HdDependenciesSchemaTokens->__dependencies, dependenciesDataSource),
        primDataSource
    };

    return HdOverlayContainerDataSource::New(TfArraySize(sources), sources);
}

} // namespace _CylinderToTransformedCylinder

namespace _ConeToTransformedCone
{

class _MatrixDataSource : public HdMatrixDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MatrixDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    GfMatrix4d GetTypedValue(const Time shutterOffset) override {
        return
            _GetHeightOffsetMatrix(shutterOffset) *
            _GetAdjustmentMatrix(shutterOffset) *
            _GetMatrix(shutterOffset);
    }

    bool GetContributingSampleTimesForInterval(
                            const Time startTime,
                            const Time endTime,
                            std::vector<Time> * const outSampleTimes) override {

        HdSampledDataSourceHandle sources[] = {
            _GetMatrixSource(), _GetAxisSource(), _GetHeightSource() };
        return HdGetMergedContributingSampleTimesForInterval(
            TfArraySize(sources), sources, startTime, endTime, outSampleTimes);
    }

private:
    _MatrixDataSource(const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    HdMatrixDataSourceHandle _GetMatrixSource() const {
        return HdXformSchema::GetFromParent(_primDataSource).GetMatrix();
    }

    GfMatrix4d _GetMatrix(const Time shutterOffset) const {
        if (HdMatrixDataSourceHandle const src = _GetMatrixSource()) {
            return src->GetTypedValue(shutterOffset);
        }
        return GfMatrix4d(1.0);
    }

    HdTokenDataSourceHandle _GetAxisSource() const {
        static const HdDataSourceLocator locator(
            HdConeSchemaTokens->cone, HdConeSchemaTokens->axis);
        return HdTokenDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, locator));
    }

    TfToken _GetAxis(const Time shutterOffset) const {
        if (HdTokenDataSourceHandle const src = _GetAxisSource()) {
            return src->GetTypedValue(shutterOffset);
        }
        return TfToken();
    }
    
    const GfMatrix4d &_GetAdjustmentMatrix(const Time shutterOffset) const {
        const TfToken axis = _GetAxis(shutterOffset);
        if (axis == HdConeSchemaTokens->X) {
            static GfMatrix4d result(  0.0,  1.0,  0.0,  0.0,
                                       0.0,  0.0,  1.0,  0.0,
                                       1.0,  0.0,  0.0,  0.0,
                                       0.0,  0.0,  0.0,  1.0);
            return result;
        }
        if (axis == HdConeSchemaTokens->Y) {
            static GfMatrix4d result(  0.0,  0.0,  1.0,  0.0,
                                       1.0,  0.0,  0.0,  0.0,
                                       0.0,  1.0,  0.0,  0.0,
                                       0.0,  0.0,  0.0,  1.0);
            return result;
        }
        {
            static GfMatrix4d result(1.0);
            return result;
        }
    }

    HdDoubleDataSourceHandle _GetHeightSource() const {
        static const HdDataSourceLocator sizeLocator(
            HdConeSchemaTokens->cone, HdConeSchemaTokens->height);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    double _GetHeight(const Time shutterOffset) const {
        if (HdDoubleDataSourceHandle const s = _GetHeightSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 1.0;
    }

    GfMatrix4d _GetHeightOffsetMatrix(const Time shutterOffset) const {
        const GfVec3d t(0.0, 0.0, -0.5 * _GetHeight(shutterOffset));
        return GfMatrix4d(1.0).SetTranslate(t);
    }

    HdContainerDataSourceHandle _primDataSource;
};

HdContainerDataSourceHandle
_ComputePrimDataSource(
    const SdfPath &primPath,
    const HdContainerDataSourceHandle &primDataSource)
{
    HdContainerDataSourceHandle xformSrc =
        HdXformSchema::Builder()
            .SetMatrix(_MatrixDataSource::New(primDataSource))
            .Build();
    HdDataSourceBaseHandle const dependenciesDataSource =
        _ComputeMatrixDependenciesDataSource<HdConeSchema>(primPath);
    
    HdContainerDataSourceHandle sources[] = {
        HdRetainedContainerDataSource::New(
            HdXformSchemaTokens->xform, std::move(xformSrc),
            HdDependenciesSchemaTokens->__dependencies, dependenciesDataSource),
        primDataSource
    };

    return HdOverlayContainerDataSource::New(TfArraySize(sources), sources);
}

} // namespace _AxisToTransform

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Plane

namespace _PlaneToMesh
{

HdContainerDataSourceHandle
_ComputeMeshDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    const PxOsdMeshTopology topology = 
        GeomUtilPlaneMeshGenerator::GenerateTopology();

    const HdDataSourceLocator doubleSidedLocator =
        HdPlaneSchema::GetDefaultLocator().Append(
            HdPlaneSchemaTokens->doubleSided);
    HdBoolDataSourceHandle doubleSidedDs =
        HdBoolDataSource::Cast(
            HdContainerDataSource::Get(primDataSource, doubleSidedLocator));

    return
        HdMeshSchema::Builder()
            .SetTopology(
                HdMeshTopologySchema::Builder()
                    .SetFaceVertexCounts(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexCounts()))
                    .SetFaceVertexIndices(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexIndices()))
                    .SetOrientation(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            HdMeshTopologySchemaTokens->rightHanded))
                    .Build())
            .SetSubdivisionScheme(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    topology.GetScheme()))
            .SetDoubleSided(doubleSidedDs)
            .Build();
}

class _PointsDataSource : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtVec3fArray GetTypedValue(const Time shutterOffset) override {
        const GfMatrix4d basis = _GetBasis(_GetAxis(shutterOffset));
        const size_t numPoints =
            GeomUtilPlaneMeshGenerator::ComputeNumPoints();
        VtVec3fArray points(numPoints);

        GeomUtilPlaneMeshGenerator::GeneratePoints(
            points.begin(),
            _GetWidth(shutterOffset),
            _GetLength(shutterOffset),
            &basis
        );
        
        return points;
    }

    bool GetContributingSampleTimesForInterval(
                            const Time startTime,
                            const Time endTime,
                            std::vector<Time> * const outSampleTimes) override {
        HdSampledDataSourceHandle sources[] = {
            _GetWidthSource(), _GetLengthSource(), _GetAxisSource() };
        return HdGetMergedContributingSampleTimesForInterval(
            TfArraySize(sources), sources, startTime, endTime, outSampleTimes);
    }

private:
    _PointsDataSource(const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    HdDoubleDataSourceHandle _GetWidthSource() const {
        static const HdDataSourceLocator widthLocator =
            HdPlaneSchema::GetDefaultLocator().Append(
                HdPlaneSchemaTokens->width);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, widthLocator));
    }

    double _GetWidth(const Time shutterOffset) const {
        if (HdDoubleDataSourceHandle const s = _GetWidthSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 1.0;
    }

    HdDoubleDataSourceHandle _GetLengthSource() const {
        static const HdDataSourceLocator sizeLocator =
            HdPlaneSchema::GetDefaultLocator().Append(
                HdPlaneSchemaTokens->length);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    double _GetLength(const Time shutterOffset) const {
        if (HdDoubleDataSourceHandle const s = _GetLengthSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 1.0;
    }

    HdTokenDataSourceHandle _GetAxisSource() const {
        static const HdDataSourceLocator locator =
            HdPlaneSchema::GetDefaultLocator().Append(
                HdPlaneSchemaTokens->axis);
        return HdTokenDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, locator));
    }

    TfToken _GetAxis(const Time shutterOffset) const {
        if (HdTokenDataSourceHandle const s = _GetAxisSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return HdPlaneSchemaTokens->X;
    }

    HdContainerDataSourceHandle _primDataSource;
};

HdContainerDataSourceHandle
_ComputePointsPrimvarDataSource(
    const HdContainerDataSourceHandle &primDataSource)
{
    static HdTokenDataSourceHandle const roleDataSource =
        HdPrimvarSchema::BuildRoleDataSource(
            HdPrimvarSchemaTokens->point);
    static HdTokenDataSourceHandle const interpolationDataSource =
        HdPrimvarSchema::BuildInterpolationDataSource(
            HdPrimvarSchemaTokens->vertex);

    return
        HdPrimvarSchema::Builder()
            .SetRole(roleDataSource)
            .SetInterpolation(interpolationDataSource)
            .SetPrimvarValue(_PointsDataSource::New(primDataSource))
            .Build();
}

HdContainerDataSourceHandle
_ComputePrimvarsDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    return
        HdRetainedContainerDataSource::New(
            HdPrimvarsSchemaTokens->points,
                    _ComputePointsPrimvarDataSource(primDataSource));
}

HdContainerDataSourceHandle
_ComputePrimDataSource(
    const SdfPath &primPath,
    const HdContainerDataSourceHandle &primDataSource)
{
    static HdDataSourceBaseHandle const planeDataSource =
        HdBlockDataSource::New();
    HdDataSourceBaseHandle const meshDataSource =
        _ComputeMeshDataSource(primDataSource);
    HdDataSourceBaseHandle const primvarsDataSource =
        _ComputePrimvarsDataSource(primDataSource);
    HdDataSourceBaseHandle const dependenciesDataSource =
        _ComputePointsDependenciesDataSource<HdPlaneSchema>(primPath);

    HdContainerDataSourceHandle sources[] = {
        HdRetainedContainerDataSource::New(
            HdPlaneSchema::GetSchemaToken(), planeDataSource,
            HdMeshSchema::GetSchemaToken(), meshDataSource,
            HdPrimvarsSchema::GetSchemaToken(), primvarsDataSource,
            HdDependenciesSchema::GetSchemaToken(), dependenciesDataSource),
        primDataSource
    };

    return HdOverlayContainerDataSource::New(TfArraySize(sources), sources);
}

} // namespace _PlaneToMesh

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Scene index implementation

TfToken
_GetMode(const HdContainerDataSourceHandle &inputArgs,
         const TfToken &primType)
{
    if (!inputArgs) {
        return TfToken();
    }

    HdTokenDataSourceHandle const src =
        HdTokenDataSource::Cast(inputArgs->Get(primType));
    if (!src) {
        return TfToken();
    }

    return src->GetTypedValue(0.0f);
}

}

HdsiImplicitSurfaceSceneIndexRefPtr
HdsiImplicitSurfaceSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
{
    return TfCreateRefPtr(
        new HdsiImplicitSurfaceSceneIndex(
            inputSceneIndex, inputArgs));
}

HdsiImplicitSurfaceSceneIndex::HdsiImplicitSurfaceSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _capsuleMode(_GetMode(inputArgs, HdPrimTypeTokens->capsule))
  , _coneMode(_GetMode(inputArgs, HdPrimTypeTokens->cone))
  , _cubeMode(_GetMode(inputArgs, HdPrimTypeTokens->cube))
  , _cylinderMode(_GetMode(inputArgs, HdPrimTypeTokens->cylinder))
  , _planeMode(_GetMode(inputArgs, HdPrimTypeTokens->plane))
  , _sphereMode(_GetMode(inputArgs, HdPrimTypeTokens->sphere))
{

}

HdSceneIndexPrim
HdsiImplicitSurfaceSceneIndex::GetPrim(const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.primType == HdPrimTypeTokens->cube) {
        if (_cubeMode == HdsiImplicitSurfaceSceneIndexTokens->toMesh) {
            return {
                HdPrimTypeTokens->mesh,
                _CubeToMesh::_ComputePrimDataSource(
                    primPath, prim.dataSource) };
        }
    }
    if (prim.primType == HdPrimTypeTokens->cone) {
        if (_coneMode == HdsiImplicitSurfaceSceneIndexTokens->toMesh) {
            return {
                HdPrimTypeTokens->mesh,
                _ConeToMesh::_ComputePrimDataSource(
                    primPath, prim.dataSource) };
        }
        if (_coneMode ==
                HdsiImplicitSurfaceSceneIndexTokens->axisToTransform) {
            return {
                prim.primType,
                _ConeToTransformedCone::_ComputePrimDataSource(
                    primPath, prim.dataSource) };
        }
    }
    if (prim.primType == HdPrimTypeTokens->cylinder) {
        if (_cylinderMode == HdsiImplicitSurfaceSceneIndexTokens->toMesh) {
            return {
                HdPrimTypeTokens->mesh,
                _CylinderToMesh::_ComputePrimDataSource(
                    primPath, prim.dataSource) };
        }
        if (_cylinderMode ==
                HdsiImplicitSurfaceSceneIndexTokens->axisToTransform) {
            return {
                prim.primType,
                _CylinderToTransformedCylinder::_ComputePrimDataSource(
                    primPath, prim.dataSource) };
        }
    }
    if (prim.primType == HdPrimTypeTokens->sphere) {
        if (_sphereMode == HdsiImplicitSurfaceSceneIndexTokens->toMesh) {
            return {
                HdPrimTypeTokens->mesh,
                _SphereToMesh::_ComputePrimDataSource(
                    primPath, prim.dataSource) };
        }
    }
    if (prim.primType == HdPrimTypeTokens->capsule) {
        if (_capsuleMode == HdsiImplicitSurfaceSceneIndexTokens->toMesh) {
            return {
                HdPrimTypeTokens->mesh,
                _CapsuleToMesh::_ComputePrimDataSource(
                    primPath, prim.dataSource) };
        }
    }
    if (prim.primType == HdPrimTypeTokens->plane) {
        if (_planeMode == HdsiImplicitSurfaceSceneIndexTokens->toMesh) {
            return {
                HdPrimTypeTokens->mesh,
                _PlaneToMesh::_ComputePrimDataSource(
                    primPath, prim.dataSource) };
        }
    }
    return prim;
}

SdfPathVector
HdsiImplicitSurfaceSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiImplicitSurfaceSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }
    
    TRACE_FUNCTION();

    std::vector<size_t> indices;
    for (size_t i = 0; i < entries.size(); i++) {
        if ((entries[i].primType == HdPrimTypeTokens->cube &&
             _cubeMode == HdsiImplicitSurfaceSceneIndexTokens->toMesh) ||
            (entries[i].primType == HdPrimTypeTokens->cone &&
             _coneMode == HdsiImplicitSurfaceSceneIndexTokens->toMesh) ||
            (entries[i].primType == HdPrimTypeTokens->cylinder &&
             _cylinderMode == HdsiImplicitSurfaceSceneIndexTokens->toMesh) ||
            (entries[i].primType == HdPrimTypeTokens->sphere &&
             _sphereMode == HdsiImplicitSurfaceSceneIndexTokens->toMesh) ||
            (entries[i].primType == HdPrimTypeTokens->capsule &&
             _capsuleMode == HdsiImplicitSurfaceSceneIndexTokens->toMesh) ||
            (entries[i].primType == HdPrimTypeTokens->plane &&
             _planeMode == HdsiImplicitSurfaceSceneIndexTokens->toMesh)) {
            indices.push_back(i);
        }
    }

    if (indices.empty()) {
        _SendPrimsAdded(entries);
        return;
    }

    HdSceneIndexObserver::AddedPrimEntries newEntries(entries);
    for (const size_t i : indices) {
        newEntries[i].primType = HdPrimTypeTokens->mesh;
    }
    
    _SendPrimsAdded(newEntries);
}

void 
HdsiImplicitSurfaceSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }
    
    _SendPrimsRemoved(entries);
}

void
HdsiImplicitSurfaceSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    _SendPrimsDirtied(entries);
}


PXR_NAMESPACE_CLOSE_SCOPE
