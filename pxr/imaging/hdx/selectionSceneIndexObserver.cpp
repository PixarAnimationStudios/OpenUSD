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
#include "pxr/imaging/hdx/selectionSceneIndexObserver.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/selectionSchema.h"
#include "pxr/imaging/hd/selectionsSchema.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

HdxSelectionSceneIndexObserver::HdxSelectionSceneIndexObserver()
 : _version(0)
 , _selection(std::make_shared<HdSelection>())
{
}

// TODO: Move UsdImaging_SceneIndexPrimView to hd and use it here.
static
void
_PopulateFromSceneIndex(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &path,
    SdfPathSet * const paths)
{
    paths->insert(path);
    for (const SdfPath &child : sceneIndex->GetChildPrimPaths(path)) {
        _PopulateFromSceneIndex(sceneIndex, child, paths);
    }
}

void
HdxSelectionSceneIndexObserver::SetSceneIndex(
    HdSceneIndexBaseRefPtr const &sceneIndex)
{
    if (sceneIndex == _sceneIndex) {
        return;
    }

    HdSceneIndexObserverPtr self(this);

    if (_sceneIndex) {
        _sceneIndex->RemoveObserver(self);
    }

    if (sceneIndex) {
        sceneIndex->AddObserver(self);
        _PopulateFromSceneIndex(
            sceneIndex, SdfPath::AbsoluteRootPath(), &_dirtiedPrims);
    }
    
    _sceneIndex = sceneIndex;

    _version++;
}

int
HdxSelectionSceneIndexObserver::GetVersion() const
{
    return _version;
}

HdSelectionSharedPtr
HdxSelectionSceneIndexObserver::GetSelection()
{
    if (!_dirtiedPrims.empty()) {
        _selection = _ComputeSelection();
    }

    return _selection;
}

HdSelectionSharedPtr
HdxSelectionSceneIndexObserver::_ComputeSelection()
{
    TRACE_FUNCTION();

    HdSelectionSharedPtr result = std::make_shared<HdSelection>();

    if (!_sceneIndex) {
        return result;
    }

    const SdfPathVector prims = _selection->GetAllSelectedPrimPaths();

    _dirtiedPrims.insert(prims.begin(), prims.end());

    for (const SdfPath &path : _dirtiedPrims) {
        const HdSceneIndexPrim prim = _sceneIndex->GetPrim(path);
        if (!prim.dataSource) {
            continue;
        }
        HdSelectionsSchema selectionsSchema =
            HdSelectionsSchema::GetFromParent(prim.dataSource);
        const size_t n = selectionsSchema.GetNumElements();
        for (size_t i = 0; i < n; ++i) {
            HdSelectionSchema selectionSchema =
                selectionsSchema.GetElement(i);
            if (HdBoolDataSourceHandle const ds =
                            selectionSchema.GetFullySelected()) {
                if (ds->GetTypedValue(0.0f)) {
                    result->AddRprim(HdSelection::HighlightModeSelect, path);
                    break;
                }
            }
        }
    }

    _dirtiedPrims.clear();
                     
    return result;
}

void
HdxSelectionSceneIndexObserver::PrimsAdded(
    const HdSceneIndexBase &sender,
    const AddedPrimEntries &entries)
{
    if (entries.empty()) {
        return;
    }

    ++_version;

    for (const AddedPrimEntry &entry : entries) {
        _dirtiedPrims.insert(entry.primPath);
    }
}

void
HdxSelectionSceneIndexObserver::PrimsDirtied(
    const HdSceneIndexBase &sender,
    const DirtiedPrimEntries &entries)
{
    for (const DirtiedPrimEntry &entry : entries) {
        if (entry.dirtyLocators.Contains(
                HdSelectionsSchema::GetDefaultLocator())) {
            ++_version;
            _dirtiedPrims.insert(entry.primPath);
        }
    }
}

void
HdxSelectionSceneIndexObserver::PrimsRemoved(
    const HdSceneIndexBase &sender,
    const RemovedPrimEntries &entries)
{
    if (entries.empty()) {
        return;
    }

    ++_version;
}

PXR_NAMESPACE_CLOSE_SCOPE
