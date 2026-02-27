//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_RESOLVED_POINTS_BASED_PRIM_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_RESOLVED_POINTS_BASED_PRIM_H

#include "pxr/usdImaging/usdSkelImaging/api.h"
#include "pxr/usdImaging/usdSkelImaging/dataSourceUtils.h"
#include "pxr/usdImaging/usdSkelImaging/resolvedSkeletonSchema.h"
#include "pxr/usdImaging/usdSkelImaging/xformResolver.h"

#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"

#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdSkelImagingBlendShapeData;
struct UsdSkelImagingJointInfluencesData;
TF_DECLARE_REF_PTRS(HdSceneIndexBase);

/// \class UsdSkelImagingDataSourceResolvedPointsBasedPrim
///
/// A prim data source providing resolved data for a points based prim (mesh,
/// basisCurves, points) deformed by a skeleton.
/// As a data source, it populates the HdExtComputationPrimvarsSchema for points
/// and possibly for normals and removes points and normals from the
/// HdPrimvarsSchema.
///
/// Used by the UsdSkelImagingPointsResolvingSceneIndex in conjunction with the
/// UsdSkelImagingDataSourceResolvedExtComputationPrim.
///
class UsdSkelImagingDataSourceResolvedPointsBasedPrim
    : public HdContainerDataSource
    , public std::enable_shared_from_this<
                        UsdSkelImagingDataSourceResolvedPointsBasedPrim>
{
public:
    HD_DECLARE_DATASOURCE(UsdSkelImagingDataSourceResolvedPointsBasedPrim);

    /// C'tor.
    ///
    /// Note that it takes the data source for the prim at primPath in the
    /// given sceneIndex. This is for performance: the client probably already
    /// retrieved this data source so we want to avoid looking it up again here.
    ///
    /// Returns a nullptr if there the prim in the input scene does not bind a
    /// skeleton.
    USDSKELIMAGING_API
    static
    Handle New(
        HdSceneIndexBaseRefPtr const &sceneIndex,
        SdfPath primPath,
        HdContainerDataSourceHandle primSource);

    USDSKELIMAGING_API
    ~UsdSkelImagingDataSourceResolvedPointsBasedPrim();

    USDSKELIMAGING_API
    TfTokenVector GetNames() override;

    USDSKELIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    /// Path of prim in input scene (and for prim this data source is for).
    const SdfPath &GetPrimPath() const { return _primPath; }

    /// Path of bound skeleton.
    const SdfPath &GetSkeletonPath() const { return _skeletonPath; }

    /// Paths to BlendShape prims.
    const VtArray<SdfPath> &GetBlendShapeTargetPaths() const {
        return _blendShapeTargetPaths;
    }

    /// Paths to instancers instancing this prim - not including ones
    /// outside the skel root.
    ///
    /// See UsdSkelImagingDataSourceXformResolver for details.
    ///
    const VtArray<SdfPath> &GetInstancerPaths() const {
        return _xformResolver.GetInstancerPaths();
    }
    
    /// Primvars of prim in the input scene.
    const HdPrimvarsSchema &GetPrimvars() const { return _primvars; }

    /// Resolved skeleton of prim in the input scene.
    const UsdSkelImagingResolvedSkeletonSchema &GetResolvedSkeletonSchema() {
        return _resolvedSkeletonSchema;
    }

    USDSKELIMAGING_API
    HdMatrix4fArrayDataSourceHandle GetSkinningTransforms();

    /// Only valid if GetSkinningMethod() == UsdSkelTokens->dualQuaternion
    USDSKELIMAGING_API
    HdMatrix3fArrayDataSourceHandle GetSkinningScaleTransforms();

    /// Only valid if GetSkinningMethod() == UsdSkelTokens->dualQuaternion
    USDSKELIMAGING_API
    HdVec4fArrayDataSourceHandle GetSkinningDualQuats();

    USDSKELIMAGING_API
    HdFloatArrayDataSourceHandle GetBlendShapeWeights();

    /// Transfrom to go from common space (as defined by
    /// UsdSkelImagingDataSourceXformResolver) to the local space of this
    /// prim.
    USDSKELIMAGING_API
    HdMatrixDataSourceHandle GetCommonSpaceToPrimLocal() const;

    USDSKELIMAGING_API
    HdSampledDataSourceHandle GetPoints();

    USDSKELIMAGING_API
    HdDataSourceBaseHandle GetGeomBindTransform();

    USDSKELIMAGING_API
    HdDataSourceBaseHandle GetHasConstantInfluences();

    USDSKELIMAGING_API
    HdDataSourceBaseHandle GetNumInfluencesPerComponent();

    USDSKELIMAGING_API
    HdDataSourceBaseHandle GetInfluences();

    USDSKELIMAGING_API
    HdDataSourceBaseHandle GetBlendShapeOffsets();

    USDSKELIMAGING_API
    HdDataSourceBaseHandle GetBlendShapeOffsetRanges();

    USDSKELIMAGING_API
    HdDataSourceBaseHandle GetNumBlendShapeOffsetRanges();

    USDSKELIMAGING_API
    HdSampledDataSourceHandle GetNormals() const;

    USDSKELIMAGING_API
    HdSampledDataSourceHandle GetFaceVertexIndices();

    USDSKELIMAGING_API
    HdDataSourceBaseHandle GetHasFaceVaryingNormals();

    /// Blend shape data computed from primvars, skel bindings and skeleton.
    USDSKELIMAGING_API
    std::shared_ptr<UsdSkelImagingBlendShapeData> GetBlendShapeData();

    /// Joint influences data computed from primvars.
    USDSKELIMAGING_API
    std::shared_ptr<UsdSkelImagingJointInfluencesData> GetJointInfluencesData();

    /// Skinning method computed from corresponding primvar.
    const TfToken &GetSkinningMethod() const {
        return _skinningMethod;
    }

    /// Should the points for this primvar be given by an ext computation
    /// or from the primvars schema.
    bool HasExtComputations() const;

    /// Should the normals for this primvar be given by an ext computation.
    bool HasNormalsExtComputations() const;

    /// Data source locators (on this prim) that this prim depends on.
    ///
    /// That is, if the input scene sends a dirty entry for this prim path
    /// with dirty locators intersecting these data source locators, we need
    /// to call ProcessDirtyLocators.
    ///
    /// (Similar to dependendedOnDataSourceLocator in HdDependencySchema).
    ///
    USDSKELIMAGING_API
    static const HdDataSourceLocatorSet &
    GetDependendendOnDataSourceLocators();

    /// Dirty internal structures in response to dirty locators for the
    /// target (resolved) skeleton prim  (dirtiedPrimType = "skeleton"), a
    /// targeted skelBlendShape prim (dirtiedPrimType = "skelBlendShape") or
    /// the prim in the input scene itself (any other dirtiedPrimType).
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
    UsdSkelImagingDataSourceResolvedPointsBasedPrim(
        HdSceneIndexBaseRefPtr const &sceneIndex,
        SdfPath primPath,
        HdContainerDataSourceHandle primSource,
        bool hasSkelRoot,
        VtArray<SdfPath> blendShapeTargetPaths,
        SdfPath skelPath,
        HdContainerDataSourceHandle skeletonPrimSource,
        UsdSkelImagingResolvedSkeletonSchema resolvedSkeletonSchema);

    bool
    _ProcessDirtyLocators(
        const HdDataSourceLocatorSet &dirtyLocators,
        HdDataSourceLocatorSet * dirtyLocatorsForAggregatorComputation,
        HdDataSourceLocatorSet * dirtyLocatorsForComputation,
        TfTokenVector * dirtyPrimvars);

    bool
    _ProcessDirtySkeletonLocators(
        const HdDataSourceLocatorSet &dirtyLocators,
        HdDataSourceLocatorSet * dirtyLocatorsForAggregatorComputation,
        HdDataSourceLocatorSet * dirtyLocatorsForComputation,
        TfTokenVector * dirtyPrimvars);

    bool
    _ProcessDirtySkelBlendShapeLocators(
        const HdDataSourceLocatorSet &dirtyLocators,
        HdDataSourceLocatorSet * dirtyLocatorsForAggregatorComputation,
        HdDataSourceLocatorSet * dirtyLocatorsForComputation,
        TfTokenVector * dirtyPrimvars);

    bool
    _ProcessDirtyInstancerLocators(
        const HdDataSourceLocatorSet &dirtyLocators,
        HdDataSourceLocatorSet * dirtyLocatorsForAggregatorComputation,
        HdDataSourceLocatorSet * dirtyLocatorsForComputation);
    
    // Input scene.
    HdSceneIndexBaseRefPtr const _sceneIndex;
    // Path of prim in the input scene.
    const SdfPath _primPath;
    // Data source for _primPath from input scene _sceneIndex.
    HdContainerDataSourceHandle const _primSource;
    const bool _hasSkelRoot;
    // From prim at _primPath in input scene _sceneIndex.
    HdPrimvarsSchema const _primvars;
    HdMeshSchema const _mesh;
    const TfToken _skinningMethod;
    VtArray<SdfPath> _blendShapeTargetPaths;
    const SdfPath _skeletonPath;
    HdContainerDataSourceHandle const _skeletonPrimSource;
    const UsdSkelImagingResolvedSkeletonSchema _resolvedSkeletonSchema;

    class _BlendShapeDataCache
        : public UsdSkelImagingSharedPtrThunk<UsdSkelImagingBlendShapeData>
    {
    public:
        _BlendShapeDataCache(
            HdSceneIndexBaseRefPtr const &sceneIndex,
            const SdfPath &primPath);
    protected:
        Handle _Compute() override;
    private:
        HdSceneIndexBaseRefPtr const _sceneIndex;
        const SdfPath _primPath;
    };
    _BlendShapeDataCache _blendShapeDataCache;

    class _JointInfluencesDataCache
        : public UsdSkelImagingSharedPtrThunk<UsdSkelImagingJointInfluencesData>
    {
    public:
        _JointInfluencesDataCache(
            HdContainerDataSourceHandle const &primSource,
            HdContainerDataSourceHandle const &skeletonPrimSource);
    protected:
        Handle _Compute() override;
    private:
        HdContainerDataSourceHandle const _primSource;
        HdContainerDataSourceHandle const _skeletonPrimSource;
    };
    _JointInfluencesDataCache _jointInfluencesDataCache;

    // Serves GetPrimWorldToLocal - taking instancing into account.
    UsdSkelImagingDataSourceXformResolver _xformResolver;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdSkelImagingDataSourceResolvedPointsBasedPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
