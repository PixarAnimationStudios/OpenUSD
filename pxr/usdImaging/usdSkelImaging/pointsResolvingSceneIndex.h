//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_POINTS_RESOLVING_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_POINTS_RESOLVING_SCENE_INDEX_H

#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdSkelImagingPointsResolvingSceneIndex);

/// \class UsdSkelImagingPointsResolvingSceneIndex
///
/// Adds ext computations to skin to points of a mesh, point, basisCurves prims.
/// It uses the prim from the input scene, the targeted skelBlendShape's as well
/// as the resolved skeleton schema from the targeted skeleton.
///
/// Thus, this scene index has to run after the
/// UsdSkelImagingSkeletonResolvingSceneIndex.
///
class UsdSkelImagingPointsResolvingSceneIndex
            : public HdSingleInputFilteringSceneIndexBase
{
public:
    USDSKELIMAGING_API
    static
    UsdSkelImagingPointsResolvingSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex);

    USDSKELIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    USDSKELIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    void _PrimsAdded(
        const HdSceneIndexBase&,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase&,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase&,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

private:
    UsdSkelImagingPointsResolvingSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex);

    // Helper to reconstruct the data source stored for a particular prim
    // in _pathToResolvedPrim, update dependencies in _skelPathToPrimPaths
    // and _blendShapePathToPrimPaths and fill in dirty notifications.
    //
    // The input are prim paths with an annotation (stored in a map). If the
    // annotation is true, we do not fill the dirty notifcation for the prim
    // itself.
    //
    void _ProcessPrimsNeedingRefreshAndSendNotices(
        const std::map<SdfPath, bool> &primsNeedingRefreshToHasAddedEntry,
        HdSceneIndexObserver::AddedPrimEntries * addedEntries,
        HdSceneIndexObserver::RemovedPrimEntries * removedEntries,
        HdSceneIndexObserver::DirtiedPrimEntries * dirtiedEntries);

    // Helper to process dirtied prim entries.
    bool _ProcessDirtyLocators(
        const SdfPath &primPath,
        const TfToken &dirtiedPrimType,
        const HdDataSourceLocatorSet &dirtyLocators,
        HdSceneIndexObserver::DirtiedPrimEntries * entries);

    using _DsHandle =
        std::shared_ptr<class UsdSkelImagingDataSourceResolvedPointsBasedPrim>;

    // Query input scene for prim at path. If that prim is potentially
    // affected by a skeleton, construct the resolving data source,
    // store it, update the dependencies.
    bool _AddResolvedPrim(
        const SdfPath &path,
        bool * hasExtComputations = nullptr);
    void _AddDependenciesForResolvedPrim(
        const SdfPath &primPath,
        _DsHandle const &resolvedPrim);

    // Remove from _pathToResolvedPrim and dependencies.
    bool _RemoveResolvedPrim(
        const SdfPath &path,
        bool * hadExtComputations = nullptr);
    void _RemoveDependenciesForResolvedPrim(
        const SdfPath &primPath,
        _DsHandle const &resolvedPrim);

    // Refetch data source from input scene and refresh resolved data source in
    // _pathToResolvedPrim. This does not update the dependencies.
    //
    // Call this if refetching the data source is necessary, but the paths to
    // the skeleton and blend shapes have not changed.
    void _RefreshResolvedPrimDataSource(
        const SdfPath &primPath,
        bool * hasExtComputations);
    // Refetch data source as above - filling the dirty notications.
    void _RefreshResolvedPrimDataSources(
        const SdfPathSet &primPaths,
        HdSceneIndexObserver::DirtiedPrimEntries * entries,
        SdfPathSet * addedResolvedPrimsWithComputations,
        SdfPathSet * removedResolvedPrimsWithComputations);

    // For each mesh, point, basisCurve in the input scene that has a bound
    // skeleton (even if the prim at the targeted path is not a Skeleton or
    // empty), store the resolved data source.
    //
    // This scene index overlays it with the input data source.
    //
    std::map<SdfPath, _DsHandle> _pathToResolvedPrim;

    // Path of a skeleton to paths of resolved prim's depending on that
    // skeleton.
    std::map<SdfPath, SdfPathSet> _skelPathToPrimPaths;
    // Same for blend shapes.
    std::map<SdfPath, SdfPathSet> _blendShapePathToPrimPaths;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
