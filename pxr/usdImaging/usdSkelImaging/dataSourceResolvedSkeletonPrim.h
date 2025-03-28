//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_RESOLVED_SKELETON_PRIM_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_RESOLVED_SKELETON_PRIM_H

#include "pxr/imaging/hd/sceneIndex.h"

#include "pxr/usdImaging/usdSkelImaging/animationSchema.h"
#include "pxr/usdImaging/usdSkelImaging/dataSourceUtils.h"
#include "pxr/usdImaging/usdSkelImaging/skeletonSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdSkelImagingSkelData;
class UsdSkelImagingSkelGuideData;

/// \class UsdSkelImagingDataSourceResolvedSkeletonPrim
///
/// A data source providing data for the UsdSkelImagingResolvedSkeletonSchema
/// and for drawing the guide as a mesh.
///
/// Used by skeleton resolving scene index.
///
class UsdSkelImagingDataSourceResolvedSkeletonPrim
    : public HdContainerDataSource
    , public std::enable_shared_from_this<
                        UsdSkelImagingDataSourceResolvedSkeletonPrim>
{
public:
    HD_DECLARE_DATASOURCE(UsdSkelImagingDataSourceResolvedSkeletonPrim);

    USDSKELIMAGING_API
    ~UsdSkelImagingDataSourceResolvedSkeletonPrim();

    USDSKELIMAGING_API
    TfTokenVector GetNames() override;

    USDSKELIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    /// skelAnimation targeted by the skeleton. Used to track dependency
    /// of this prim on the skelAnimation.
    const SdfPath &GetAnimationSource() const {
        return _animationSource;
    }

    /// Schema from skelAnimation at GetAnimationSource().
    const UsdSkelImagingAnimationSchema &GetAnimationSchema() const {
        return _animationSchema;
    }

    /// Inverse transform matrix of this skeleton prim.
    HdMatrixDataSourceHandle GetSkelLocalToWorld() const;

    /// Skinning transforms.
    HdMatrix4fArrayDataSourceHandle GetSkinningTransforms();

    /// (Non-animated) skel data computed from this skeleton and the parts of
    /// skelAnimation relating to the topology/remapping.
    std::shared_ptr<UsdSkelImagingSkelData> GetSkelData() {
        return _skelDataCache.Get();
    }

    /// Some of the (non-animated) data to compute the points and topology
    /// for the mesh guide.
    std::shared_ptr<UsdSkelImagingSkelGuideData> GetSkelGuideData() {
        return _skelGuideDataCache.Get();
    }

    /// Data source locators (on this prim) that this prim depends on.
    ///
    /// That is, if the input scene sends a dirty entry for this prim path
    /// with dirty locators intersecting these data source locators, we need
    /// to call ProcessDirtyLocators.
    ///
    /// (Similar to dependendedOnDataSourceLocator in HdDependencySchema).
    ///
    static const HdDataSourceLocatorSet &
    GetDependendendOnDataSourceLocators();

    /// Dirty internal structures in response to dirty locators for
    /// skeleton prim  (dirtiedPrimType = "skeleton") or the targeted
    /// skelAnimaton prim (dirtiedPrimType = "skelAnimation").
    /// Fills dirtied prim entries with affected locators for this prim
    /// or returns true to indicate that we could not dirty this data
    /// source and need to refetch it.
    ///
    USDSKELIMAGING_API
    bool ProcessDirtyLocators(
        const TfToken &dirtiedPrimType,
        const HdDataSourceLocatorSet &dirtyLocators,
        HdSceneIndexObserver::DirtiedPrimEntries * entries);

private:
    USDSKELIMAGING_API
    UsdSkelImagingDataSourceResolvedSkeletonPrim(
        HdSceneIndexBaseRefPtr const &sceneIndex,
        const SdfPath &primPath,
        HdContainerDataSourceHandle const &primSource);

    bool _ProcessSkeletonDirtyLocators(
        const HdDataSourceLocatorSet &dirtyLocators,
        HdDataSourceLocatorSet * newDirtyLocators);

    bool _ProcessSkelAnimationDirtyLocators(
        const HdDataSourceLocatorSet &dirtyLocators,
        HdDataSourceLocatorSet * newDirtyLocators);

    // Path to this skeleton prim.
    const SdfPath _primPath;
    // Input data source for this skeleton prim.
    HdContainerDataSourceHandle const _primSource;
    // Path of skel animation prim.
    const SdfPath _animationSource;
    // Animation schema from skel animation prim.
    const UsdSkelImagingAnimationSchema _animationSchema;

    class _SkelDataCache
        : public UsdSkelImagingSharedPtrThunk<UsdSkelImagingSkelData>
    {
    public:
        _SkelDataCache(HdSceneIndexBaseRefPtr const &sceneIndex,
                       const SdfPath &primPath);
    protected:
        Handle _Compute() override;
    private:
        HdSceneIndexBaseRefPtr const _sceneIndex;
        const SdfPath _primPath;
    };
    _SkelDataCache _skelDataCache;

    class _SkelGuideDataCache
        : public UsdSkelImagingSharedPtrThunk<UsdSkelImagingSkelGuideData>
    {
    public:
        _SkelGuideDataCache(
            UsdSkelImagingDataSourceResolvedSkeletonPrim * resolvedSkeleton);
    protected:
        Handle _Compute() override;
    private:
        UsdSkelImagingDataSourceResolvedSkeletonPrim * const _resolvedSkeleton;
    };
    _SkelGuideDataCache _skelGuideDataCache;

    // Converts rest transforms to VtArray<GfMatrix4f>.
    //
    // Note that rest transforms is only needed if there is no animation or the
    // animation is not sparse. Thus, this data source lazily reads it from the
    // skeleton schema.
    //
    class _RestTransformsDataSource;
    std::shared_ptr<_RestTransformsDataSource> const _restTransformsDataSource;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdSkelImagingDataSourceResolvedSkeletonPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
