//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_SKELETON_RESOLVING_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_SKELETON_RESOLVING_SCENE_INDEX_H

#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdSkelImagingSkeletonResolvingSceneIndex);

/// \class UsdSkelImagingSkeletonResolvingSceneIndex
///
/// For each skeleton prim in the input scene index, populate the
/// UsdSkelImagingResolvedSkeletonSchema. It also changes the prim type
/// to mesh and populates the necessary data for the mesh to serve as guide.
///
class UsdSkelImagingSkeletonResolvingSceneIndex
            : public HdSingleInputFilteringSceneIndexBase
{
public:
    USDSKELIMAGING_API
    static
    UsdSkelImagingSkeletonResolvingSceneIndexRefPtr
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
    UsdSkelImagingSkeletonResolvingSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex);

    // Helper to process dirtied prim entries.
    void _ProcessDirtyLocators(
        const SdfPath &skelPath,
        const TfToken &dirtiedPrimType,
        const HdDataSourceLocatorSet &dirtyLocators,
        HdSceneIndexObserver::DirtiedPrimEntries * entries);

    // Prim data source returned by this scene index for a skeleton prim.
    using _DsHandle =
        std::shared_ptr<class UsdSkelImagingDataSourceResolvedSkeletonPrim>;

    // See whether prim at path is a skeleton. If yes, add the resolved
    // skeleton data source to internal data structures including its
    /// dependencies - and return true.
    bool _AddResolvedSkeleton(
        const SdfPath &path);
    // Add dependencies for skeleton at given path with given data source.
    void _AddDependenciesForResolvedSkeleton(
        const SdfPath &skeletonPath,
        _DsHandle const &resolvedSkeleton);

    // See whether there was a skeleton registered at given path. If yes,
    // remove it including its dependencies - and return true.
    bool _RemoveResolvedSkeleton(
        const SdfPath &path);
    // Remove dependencies.
    void _RemoveDependenciesForResolvedSkeleton(
        const SdfPath &skeletonPath,
        _DsHandle const &resolvedSkeleton);

    // Refreshes data source without updating dependencies.
    //
    // Note that this repulls the data sources from the dependencies
    // but does not update the paths we depend on.
    //
    void _RefreshResolvedSkeletonDataSource(
        const SdfPath &skeletonPath);

    // For each skeleton in input scene, the resolved data source.
    // The scene index overlays it with the input data source.
    std::map<SdfPath, _DsHandle> _pathToResolvedSkeleton;
    // Prims targeted as animation by skeletons to skeletons.
    std::map<SdfPath, SdfPathSet> _skelAnimPathToSkeletonPaths;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
