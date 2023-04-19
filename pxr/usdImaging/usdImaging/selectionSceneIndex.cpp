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
#include "pxr/usdImaging/usdImaging/selectionSceneIndex.h"

#include "pxr/imaging/hd/instanceSchema.h"
#include "pxr/imaging/hd/instanceIndicesSchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/selectionSchema.h"
#include "pxr/imaging/hd/selectionsSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdImagingSelectionSceneIndex_Impl
{

struct _PrimSelectionState
{
    // Container data sources conforming to HdSelectionSchema
    std::vector<HdDataSourceBaseHandle> selectionSources;

    HdDataSourceBaseHandle GetVectorDataSource() {
        return HdSelectionsSchema::BuildRetained(
            selectionSources.size(),
            selectionSources.data());
    }
};

struct _Selection
{
    // Maps prim path to data sources to be returned by the vector data
    // source at locator selections.
    std::map<SdfPath, _PrimSelectionState> pathToState;
};

class _PrimSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimSource);

    _PrimSource(HdContainerDataSourceHandle const &inputSource,
                _SelectionSharedPtr const selection,
                const SdfPath &primPath)
        : _inputSource(inputSource)
        , _selection(selection)
        , _primPath(primPath)
    {
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector names = _inputSource->GetNames();
        auto it = _selection->pathToState.find(_primPath);
        if (it != _selection->pathToState.end()) {
            names.push_back(HdSelectionsSchemaTokens->selections);
        }
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdSelectionsSchemaTokens->selections) {
            auto it = _selection->pathToState.find(_primPath);
            if (it != _selection->pathToState.end()) {
                return it->second.GetVectorDataSource();
            } else {
                return nullptr;
            }
        }

        return _inputSource->Get(name);
    }

private:
    HdContainerDataSourceHandle const _inputSource;
    _SelectionSharedPtr const _selection;
    const SdfPath _primPath;
};

SdfPath
_GetPath(HdPathArrayDataSourceHandle const dataSource,
         const int index)
{
    if (index < 0) {
        return SdfPath();
    }

    if (!dataSource) {
        return SdfPath();
    }

    const VtArray<SdfPath> paths = dataSource->GetTypedValue(0.0f);

    if (!(static_cast<size_t>(index) < paths.size())) {
        return SdfPath();
    }

    return paths[index];
}

// Compute prototype path and a container data source conforming to
// HdInstanceIndicesSchema given the instance schema of an instance prim
// and the instancer topology schema of the corresponding instancer
// prim.
std::pair<SdfPath, HdDataSourceBaseHandle>
_ComputePrototypePathAndInstanceIndices(
    HdInstanceSchema &instance,
    HdInstancerTopologySchema &instancerTopology)
{
    SdfPath prototypePath;
    HdInstanceIndicesSchema::Builder instanceIndicesBuilder;

    // Set instancer path.
    instanceIndicesBuilder.SetInstancer(instance.GetInstancer());

    if (HdIntDataSourceHandle const prototypeIndexDs =
                instance.GetPrototypeIndex()) {
        // Set prototype id, the index into the prototypes of the instancer.
        instanceIndicesBuilder.SetPrototypeIndex(prototypeIndexDs);

        // Use the index to get the prototype path from the instancer.
        prototypePath = _GetPath(
            instancerTopology.GetPrototypes(),
            prototypeIndexDs->GetTypedValue(0.0f));
    }

    if (HdIntDataSourceHandle const instanceIndexDs =
                instance.GetInstanceIndex()) {
        // Note that an instance has a unique instance index, but
        // HdInstanceIndicesSchema can have a list of indices.
        // So we need to wrap it.
        instanceIndicesBuilder.SetInstanceIndices(
            HdRetainedTypedSampledDataSource<VtIntArray>::New(
                { instanceIndexDs->GetTypedValue(0.0f) }));
    }        

    return { prototypePath, instanceIndicesBuilder.Build() };
}

// Check whether the prim at the given path is an instance.
// If yes, return the prototype path and a container data source
// conforming to HdInstanceIndicesSchema.
std::pair<SdfPath, HdDataSourceBaseHandle>
_ComputePrototypePathAndInstanceIndices(
    const SdfPath &primPath, HdSceneIndexBaseRefPtr const &sceneIndex)
{
    // Extract instance information.
    HdInstanceSchema instanceSchema =
        HdInstanceSchema::GetFromParent(
            sceneIndex->GetPrim(primPath).dataSource);

    HdPathDataSourceHandle const instancerPathDs = instanceSchema.GetInstancer();
    if (!instancerPathDs) {
        return { SdfPath(), nullptr };
    }

    // Extract information of instancer realizing this instance.
    const SdfPath instancerPath = instancerPathDs->GetTypedValue(0.0f);
    HdInstancerTopologySchema instancerTopologySchema =
        HdInstancerTopologySchema::GetFromParent(
            sceneIndex->GetPrim(instancerPath).dataSource);
    
    return _ComputePrototypePathAndInstanceIndices(
        instanceSchema, instancerTopologySchema);
}

// Given a usd proxy path, computes the path in the scene index and
// the necessary instancing information.
std::pair<SdfPath, std::vector<HdDataSourceBaseHandle>>
_ComputeSceneIndexPathAndNestedInstanceIndices(
    const SdfPath &usdPath, HdSceneIndexBaseRefPtr const &sceneIndex)
{
    SdfPath sceneIndexPath = SdfPath::AbsoluteRootPath();
    std::vector<HdDataSourceBaseHandle> nestedInstanceIndices;

    // Iterate through elements of path and build up path in scene index,
    // replacing the path if we hit a native instance.
    for (const SdfPath &usdPrefix : usdPath.GetPrefixes()) {
        const TfToken primName = usdPrefix.GetNameToken();
        sceneIndexPath = sceneIndexPath.AppendChild(primName);

        SdfPath prototypePath;
        HdDataSourceBaseHandle instanceIndices;
        std::tie(prototypePath, instanceIndices) =
            _ComputePrototypePathAndInstanceIndices(
                sceneIndexPath, sceneIndex);
        
        if (!prototypePath.IsEmpty()) {
            // If we hit an instance, we need to replace the path to
            // the prototype (in the scene index) that this instance
            // is instancing. More precisely, the prototype that
            // was added by the prototype propagating scene index after
            // instancing aggregation.
            sceneIndexPath = std::move(prototypePath);
        }
        if (instanceIndices) {
            // If we hit an instance, record the instancing info such
            // as what instancer was added by instance aggregation to
            // realize this instance and what the instance index within
            // that instancer is.
            nestedInstanceIndices.push_back(std::move(instanceIndices));
        }
    }

    return { sceneIndexPath, nestedInstanceIndices };
}

}

using namespace UsdImagingSelectionSceneIndex_Impl;

UsdImagingSelectionSceneIndexRefPtr
UsdImagingSelectionSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
{
    return TfCreateRefPtr(
        new UsdImagingSelectionSceneIndex(
            inputSceneIndex));
}

UsdImagingSelectionSceneIndex::
UsdImagingSelectionSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _selection(std::make_shared<_Selection>())
{
}

HdSceneIndexPrim
UsdImagingSelectionSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim result = _GetInputSceneIndex()->GetPrim(primPath);
    if (!result.dataSource) {
        return result;
    }
    
    result.dataSource = _PrimSource::New(
        result.dataSource, _selection, primPath);

    return result;
}

SdfPathVector
UsdImagingSelectionSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
UsdImagingSelectionSceneIndex::AddSelection(
    const SdfPath &usdPath)
{
    // Translate Usd (proxy) path to scene index path and
    // information identifying the instance.
    SdfPath sceneIndexPath;
    std::vector<HdDataSourceBaseHandle> nestedInstanceIndices;
    std::tie(sceneIndexPath, nestedInstanceIndices) =
        _ComputeSceneIndexPathAndNestedInstanceIndices(
            usdPath, _GetInputSceneIndex());

    HdSelectionSchema::Builder selectionBuilder;
    selectionBuilder.SetFullySelected(
        HdRetainedTypedSampledDataSource<bool>::New(true));
    if (!nestedInstanceIndices.empty()) {
        selectionBuilder.SetNestedInstanceIndices(
            HdRetainedSmallVectorDataSource::New(
                nestedInstanceIndices.size(), nestedInstanceIndices.data()));
    }

    _selection->pathToState[sceneIndexPath].selectionSources.push_back(
        selectionBuilder.Build());

    static const HdDataSourceLocatorSet locators{
        HdSelectionsSchema::GetDefaultLocator()};
    _SendPrimsDirtied({{sceneIndexPath, locators}});
}

void
UsdImagingSelectionSceneIndex::ClearSelection()
{
    if (_selection->pathToState.empty()) {
        return;
    }

    HdSceneIndexObserver::DirtiedPrimEntries entries;
    entries.reserve(_selection->pathToState.size());
    for (const auto &pathAndSelections : _selection->pathToState) {
        static const HdDataSourceLocatorSet locators{
            HdSelectionsSchema::GetDefaultLocator()};
        entries.emplace_back(pathAndSelections.first, locators);
    }

    _selection->pathToState.clear();

    _SendPrimsDirtied(entries);
}

void
UsdImagingSelectionSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
UsdImagingSelectionSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

void
UsdImagingSelectionSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    if (!_selection->pathToState.empty()) {
        for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
            auto it = _selection->pathToState.lower_bound(entry.primPath);
            while (it != _selection->pathToState.end() &&
                   it->first.HasPrefix(entry.primPath)) {
                it = _selection->pathToState.erase(it);
            }
        }
    }

    _SendPrimsRemoved(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
