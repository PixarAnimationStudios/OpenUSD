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
#include "pxr/usdImaging/usdImaging/usdPrimInfoSchema.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

bool
_IsUsdNativeInstance(const HdSceneIndexPrim &prim)
{
    UsdImagingUsdPrimInfoSchema primInfoSchema =
        UsdImagingUsdPrimInfoSchema::GetFromParent(prim.dataSource);

    HdPathDataSourceHandle const ds = primInfoSchema.GetNiPrototypePath();
    if (!ds) {
        return false;
    }
    return !ds->GetTypedValue(0.0f).IsEmpty();
}

// Resolve draw mode for prim from input scene index.
// Default draw mode can be expressed by either the empty token
// or UsdGeomTokens->default_.
TfToken
_GetDrawMode(const HdSceneIndexPrim &prim)
{
    static const TfToken empty;

    if (_IsUsdNativeInstance(prim)) {
        // Do not apply draw mode to native instance.
        // Instead, the native instance prototype propagating scene index
        // will create a copy of the prototype with the apply draw mode set
        // and the draw mode scene index processing that prototype applies
        // the draw mode.
        return empty;
    }

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

}

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
    const SdfPath &rootPath = SdfPath::AbsoluteRootPath();

    const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(rootPath);

    _RecursePrims(_GetDrawMode(prim), rootPath, prim, nullptr);
}

UsdImagingDrawModeSceneIndex::~UsdImagingDrawModeSceneIndex() = default;

static
auto _FindPrefixOfPath(
    const std::map<SdfPath, UsdImaging_DrawModeStandinSharedPtr> &container,
    const SdfPath &path)
{
    // Use std::map::lower_bound over std::lower_bound since the latter
    // is slow given that std::map iterators are not random access.
    auto it = container.lower_bound(path);
    if (it != container.end() && path == it->first) {
        // Path itself is in the container
        return it;
    }
    // If a prefix of path is in container, it will point to the next element
    // in the container, rather than the prefix itself.
    if (it == container.begin()) {
        return container.end();
    }
    --it;
    if (path.HasPrefix(it->first)) {
        return it;
    }
    return container.end();
}

UsdImaging_DrawModeStandinSharedPtr
UsdImagingDrawModeSceneIndex::_FindStandinForPrimOrAncestor(
    const SdfPath &path,
    size_t * const relPathLen) const
{
    const auto it = _FindPrefixOfPath(_prims, path);
    if (it == _prims.end()) {
        return nullptr;
    }
    *relPathLen = path.GetPathElementCount() - it->first.GetPathElementCount();
    return it->second;
}

HdSceneIndexPrim
UsdImagingDrawModeSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    // Do we have this prim path or an ancestor prim path in the
    // _prims map?
    size_t relPathLen;
    if (UsdImaging_DrawModeStandinSharedPtr const standin =
            _FindStandinForPrimOrAncestor(primPath, &relPathLen)) {
        
        if (relPathLen == 0) {
            // Example
            // Querried prim is /Foo and the DrawModeStandin is at /Foo.
            //
            // We query the DrawmodeStandin for its prim.
            return standin->GetPrim();
        }
        if (relPathLen == 1) {
            // Example:
            // Querried prim is /Foo/mesh and the DrawModeStandin is at /Foo.
            //
            // We query the DrawmodeStandin for the child prim mesh.
            return standin->GetChildPrim(primPath.GetNameToken());
        }
        // Example:
        // Querried prim is /Foo/A/B and the DrawModeStandin is at /Foo.
        //
        // We block everything at this level since draw mode standin's
        // only have immediate children.
        return { TfToken(), nullptr };
    }

    return _GetInputSceneIndex()->GetPrim(primPath);
}

SdfPathVector
UsdImagingDrawModeSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    // Do we have this prim path or an ancestor prim path in the
    // _prims map?
    size_t relPathLen;
    if (UsdImaging_DrawModeStandinSharedPtr const standin =
            _FindStandinForPrimOrAncestor(primPath, &relPathLen)) {
        if (relPathLen == 0) {
            // List immediate children of standin.
            return standin->GetChildPrimPaths();
        }
        // Standin only has immediate children. So just block.
        return {};
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
        if (entries) {
            standin->ComputePrimAddedEntries(entries);
        }
        // And store it.
        _prims[path] = std::move(standin);
    } else {
        // Mark prim as added and recurse to children.
        if (entries) {
            entries->push_back({path, prim.primType});
        }
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
    HdSceneIndexObserver::RemovedPrimEntries removedEntries;

    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        const SdfPath &path = entry.primPath;

        // Suppress prims from input scene delegate that have an ancestor
        // with a draw mode.
        size_t relPathLen;
        if (UsdImaging_DrawModeStandinSharedPtr standin =
               _FindStandinForPrimOrAncestor(path, &relPathLen)) {
            if (relPathLen > 0) {
                continue;
            }
        }
               
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
        const TfToken drawMode = _GetDrawMode(prim);

        if (UsdImaging_DrawModeStandinSharedPtr standin =
                UsdImaging_GetDrawModeStandin(
                    drawMode, path, prim.dataSource)) {

            // Sending out removed entry here for the following scenario:
            // Assume that the input to the draw mode scene index has a
            // prim with non-default draw mode at /Foo and a prim at /Foo/Bar.
            // The draw mode scene index has not yet received a prims added
            // call for /Foo (thus, there is no entry for /Foo in
            // UsdImagingDrawModeSceneIndex::_prim), yet a client scene
            // index asked for the prim at /Foo/Bar.
            // At this point, the draw mode scene index returns a valid
            // prim for GetPrim(/Foo/Bar) with prim type determined from the
            // input scene index. This is incorrect as the prim should be
            // dropped because of /Foo's draw mode. Similarly, for
            // GetChildPrimPaths.
            // When the PrimsAdded message for /Foo arrived, the
            // UsdImagingDrawModeSceneIndex will update _prim. And it can
            // now rectify the situation by sending out a removes prim
            // message for /Foo.
            //
            // Note that this happens when there are prototype propagating
            // scene indices have been connected to a
            // UsdImagingStageSceneIndex before the call to
            // UsdImagingStageSceneIndex::SetStage.
            // The prototype propagating scene index inserts propagated
            // prototypes into the merging scene index. When a scene
            // index is added to the merging scene index, it traverses
            // it through GetChildPrimPaths to emit the necessary prims
            // added messages. In particular, it might call
            // GetChildPrimPaths for a prim inside a prototype before
            // the PrimsAdded message for that prim was emitted by the
            // UsdImagingStageSceneIndex.
            //
            _DeleteSubtree(path);
            removedEntries.push_back({path});

            // The prim needs to be replaced by stand-in geometry.
            standin->ComputePrimAddedEntries(&newEntries);
            _prims[path] = std::move(standin);
        } else {
            // Just forward added entry.
            newEntries.push_back(entry);
        }
    }

    if (!removedEntries.empty()) {
        _SendPrimsRemoved(removedEntries);
    }
     
    if (!newEntries.empty()) {
        _SendPrimsAdded(newEntries);
    }
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

            // Suppress prims from input scene delegate that have an ancestor
            // with a draw mode.
            size_t relPathLen;
            if (UsdImaging_DrawModeStandinSharedPtr standin =
                _FindStandinForPrimOrAncestor(path, &relPathLen)) {
                if (relPathLen > 0) {
                    continue;
                }
            }
            
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
        size_t relPathLen;
        UsdImaging_DrawModeStandinSharedPtr const standin =
            _FindStandinForPrimOrAncestor(path, &relPathLen);
        if (!standin) {
            // Prim and all its ancestors have default draw mode,
            // just forward entry.
            dirtiedEntries.push_back(entry);
            continue;
        }

        if (relPathLen > 0) {
            // Ancestors of prims with non-default draw mode can be ignored.
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
        standin->ProcessDirtyLocators(
            entry.dirtyLocators, &dirtiedEntries, &needsRefresh);
        if (needsRefresh) {
            UsdImaging_DrawModeStandinSharedPtr newStandin =
                UsdImaging_GetDrawModeStandin(
                    standin->GetDrawMode(),
                    path,
                    _GetInputSceneIndex()->GetPrim(path).dataSource);
            if (!TF_VERIFY(newStandin)) {
                continue;
            }
            removedEntries.push_back({path});
            newStandin->ComputePrimAddedEntries(&addedEntries);
            _prims[path] = std::move(newStandin);
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
