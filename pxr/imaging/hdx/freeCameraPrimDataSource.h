//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_FREE_CAMERA_DATA_SOURCE_H
#define PXR_IMAGING_HDX_FREE_CAMERA_DATA_SOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"
#include "pxr/imaging/hd/dataSource.h"

#include "pxr/base/gf/camera.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace HdxFreeCameraPrimDataSource_Impl {
struct _Info;
}

/// \class HdxFreeCameraPrimDataSource
///
/// A data source conforming to the HdCameraSchema and HdXformSchema
/// populated from a GfCamera or camera matrices and a window policy.
///
/// It is intended to replace the HdxFreeCameraSceneDelegate.
///
class HdxFreeCameraPrimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(HdxFreeCameraPrimDataSource);

    HDX_API
    ~HdxFreeCameraPrimDataSource() override;

    HDX_API
    void SetCamera(
        const GfCamera &camera,
        HdDataSourceLocatorSet * dirtyLocators = nullptr);

    HDX_API
    void SetWindowPolicy(
        CameraUtilConformWindowPolicy policy,
        HdDataSourceLocatorSet * dirtyLocators = nullptr);

    HDX_API
    void SetViewAndProjectionMatrix(
        const GfMatrix4d& viewMatrix,
        const GfMatrix4d& projectionMatrix,
        HdDataSourceLocatorSet * dirtyLocators = nullptr);

    HDX_API
    void SetClippingPlanes(
        const std::vector<GfVec4f> &clippingPlanes,
        HdDataSourceLocatorSet * dirtyLocators = nullptr);

    HDX_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    HDX_API
    TfTokenVector GetNames() override;

private:
    HDX_API
    HdxFreeCameraPrimDataSource(
        const GfCamera &camera = GfCamera(),
        CameraUtilConformWindowPolicy policy = CameraUtilFit);

    HDX_API
    HdxFreeCameraPrimDataSource(
        const GfMatrix4d& viewMatrix,
        const GfMatrix4d& projectionMatrix,
        CameraUtilConformWindowPolicy policy = CameraUtilFit);

    std::shared_ptr<HdxFreeCameraPrimDataSource_Impl::_Info> const _info;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_FREE_CAMERA_DATA_SOURCE_H
