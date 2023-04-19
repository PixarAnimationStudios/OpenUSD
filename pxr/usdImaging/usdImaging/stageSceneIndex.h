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
#ifndef PXR_USD_IMAGING_USD_IMAGING_STAGE_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_STAGE_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/imaging/hd/sceneIndex.h"

#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/stage.h"

#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_unordered_map.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdImagingPrimAdapter;
using UsdImagingPrimAdapterSharedPtr = std::shared_ptr<UsdImagingPrimAdapter>;

class UsdImagingAPISchemaAdapter;
using UsdImagingAPISchemaAdapterSharedPtr =
    std::shared_ptr<UsdImagingAPISchemaAdapter>;


class UsdImagingStageSceneIndex;
TF_DECLARE_REF_PTRS(UsdImagingStageSceneIndex);

class UsdImagingStageSceneIndex : public HdSceneIndexBase
{
public:

    static UsdImagingStageSceneIndexRefPtr New() {
        return TfCreateRefPtr(new UsdImagingStageSceneIndex());
    }

    USDIMAGING_API
    ~UsdImagingStageSceneIndex();

    // ------------------------------------------------------------------------
    // Scene index API

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath & primPath) const override;

    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath & primPath) const override;

    // ------------------------------------------------------------------------
    // App-facing API

    // Set the USD stage to pull data from. Note that this will delete all
    // scene index prims and reset stage global data.
    USDIMAGING_API
    void SetStage(UsdStageRefPtr stage);

    // Set the time, and call PrimsDirtied for any time-varying attributes.
    USDIMAGING_API
    void SetTime(UsdTimeCode time);

    // Return the current time.
    USDIMAGING_API
    UsdTimeCode GetTime() const;

    // Apply queued stage edits to imaging scene.
    // If the USD stage is edited while the scene index is pulling from it,
    // those edits get queued and deferred.  Calling ApplyPendingUpdates will
    // turn resync requests into PrimsAdded/PrimsRemoved, and property changes
    // into PrimsDirtied.
    USDIMAGING_API
    void ApplyPendingUpdates();

private:
    USDIMAGING_API
    UsdImagingStageSceneIndex();

    Usd_PrimFlagsConjunction _GetTraversalPredicate() const;

    using _APISchemaEntry =
        std::pair<UsdImagingAPISchemaAdapterSharedPtr, TfToken>;
    using _APISchemaAdapters = TfSmallVector<_APISchemaEntry, 8>;

    // Adapter delegation.

    _APISchemaAdapters _AdapterSetLookup(UsdPrim prim,
            // optionally return the prim adapter (which will also be
            // included in wrapped form as  part of the ordered main result)
            UsdImagingPrimAdapterSharedPtr *outputPrimAdapter=nullptr) const;

    UsdImagingAPISchemaAdapterSharedPtr _APIAdapterLookup(
            const TfToken &adapterKey) const;

    UsdImagingPrimAdapterSharedPtr _PrimAdapterLookup(
            const TfToken &adapterKey) const;

    TfTokenVector _GetImagingSubprims(
            UsdPrim const& prim, const _APISchemaAdapters &adapters) const;
    
    TfToken _GetImagingSubprimType(
            const _APISchemaAdapters &adapters,
            UsdPrim const& prim,
            const TfToken &subprim) const;

    HdContainerDataSourceHandle _GetImagingSubprimData(
            const _APISchemaAdapters &adapters,
            UsdPrim const& prim, const TfToken& subprim) const;

    HdDataSourceLocatorSet _InvalidateImagingSubprim(
            const _APISchemaAdapters &adapters,
            UsdPrim const& prim,
            TfToken const& subprim, TfTokenVector const& properties) const;

    void _ApplyPendingResyncs();

    class _StageGlobals : public UsdImagingDataSourceStageGlobals
    {
    public:
        // Datasource-facing API
        void FlagAsTimeVarying(
            const SdfPath & primPath,
            const HdDataSourceLocator & locator) const override;

        UsdTimeCode GetTime() const override;

        // Scene index-facing API
        void SetTime(UsdTimeCode time,
                HdSceneIndexObserver::DirtiedPrimEntries *dirtied);
        void Clear();

    private:
        struct _PathHashCompare {
            static bool equal(const SdfPath &a, const SdfPath &b) {
                return a == b;
            }
            static size_t hash(const SdfPath &p) {
                return hash_value(p);
            }
        };
        using _VariabilityMap = tbb::concurrent_hash_map<SdfPath,
                HdDataSourceLocatorSet, _PathHashCompare>;
        mutable _VariabilityMap _timeVaryingLocators;

        UsdTimeCode _time;
    };

    UsdStageRefPtr _stage;
    _StageGlobals _stageGlobals;

    // Population
    void _Populate();
    void _PopulateSubtree(UsdPrim subtreeRoot);

    // Edit processing
    void _OnUsdObjectsChanged(UsdNotice::ObjectsChanged const& notice,
                              UsdStageWeakPtr const& sender);
    TfNotice::Key _objectsChangedNoticeKey;

    // Note: resync paths mean we remove the whole subtree and repopulate.
    SdfPathVector _usdPrimsToResync;
    // Property changes get converted into PrimsDirtied messages.
    std::map<SdfPath, TfTokenVector> _usdPropertiesToUpdate;

    // Usd Prim Type to Adapter lookup table, concurrent because it could
    // be potentially filled during concurrent GetPrim calls rather than
    // just during single-threaded population.
    using _PrimAdapterMap = tbb::concurrent_unordered_map<
        TfToken, UsdImagingPrimAdapterSharedPtr, TfHash>;

    mutable _PrimAdapterMap _primAdapterMap;

    using _ApiAdapterMap = tbb::concurrent_unordered_map<
        TfToken, UsdImagingAPISchemaAdapterSharedPtr, TfHash>;

    mutable _ApiAdapterMap _apiAdapterMap;

    struct _AdapterSetEntry
    {
         // ordered and inclusive of primAdapter
        _APISchemaAdapters allAdapters;

        // for identifying prim adapter within same lookup
        UsdImagingPrimAdapterSharedPtr primAdapter; 
    };

    // Use UsdPrimTypeInfo pointer as key because they are guaranteed to be
    // cached at least as long as the stage is open.
    using _AdapterSetMap = tbb::concurrent_unordered_map<
        const UsdPrimTypeInfo *, _AdapterSetEntry, TfHash>;

    mutable _AdapterSetMap _adapterSetMap;

    _APISchemaAdapters _keylessAdapters;

    using _PrimAdapterPair = std::pair<UsdPrim, UsdImagingPrimAdapterSharedPtr>;
    _PrimAdapterPair _FindResponsibleAncestor(const UsdPrim &prim);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
