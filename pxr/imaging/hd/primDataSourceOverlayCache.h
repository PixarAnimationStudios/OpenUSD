//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_PRIM_DATA_SOURCE_OVERLAY_CACHE_H
#define PXR_IMAGING_HD_PRIM_DATA_SOURCE_OVERLAY_CACHE_H

#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/dataSource.h"

#include "pxr/usd/sdf/pathTable.h"

PXR_NAMESPACE_OPEN_SCOPE

// ----------------------------------------------------------------------------

// A utility class to handle caching of datasource overlays, along with
// invalidation functions to clear the cache.

class HdPrimDataSourceOverlayCache :
    public std::enable_shared_from_this<HdPrimDataSourceOverlayCache>
{
public:
    virtual ~HdPrimDataSourceOverlayCache();

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const;

    void HandlePrimsAdded(
        const HdSceneIndexObserver::AddedPrimEntries &entries,
        const HdSceneIndexBaseRefPtr &source);
    void HandlePrimsRemoved(
        const HdSceneIndexObserver::RemovedPrimEntries &entries);
    void HandlePrimsDirtied(
        const HdSceneIndexObserver::DirtiedPrimEntries &entries,
        HdSceneIndexObserver::DirtiedPrimEntries *additionalDirtied);

protected:
    HdPrimDataSourceOverlayCache() = default;

    // This struct provides a way for users of the cache to describe the
    // structure of synthetic attributes. For example, if you compute
    // "xformInverse" from "xform", the topology would look like:
    // OverlayTopology topo = { "xformInverse" ->
    //   { .onPrim = { "xform" }, .onParent = { }, false }
    // }
    // ... notably, this tells us what attributes we're adding (xformInverse,
    // but only when xform is present); and also to dirty xformInverse when
    // xform is dirty.
    //
    // For attributes we always want to add (even if their dependents are not
    // present), dependenciesOptional lets us say as much.
    //
    // Note: _ComputeOverlayDataSource should respect this topology, or behavior
    // is undefined...
    //
    // XXX: the "onParent" dependencies here are to support eventual inherited
    // attribute caching, but this feature hasn't been implemented yet.
    struct _OverlayDependencies
    {
        _OverlayDependencies()
            : onPrim(), onParent(), dependenciesOptional(false) {}

        HdDataSourceLocatorSet onPrim;
        HdDataSourceLocatorSet onParent;
        bool dependenciesOptional;
    };
    using _OverlayTopology = std::map<TfToken, _OverlayDependencies>;


    // Topology should be set once, from the derived class constructor.
    void _SetOverlayTopology(const _OverlayTopology &topology) {
        _overlayTopology = topology;
    }

    // Compute the named datasource. Note that inputDataSource comes from the
    // source scene index, while parentOverlayDataSource comes from the cache
    // and is consequently recursively composed.
    //
    // XXX: the "parentOverlayDataSource" is here to support eventual inherited
    // attribute caching, but this feature hasn't been implemented yet. For now,
    // it will always be null.
    virtual HdDataSourceBaseHandle _ComputeOverlayDataSource(
        const TfToken &name,
        HdContainerDataSourceHandle inputDataSource,
        HdContainerDataSourceHandle parentOverlayDataSource) const = 0;

private:
    class _HdPrimDataSourceOverlay : public HdContainerDataSource
    {
    public:
        HD_DECLARE_DATASOURCE(_HdPrimDataSourceOverlay);

        _HdPrimDataSourceOverlay(
            HdContainerDataSourceHandle inputDataSource,
            HdContainerDataSourceHandle parentOverlayDataSource,
            const std::weak_ptr<const HdPrimDataSourceOverlayCache> cache);

        void UpdateInputDataSource(HdContainerDataSourceHandle inputDataSource);

        void PrimDirtied(const HdDataSourceLocatorSet &dirtyAttributes);

        TfTokenVector GetNames() override;
        HdDataSourceBaseHandle Get(const TfToken &name) override;

    private:
        HdContainerDataSourceHandle _inputDataSource;
        HdContainerDataSourceHandle _parentOverlayDataSource;
        const std::weak_ptr<const HdPrimDataSourceOverlayCache> _cache;

        using _OverlayMap = std::map<TfToken, HdDataSourceBaseHandle>;

        _OverlayMap _overlayMap;
    };

    SdfPathTable<HdSceneIndexPrim> _cache;
    _OverlayTopology _overlayTopology;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
