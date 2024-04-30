//
// Copyright 2024 Pixar
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

#include "pxr/imaging/hdsi/tetMeshConversionSceneIndex.h"

#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/tetMeshSchema.h"
#include "pxr/imaging/hd/tetMeshTopologySchema.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/pxOsd/tokens.h"


PXR_NAMESPACE_OPEN_SCOPE


// Compute the MeshTopology Data Source translating the TetVertexIndices into 
// FaceVertexIndices and FaceVertexCounts
HdContainerDataSourceHandle
_ComputeMeshTopologyDataSource(
    HdContainerDataSourceHandle const &primDataSource)
{
    HdTetMeshTopologySchema meshTopoSchema = 
        HdTetMeshSchema::GetFromParent(primDataSource).GetTopology();
    // Get the TetVertexIndices
    const HdVec3iArrayDataSourceHandle surfaceFaceIndicesDS =
        meshTopoSchema.GetSurfaceFaceVertexIndices();

    if (!surfaceFaceIndicesDS) {
        return HdMeshTopologySchema::Builder()
            .SetOrientation(meshTopoSchema.GetOrientation())
            .Build();
    }

    // Compute the FaceVertexIndices and FaceVertexIndicesCounts
    const VtVec3iArray surfaceFaceIndices = 
            surfaceFaceIndicesDS->GetTypedValue(0.0f);

    const size_t n = surfaceFaceIndices.size();
    VtIntArray faceVertexCounts(n, 3);

    VtIntArray faceVertexIndices;
    faceVertexIndices.reserve(n*3);
    for (const GfVec3i &faceIndices : surfaceFaceIndices) {
        faceVertexIndices.push_back(faceIndices[0]);
        faceVertexIndices.push_back(faceIndices[1]);
        faceVertexIndices.push_back(faceIndices[2]);
    }

    return
        HdMeshTopologySchema::Builder()
            .SetFaceVertexCounts(
                HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    faceVertexCounts))
            .SetFaceVertexIndices(
                HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    faceVertexIndices))
            .SetOrientation(meshTopoSchema.GetOrientation())
            .Build();
}

// Data source for locator mesh
HdDataSourceBaseHandle
_ComputeMeshDataSource(HdContainerDataSourceHandle const &primDataSource)
{
    HdTetMeshSchema tetMeshSchema = HdTetMeshSchema::GetFromParent(primDataSource);
    
    return
        HdMeshSchema::Builder()
            .SetTopology(_ComputeMeshTopologyDataSource(primDataSource))
            .SetSubdivisionScheme(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    PxOsdOpenSubdivTokens->catmullClark))
            .SetDoubleSided(tetMeshSchema.GetDoubleSided())
            .Build();
}

HdContainerDataSourceHandle
_ComputePrimDataSource(
    const SdfPath &primPath,
    const HdContainerDataSourceHandle &primDataSource)
{
    return HdOverlayContainerDataSource::New(
        HdRetainedContainerDataSource::New(
            HdMeshSchemaTokens->mesh,
            _ComputeMeshDataSource(primDataSource)),
        primDataSource);
}

HdsiTetMeshConversionSceneIndexRefPtr
HdsiTetMeshConversionSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
{
    return TfCreateRefPtr(
        new HdsiTetMeshConversionSceneIndex(inputSceneIndex));
}

HdsiTetMeshConversionSceneIndex::HdsiTetMeshConversionSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

HdSceneIndexPrim
HdsiTetMeshConversionSceneIndex::GetPrim(const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.primType == HdPrimTypeTokens->tetMesh) {
        return {
            HdPrimTypeTokens->mesh,
            _ComputePrimDataSource(primPath, prim.dataSource) };
    }
    return prim;
}

SdfPathVector
HdsiTetMeshConversionSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}


void
HdsiTetMeshConversionSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    // Gather prims added indices for Tet Mesh prims
    std::vector<size_t> tetMeshIndices;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].primType == HdPrimTypeTokens->tetMesh) {
            tetMeshIndices.push_back(i);
        }
    }
    
    if (tetMeshIndices.empty()) {
        _SendPrimsAdded(entries);
        return;
    }

    // Change the prim types for Tet Meshes to Mesh
    HdSceneIndexObserver::AddedPrimEntries entriesToAdd(entries);
    for (const size_t i : tetMeshIndices) {
        entriesToAdd[i].primType = HdPrimTypeTokens->mesh;
    }

    _SendPrimsAdded(entriesToAdd);
}

void
HdsiTetMeshConversionSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    _SendPrimsRemoved(entries);
}

void
HdsiTetMeshConversionSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    // Gather indices for Tet Mesh locators that need to be updated.
    std::vector<size_t> tetMeshIndices;
    for (size_t i = 0; i < entries.size(); ++i) {
        const HdDataSourceLocatorSet& dirtyLocators = entries[i].dirtyLocators;
        if (dirtyLocators.Contains(HdDataSourceLocator::EmptyLocator())) {
            // If the locator set contains the empty locator, it means it will
            // intersect everything.  It does not need to be updated.
            continue;
        }
        if (dirtyLocators.Intersects(HdTetMeshSchema::GetDefaultLocator())) {
            tetMeshIndices.push_back(i);
        }
    }
    
    if (tetMeshIndices.empty()) {
        _SendPrimsDirtied(entries);
        return;
    }

    // Convert TetMesh Locators to Mesh Locators
    HdSceneIndexObserver::DirtiedPrimEntries newEntries(entries);
    for (const size_t i : tetMeshIndices) {
        HdDataSourceLocatorSet& dirtyLocators = newEntries[i].dirtyLocators;
        // The code that populated tetMeshIndices skips anything that
        // contains the empty locator.
        TF_VERIFY(!dirtyLocators.Contains(HdDataSourceLocator::EmptyLocator()));

        if (dirtyLocators.Intersects(HdTetMeshSchema::GetDoubleSidedLocator())) {
            dirtyLocators.append(HdMeshSchema::GetDoubleSidedLocator());
        }
        if (dirtyLocators.Intersects(HdTetMeshSchema::GetTopologyLocator())) {
            dirtyLocators.append(HdMeshSchema::GetTopologyLocator());
        }
    }

    _SendPrimsDirtied(newEntries);
}


PXR_NAMESPACE_CLOSE_SCOPE
