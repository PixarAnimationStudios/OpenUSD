//
// Copyright 2023 Pixar
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

#include "pxr/imaging/hdsi/nurbsApproximatingSceneIndex.h"

#include "pxr/imaging/hd/basisCurvesSchema.h"
#include "pxr/imaging/hd/basisCurvesTopologySchema.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/dependencySchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/nurbsCurvesSchema.h"
#include "pxr/imaging/hd/nurbsPatchSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/pxOsd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace _NurbsCurvesToBasisCurves
{

// To convert a nurbsCurves prim to a basisCurves prim, we compute a data source
// for locator basisCurves using the data from the data source at locator
// nurbsCurves.

// The only data we actually use from nurbsCurves is the curveVertexCounts.

const HdDataSourceLocator curveVertexCountsSourceLocator =
    HdNurbsCurvesSchema::GetDefaultLocator().Append(
        HdNurbsCurvesSchemaTokens->curveVertexCounts);

// Data source for __dependencies.
//
// Only dependency is curveVertexCounts which we computed for basisCurves from
// nurbsCurves.
HdDataSourceBaseHandle
_ComputeDependenciesDataSource(const SdfPath &primPath)
{
    HdPathDataSourceHandle const dependedOnPrimPathDataSource =
        HdRetainedTypedSampledDataSource<SdfPath>::New(
            primPath);
    static HdLocatorDataSourceHandle const dependedOnLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            curveVertexCountsSourceLocator);
    static HdLocatorDataSourceHandle const affectedLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdBasisCurvesTopologySchema::GetDefaultLocator().Append(
                HdBasisCurvesTopologySchemaTokens->curveVertexCounts));

    return
        HdRetainedContainerDataSource::New(
            HdBasisCurvesTopologySchemaTokens->curveVertexCounts,
            HdDependencySchema::Builder()
                .SetDependedOnPrimPath(dependedOnPrimPathDataSource)
                .SetDependedOnDataSourceLocator(dependedOnLocatorDataSource)
                .SetAffectedDataSourceLocator(affectedLocatorDataSource)
                .Build());
}

// Data Source for basisCurves/topology.
class _BasisCurvesTopologyDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_BasisCurvesTopologyDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector names = {
            HdBasisCurvesTopologySchemaTokens->curveVertexCounts,
            HdBasisCurvesTopologySchemaTokens->basis,
            HdBasisCurvesTopologySchemaTokens->type,
            HdBasisCurvesTopologySchemaTokens->wrap
        };
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdBasisCurvesTopologySchemaTokens->curveVertexCounts) {
            return HdContainerDataSource::Get(
                _primDataSource, curveVertexCountsSourceLocator);
        }

        if (name == HdBasisCurvesTopologySchemaTokens->basis ||
            name == HdBasisCurvesTopologySchemaTokens->type) {
            static const HdDataSourceBaseHandle ds =
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdTokens->linear);
            return ds;
        }

        if (name == HdBasisCurvesTopologySchemaTokens->wrap) {
            static const HdDataSourceBaseHandle ds =
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdTokens->nonperiodic);
            return ds;
        }

        return nullptr;
    }

private:
    _BasisCurvesTopologyDataSource(
        const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    HdContainerDataSourceHandle const _primDataSource;
};

HdContainerDataSourceHandle
_ComputePrimDataSource(
    const SdfPath &primPath,
    const HdContainerDataSourceHandle &primDataSource)
{
    static HdDataSourceBaseHandle const nurbsCurvesDataSource =
        HdBlockDataSource::New();
    HdDataSourceBaseHandle const basisCurvesDataSource =
        HdRetainedContainerDataSource::New(
            HdBasisCurvesTopologySchemaTokens->topology,
            _BasisCurvesTopologyDataSource::New(primDataSource));
    HdDataSourceBaseHandle const dependenciesDataSource =
        _ComputeDependenciesDataSource(primPath);

    HdContainerDataSourceHandle sources[] = {
        HdRetainedContainerDataSource::New(
            HdNurbsCurvesSchemaTokens->nurbsCurves, nurbsCurvesDataSource,
            HdBasisCurvesSchemaTokens->basisCurves, basisCurvesDataSource,
            HdDependenciesSchemaTokens->__dependencies, dependenciesDataSource),
        primDataSource
    };

    return HdOverlayContainerDataSource::New(TfArraySize(sources), sources);
}

}

namespace _NurbsPatchToMesh
{

// To convert a nurbsPatch prim to a mesh prim, we create a data source for
// locator mesh using the data from the data source at locator nurbsPatch.

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (basisCurvesToMesh)
);

// Data source for __dependencies.
//
// We nuke all of mesh whenever anything underneath nurbsPatch gets dirtied.
//
HdDataSourceBaseHandle
_ComputeDependenciesDataSource(const SdfPath &primPath)
{
    HdPathDataSourceHandle const dependedOnPrimPathDataSource =
        HdRetainedTypedSampledDataSource<SdfPath>::New(
            primPath);
    static HdLocatorDataSourceHandle const dependedOnLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdNurbsPatchSchema::GetDefaultLocator());
    static HdLocatorDataSourceHandle const affectedLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdMeshSchema::GetDefaultLocator());

    return
        HdRetainedContainerDataSource::New(
            _tokens->basisCurvesToMesh,
            HdDependencySchema::Builder()
                .SetDependedOnPrimPath(dependedOnPrimPathDataSource)
                .SetDependedOnDataSourceLocator(dependedOnLocatorDataSource)
                .SetAffectedDataSourceLocator(affectedLocatorDataSource)
                .Build());
}

// Get the data sources for uVertexCount and vVertexCount from nurbsPath.
std::pair<HdIntDataSourceHandle, HdIntDataSourceHandle>
_GetUVVertexCountDataSources(const HdContainerDataSourceHandle &primDataSource)
{
    HdNurbsPatchSchema schema = HdNurbsPatchSchema::GetFromParent(primDataSource);
    return { schema.GetUVertexCount(), schema.GetVVertexCount() };
}

// Get the uVertexCount and vVertexCount from nurbsPath.
std::pair<int, int>
_GetUVVertexCounts(const HdContainerDataSourceHandle &primDataSource,
                   const HdSampledDataSource::Time shutterOffset)
{
    const std::pair<HdIntDataSourceHandle, HdIntDataSourceHandle> sources =
        _GetUVVertexCountDataSources(primDataSource);
    
    return {
        sources.first  ? sources.first ->GetTypedValue(shutterOffset) : 0,
        sources.second ? sources.second->GetTypedValue(shutterOffset) : 0 };
}

// Implements HdSampledDataSource::GetContributingSampleTimesForInterval for
// data sources that depend on the uVertexCount and vVertexCount of nurbsPath.
bool
_GetMergedContributingSampleTimesForIntervalForTopology(
    const HdContainerDataSourceHandle &primDataSource,
    const HdSampledDataSource::Time startTime,
    const HdSampledDataSource::Time endTime,
    std::vector<HdSampledDataSource::Time> * const outSampleTimes)
{
    const std::pair<HdIntDataSourceHandle, HdIntDataSourceHandle> sources =
        _GetUVVertexCountDataSources(primDataSource);
    HdSampledDataSourceHandle srcs[] = { sources.first, sources.second };
    return HdGetMergedContributingSampleTimesForInterval(
        TfArraySize(srcs), srcs, startTime, endTime, outSampleTimes);
}

// Data source for locator mesh/topology/faceVertexCounts
class _FaceVertexCountsDataSource : public HdIntArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_FaceVertexCountsDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtIntArray GetTypedValue(const Time shutterOffset) override {
        int uVertexCount, vVertexCount;
        std::tie(uVertexCount, vVertexCount) =
            _GetUVVertexCounts(_primDataSource, shutterOffset);

        const size_t numFaces =
            std::max(0, uVertexCount - 1) * std::max(0, vVertexCount - 1);

        return VtIntArray(numFaces, 4);
    }

    bool GetContributingSampleTimesForInterval(
                            const Time startTime,
                            const Time endTime,
                            std::vector<Time> * const outSampleTimes) override {
        return
            _GetMergedContributingSampleTimesForIntervalForTopology(
                _primDataSource, startTime, endTime, outSampleTimes);
    }

private:
    _FaceVertexCountsDataSource(
        const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    HdContainerDataSourceHandle _primDataSource;
};

// Data source for locator mesh/topology/faceVertexIndices
class _FaceVertexIndicesDataSource : public HdIntArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_FaceVertexIndicesDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtIntArray GetTypedValue(const Time shutterOffset) override {
        int uVertexCount, vVertexCount;
        std::tie(uVertexCount, vVertexCount) =
            _GetUVVertexCounts(_primDataSource, shutterOffset);

        const size_t numFaces =
            std::max(0, uVertexCount - 1) * std::max(0, vVertexCount - 1);
        const size_t numIndices =
            4 * numFaces;

        VtIntArray result(numIndices);

        if (numIndices == 0) {
            return result;
        }

        for (int row = 0; row < vVertexCount - 1; ++row) {
            for (int col = 0; col < uVertexCount - 1; ++col) {
                const size_t faceIdx = row * (uVertexCount - 1) + col;
                const int vertexIdx = row * uVertexCount + col;
                result[4 * faceIdx   ] = vertexIdx;
                result[4 * faceIdx + 1] = vertexIdx + 1;
                result[4 * faceIdx + 2] = vertexIdx + uVertexCount + 1;
                result[4 * faceIdx + 3] = vertexIdx + uVertexCount;
            }
        }

       return result;
    }

    bool GetContributingSampleTimesForInterval(
                            const Time startTime,
                            const Time endTime,
                            std::vector<Time> * const outSampleTimes) override {
        return
            _GetMergedContributingSampleTimesForIntervalForTopology(
                _primDataSource, startTime, endTime, outSampleTimes);
    }

private:
    _FaceVertexIndicesDataSource(
        const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    HdContainerDataSourceHandle _primDataSource;
};

// Data source for locator mesh/topology
class _MeshTopologyDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MeshTopologyDataSource);

    TfTokenVector GetNames() override {
        static TfTokenVector names = {
            HdMeshTopologySchemaTokens->faceVertexCounts,
            HdMeshTopologySchemaTokens->faceVertexIndices,
            HdMeshTopologySchemaTokens->orientation };
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdMeshTopologySchemaTokens->faceVertexCounts) {
            return _FaceVertexCountsDataSource::New(_primDataSource);
        }
        if (name == HdMeshTopologySchemaTokens->faceVertexIndices) {
            return _FaceVertexIndicesDataSource::New(_primDataSource);
        }
        if (name == HdMeshTopologySchemaTokens->orientation) {
            static const HdDataSourceLocator locator =
                HdNurbsPatchSchema::GetDefaultLocator()
                    .Append(HdNurbsPatchSchemaTokens->orientation);
            return HdContainerDataSource::Get(_primDataSource, locator);
        }

        return nullptr;
    }

private:
    _MeshTopologyDataSource(
        const HdContainerDataSourceHandle &primDataSource)
      : _primDataSource(primDataSource)
    {
    }

    HdContainerDataSourceHandle const _primDataSource;
};

// Data source for locator mesh
HdDataSourceBaseHandle
_ComputeMeshDataSource(HdContainerDataSourceHandle const &primDataSource)
{
    HdContainerDataSourceHandle const topologyDs =
        _MeshTopologyDataSource::New(primDataSource);

    static HdTokenDataSourceHandle const subdivisionSchemeDs =
        HdRetainedTypedSampledDataSource<TfToken>::New(
            PxOsdOpenSubdivTokens->catmullClark);

    static const HdDataSourceLocator doubleSidedLocator =
        HdNurbsPatchSchema::GetDefaultLocator()
            .Append(HdNurbsPatchSchemaTokens->doubleSided);
    HdBoolDataSourceHandle const doubleSidedDs =
        HdBoolDataSource::Cast(
            HdContainerDataSource::Get(
                primDataSource, doubleSidedLocator));

    return
        HdMeshSchema::Builder()
            .SetTopology(topologyDs)
            .SetSubdivisionScheme(subdivisionSchemeDs)
            .SetDoubleSided(doubleSidedDs)
            .Build();
}

HdContainerDataSourceHandle
_ComputePrimDataSource(
    const SdfPath &primPath,
    const HdContainerDataSourceHandle &primDataSource)
{
    static HdDataSourceBaseHandle const nurbsPatchDataSource =
        HdBlockDataSource::New();
    HdDataSourceBaseHandle const meshDataSource =
        _ComputeMeshDataSource(primDataSource);
    HdDataSourceBaseHandle const dependenciesDataSource =
        _ComputeDependenciesDataSource(primPath);

    HdContainerDataSourceHandle sources[] = {
        HdRetainedContainerDataSource::New(
            HdNurbsPatchSchemaTokens->nurbsPatch, nurbsPatchDataSource,
            HdMeshSchemaTokens->mesh, meshDataSource,
            HdDependenciesSchemaTokens->__dependencies, dependenciesDataSource),
        primDataSource
    };

    return HdOverlayContainerDataSource::New(TfArraySize(sources), sources);
}

}

HdsiNurbsApproximatingSceneIndexRefPtr
HdsiNurbsApproximatingSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
{
    return TfCreateRefPtr(
        new HdsiNurbsApproximatingSceneIndex(
            inputSceneIndex));
}

HdsiNurbsApproximatingSceneIndex::HdsiNurbsApproximatingSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

HdSceneIndexPrim
HdsiNurbsApproximatingSceneIndex::GetPrim(const SdfPath &primPath) const
{
    const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.primType == HdPrimTypeTokens->nurbsCurves) {
        return {
            HdPrimTypeTokens->basisCurves,
            _NurbsCurvesToBasisCurves::_ComputePrimDataSource(
                primPath, prim.dataSource) };
    }
    if (prim.primType == HdPrimTypeTokens->nurbsPatch) {
        return {
            HdPrimTypeTokens->mesh,
            _NurbsPatchToMesh::_ComputePrimDataSource(
                primPath, prim.dataSource) };
    }

    return prim;
}

SdfPathVector
HdsiNurbsApproximatingSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiNurbsApproximatingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    // Replace nurbsCurves by basisCurves
    //         nurbsPatch  by mesh

    std::vector<size_t> indices;
    for (size_t i = 0; i < entries.size(); i++) {
        const TfToken &primType = entries[i].primType;
        if ( primType == HdPrimTypeTokens->nurbsCurves ||
             primType == HdPrimTypeTokens->nurbsPatch) {
            indices.push_back(i);
        }
    }

    if (indices.empty()) {
        _SendPrimsAdded(entries);
        return;
    }

    HdSceneIndexObserver::AddedPrimEntries newEntries(entries);
    for (const size_t i : indices) {
        TfToken &primType = newEntries[i].primType;
        if (primType == HdPrimTypeTokens->nurbsCurves) {
            primType = HdPrimTypeTokens->basisCurves;
        } else {
            primType = HdPrimTypeTokens->mesh;
        }
    }

    _SendPrimsAdded(newEntries);
}

void
HdsiNurbsApproximatingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    _SendPrimsRemoved(entries);
}

void
HdsiNurbsApproximatingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    _SendPrimsDirtied(entries);
}


PXR_NAMESPACE_CLOSE_SCOPE
