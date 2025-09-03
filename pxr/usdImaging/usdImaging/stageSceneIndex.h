//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_STAGE_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_STAGE_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"
#include "pxr/usdImaging/usdImaging/types.h"

#include "pxr/imaging/hd/sceneIndex.h"

#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/stage.h"

#include <mutex>
#include <set>

PXR_NAMESPACE_OPEN_SCOPE

#define USDIMAGING_STAGE_SCENE_INDEX_TOKENS \
    (includeUnloadedPrims)                  \

TF_DECLARE_PUBLIC_TOKENS(UsdImagingStageSceneIndexTokens, USDIMAGING_API,
                         USDIMAGING_STAGE_SCENE_INDEX_TOKENS);

using UsdImagingPrimAdapterSharedPtr =
    std::shared_ptr<class UsdImagingPrimAdapter>;
class UsdImaging_AdapterManager;

TF_DECLARE_REF_PTRS(UsdImagingStageSceneIndex);

class UsdImagingStageSceneIndex : public HdSceneIndexBase
{
public:
    static UsdImagingStageSceneIndexRefPtr New(
                    HdContainerDataSourceHandle const &inputArgs = nullptr) {
        return TfCreateRefPtr(new UsdImagingStageSceneIndex(inputArgs));
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
    //
    // PrimsDirtied is only called if the time is different from the last call
    // or forceDirtyingTimeDeps is true.
    USDIMAGING_API
    void SetTime(UsdTimeCode time, bool forceDirtyingTimeDeps = false);

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
    UsdImagingStageSceneIndex(HdContainerDataSourceHandle const &inputArgs);

    Usd_PrimFlagsPredicate _GetPrimPredicate() const;

    void _ApplyPendingResyncs();
    void _ComputeDirtiedEntries(
        const std::map<SdfPath, TfTokenVector> &pathToUsdProperties,
        SdfPathVector * primPathsToResync,
        UsdImagingPropertyInvalidationType invalidationType,
        HdSceneIndexObserver::DirtiedPrimEntries * dirtiedPrims) const;

    class _StageGlobals : public UsdImagingDataSourceStageGlobals
    {
    public:
        // Datasource-facing API
        void FlagAsTimeVarying(
            const SdfPath & hydraPath,
            const HdDataSourceLocator & locator) const override;

        void FlagAsAssetPathDependent(
            const SdfPath & usdPath) const override;

        UsdTimeCode GetTime() const override;

        // Scene index-facing API
        void SetTime(UsdTimeCode time,
                HdSceneIndexObserver::DirtiedPrimEntries *dirtied);

        void RemoveAssetPathDependentsUnder(const SdfPath &path);

        void InvalidateAssetPathDependentsUnder(
            const SdfPath &path,
            std::vector<SdfPath> *primsToInvalidate,
            std::map<SdfPath, TfTokenVector> *propertiesToInvalidate) const;

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

        using _AssetPathDependentsSet = std::set<SdfPath>;
        mutable _AssetPathDependentsSet _assetPathDependents;
        mutable std::mutex _assetPathDependentsMutex;

        UsdTimeCode _time;
    };

    const bool _includeUnloadedPrims;

    UsdStageRefPtr _stage;
    _StageGlobals _stageGlobals;

    // Population
    void _Populate();
    void _PopulateSubtree(UsdPrim subtreeRoot,
        HdSceneIndexObserver::AddedPrimEntries *addedPrims);

    // Edit processing
    void _OnUsdObjectsChanged(UsdNotice::ObjectsChanged const& notice,
                              UsdStageWeakPtr const& sender);
    TfNotice::Key _objectsChangedNoticeKey;

    // Keep track of populated paths for use in _ApplyPendingResyncs().
    SdfPathSet _populatedPaths;

    // Note: resync paths mean we remove the whole subtree and repopulate.
    SdfPathVector _usdPrimsToResync;
    // Property changes get converted into PrimsDirtied messages.
    std::map<SdfPath, TfTokenVector> _usdPropertiesToUpdate;
    std::map<SdfPath, TfTokenVector> _usdPropertiesToResync;

    using _PrimAdapterPair = std::pair<UsdPrim, UsdImagingPrimAdapterSharedPtr>;
    _PrimAdapterPair _FindResponsibleAncestor(const UsdPrim &prim) const;

    std::unique_ptr<UsdImaging_AdapterManager> const _adapterManager;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
