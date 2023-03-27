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

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/selectionSchema.h"
#include "pxr/imaging/hd/selectionsSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

HdDataSourceBaseHandle
_FullySelected()
{
    HdDataSourceBaseHandle selectionDataSources[] =
        { HdSelectionSchema::Builder()
            .SetFullySelected(
                HdRetainedTypedSampledDataSource<bool>::New(true))
            .Build() };
    return HdSelectionsSchema::BuildRetained(
        TfArraySize(selectionDataSources), selectionDataSources);
}

class _PrimSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimSource);

    _PrimSource(HdContainerDataSourceHandle const &inputSource,
                const UsdImagingSelectionSceneIndex * const sceneIndex,
                const SdfPath &primPath)
        : _inputSource(inputSource)
        , _sceneIndex(sceneIndex)
        , _primPath(primPath)
    {
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector names = _inputSource->GetNames();
        if (_sceneIndex->_selectedPaths.count(_primPath)) {
            names.push_back(HdSelectionsSchemaTokens->selections);
        }
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdSelectionsSchemaTokens->selections) {
            if (_sceneIndex->_selectedPaths.count(_primPath)) {
                static HdDataSourceBaseHandle const result =
                    _FullySelected();
                return result;
            } else {
                return nullptr;
            }
        }

        return _inputSource->Get(name);
    }


    HdContainerDataSourceHandle _inputSource;
    const UsdImagingSelectionSceneIndex *_sceneIndex;
    SdfPath _primPath;
};

}

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
        result.dataSource, this, primPath);

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
    const SdfPath &path)
{
    if (_selectedPaths.insert(path).second) {
        static const HdDataSourceLocatorSet locators{
            HdSelectionsSchema::GetDefaultLocator()};
        _SendPrimsDirtied({{path, locators}});
    }
}

void
UsdImagingSelectionSceneIndex::ClearSelection()
{
    if (_selectedPaths.empty()) {
        return;
    }

    HdSceneIndexObserver::DirtiedPrimEntries entries;
    entries.reserve(_selectedPaths.size());
    for (const SdfPath &path : _selectedPaths) {
        static const HdDataSourceLocatorSet locators{
            HdSelectionsSchema::GetDefaultLocator()};
        entries.emplace_back(path, locators);
    }

    _selectedPaths.clear();

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
    if (!_selectedPaths.empty()) {
        for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
            auto it = _selectedPaths.lower_bound(entry.primPath);
            while (it != _selectedPaths.end() && it->HasPrefix(entry.primPath)) {
                it = _selectedPaths.erase(it);
            }
        }
    }

    _SendPrimsRemoved(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
