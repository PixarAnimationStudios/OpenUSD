//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdsi/primManagingSceneIndexObserver.h"

#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdsiPrimManagingSceneIndexObserverTokens,
                        HDSI_PRIM_MANAGING_SCENE_INDEX_OBSERVER_TOKENS);

HdsiPrimManagingSceneIndexObserver::
PrimBase::~PrimBase() = default;

HdsiPrimManagingSceneIndexObserver::
PrimFactoryBase::~PrimFactoryBase() = default;

static
HdsiPrimManagingSceneIndexObserver::PrimFactoryBaseHandle
_GetPrimFactory(HdContainerDataSourceHandle const &inputArgs)
{
    if (!inputArgs) {
        return nullptr;
    }

    using DataSource =
        HdTypedSampledDataSource<
            HdsiPrimManagingSceneIndexObserver::PrimFactoryBaseHandle>;
    typename DataSource::Handle const ds =
        DataSource::Cast(
            inputArgs->Get(
                HdsiPrimManagingSceneIndexObserverTokens->primFactory));
    if (!ds) {
        return nullptr;
    }
    return ds->GetTypedValue(0.0f);
}   

HdsiPrimManagingSceneIndexObserver::HdsiPrimManagingSceneIndexObserver(
        HdSceneIndexBaseRefPtr const &sceneIndex,
        HdContainerDataSourceHandle const &inputArgs)
  : _sceneIndex(sceneIndex)
  , _primFactory(_GetPrimFactory(inputArgs))
{
    if (!sceneIndex) {
        return;
    }

    HdSceneIndexObserverPtr const self(this);
    sceneIndex->AddObserver(self);

    if (!_primFactory) {
        return;
    }

    // This loop can be made multi-threaded through a technique
    // similar to the one suggested in PrimsAdded or by adding
    // all paths to _prims first and then set the shared_points
    // in a parallel loop.
    for (const SdfPath &path : HdSceneIndexPrimView(sceneIndex)) {
        const AddedPrimEntry entry{path, sceneIndex->GetPrim(path).primType};
        if (PrimBaseHandle prim = _primFactory->CreatePrim(entry, this)) {
            _prims[path] = std::move(prim);
        }
    }
}

HdsiPrimManagingSceneIndexObserver::
~HdsiPrimManagingSceneIndexObserver() = default;

const HdsiPrimManagingSceneIndexObserver::PrimBaseHandle &
HdsiPrimManagingSceneIndexObserver::GetPrim(const SdfPath &primPath) const
{
    auto it = _prims.find(primPath);
    if (it == _prims.end()) {
        static PrimBaseHandle null;
        return null;
    }
    return it->second;
}

void
HdsiPrimManagingSceneIndexObserver::PrimsAdded(
    const HdSceneIndexBase &sender,
    const AddedPrimEntries &entries)
{
    if (!_primFactory) {
        return;
    }

    // This loop can be made multi-threaded as (potentially
    // configured through inputArgs):
    //
    // std::vector<PrimBaseHandle> newPrims(entries.size);
    // parallel-loop:
    //     newPrims[i] = _primFactory->CreatePrim(entries[i], this)
    // loop:
    //     _prims[entries[i].primPath] = newPrims[i];
    //
    for (const AddedPrimEntry &entry : entries) {
        if (PrimBaseHandle prim = _primFactory->CreatePrim(entry, this)) {
            // If prim at path already existed (AddedPrimEntry is a resync),
            // previous handle stored in _prims will be destroyed,
            // resulting in destruction of PrimBase if it was the
            // the only handle.
            _prims[entry.primPath] = std::move(prim);
        } else {
            // Delete entry if prim type after resyncing is not supported.
            _prims.erase(entry.primPath);
        }
    }
}

void
HdsiPrimManagingSceneIndexObserver::PrimsDirtied(
    const HdSceneIndexBase &sender,
    const DirtiedPrimEntries &entries)
{
    // This loop can be made parallel.
    //
    // Note that if we put this into a parallel
    // for-loop, we might call _PrimBase::Dirty on
    // the same prim from several threads simultaneously
    // if the same path appears in multiple DirtiedPrimEntry's.
    // We might need to put a mutex into _PrimBase.
    //
    for (const DirtiedPrimEntry &entry : entries) {
        const auto it = _prims.find(entry.primPath);
        if (it == _prims.end()) {
            continue;
        }

        if (!TF_VERIFY(it->second)) {
            continue;
        }

        it->second->Dirty(entry, this);
    }
}

void
HdsiPrimManagingSceneIndexObserver::PrimsRemoved(
    const HdSceneIndexBase &sender,
    const RemovedPrimEntries &entries)
{
    // This loop can be made parallel by saving the handles to be
    // destroyed in a vector oldPrims and calling oldPrims[i].reset()
    // in a parallel-for loop.
    //
    for (const RemovedPrimEntry &entry : entries) {
        auto it = _prims.lower_bound(entry.primPath);
        while (it != _prims.end() && it->first.HasPrefix(entry.primPath)) {
            it = _prims.erase(it);
        }
    }
}

void
HdsiPrimManagingSceneIndexObserver::PrimsRenamed(
    const HdSceneIndexBase &sender,
    const RenamedPrimEntries &entries)
{
    ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
}

PXR_NAMESPACE_CLOSE_SCOPE
