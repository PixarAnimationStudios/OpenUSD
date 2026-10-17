//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_BACK_PLATE_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_BACK_PLATE_SCENE_INDEX_H
 
#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/backPlateSchema.h"
#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE
 
TF_DECLARE_WEAK_AND_REF_PTRS(HdsiBackPlateSceneIndex);

/// \class HdsiBackPlateSceneIndex
///
/// The back plate scene index converts back plate prims into mesh and material
/// prims that align with the specifications outlined in:
/// https://github.com/PixarAnimationStudios/OpenUSD-proposals/tree/main/proposals/back-plates
/// 
class HdsiBackPlateSceneIndex :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override final;
 
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override final;
 
protected:
    HDSI_API
    virtual HdContainerDataSourceHandle
    _BuildMeshPrimDataSource(
        const SdfPath &platePath,
        const TfToken &instanceName,
        const HdContainerDataSourceHandle &primDataSource) const = 0;

    HDSI_API
    virtual HdContainerDataSourceHandle
    _BuildMaterialDataSource(
        const TfToken &instanceName,
        const HdContainerDataSourceHandle &primDataSource) const = 0;
    
    HDSI_API
    virtual HdDataSourceLocatorSet
    _GetAdditionalDirtyLocators() const;
    
    HDSI_API
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override final;
 
    HDSI_API
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override final;
 
    HDSI_API
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override final;

    static HdBackPlateSchema 
    _GetBackPlateSchema(TfToken const &instanceName, 
                        HdContainerDataSourceHandle const &primDataSource)
    {
        HdCameraSchema cam = HdCameraSchema::GetFromParent(primDataSource);
        HdBackPlateSchema backplate = cam.GetBackPlate().Get(instanceName);
        return backplate;
    }

    template <typename DataSourceHandle, typename T>
    static bool
    _GetDefaultTypedValue(const DataSourceHandle &dataSource, 
                        HdSampledDataSource::Time const shutterOffset,
                        T* result)
    {
        if (!dataSource || !result) {
            return false;
        }
        *result = dataSource->GetTypedValue(shutterOffset);
        return true;
    }

    HDSI_API
    static GfVec2f
    _ComputeEffectiveDimensions(
        TfToken const &instanceName,
        HdContainerDataSourceHandle const &primDataSource,
        HdSampledDataSource::Time const shutterOffset) ;

    HDSI_API
    static GfMatrix4d 
    _ComputeLocalXform(HdContainerDataSourceHandle const &primDataSource,
        TfToken const &instanceName,
        HdSampledDataSource::Time const shutterOffset);
 
    HDSI_API
    HdsiBackPlateSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

private:
    void
    _RemoveBackPlateChildren(
        const SdfPath &primPath,
        SdfPathSet * const removedBackPlatePrims);
    void
    _AddBackPlateChildren(
        const SdfPath &primPath,
        SdfPathSet * const addedBackPlatePrims);
    void
    _DirtyBackPlateChildren(
        const SdfPath &primPath,
        HdSceneIndexObserver::DirtiedPrimEntries * const 
            dirtiedBackPlatePrims);

    HdSceneIndexPrim
    _GetMeshOrMaterialSiPrim(
        const SdfPath &plateInstancePath,
        const TfToken &plateChildType) const;
    
    bool
    _IsBackPlateInMap(
        const SdfPath &platePath) const;

    bool
    _HasTrackedBackPlates() const;

    using _BackPlates = std::unordered_set<TfToken,TfToken::HashFunctor>;
    using _CameraToBackPlatesMap = std::map<SdfPath, _BackPlates>;

    // Maps the camera prim path to all the names of the back plate instances 
    // associated with it. 
    _CameraToBackPlatesMap _cameraToBackPlates;
};
 
PXR_NAMESPACE_CLOSE_SCOPE
 
#endif //PXR_IMAGING_HDSI_BACK_PLATE_SCENE_INDEX_H