//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/usd/usd/timeCode.h"

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

using UsdImagingSceneIndexAppendCallback =
    std::function<
        HdSceneIndexBaseRefPtr(HdSceneIndexBaseRefPtr const &)>;

struct UsdImagingSceneIndices;
TF_DECLARE_REF_PTRS(HdNoticeBatchingSceneIndex);
TF_DECLARE_REF_PTRS(UsdStage);
TF_DECLARE_REF_PTRS(UsdImagingStageSceneIndex);
TF_DECLARE_REF_PTRS(UsdImagingSelectionSceneIndex);
TF_DECLARE_REF_PTRS(UsdImagingSceneIndex);

///
/// A scene index encapsulating the chain of scene indices (resolving, e.g.,
/// USD native instancing) that is needed by a client to consume a UsdStage.
///
class UsdImagingSceneIndex
            : public HdSceneIndexBase, public HdEncapsulatingSceneIndexBase
{
public:
    /// Create the chain of scene indices.
    ///
    /// Note that UsdImagingSceneIndexAppendCallback cannot be part of the
    /// UsdImagingUsdSceneIndexInputArgsSchema since std::function is not a
    /// value type (no operator==) and, thus, cannot be returned by an
    /// HdTypedSampledDataSource. We might revisit the mechanism for clients
    /// to specify the callback (through, e.g., registration) later.
    ///
    static UsdImagingSceneIndexRefPtr New(
            HdContainerDataSourceHandle const &inputArgs,
            const UsdImagingSceneIndexAppendCallback &
                overridesSceneIndexCallback) {
        return TfCreateRefPtr(
            new UsdImagingSceneIndex(
                inputArgs, overridesSceneIndexCallback));
    }

    USDIMAGING_API
    ~UsdImagingSceneIndex();

    // ------------------------------------------------------------------------
    // (Encapsulating) Scene index API

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath & primPath) const override;

    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath & primPath) const override;

    USDIMAGING_API
    HdSceneIndexBaseRefPtrVector GetEncapsulatedScenes() const override;

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

    /// Given a path (including usd proxy path inside a native instance) of
    /// a USD prim, determine the corresponding prim in the Usd scene index
    /// (filtered by the UsdImagingNiPrototypePropagatingSceneIndex) and
    /// populate its selections data source.
    USDIMAGING_API
    void AddSelection(const SdfPath &path);

    /// Reset the scene index selection state.
    USDIMAGING_API
    void ClearSelection();

    /// Can be used to batch notices.
    const HdNoticeBatchingSceneIndexRefPtr &
    GetPostInstancingNoticeBatchingSceneIndex() const {
        return _postInstancingNoticeBatchingSceneIndex;
    }

private:
    class _Observer : public HdSceneIndexObserver
    {
    public:
        _Observer(UsdImagingSceneIndex *owner)
          : _owner(owner) {}

        void PrimsAdded(
            const HdSceneIndexBase &sender,
            const AddedPrimEntries &entries) override
        {
            _owner->_SendPrimsAdded(entries);
        }

        void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override
        {
            _owner->_SendPrimsRemoved(entries);
        }

        void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override
        {
            _owner->_SendPrimsDirtied(entries);
        }

        void PrimsRenamed(
            const HdSceneIndexBase &sender,
            const RenamedPrimEntries &entries) override
        {
            _owner->_SendPrimsRenamed(entries);
        }
    private:
        UsdImagingSceneIndex * const _owner;
    };

    USDIMAGING_API
    UsdImagingSceneIndex(
        HdContainerDataSourceHandle const &inputArgs,
        const UsdImagingSceneIndexAppendCallback &overridesSceneIndexCallback);

    USDIMAGING_API
    UsdImagingSceneIndex(
        const UsdImagingSceneIndices &sceneIndices);

    UsdImagingStageSceneIndexRefPtr _stageSceneIndex;
    HdNoticeBatchingSceneIndexRefPtr _postInstancingNoticeBatchingSceneIndex;
    UsdImagingSelectionSceneIndexRefPtr _selectionSceneIndex;
    HdSceneIndexBaseRefPtr _finalSceneIndex;

    _Observer _observer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
