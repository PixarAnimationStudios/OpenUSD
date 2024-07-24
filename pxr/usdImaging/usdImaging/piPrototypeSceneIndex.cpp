//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/piPrototypeSceneIndex.h"

#include "pxr/usdImaging/usdImaging/usdPrimInfoSchema.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"

#include <tbb/enumerable_thread_specific.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

bool
_ContainsStrictPrefixOfPath(
    const std::unordered_set<SdfPath, SdfPath::Hash> &pathSet,
    const SdfPath &path)
{
    for (SdfPath p=path.GetParentPath(); !p.IsEmpty(); p = p.GetParentPath()) {
        if (pathSet.find(p) != pathSet.end()) {
            return true;
        }
    }
    return false;
}

HdContainerDataSourceHandle
_ComputeUnderlaySource(const SdfPath &instancer, const SdfPath &prototypeRoot)
{
    if (instancer.IsEmpty()) {
        return nullptr;
    }

    using DataSource = HdRetainedTypedSampledDataSource<VtArray<SdfPath>>;

    return
        HdRetainedContainerDataSource::New(
            HdInstancedBySchema::GetSchemaToken(),
            HdInstancedBySchema::Builder()
                .SetPaths(DataSource::New({ instancer }))
                .SetPrototypeRoots(DataSource::New({ prototypeRoot }))
                .Build()); 
}

HdContainerDataSourceHandle
_ComputePrototypeRootOverlaySource(const SdfPath &instancer)
{
    if (instancer.IsEmpty()) {
        return nullptr;
    }
    
    return
        HdRetainedContainerDataSource::New(
            HdXformSchema::GetSchemaToken(),
            HdXformSchema::Builder()
                .SetResetXformStack(
                    HdRetainedTypedSampledDataSource<bool>::New(
                        true))
            .Build());
}

bool
_IsOver(const HdSceneIndexPrim &prim)
{
    UsdImagingUsdPrimInfoSchema schema =
        UsdImagingUsdPrimInfoSchema::GetFromParent(prim.dataSource);
    HdTokenDataSourceHandle const ds = schema.GetSpecifier();
    if (!ds) {
        return false;
    }
    return ds->GetTypedValue(0.0f) == UsdImagingUsdPrimInfoSchemaTokens->over;
}

}

UsdImaging_PiPrototypeSceneIndexRefPtr
UsdImaging_PiPrototypeSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    const SdfPath &instancer,
    const SdfPath &prototypeRoot)
{
    return TfCreateRefPtr(
        new UsdImaging_PiPrototypeSceneIndex(
            inputSceneIndex, instancer, prototypeRoot));
}

UsdImaging_PiPrototypeSceneIndex::
UsdImaging_PiPrototypeSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    const SdfPath &instancer,
    const SdfPath &prototypeRoot)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _prototypeRoot(prototypeRoot)
  , _underlaySource(
      _ComputeUnderlaySource(instancer, prototypeRoot))
  , _prototypeRootOverlaySource(
      _ComputePrototypeRootOverlaySource(instancer))
{
    _Populate();
}

void
UsdImaging_PiPrototypeSceneIndex::_Populate()
{
    HdSceneIndexPrimView view(_GetInputSceneIndex(), _prototypeRoot);
    for (auto it = view.begin(); it != view.end(); ++it) {
        const SdfPath &path = *it;
        
        HdSceneIndexPrim const prim = _GetInputSceneIndex()->GetPrim(path);
        if (prim.primType == HdPrimTypeTokens->instancer ||
            _IsOver(prim)) {
            _instancersAndOvers.insert(path);
            
            it.SkipDescendants();
        }
    }
}

static
void
_MakeUnrenderable(HdSceneIndexPrim * const prim)
{
    // Force the prim type to empty.
    prim->primType = TfToken();

    if (!prim->dataSource) {
        return;
    }

    // Note that native USD instances are still picked up by the
    // native instance scene indices even when the prim type is empty.
    //
    // We explicitly block the data source indicating a USD instanced.
    //
    // This, unfortuantely, means that a point instancing scene index
    // needs to know about a native instancing token.
    //
    static HdContainerDataSourceHandle const overlaySource =
        HdRetainedContainerDataSource::New(
            UsdImagingUsdPrimInfoSchema::GetSchemaToken(),
            HdRetainedContainerDataSource::New(
                UsdImagingUsdPrimInfoSchemaTokens->niPrototypePath,
                HdBlockDataSource::New()));
    prim->dataSource = HdOverlayContainerDataSource::New(
        overlaySource,
        prim->dataSource);
}

HdSceneIndexPrim
UsdImaging_PiPrototypeSceneIndex::GetPrim(const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (!primPath.HasPrefix(_prototypeRoot)) {
        return prim;
    }

    if (_ContainsStrictPrefixOfPath(_instancersAndOvers, primPath)) {
        // Render all prims under an instancer or over invisible.
        _MakeUnrenderable(&prim);
        return prim;
    }

    if (!prim.dataSource) {
        return prim;
    }

    if (_underlaySource) {
        prim.dataSource = HdOverlayContainerDataSource::New(
            prim.dataSource,
            _underlaySource);
    }

    if (_prototypeRootOverlaySource) {
        if (primPath == _prototypeRoot) {
            prim.dataSource = HdOverlayContainerDataSource::New(
                _prototypeRootOverlaySource,
                prim.dataSource);
        }
    }
    
    return prim;
}

SdfPathVector
UsdImaging_PiPrototypeSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
UsdImaging_PiPrototypeSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    // First pass: Identify instancers and overs.
    // Use thread-local results to avoid synchronizing.
    tbb::enumerable_thread_specific<SdfPathVector> perThreadResults;
    WorkParallelForN(
        //entries.begin(), entries.end(),
        entries.size(),
        [&](size_t begin, size_t end)
        {
            SdfPathVector &results = perThreadResults.local();
            for (size_t i=begin; i<end; ++i) {
                const HdSceneIndexObserver::AddedPrimEntry &entry = entries[i];
                if (entry.primType == HdPrimTypeTokens->instancer ||
                    _IsOver(_GetInputSceneIndex()->GetPrim(entry.primPath))) {
                    results.push_back(entry.primPath);
                }
            }
        },
        256 /* note: relatively coarse grain size */ );

    // Commit per-thread results back into _instancersAndOvers.
    for (const SdfPath &path: tbb::flatten2d(perThreadResults)) {
        _instancersAndOvers.insert(path);
    }

    // Second pass: Clear out types for any prims under instancers or overs.
    HdSceneIndexObserver::AddedPrimEntries newEntries(entries);
    WorkParallelForEach(
        newEntries.begin(), newEntries.end(),
        [&](HdSceneIndexObserver::AddedPrimEntry &entry)
    {
        if (_ContainsStrictPrefixOfPath(_instancersAndOvers, entry.primPath)) {
            entry.primType = TfToken();
        }
    });

    // Note that we do not handle the case that the type of a prim
    // changes and we get a single AddedPrimEntry about it.
    //
    // E.g. if a prim becomes an instancer, we need to re-sync
    // its namespace descendants since their type change to empty.
    // Similarly, if a prim was an instancer.

    _SendPrimsAdded(newEntries);
}

void
UsdImaging_PiPrototypeSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

void
UsdImaging_PiPrototypeSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        // Remove all items in _instancersAndOvers that have the removed
        // path as a prefix.
        for (_PathSet::iterator i = _instancersAndOvers.begin();
             i != _instancersAndOvers.end();) {
            if (i->HasPrefix(entry.primPath)) {
                i = _instancersAndOvers.erase(i);
            } else {
                ++i;
            }
        }
    }

    _SendPrimsRemoved(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
