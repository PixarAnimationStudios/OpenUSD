//
// Copyright 2021 Pixar
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
        const HdSceneIndexObserver::DirtiedPrimEntries &entries);

protected:
    // The "hierarchical" flag controls whether the parent overlay data source
    // is passed into _ComputeOverlayDataSource. This is needed for hierarchical
    // operations like transform concatenation, but can be an unnecessary
    // expense for other operations, so we allow derived classes to turn it off.
    HdPrimDataSourceOverlayCache(bool hierarchical)
        : _hierarchical(hierarchical) {}

    // A list of top-level datasources this overlay adds/overrides.
    virtual TfTokenVector _GetOverlayNames(
        HdContainerDataSourceHandle inputDataSource) const = 0;

    // Compute the named datasource. Note that inputDataSource comes from the
    // source scene index, while parentOverlayDataSource comes from the cache.
    // If _hierarchical is false, parentOverlayDataSource will be null.
    virtual HdDataSourceBaseHandle _ComputeOverlayDataSource(
        const TfToken &name,
        HdContainerDataSourceHandle inputDataSource,
        HdContainerDataSourceHandle parentOverlayDataSource) const = 0;

    // Return the dependencies on inputDataSource that can invalidate a named
    // overlay.
    virtual HdDataSourceLocatorSet _GetOverlayDependencies(
        const TfToken &name) const = 0;

private:

    HdContainerDataSourceHandle _AddPrim(
        const SdfPath &primPath,
        const HdSceneIndexBaseRefPtr &source);

    class _HdPrimDataSourceOverlay : public HdContainerDataSource
    {
    public:
        HD_DECLARE_DATASOURCE(_HdPrimDataSourceOverlay);

        _HdPrimDataSourceOverlay(
            HdContainerDataSourceHandle inputDataSource,
            HdContainerDataSourceHandle parentOverlayDataSource,
            const std::weak_ptr<const HdPrimDataSourceOverlayCache> cache);

        void PrimDirtied(const HdDataSourceLocatorSet &locators);

        bool Has(const TfToken &name) override;
        TfTokenVector GetNames() override;
        HdDataSourceBaseHandle Get(const TfToken &name) override;

    private:
        HdContainerDataSourceHandle _inputDataSource;
        HdContainerDataSourceHandle _parentOverlayDataSource;
        const std::weak_ptr<const HdPrimDataSourceOverlayCache> _cache;

        using _OverlayMap = std::map<TfToken, HdDataSourceBaseHandle>;

        _OverlayMap _overlayMap;
        TfTokenVector _overlayNames;
    };

    const bool _hierarchical;
    SdfPathTable<HdSceneIndexPrim> _cache;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
