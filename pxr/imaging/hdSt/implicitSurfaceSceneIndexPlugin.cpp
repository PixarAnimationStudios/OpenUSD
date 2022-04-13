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

#include "pxr/imaging/hdSt/implicitSurfaceSceneIndexPlugin.h"

#include "pxr/imaging/hd/coneSchema.h"
#include "pxr/imaging/hd/cubeSchema.h"
#include "pxr/imaging/hd/cylinderSchema.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/sphereSchema.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/tf/iterator.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdSt_ImplicitSurfaceSceneIndexPlugin"))
);

static const char * const _pluginDisplayName = "GL";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdSt_ImplicitSurfaceSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        nullptr,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

namespace
{

GfMatrix4d
_ConeAndCylinderTransform(const double height,
                          const double radius,
                          const TfToken &axis)
{
    const double diameter = 2.0 * radius;
    if (axis == HdConeSchemaTokens->X) {
        return GfMatrix4d(     0.0, diameter,      0.0, 0.0,
                               0.0,      0.0, diameter, 0.0,
                            height,      0.0,      0.0, 0.0,
                               0.0,      0.0,      0.0, 1.0);
    }
    else if (axis == HdConeSchemaTokens->Y) {
        return GfMatrix4d(     0.0,      0.0, diameter, 0.0,
                          diameter,      0.0,      0.0, 0.0,
                               0.0,   height,      0.0, 0.0,
                               0.0,      0.0,      0.0, 1.0);
    }
    else { // (axis == HdConeSchemaTokens->Z)
        return GfMatrix4d(diameter,      0.0,      0.0, 0.0,
                               0.0, diameter,      0.0, 0.0,
                               0.0,      0.0,   height, 0.0,
                               0.0,      0.0,      0.0, 1.0);
    }
}

using Time = HdSampledDataSource::Time;

// Wrapper around std::set_union to compute the set-wise union of two
// sorted vectors of sample times.
std::vector<Time> _Union(const std::vector<Time> &a, const std::vector<Time> &b)
{
    std::vector<Time> result;
    std::set_union(a.begin(), a.end(),
                   b.begin(), b.end(),
                   std::back_inserter(result));
    return result;
}

// Computes union of contributing sample times from several data sources.
template<size_t N>
bool _GetContributingSampleTimesForInterval(
    const std::array<HdSampledDataSourceHandle, N> &srcs,
    const Time startTime,
    const Time endTime,
    std::vector<Time> * const outSampleTimes)
{
    bool result = false;
    for (size_t i = 0; i < N; i++) {
        if (!srcs[i]) {
            continue;
        }
        std::vector<Time> times;
        if (!srcs[i]->GetContributingSampleTimesForInterval(
                startTime, endTime, &times)) {
            continue;
        }
        result = true;
        if (!outSampleTimes) {
            continue;
        }
        if (outSampleTimes->empty()) {
            *outSampleTimes = std::move(times);
        } else {
            *outSampleTimes = _Union(*outSampleTimes, times);
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Cube

namespace _CubeToMesh
{

HdContainerDataSourceHandle
_ComputeMeshDataSource()
{
    static const VtIntArray numVerts{ 4, 4, 4, 4, 4, 4 };
    static const VtIntArray verts{ 0, 1, 2, 3,
                                   4, 5, 6, 7,
                                   0, 6, 5, 1,
                                   4, 7, 3, 2,
                                   0, 3, 7, 6,
                                   4, 2, 1, 5 };

    return
        HdMeshSchema::Builder()
            .SetTopology(
                HdMeshTopologySchema::Builder()
                    .SetFaceVertexCounts(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            numVerts))
                    .SetFaceVertexIndices(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            verts))
                    .SetOrientation(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            HdMeshTopologySchemaTokens->rightHanded))
                    .Build())
            .SetDoubleSided(
                HdRetainedTypedSampledDataSource<bool>::New(false))
            .Build();
}

class _PointsDataSource : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsDataSource);

    _PointsDataSource(const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtVec3fArray GetTypedValue(const Time shutterOffset) override {
        static const VtVec3fArray points{
            GfVec3f( 0.5f,  0.5f,  0.5f),
            GfVec3f(-0.5f,  0.5f,  0.5f),
            GfVec3f(-0.5f, -0.5f,  0.5f),
            GfVec3f( 0.5f, -0.5f,  0.5f),
            GfVec3f(-0.5f, -0.5f, -0.5f),
            GfVec3f(-0.5f,  0.5f, -0.5f),
            GfVec3f( 0.5f,  0.5f, -0.5f),
            GfVec3f( 0.5f, -0.5f, -0.5f)
        };

        const double size = _GetSize(shutterOffset);

        VtVec3fArray scaledPoints;
        scaledPoints.resize(points.size());
        for (size_t i = 0; i < points.size(); i++) {
            scaledPoints[i] = size * points[i];
        }
        
        return scaledPoints;
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
_ComputePointsPrimvarDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    static HdTokenDataSourceHandle const roleDataSource =
        HdPrimvarSchema::BuildRoleDataSource(
            HdPrimvarSchemaTokens->Point);
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
_ComputePrimDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    static HdDataSourceBaseHandle const cubeDataSource =
        HdBlockDataSource::New();
    static HdDataSourceBaseHandle const meshDataSource =
        _ComputeMeshDataSource();
    HdDataSourceBaseHandle const primvarsDataSource =
        _ComputePrimvarsDataSource(primDataSource);

    HdContainerDataSourceHandle sources[] = {
        HdRetainedContainerDataSource::New(
            HdCubeSchemaTokens->cube, cubeDataSource,
            HdMeshSchemaTokens->mesh, meshDataSource,
            HdPrimvarsSchemaTokens->primvars, primvarsDataSource),
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

HdContainerDataSourceHandle
_ComputeMeshDataSource()
{
    static const VtIntArray numVerts{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                                      4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };
    static const VtIntArray verts{
        // Tris
         2,  1,  0,    3,  2,  0,    4,  3,  0,    5,  4,  0,    6,  5,  0,
         7,  6,  0,    8,  7,  0,    9,  8,  0,   10,  9,  0,    1, 10,  0,
        // Quads
        11, 12, 22, 21,   12, 13, 23, 22,   13, 14, 24, 23,   14, 15, 25, 24,
        15, 16, 26, 25,   16, 17, 27, 26,   17, 18, 28, 27,   18, 19, 29, 28,
        19, 20, 30, 29,   20, 11, 21, 30 };

    return
        HdMeshSchema::Builder()
            .SetTopology(
                HdMeshTopologySchema::Builder()
                    .SetFaceVertexCounts(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            numVerts))
                    .SetFaceVertexIndices(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            verts))
                    .SetOrientation(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            HdMeshTopologySchemaTokens->rightHanded))
                    .Build())
            .SetSubdivisionScheme(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    PxOsdOpenSubdivTokens->catmullClark))
            .SetDoubleSided(
                HdRetainedTypedSampledDataSource<bool>::New(false))
            .Build();
}

class _PointsDataSource : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsDataSource);

    _PointsDataSource(const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtVec3fArray GetTypedValue(const Time shutterOffset) override {
        static const VtVec3fArray points{
            GfVec3f( 0.0000,  0.0000, -0.5000),
            GfVec3f( 0.5000,  0.0000, -0.5000),
            GfVec3f( 0.4045,  0.2939, -0.5000),
            GfVec3f( 0.1545,  0.4755, -0.5000),
            GfVec3f(-0.1545,  0.4755, -0.5000),
            GfVec3f(-0.4045,  0.2939, -0.5000),
            GfVec3f(-0.5000,  0.0000, -0.5000),
            GfVec3f(-0.4045, -0.2939, -0.5000),
            GfVec3f(-0.1545, -0.4755, -0.5000),
            GfVec3f( 0.1545, -0.4755, -0.5000),
            GfVec3f( 0.4045, -0.2939, -0.5000),
            GfVec3f( 0.5000,  0.0000, -0.5000),
            GfVec3f( 0.4045,  0.2939, -0.5000),
            GfVec3f( 0.1545,  0.4755, -0.5000),
            GfVec3f(-0.1545,  0.4755, -0.5000),
            GfVec3f(-0.4045,  0.2939, -0.5000),
            GfVec3f(-0.5000,  0.0000, -0.5000),
            GfVec3f(-0.4045, -0.2939, -0.5000),
            GfVec3f(-0.1545, -0.4755, -0.5000),
            GfVec3f( 0.1545, -0.4755, -0.5000),
            GfVec3f( 0.4045, -0.2939, -0.5000),
            GfVec3f( 0.0000,  0.0000,  0.5000),
            GfVec3f( 0.0000,  0.0000,  0.5000),
            GfVec3f( 0.0000,  0.0000,  0.5000),
            GfVec3f( 0.0000,  0.0000,  0.5000),
            GfVec3f( 0.0000,  0.0000,  0.5000),
            GfVec3f( 0.0000,  0.0000,  0.5000),
            GfVec3f( 0.0000,  0.0000,  0.5000),
            GfVec3f( 0.0000,  0.0000,  0.5000),
            GfVec3f( 0.0000,  0.0000,  0.5000),
            GfVec3f( 0.0000,  0.0000,  0.5000) };

        const GfMatrix4d t = _ConeAndCylinderTransform(
            _GetHeight(shutterOffset),
            _GetRadius(shutterOffset),
            _GetAxis(shutterOffset));

        VtVec3fArray scaledPoints;
        scaledPoints.resize(points.size());
        for (size_t i = 0; i < points.size(); i++) {
            scaledPoints[i] = t.Transform(points[i]);
        }

        return scaledPoints;
    }

    bool GetContributingSampleTimesForInterval(
                            const Time startTime,
                            const Time endTime,
                            std::vector<Time> * const outSampleTimes) override {
        return _GetContributingSampleTimesForInterval<3>(
            { _GetHeightSource(), _GetRadiusSource(), _GetAxisSource() },
            startTime,
            endTime,
            outSampleTimes);
    }

private:
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

    HdDoubleDataSourceHandle _GetRadiusSource() const {
        static const HdDataSourceLocator sizeLocator(
            HdConeSchemaTokens->cone, HdConeSchemaTokens->radius);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    double _GetRadius(const Time shutterOffset) const {
        if (HdDoubleDataSourceHandle const s = _GetRadiusSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 1.0;
    }

    HdTokenDataSourceHandle _GetAxisSource() const {
        static const HdDataSourceLocator sizeLocator(
            HdConeSchemaTokens->cone, HdConeSchemaTokens->axis);
        return HdTokenDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
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
_ComputePointsPrimvarDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    static HdTokenDataSourceHandle const roleDataSource =
        HdPrimvarSchema::BuildRoleDataSource(
            HdPrimvarSchemaTokens->Point);
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
_ComputePrimDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    static HdDataSourceBaseHandle const coneDataSource =
        HdBlockDataSource::New();
    static HdDataSourceBaseHandle const meshDataSource =
        _ComputeMeshDataSource();
    HdDataSourceBaseHandle const primvarsDataSource =
        _ComputePrimvarsDataSource(primDataSource);

    HdContainerDataSourceHandle sources[] = {
        HdRetainedContainerDataSource::New(
            HdConeSchemaTokens->cone, coneDataSource,
            HdMeshSchemaTokens->mesh, meshDataSource,
            HdPrimvarsSchemaTokens->primvars, primvarsDataSource),
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

HdContainerDataSourceHandle
_ComputeMeshDataSource()
{
    static const VtIntArray numVerts{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                                      4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                                      3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
    static const VtIntArray verts{
        // Tris
         2,  1,  0,    3,  2,  0,    4,  3,  0,    5,  4,  0,    6,  5,  0,
         7,  6,  0,    8,  7,  0,    9,  8,  0,   10,  9,  0,    1, 10,  0,
        // Quads
        11, 12, 22, 21,   12, 13, 23, 22,   13, 14, 24, 23,   14, 15, 25, 24,
        15, 16, 26, 25,   16, 17, 27, 26,   17, 18, 28, 27,   18, 19, 29, 28,
        19, 20, 30, 29,   20, 11, 21, 30,
        // Tris
        31, 32, 41,   32, 33, 41,   33, 34, 41,   34, 35, 41,   35, 36, 41,
        36, 37, 41,   37, 38, 41,   38, 39, 41,   39, 40, 41,   40, 31, 41 };

    return
        HdMeshSchema::Builder()
            .SetTopology(
                HdMeshTopologySchema::Builder()
                    .SetFaceVertexCounts(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            numVerts))
                    .SetFaceVertexIndices(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            verts))
                    .SetOrientation(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            HdMeshTopologySchemaTokens->rightHanded))
                    .Build())
            .SetSubdivisionScheme(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    PxOsdOpenSubdivTokens->catmullClark))
            .SetDoubleSided(
                HdRetainedTypedSampledDataSource<bool>::New(false))
            .Build();
}

class _PointsDataSource : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsDataSource);

    _PointsDataSource(const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtVec3fArray GetTypedValue(const Time shutterOffset) override {
        static const VtVec3fArray points{
            GfVec3f( 0.0000,  0.0000, -0.5000),
            GfVec3f( 0.5000,  0.0000, -0.5000),
            GfVec3f( 0.4045,  0.2939, -0.5000),
            GfVec3f( 0.1545,  0.4755, -0.5000),
            GfVec3f(-0.1545,  0.4755, -0.5000),
            GfVec3f(-0.4045,  0.2939, -0.5000),
            GfVec3f(-0.5000,  0.0000, -0.5000),
            GfVec3f(-0.4045, -0.2939, -0.5000),
            GfVec3f(-0.1545, -0.4755, -0.5000),
            GfVec3f( 0.1545, -0.4755, -0.5000),
            GfVec3f( 0.4045, -0.2939, -0.5000),
            GfVec3f( 0.5000,  0.0000, -0.5000),
            GfVec3f( 0.4045,  0.2939, -0.5000),
            GfVec3f( 0.1545,  0.4755, -0.5000),
            GfVec3f(-0.1545,  0.4755, -0.5000),
            GfVec3f(-0.4045,  0.2939, -0.5000),
            GfVec3f(-0.5000,  0.0000, -0.5000),
            GfVec3f(-0.4045, -0.2939, -0.5000),
            GfVec3f(-0.1545, -0.4755, -0.5000),
            GfVec3f( 0.1545, -0.4755, -0.5000),
            GfVec3f( 0.4045, -0.2939, -0.5000),
            GfVec3f( 0.5000,  0.0000,  0.5000),
            GfVec3f( 0.4045,  0.2939,  0.5000),
            GfVec3f( 0.1545,  0.4755,  0.5000),
            GfVec3f(-0.1545,  0.4755,  0.5000),
            GfVec3f(-0.4045,  0.2939,  0.5000),
            GfVec3f(-0.5000,  0.0000,  0.5000),
            GfVec3f(-0.4045, -0.2939,  0.5000),
            GfVec3f(-0.1545, -0.4755,  0.5000),
            GfVec3f( 0.1545, -0.4755,  0.5000),
            GfVec3f( 0.4045, -0.2939,  0.5000),
            GfVec3f( 0.5000,  0.0000,  0.5000),
            GfVec3f( 0.4045,  0.2939,  0.5000),
            GfVec3f( 0.1545,  0.4755,  0.5000),
            GfVec3f(-0.1545,  0.4755,  0.5000),
            GfVec3f(-0.4045,  0.2939,  0.5000),
            GfVec3f(-0.5000,  0.0000,  0.5000),
            GfVec3f(-0.4045, -0.2939,  0.5000),
            GfVec3f(-0.1545, -0.4755,  0.5000),
            GfVec3f( 0.1545, -0.4755,  0.5000),
            GfVec3f( 0.4045, -0.2939,  0.5000),
            GfVec3f( 0.0000,  0.0000,  0.5000)};   

        const GfMatrix4d t = _ConeAndCylinderTransform(
            _GetHeight(shutterOffset),
            _GetRadius(shutterOffset),
            _GetAxis(shutterOffset));

        VtVec3fArray scaledPoints;
        scaledPoints.resize(points.size());
        for (size_t i = 0; i < points.size(); i++) {
            scaledPoints[i] = t.Transform(points[i]);
        }

        return scaledPoints;
    }

    bool GetContributingSampleTimesForInterval(
                            const Time startTime,
                            const Time endTime,
                            std::vector<Time> * const outSampleTimes) override {
        return _GetContributingSampleTimesForInterval<3>(
            { _GetHeightSource(), _GetRadiusSource(), _GetAxisSource() },
            startTime,
            endTime,
            outSampleTimes);
    }

private:
    HdDoubleDataSourceHandle _GetHeightSource() const {
        static const HdDataSourceLocator sizeLocator(
            HdCylinderSchemaTokens->cylinder, HdCylinderSchemaTokens->height);
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
        static const HdDataSourceLocator sizeLocator(
            HdCylinderSchemaTokens->cylinder, HdCylinderSchemaTokens->radius);
        return HdDoubleDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    double _GetRadius(const Time shutterOffset) const {
        if (HdDoubleDataSourceHandle const s = _GetRadiusSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return 1.0;
    }

    HdTokenDataSourceHandle _GetAxisSource() const {
        static const HdDataSourceLocator sizeLocator(
            HdCylinderSchemaTokens->cylinder, HdCylinderSchemaTokens->axis);
        return HdTokenDataSource::Cast(
            HdContainerDataSource::Get(_primDataSource, sizeLocator));
    }

    TfToken _GetAxis(const Time shutterOffset) const {
        if (HdTokenDataSourceHandle const s = _GetAxisSource()) {
            return s->GetTypedValue(shutterOffset);
        }
        return HdCylinderSchemaTokens->X;
    }

    HdContainerDataSourceHandle _primDataSource;
};

HdContainerDataSourceHandle
_ComputePointsPrimvarDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    static HdTokenDataSourceHandle const roleDataSource =
        HdPrimvarSchema::BuildRoleDataSource(
            HdPrimvarSchemaTokens->Point);
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
_ComputePrimDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    static HdDataSourceBaseHandle const cylinderDataSource =
        HdBlockDataSource::New();
    static HdDataSourceBaseHandle const meshDataSource =
        _ComputeMeshDataSource();
    HdDataSourceBaseHandle const primvarsDataSource =
        _ComputePrimvarsDataSource(primDataSource);

    HdContainerDataSourceHandle sources[] = {
        HdRetainedContainerDataSource::New(
            HdCylinderSchemaTokens->cylinder, cylinderDataSource,
            HdMeshSchemaTokens->mesh, meshDataSource,
            HdPrimvarsSchemaTokens->primvars, primvarsDataSource),
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

HdContainerDataSourceHandle
_ComputeMeshDataSource()
{
    static const VtIntArray numVerts{
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
    static const VtIntArray verts{
        // Quads
         0,  1, 11, 10,    1,  2, 12, 11,    2,  3, 13, 12,    3,  4, 14, 13,
         4,  5, 15, 14,    5,  6, 16, 15,    6,  7, 17, 16,    7,  8, 18, 17,
         8,  9, 19, 18,    9,  0, 10, 19,   10, 11, 21, 20,   11, 12, 22, 21,
        12, 13, 23, 22,   13, 14, 24, 23,   14, 15, 25, 24,   15, 16, 26, 25,
        16, 17, 27, 26,   17, 18, 28, 27,   18, 19, 29, 28,   19, 10, 20, 29,
        20, 21, 31, 30,   21, 22, 32, 31,   22, 23, 33, 32,   23, 24, 34, 33,
        24, 25, 35, 34,   25, 26, 36, 35,   26, 27, 37, 36,   27, 28, 38, 37,
        28, 29, 39, 38,   29, 20, 30, 39,   30, 31, 41, 40,   31, 32, 42, 41,
        32, 33, 43, 42,   33, 34, 44, 43,   34, 35, 45, 44,   35, 36, 46, 45,
        36, 37, 47, 46,   37, 38, 48, 47,   38, 39, 49, 48,   39, 30, 40, 49,
        40, 41, 51, 50,   41, 42, 52, 51,   42, 43, 53, 52,   43, 44, 54, 53,
        44, 45, 55, 54,   45, 46, 56, 55,   46, 47, 57, 56,   47, 48, 58, 57,
        48, 49, 59, 58,   49, 40, 50, 59,   50, 51, 61, 60,   51, 52, 62, 61,
        52, 53, 63, 62,   53, 54, 64, 63,   54, 55, 65, 64,   55, 56, 66, 65,
        56, 57, 67, 66,   57, 58, 68, 67,   58, 59, 69, 68,   59, 50, 60, 69,
        60, 61, 71, 70,   61, 62, 72, 71,   62, 63, 73, 72,   63, 64, 74, 73,
        64, 65, 75, 74,   65, 66, 76, 75,   66, 67, 77, 76,   67, 68, 78, 77,
        68, 69, 79, 78,   69, 60, 70, 79,   70, 71, 81, 80,   71, 72, 82, 81,
        72, 73, 83, 82,   73, 74, 84, 83,   74, 75, 85, 84,   75, 76, 86, 85,
        76, 77, 87, 86,   77, 78, 88, 87,   78, 79, 89, 88,   79, 70, 80, 89,
        // Tris
         1,  0, 90,    2,  1, 90,    3,  2, 90,    4,  3, 90,    5,  4, 90,
         6,  5, 90,    7,  6, 90,    8,  7, 90,    9,  8, 90,    0,  9, 90,
        80, 81, 91,   81, 82, 91,   82, 83, 91,   83, 84, 91,   84, 85, 91,
        85, 86, 91,   86, 87, 91,   87, 88, 91,   88, 89, 91,   89, 80, 91 };

    return
        HdMeshSchema::Builder()
            .SetTopology(
                HdMeshTopologySchema::Builder()
                    .SetFaceVertexCounts(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            numVerts))
                    .SetFaceVertexIndices(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            verts))
                    .SetOrientation(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            HdMeshTopologySchemaTokens->rightHanded))
                    .Build())
            .SetSubdivisionScheme(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    PxOsdOpenSubdivTokens->catmullClark))
            .SetDoubleSided(
                HdRetainedTypedSampledDataSource<bool>::New(false))
            .Build();
}

class _PointsDataSource : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsDataSource);

    _PointsDataSource(const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtVec3fArray GetTypedValue(const Time shutterOffset) override {
    static const VtVec3fArray points{
        GfVec3f( 0.1250,  0.0908, -0.4755), GfVec3f( 0.0477,  0.1469, -0.4755),
        GfVec3f(-0.0477,  0.1469, -0.4755), GfVec3f(-0.1250,  0.0908, -0.4755),
        GfVec3f(-0.1545, -0.0000, -0.4755), GfVec3f(-0.1250, -0.0908, -0.4755),
        GfVec3f(-0.0477, -0.1469, -0.4755), GfVec3f( 0.0477, -0.1469, -0.4755),
        GfVec3f( 0.1250, -0.0908, -0.4755), GfVec3f( 0.1545, -0.0000, -0.4755),
        GfVec3f( 0.2378,  0.1727, -0.4045), GfVec3f( 0.0908,  0.2795, -0.4045),
        GfVec3f(-0.0908,  0.2795, -0.4045), GfVec3f(-0.2378,  0.1727, -0.4045),
        GfVec3f(-0.2939, -0.0000, -0.4045), GfVec3f(-0.2378, -0.1727, -0.4045),
        GfVec3f(-0.0908, -0.2795, -0.4045), GfVec3f( 0.0908, -0.2795, -0.4045),
        GfVec3f( 0.2378, -0.1727, -0.4045), GfVec3f( 0.2939, -0.0000, -0.4045),
        GfVec3f( 0.3273,  0.2378, -0.2939), GfVec3f( 0.1250,  0.3847, -0.2939),
        GfVec3f(-0.1250,  0.3847, -0.2939), GfVec3f(-0.3273,  0.2378, -0.2939),
        GfVec3f(-0.4045, -0.0000, -0.2939), GfVec3f(-0.3273, -0.2378, -0.2939),
        GfVec3f(-0.1250, -0.3847, -0.2939), GfVec3f( 0.1250, -0.3847, -0.2939),
        GfVec3f( 0.3273, -0.2378, -0.2939), GfVec3f( 0.4045, -0.0000, -0.2939),
        GfVec3f( 0.3847,  0.2795, -0.1545), GfVec3f( 0.1469,  0.4523, -0.1545),
        GfVec3f(-0.1469,  0.4523, -0.1545), GfVec3f(-0.3847,  0.2795, -0.1545),
        GfVec3f(-0.4755, -0.0000, -0.1545), GfVec3f(-0.3847, -0.2795, -0.1545),
        GfVec3f(-0.1469, -0.4523, -0.1545), GfVec3f( 0.1469, -0.4523, -0.1545),
        GfVec3f( 0.3847, -0.2795, -0.1545), GfVec3f( 0.4755, -0.0000, -0.1545),
        GfVec3f( 0.4045,  0.2939, -0.0000), GfVec3f( 0.1545,  0.4755, -0.0000),
        GfVec3f(-0.1545,  0.4755, -0.0000), GfVec3f(-0.4045,  0.2939, -0.0000),
        GfVec3f(-0.5000, -0.0000,  0.0000), GfVec3f(-0.4045, -0.2939,  0.0000),
        GfVec3f(-0.1545, -0.4755,  0.0000), GfVec3f( 0.1545, -0.4755,  0.0000),
        GfVec3f( 0.4045, -0.2939,  0.0000), GfVec3f( 0.5000,  0.0000,  0.0000),
        GfVec3f( 0.3847,  0.2795,  0.1545), GfVec3f( 0.1469,  0.4523,  0.1545),
        GfVec3f(-0.1469,  0.4523,  0.1545), GfVec3f(-0.3847,  0.2795,  0.1545),
        GfVec3f(-0.4755, -0.0000,  0.1545), GfVec3f(-0.3847, -0.2795,  0.1545),
        GfVec3f(-0.1469, -0.4523,  0.1545), GfVec3f( 0.1469, -0.4523,  0.1545),
        GfVec3f( 0.3847, -0.2795,  0.1545), GfVec3f( 0.4755,  0.0000,  0.1545),
        GfVec3f( 0.3273,  0.2378,  0.2939), GfVec3f( 0.1250,  0.3847,  0.2939),
        GfVec3f(-0.1250,  0.3847,  0.2939), GfVec3f(-0.3273,  0.2378,  0.2939),
        GfVec3f(-0.4045, -0.0000,  0.2939), GfVec3f(-0.3273, -0.2378,  0.2939),
        GfVec3f(-0.1250, -0.3847,  0.2939), GfVec3f( 0.1250, -0.3847,  0.2939),
        GfVec3f( 0.3273, -0.2378,  0.2939), GfVec3f( 0.4045,  0.0000,  0.2939),
        GfVec3f( 0.2378,  0.1727,  0.4045), GfVec3f( 0.0908,  0.2795,  0.4045),
        GfVec3f(-0.0908,  0.2795,  0.4045), GfVec3f(-0.2378,  0.1727,  0.4045),
        GfVec3f(-0.2939, -0.0000,  0.4045), GfVec3f(-0.2378, -0.1727,  0.4045),
        GfVec3f(-0.0908, -0.2795,  0.4045), GfVec3f( 0.0908, -0.2795,  0.4045),
        GfVec3f( 0.2378, -0.1727,  0.4045), GfVec3f( 0.2939,  0.0000,  0.4045),
        GfVec3f( 0.1250,  0.0908,  0.4755), GfVec3f( 0.0477,  0.1469,  0.4755),
        GfVec3f(-0.0477,  0.1469,  0.4755), GfVec3f(-0.1250,  0.0908,  0.4755),
        GfVec3f(-0.1545, -0.0000,  0.4755), GfVec3f(-0.1250, -0.0908,  0.4755),
        GfVec3f(-0.0477, -0.1469,  0.4755), GfVec3f( 0.0477, -0.1469,  0.4755),
        GfVec3f( 0.1250, -0.0908,  0.4755), GfVec3f( 0.1545,  0.0000,  0.4755),
        GfVec3f( 0.0000, -0.0000, -0.5000), GfVec3f( 0.0000,  0.0000,  0.5000)};

        const double diameter = 2.0 * _GetRadius(shutterOffset);

        VtVec3fArray scaledPoints;
        scaledPoints.resize(points.size());
        for (size_t i = 0; i < points.size(); i++) {
            scaledPoints[i] = diameter * points[i];
        }
        
        return scaledPoints;
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
_ComputePointsPrimvarDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    static HdTokenDataSourceHandle const roleDataSource =
        HdPrimvarSchema::BuildRoleDataSource(
            HdPrimvarSchemaTokens->Point);
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
_ComputePrimDataSource(const HdContainerDataSourceHandle &primDataSource)
{
    static HdDataSourceBaseHandle const sphereDataSource =
        HdBlockDataSource::New();
    static HdDataSourceBaseHandle const meshDataSource =
        _ComputeMeshDataSource();
    HdDataSourceBaseHandle const primvarsDataSource =
        _ComputePrimvarsDataSource(primDataSource);
    
    HdContainerDataSourceHandle sources[] = {
        HdRetainedContainerDataSource::New(
            HdSphereSchemaTokens->sphere, sphereDataSource,
            HdMeshSchemaTokens->mesh, meshDataSource,
            HdPrimvarsSchemaTokens->primvars, primvarsDataSource),
        primDataSource
    };
    
    return HdOverlayContainerDataSource::New(TfArraySize(sources), sources);
}

} // namespace _SphereToMesh

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Implementation of the scene index

TF_DECLARE_REF_PTRS(_SceneIndex);

class _SceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _SceneIndexRefPtr New(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
    {
        return TfCreateRefPtr(new _SceneIndex(inputSceneIndex));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override
    {
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
        if (prim.primType == HdPrimTypeTokens->cube) {
            return {
                HdPrimTypeTokens->mesh,
                _CubeToMesh::_ComputePrimDataSource(prim.dataSource) };
        }
        if (prim.primType == HdPrimTypeTokens->cone) {
            return {
                HdPrimTypeTokens->mesh,
                _ConeToMesh::_ComputePrimDataSource(prim.dataSource) };
        }
        if (prim.primType == HdPrimTypeTokens->cylinder) {
            return {
                HdPrimTypeTokens->mesh,
                _CylinderToMesh::_ComputePrimDataSource(prim.dataSource) };
        }
        if (prim.primType == HdPrimTypeTokens->sphere) {
            return {
                HdPrimTypeTokens->mesh,
                _SphereToMesh::_ComputePrimDataSource(prim.dataSource) };
        }
        return prim;
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _SceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
      : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    {
    }

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override
    {
        if (!_IsObserved()) {
            return;
        }

        std::vector<size_t> indices;
        for (size_t i = 0; i < entries.size(); i++) {
            if (entries[i].primType == HdPrimTypeTokens->cube ||
                entries[i].primType == HdPrimTypeTokens->cone ||
                entries[i].primType == HdPrimTypeTokens->cylinder ||
                entries[i].primType == HdPrimTypeTokens->sphere) {
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

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override
    {
        if (!_IsObserved()) {
            return;
        }

        _SendPrimsRemoved(entries);
    }

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override
    {
        if (!_IsObserved()) {
            return;
        }

        std::vector<size_t> indices;
        for (size_t i = 0; i < entries.size(); i++) {
            const HdDataSourceLocatorSet &locators = entries[i].dirtyLocators;

            static const HdDataSourceLocatorSet implicitsLocators
                { HdCubeSchema::GetDefaultLocator(),
                  HdConeSchema::GetDefaultLocator(),
                  HdCylinderSchema::GetDefaultLocator(),
                  HdSphereSchema::GetDefaultLocator()};

            if (locators.Intersects(implicitsLocators)) {
                indices.push_back(i);
            }
        }
        
        if (indices.empty()) {
            _SendPrimsDirtied(entries);
            return;
        }

        HdSceneIndexObserver::DirtiedPrimEntries newEntries(entries);
        for (const size_t i : indices) {
            static const HdDataSourceLocator locator =
                HdPrimvarsSchema::GetPointsLocator()
                    .Append(HdPrimvarSchemaTokens->primvarValue);
            // Should we remove HdCubeSchema::GetDefaultLocator, ... from
            // dirtyLocators?
            newEntries[i].dirtyLocators.insert(locator);
        }


        _SendPrimsDirtied(newEntries);
    }
};

} // anonymous namespace

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Implementation of HdSt_ImplicitSurfaceSceneIndexPlugin

HdSt_ImplicitSurfaceSceneIndexPlugin::
HdSt_ImplicitSurfaceSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdSt_ImplicitSurfaceSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _SceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
