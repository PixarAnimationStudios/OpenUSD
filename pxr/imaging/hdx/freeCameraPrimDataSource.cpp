//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/freeCameraPrimDataSource.h"

#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (windowPolicy)
);

namespace HdxFreeCameraPrimDataSource_Impl {

struct _Info
{
    GfCamera camera;
    CameraUtilConformWindowPolicy policy;
};

HdDataSourceBaseHandle
_ToDataSource(const GfCamera::Projection p)
{
    switch(p) {
    case GfCamera::Perspective:
        return HdCameraSchema::BuildProjectionDataSource(
            HdCameraSchemaTokens->perspective);
    case GfCamera::Orthographic:
        return HdCameraSchema::BuildProjectionDataSource(
            HdCameraSchemaTokens->orthographic);
    }
    // Make compiler happy.
    return nullptr;
}

class _CameraSchemaDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_CameraSchemaDataSource);

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdCameraSchemaTokens->projection) {
            return _ToDataSource(_Camera().GetProjection());
        }
        if (name == HdCameraSchemaTokens->horizontalAperture) {
            return HdRetainedTypedSampledDataSource<float>::New(
                _Camera().GetHorizontalAperture() /
                GfCamera::APERTURE_UNIT);
        }
        if (name == HdCameraSchemaTokens->verticalAperture) {
            return HdRetainedTypedSampledDataSource<float>::New(
                _Camera().GetVerticalAperture() /
                GfCamera::APERTURE_UNIT);
        }
        if (name == HdCameraSchemaTokens->horizontalApertureOffset) {
            return HdRetainedTypedSampledDataSource<float>::New(
                _Camera().GetHorizontalApertureOffset() /
                GfCamera::APERTURE_UNIT);
        }
        if (name == HdCameraSchemaTokens->verticalApertureOffset) {
            return HdRetainedTypedSampledDataSource<float>::New(
                _Camera().GetVerticalApertureOffset() /
                GfCamera::APERTURE_UNIT);
        }
        if (name == HdCameraSchemaTokens->focalLength) {
            return HdRetainedTypedSampledDataSource<float>::New(
                _Camera().GetFocalLength() /
                GfCamera::FOCAL_LENGTH_UNIT);
        }
        if (name == HdCameraSchemaTokens->clippingRange) {
            const GfRange1f &clippingRange = _Camera().GetClippingRange();
            return HdRetainedTypedSampledDataSource<GfVec2f>::New(
                { clippingRange.GetMin(), clippingRange.GetMax() });
        }
        if (name == HdCameraSchemaTokens->clippingPlanes) {
            const std::vector<GfVec4f> &clippingPlanes =
                _Camera().GetClippingPlanes();
            if (clippingPlanes.empty()) {
                return nullptr;
            }
            return HdRetainedTypedSampledDataSource<VtArray<GfVec4d>>::New(
                VtArray<GfVec4d>(clippingPlanes.begin(), clippingPlanes.end()));
        }
        if (name == HdCameraSchemaTokens->fStop) {
            return HdRetainedTypedSampledDataSource<float>::New(
                _Camera().GetFStop());
        }
        if (name == HdCameraSchemaTokens->focusDistance) {
            return HdRetainedTypedSampledDataSource<float>::New(
                _Camera().GetFocusDistance());
        }
        if (name == _tokens->windowPolicy) {
            return
                HdRetainedTypedSampledDataSource<
                    CameraUtilConformWindowPolicy>::
                New(_info->policy);
        }
        return nullptr;
    }

    TfTokenVector GetNames() override {
        static const TfTokenVector result = {
            HdCameraSchemaTokens->projection,
            HdCameraSchemaTokens->horizontalAperture,
            HdCameraSchemaTokens->verticalAperture,
            HdCameraSchemaTokens->horizontalApertureOffset,
            HdCameraSchemaTokens->verticalApertureOffset,
            HdCameraSchemaTokens->focalLength,
            HdCameraSchemaTokens->clippingRange,
            HdCameraSchemaTokens->clippingPlanes,
            HdCameraSchemaTokens->fStop,
            HdCameraSchemaTokens->focusDistance,
            _tokens->windowPolicy
        };
        return result;
    }

private:
    _CameraSchemaDataSource(std::shared_ptr<_Info> const &info)
     : _info(info)
    {
    }

    const GfCamera &_Camera() const { return _info->camera; }
    
    std::shared_ptr<_Info> const _info;
};

GfCamera
_CameraFromViewAndProjectionMatrix(
    const GfMatrix4d& viewMatrix,
    const GfMatrix4d& projectionMatrix)
{
    GfCamera cam;
    cam.SetFromViewAndProjectionMatrix(viewMatrix, projectionMatrix);
    return cam;
}

}

using namespace HdxFreeCameraPrimDataSource_Impl;

HdxFreeCameraPrimDataSource::HdxFreeCameraPrimDataSource(
    const GfCamera &camera,
    const CameraUtilConformWindowPolicy policy)
 : _info(
     std::make_shared<_Info>(
         _Info{std::move(camera), policy}))
{
}

HdxFreeCameraPrimDataSource::HdxFreeCameraPrimDataSource(
    const GfMatrix4d& viewMatrix,
    const GfMatrix4d& projectionMatrix,
    const CameraUtilConformWindowPolicy policy)
 : HdxFreeCameraPrimDataSource(
     _CameraFromViewAndProjectionMatrix(
         viewMatrix, projectionMatrix),
     policy)
{
}

HdxFreeCameraPrimDataSource::~HdxFreeCameraPrimDataSource() = default;

void
HdxFreeCameraPrimDataSource::
SetCamera(
    const GfCamera &camera,
    HdDataSourceLocatorSet * const dirtyLocators)
{
    if (_info->camera.GetTransform() != camera.GetTransform()) {
        _info->camera.SetTransform(camera.GetTransform());
        if (dirtyLocators) {
            dirtyLocators->insert(HdXformSchema::GetDefaultLocator());
        }
    }
    if (_info->camera.GetProjection() != camera.GetProjection()) {
        _info->camera.SetProjection(camera.GetProjection());
        if (dirtyLocators) {
            static const HdDataSourceLocator locator =
                HdCameraSchema::GetDefaultLocator()
                    .Append(HdCameraSchemaTokens->projection);
            dirtyLocators->insert(locator);
        }
    }
    if (_info->camera.GetHorizontalAperture() !=
                    camera.GetHorizontalAperture()) {
        _info->camera.SetHorizontalAperture(
                    camera.GetHorizontalAperture());
        if (dirtyLocators) {
            static const HdDataSourceLocator locator =
                HdCameraSchema::GetDefaultLocator()
                    .Append(HdCameraSchemaTokens->horizontalAperture);
            dirtyLocators->insert(locator);
        }
    }
    if (_info->camera.GetVerticalAperture() !=
                    camera.GetVerticalAperture()) {
        _info->camera.SetVerticalAperture(
                    camera.GetVerticalAperture());
        if (dirtyLocators) {
            static const HdDataSourceLocator locator =
                HdCameraSchema::GetDefaultLocator()
                    .Append(HdCameraSchemaTokens->verticalAperture);
            dirtyLocators->insert(locator);
        }
    }
    if (_info->camera.GetHorizontalApertureOffset() !=
                    camera.GetHorizontalApertureOffset()) {
        _info->camera.SetHorizontalApertureOffset(
                    camera.GetHorizontalApertureOffset());
        if (dirtyLocators) {
            static const HdDataSourceLocator locator =
                HdCameraSchema::GetDefaultLocator()
                    .Append(HdCameraSchemaTokens->horizontalApertureOffset);
            dirtyLocators->insert(locator);
        }
    }
    if (_info->camera.GetVerticalApertureOffset() !=
                    camera.GetVerticalApertureOffset()) {
        _info->camera.SetVerticalApertureOffset(
                    camera.GetVerticalApertureOffset());
        if (dirtyLocators) {
            static const HdDataSourceLocator locator =
                HdCameraSchema::GetDefaultLocator()
                    .Append(HdCameraSchemaTokens->verticalApertureOffset);
            dirtyLocators->insert(locator);
        }
    }
    if (_info->camera.GetFocalLength() != camera.GetFocalLength()) {
        _info->camera.SetFocalLength(camera.GetFocalLength());
        if (dirtyLocators) {
            static const HdDataSourceLocator locator =
                HdCameraSchema::GetDefaultLocator()
                    .Append(HdCameraSchemaTokens->focalLength);
            dirtyLocators->insert(locator);
        }
    }
    if (_info->camera.GetClippingRange() != camera.GetClippingRange()) {
        _info->camera.SetClippingRange(camera.GetClippingRange());
        if (dirtyLocators) {
            static const HdDataSourceLocator locator =
                HdCameraSchema::GetDefaultLocator()
                    .Append(HdCameraSchemaTokens->clippingRange);
            dirtyLocators->insert(locator);
        }
    }
    if (_info->camera.GetClippingPlanes() != camera.GetClippingPlanes()) {
        _info->camera.SetClippingPlanes(camera.GetClippingPlanes());
        if (dirtyLocators) {
            static const HdDataSourceLocator locator =
                HdCameraSchema::GetDefaultLocator()
                    .Append(HdCameraSchemaTokens->clippingPlanes);
            dirtyLocators->insert(locator);
        }
    }
    if (_info->camera.GetFStop() != camera.GetFStop()) {
        _info->camera.SetFStop(camera.GetFStop());
        if (dirtyLocators) {
            static const HdDataSourceLocator locator =
                HdCameraSchema::GetDefaultLocator()
                    .Append(HdCameraSchemaTokens->fStop);
            dirtyLocators->insert(locator);
        }
    }
    if (_info->camera.GetFocusDistance() != camera.GetFocusDistance()) {
        _info->camera.SetFocusDistance(camera.GetFocusDistance());
        if (dirtyLocators) {
            static const HdDataSourceLocator locator =
                HdCameraSchema::GetDefaultLocator()
                    .Append(HdCameraSchemaTokens->focusDistance);
            dirtyLocators->insert(locator);
        }
    }
}

void
HdxFreeCameraPrimDataSource::SetWindowPolicy(
    const CameraUtilConformWindowPolicy policy,
    HdDataSourceLocatorSet * dirtyLocators)
{
    if (_info->policy == policy) {
        return;
    }
    _info->policy = policy;

    if (!dirtyLocators) {
        return;
    }
    static const HdDataSourceLocator locator =
        HdCameraSchema::GetDefaultLocator()
            .Append(_tokens->windowPolicy);                    
    dirtyLocators->insert(locator);
}

void
HdxFreeCameraPrimDataSource::SetViewAndProjectionMatrix(
    const GfMatrix4d& viewMatrix,
    const GfMatrix4d& projectionMatrix,
    HdDataSourceLocatorSet * dirtyLocators)
{
    SetCamera(
        _CameraFromViewAndProjectionMatrix(
            viewMatrix, projectionMatrix),
        dirtyLocators);
}

HdDataSourceBaseHandle
HdxFreeCameraPrimDataSource::Get(const TfToken &name) {
    if (name == HdCameraSchema::GetSchemaToken()) {
        return _CameraSchemaDataSource::New(_info);
    }
    if (name == HdXformSchema::GetSchemaToken()) {
        return
            HdXformSchema::Builder()
                .SetResetXformStack(
                    HdRetainedTypedSampledDataSource<bool>::New(
                        true))
                .SetMatrix(
                    HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                        _info->camera.GetTransform()))
            .Build();
    }
    return nullptr;
}

TfTokenVector
HdxFreeCameraPrimDataSource::GetNames() {
    static const TfTokenVector result = {
        HdCameraSchema::GetSchemaToken(),
        HdXformSchema::GetSchemaToken() };
    return result;
}
    
PXR_NAMESPACE_CLOSE_SCOPE

