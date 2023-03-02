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

#include "pxr/usdImaging/usdImaging/drawModeSceneIndex.h"
#include "pxr/usdImaging/usdImaging/drawModeStandin.h"

#include "pxr/usdImaging/usdImaging/modelSchema.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDrawModeSceneIndexRefPtr
UsdImagingDrawModeSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
{
    return TfCreateRefPtr(
        new UsdImagingDrawModeSceneIndex(
            inputSceneIndex, inputArgs));
}

UsdImagingDrawModeSceneIndex::UsdImagingDrawModeSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

UsdImagingDrawModeSceneIndex::~UsdImagingDrawModeSceneIndex() = default;

HdSceneIndexPrim
UsdImagingDrawModeSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    if (primPath.GetPathElementCount() > 1) {
        // Example: primPath = /A/B and /A has non-default drawmode.
        // We get the DrawModeStandin at /A and query for its child at
        // B.
        const auto it = _prims.find(primPath.GetParentPath());
        if (it != _prims.end()) {
            return it->second->GetChildPrim(primPath.GetNameToken());
        }
    }

    {
        // Example: primPath = /A and /A has non-default drawmode.
        // We ask the DrawModeStandin for the prim replacing /A.
        const auto it = _prims.find(primPath);
        if (it != _prims.end()) {
            return it->second->GetPrim();
        }
    }

    return _GetInputSceneIndex()->GetPrim(primPath);
}

SdfPathVector
UsdImagingDrawModeSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    {
        // Example: primPath = /A and /A has non-default drawmode.
        // We ask DrawModeStandin at /A for the children.
        const auto it = _prims.find(primPath);
        if (it != _prims.end()) {
            return it->second->GetChildPrimPaths();
        }
    }

    if (primPath.GetPathElementCount() > 1) {
        // Example: primPath /A/B and /A has non-default drawmode.
        // DrawModeStandin provides no grand-children, so we just return
        // empty.
        const auto it = _prims.find(primPath.GetParentPath());
        if (it != _prims.end()) {
            return { };
        }
    }

    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
UsdImagingDrawModeSceneIndex::_DeleteSubtree(const SdfPath &path)
{
    auto it = _prims.lower_bound(path);
    while (it != _prims.end() && it->first.HasPrefix(path)) {
        it = _prims.erase(it);
    }
}

// Resolve draw mode for prim from input scene index.
// Default draw mode can be expressed by either the empty token
// or UsdGeomTokens->default_.
static
TfToken
_GetDrawMode(const HdSceneIndexPrim &prim)
{
    static const TfToken empty;

    UsdImagingModelSchema modelSchema =
        UsdImagingModelSchema::GetFromParent(prim.dataSource);

    HdBoolDataSourceHandle const applySrc = modelSchema.GetApplyDrawMode();
    if (!applySrc) {
        return empty;
    }
    if (!applySrc->GetTypedValue(0.0f)) {
        return empty;
    }
    
    HdTokenDataSourceHandle const modeSrc = modelSchema.GetDrawMode();
    if (!modeSrc) {
        return empty;
    }
    return modeSrc->GetTypedValue(0.0f);
}

// If we got a { HdDataSourceLocator() } as dirty locator set,
// we need to pull the prim data source from the input scene index again
// and reconstruct the DrawModeStandin.
static
void
_RefreshDrawModeStandin(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const SdfPath &path,
    UsdImaging_DrawModeStandinSharedPtr *standin,
    HdSceneIndexObserver::RemovedPrimEntries *removedEntries,
    HdSceneIndexObserver::AddedPrimEntries *addedEntries)
{
    UsdImaging_DrawModeStandinSharedPtr newStandin =
        UsdImaging_GetDrawModeStandin(
            (*standin)->GetDrawMode(),
            path,  
            inputSceneIndex->GetPrim(path).dataSource);
    if (!TF_VERIFY(newStandin)) {
        return;
    }

    (*removedEntries).push_back({ path });
    newStandin->ComputePrimAddedEntries(addedEntries);

    static const HdDataSourceLocatorSet full {
        HdDataSourceLocator::EmptyLocator() };

    *standin = std::move(newStandin);
}

bool
UsdImagingDrawModeSceneIndex::_HasDrawModeAncestor(
    const SdfPath &path)
{
    if (_prims.empty()) {
        return false;
    }

    // We want strict ancestor and GetAncestorRange include the path
    // itself - so skip it.
    bool first = true;
    for (const SdfPath &ancestorPath : path.GetAncestorsRange()) {
        if (first) {
            first = false;
        } else {
            auto it = _prims.find(ancestorPath);
            if (it != _prims.end()) {
                return true;
            }
        }
    }
    return false;
}

// Called from _PrimsDirtied on main-thread so we have enough stack space
// to just recurse.
void
UsdImagingDrawModeSceneIndex::_RecursePrims(
    const TfToken &mode,
    const SdfPath &path,
    const HdSceneIndexPrim &prim,
    HdSceneIndexObserver::AddedPrimEntries *entries)
{
    if (UsdImaging_DrawModeStandinSharedPtr standin =
            UsdImaging_GetDrawModeStandin(mode, path, prim.dataSource)) {
        // The prim needs to be replaced by stand-in geometry.
        // Send added entries for stand-in geometry.
        standin->ComputePrimAddedEntries(entries);
        // And store it.
        _prims[path] = std::move(standin);
    } else {
        // Mark prim as added and recurse to children.
        entries->push_back({path, prim.primType});
        const HdSceneIndexBaseRefPtr &s = _GetInputSceneIndex();
        for (const SdfPath &childPath : s->GetChildPrimPaths(path)) {
            const HdSceneIndexPrim prim = s->GetPrim(childPath);
            _RecursePrims(_GetDrawMode(prim), childPath, prim, entries);
        }
    }
}

void
UsdImagingDrawModeSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    HdSceneIndexObserver::AddedPrimEntries newEntries;

    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        const SdfPath &path = entry.primPath;

        // Suppress prims from input scene delegate that have an ancestor
        // with a draw mode.
        if (_HasDrawModeAncestor(path)) {
            continue;
        }
               
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
        const TfToken drawMode = _GetDrawMode(prim);

        if (UsdImaging_DrawModeStandinSharedPtr standin =
                UsdImaging_GetDrawModeStandin(
                    drawMode, path, prim.dataSource)) {
            // The prim needs to be replaced by stand-in geometry.
            standin->ComputePrimAddedEntries(&newEntries);
            _prims[path] = std::move(standin);
        } else {
            // Just forward added entry.
            newEntries.push_back(entry);
        }
    }
     
    _SendPrimsAdded(newEntries);
}

void
UsdImagingDrawModeSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    if (!_prims.empty()) {
        for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
            // Delete corresponding stand-in geometry.
            _DeleteSubtree(entry.primPath);
        }
    }

    if (!_IsObserved()) {
        return;
    }

    _SendPrimsRemoved(entries);
}

void
UsdImagingDrawModeSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();
    
    // Determine the paths of all prims whose draw mode might have changed.
    std::set<SdfPath> paths;

    static const HdDataSourceLocatorSet drawModeLocators{
        UsdImagingModelSchema::GetDefaultLocator().Append(
            UsdImagingModelSchemaTokens->drawMode),
        UsdImagingModelSchema::GetDefaultLocator().Append(
            UsdImagingModelSchemaTokens->applyDrawMode)};
            
    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        if (drawModeLocators.Intersects(entry.dirtyLocators)) {
            paths.insert(entry.primPath);
        }
    }

    HdSceneIndexObserver::RemovedPrimEntries removedEntries;
    HdSceneIndexObserver::AddedPrimEntries addedEntries;

    if (!paths.empty()) {
        // Draw mode changed means we need to remove the stand-in geometry
        // or prims forwarded from the input scene delegate and then (re-)add
        // the stand-in geometry or prims from the input scene delegate.
        
        // Set this to skip all descendants of a given path.
        SdfPath lastPath;
        for (const SdfPath &path : paths) {
            // Skip all descendants of lastPath - if lastPath is not empty.
            if (path.HasPrefix(lastPath)) {
                continue;
            }
            lastPath = SdfPath();
            
            // Determine new draw mode.
            const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
            const TfToken drawMode = _GetDrawMode(prim);

            const auto it = _prims.find(path);
            if (it == _prims.end()) {
                // Prim used to have default draw mode.
                if (UsdImaging_DrawModeStandinSharedPtr standin =
                        UsdImaging_GetDrawModeStandin(
                            drawMode, path, prim.dataSource)) {
                    // Prim now has non-default draw mode and we need to use
                    // stand-in geometry.
                    //
                    // Delete old geometry.
                    _DeleteSubtree(path);
                    removedEntries.push_back({path});
                    // Add new stand-in geometry.
                    standin->ComputePrimAddedEntries(&addedEntries);
                    _prims[path] = std::move(standin);
                    // Do not traverse ancestors of this prim.
                    lastPath = path;
                }
            } else {
                if (it->second->GetDrawMode() != drawMode) {
                    // Draw mode has changed (including changed to default).
                    //
                    // Delete old geometry.
                    _DeleteSubtree(path);
                    removedEntries.push_back({path});
                    // Different scenarios are possible:
                    // 1. The prim was switched to default draw mode. We need
                    //    to recursively pull the geometry from the input scene
                    //    index again and send corresponding added entries.
                    //    If the prim has a descendant with non-default draw
                    //    mode, the recursion stops and we use stand-in 
                    //    geometry instead.
                    // 2. The prim switched to a different non-default draw
                    //    mode. This can be regarded as the special case where
                    //    the recursion immediately stops.
                    _RecursePrims(drawMode, path, prim, &addedEntries);
                    // Since we recursed to all descendants of the prim, ignore
                    // any descendants here.
                    lastPath = path;
                }
            }
        }
    }

    if (_prims.empty()) {
        if (!removedEntries.empty()) {
            _SendPrimsRemoved(removedEntries);
        }
        if (!addedEntries.empty()) {
            _SendPrimsAdded(addedEntries);
        }
        _SendPrimsDirtied(entries);
        return;
    }

    // Now account for dirtyLocators not related to resolving the draw mode.

    HdSceneIndexObserver::DirtiedPrimEntries dirtiedEntries;
    
    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        const SdfPath &path = entry.primPath;

        if (_HasDrawModeAncestor(entry.primPath)) {
            // Ancestors of prims with non-default draw mode can be ignored.
            continue;
        }

        auto it = _prims.find(path);
        if (it == _prims.end()) {
            // Prim has default draw mode, just forward entry.
            dirtiedEntries.push_back(entry);
            continue;
        }

        // Prim replaced by stand-in geometry has changed. Determine how
        // stand-in geometry is affected by changed attributed on prim.
        // ProcessDirtyLocators will do this; if the prim has changed in a
        // way that requires us to regenerate it (e.g., an axis has been added
        // or removed), it will set needsRefresh to true and we can then call
        // _RefreshDrawModeStandin. Note that _RefreshDrawModeStandin calls
        // _SendPrimsRemoved and _SendPrimsAdded as needed.
        
        bool needsRefresh = false;
        it->second->ProcessDirtyLocators(
            entry.dirtyLocators, &dirtiedEntries, &needsRefresh);
        if (needsRefresh) {
            _RefreshDrawModeStandin(
                _GetInputSceneIndex(),
                path,
                &it->second,
                &removedEntries,
                &addedEntries);
        }
    }
    if (!removedEntries.empty()) {
        _SendPrimsRemoved(removedEntries);
    }
    if (!addedEntries.empty()) {
        _SendPrimsAdded(addedEntries);
    }
    if (!dirtiedEntries.empty()) {
        _SendPrimsDirtied(dirtiedEntries);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
