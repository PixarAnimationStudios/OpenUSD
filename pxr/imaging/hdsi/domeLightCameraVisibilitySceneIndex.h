//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HDSI_DOME_LIGHT_CAMERA_VISIBILITY_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_DOME_LIGHT_CAMERA_VISIBILITY_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hdsi/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDSI_DOME_LIGHT_CAMERA_VISIBILITY_SCENE_INDEX_TOKENS \
    (cameraVisibility)

TF_DECLARE_PUBLIC_TOKENS(
    HdsiDomeLightCameraVisibilitySceneIndexTokens, HDSI_API,
    HDSI_DOME_LIGHT_CAMERA_VISIBILITY_SCENE_INDEX_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiDomeLightCameraVisibilitySceneIndex);

namespace HdsiDomeLightCameraVisibilitySceneIndex_Impl
{
class _CameraVisibilityDataSource;
using _CameraVisibilityDataSourceHandle =
    std::shared_ptr<_CameraVisibilityDataSource>;
}

/// \class HdsiDomeLightCameraVisibilitySceneIndex
///
/// Scene Index that overrides the cameraVisibility of each dome light.
///
/// More precisely, it overrrides the bool data source at locator
/// light:cameraVisibility for each prim of type domeLight.
///
class HdsiDomeLightCameraVisibilitySceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiDomeLightCameraVisibilitySceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex);

    HDSI_API
    void SetDomeLightCameraVisibility(bool visibility);

public:
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    HDSI_API
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    HDSI_API
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;
    HDSI_API
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

protected:
    HDSI_API
    HdsiDomeLightCameraVisibilitySceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex);
    HDSI_API
    ~HdsiDomeLightCameraVisibilitySceneIndex() override;

private:
    HdsiDomeLightCameraVisibilitySceneIndex_Impl::
    _CameraVisibilityDataSourceHandle const _cameraVisibilityDataSource;

    SdfPathSet _domeLightPaths;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
