//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PLUGIN_HD_ST_RENDER_PARAM_H
#define PXR_IMAGING_PLUGIN_HD_ST_RENDER_PARAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hdSt/api.h"

#include <atomic>
#include <shared_mutex>

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HdStRenderParam
///
/// The render delegate can create an object of type HdRenderParam, to pass
/// to each prim during Sync(). Storm uses this class to house global
/// counters amd flags that assist invalidation of draw batch caches.
/// 
class HdStRenderParam final : public HdRenderParam
{
public:
    HdStRenderParam();
    ~HdStRenderParam() override;

    // ---------------------------------------------------------------------- //
    /// Draw items cache and batch invalidation
    // ---------------------------------------------------------------------- //
    /// Marks all batches dirty, meaning they need to be validated and
    /// potentially rebuilt.
    HDST_API
    void MarkDrawBatchesDirty();

    HDST_API
    unsigned int GetDrawBatchesVersion() const;

    /// Marks material tags dirty, meaning that the draw items associated with
    /// the collection of a render pass need to be re-gathered.
    HDST_API
    void MarkMaterialTagsDirty();

    HDST_API
    unsigned int GetMaterialTagsVersion() const;

    /// Marks geom subsets draw items dirty, meaning that the draw items 
    /// associated with the collection of a render pass need to be re-gathered.
    HDST_API
    void MarkGeomSubsetDrawItemsDirty();

    HDST_API
    unsigned int GetGeomSubsetDrawItemsVersion() const;

    // ---------------------------------------------------------------------- //
    /// Material tag tracking
    // ---------------------------------------------------------------------- /

    /// Does render index have rprims with given materialTag? Note
    /// that for performance reasons and ease of implementation
    /// (HdRprimSharedData::materialTag initializes to the default
    /// material tag), this always returns true for the default (and
    /// empty) material tag.
    HDST_API
    bool HasMaterialTag(const TfToken &materialTag) const;

    /// Register that there is an rprim with given materialTag.
    HDST_API
    void IncreaseMaterialTagCount(const TfToken &materialTag);
    
    /// Unregister that there is an rprim with given materialTag.
    HDST_API
    void DecreaseMaterialTagCount(const TfToken &materialTag);

    // ---------------------------------------------------------------------- //
    /// Render tag tracking
    // ---------------------------------------------------------------------- /

    /// Does render index have rprims with given renderTag?
    HDST_API
    bool HasAnyRenderTag(const TfTokenVector &renderTags) const;

    /// Register that there is an rprim with given renderTag.
    HDST_API
    void IncreaseRenderTagCount(const TfToken &renderTag);
    
    /// Unregister that there is an rprim with given renderTag.
    HDST_API
    void DecreaseRenderTagCount(const TfToken &renderTag);

    // ---------------------------------------------------------------------- //
    /// Draw targets.
    // ---------------------------------------------------------------------- //
    /// Marks all draw targets dirty, meaning that clients that keep track of
    /// the set of active draw targets needs to refresh that set.
    HDST_API
    void MarkActiveDrawTargetSetDirty();

    HDST_API
    unsigned int GetActiveDrawTargetSetVersion() const;

    // ---------------------------------------------------------------------- //
    /// Garbage collection tracking
    // ---------------------------------------------------------------------- //
    void SetGarbageCollectionNeeded() {
        _needsGarbageCollection = true;
    }

    void ClearGarbageCollectionNeeded() {
        _needsGarbageCollection = false;
    }

    bool IsGarbageCollectionNeeded() const {
        return _needsGarbageCollection;
    }
    
private:
    typedef std::unordered_map<TfToken, std::atomic_int, TfHash> _TagToCountMap;

    void _AdjustTagCount(
        std::shared_timed_mutex *mutex,
        _TagToCountMap *tagToCountMap,
        const TfToken &tag,
        const int increment);

    bool _HasTag(
        std::shared_timed_mutex *mutex,
        const _TagToCountMap *tagToCountMap,
        const TfToken &tag) const;

    std::atomic_uint _drawBatchesVersion;
    std::atomic_uint _materialTagsVersion;
    std::atomic_uint _geomSubsetDrawItemsVersion;
    std::atomic_uint _activeDrawTargetSetVersion;
    bool _needsGarbageCollection; // Doesn't need to be atomic since parallel
                                  // sync might only set it (and not clear).

    mutable std::shared_timed_mutex _materialTagToCountMutex;
    _TagToCountMap _materialTagToCount;

    mutable std::shared_timed_mutex _renderTagToCountMutex;
    _TagToCountMap _renderTagToCount;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_ST_RENDER_PARAM_H
