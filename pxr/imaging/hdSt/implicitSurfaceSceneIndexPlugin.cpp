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

#include "pxr/imaging/hd/cubeSchema.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"

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
            if (entries[i].primType == HdPrimTypeTokens->cube) {
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
            if (locators.Intersects(HdCubeSchema::GetDefaultLocator())) {
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
            // Should we remove HdCubeSchema::GetDefaultLocator from
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
