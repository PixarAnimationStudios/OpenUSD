//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdx/freeCameraSceneDelegate.h"

#include "pxr/imaging/hd/camera.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (camera)
);

static
SdfPath
_ComputeCameraId(
    HdRenderIndex *renderIndex,
    SdfPath const &delegateId)
{
    if (!renderIndex->IsSprimTypeSupported(HdPrimTypeTokens->camera)) {
        return SdfPath();
    }
    return delegateId.AppendChild(_tokens->camera);
}    

HdxFreeCameraSceneDelegate::HdxFreeCameraSceneDelegate(
    HdRenderIndex *renderIndex,
    SdfPath const &delegateId)
  : HdSceneDelegate(renderIndex, delegateId)
  , _cameraId(_ComputeCameraId(renderIndex, delegateId))
  , _policy(CameraUtilFit)
{
    if (_cameraId.IsEmpty()) {
        return;
    }
    
    GetRenderIndex().InsertSprim(
        HdPrimTypeTokens->camera,
        this, GetCameraId());
}

HdxFreeCameraSceneDelegate::~HdxFreeCameraSceneDelegate()
{
    if (_cameraId.IsEmpty()) {
        return;
    }

    GetRenderIndex().RemoveSprim(
        HdPrimTypeTokens->camera,
        GetCameraId());
}

void
HdxFreeCameraSceneDelegate::_MarkDirty(const HdDirtyBits bits)
{
    if (_cameraId.IsEmpty()) {
        return;
    }

    GetRenderIndex().GetChangeTracker().MarkSprimDirty(
        GetCameraId(), bits);
}

void
HdxFreeCameraSceneDelegate::SetCamera(
    const GfCamera &camera)
{
    if (_camera == camera) {
        return;
    }

    // Not optimal: issuing HdCameraParams even if it is only the tranform
    // or clipping planes that changed.
    HdDirtyBits dirtyBits = HdCamera::DirtyParams;

    if (_camera.GetTransform() != camera.GetTransform()) {
        dirtyBits |= HdCamera::DirtyTransform;
    }
    if (_camera.GetClippingPlanes() != camera.GetClippingPlanes()) {
        dirtyBits |= HdCamera::DirtyClipPlanes;
    }
    _camera = camera;
    
    _MarkDirty(dirtyBits);
}

void
HdxFreeCameraSceneDelegate::SetWindowPolicy(
    const CameraUtilConformWindowPolicy policy)
{
    if (_policy == policy) {
        return;
    }
    
    _policy = policy;
    
    _MarkDirty(HdCamera::DirtyWindowPolicy);
}

void
HdxFreeCameraSceneDelegate::SetMatrices(
    GfMatrix4d const &viewMatrix,
    GfMatrix4d const &projMatrix)
{
    GfCamera cam = _camera;
    cam.SetFromViewAndProjectionMatrix(viewMatrix, projMatrix);
    SetCamera(cam);
}

void
HdxFreeCameraSceneDelegate::SetClipPlanes(
    std::vector<GfVec4f> const &clipPlanes)
{
    if (_camera.GetClippingPlanes() == clipPlanes) {
        return;
    }

    _camera.SetClippingPlanes(clipPlanes);
    
    _MarkDirty(HdCamera::DirtyClipPlanes);
}

GfMatrix4d
HdxFreeCameraSceneDelegate::GetTransform(
    SdfPath const &id)
{
    return _camera.GetTransform();
}

static
HdCamera::Projection
_ToHd(const GfCamera::Projection projection)
{
    switch(projection) {
    case GfCamera::Perspective:
        return HdCamera::Perspective;
    case GfCamera::Orthographic:
        return HdCamera::Orthographic;
    }
    TF_CODING_ERROR("Bad GfCamera::Projection value");
    return HdCamera::Perspective;
}



VtValue
HdxFreeCameraSceneDelegate::GetCameraParamValue(
    SdfPath const &id,
    TfToken const &key)
{
    if (key == HdCameraTokens->projection) {
        return VtValue(_ToHd(_camera.GetProjection()));
    }

    if (key == HdCameraTokens->focalLength) {
        const float result = 
            _camera.GetFocalLength() *
            float(GfCamera::FOCAL_LENGTH_UNIT);
        return VtValue(result);
    }

    if (key == HdCameraTokens->horizontalAperture) {
        const float result =
            _camera.GetHorizontalAperture() * 
            float(GfCamera::APERTURE_UNIT);
        return VtValue(result);
    }

    if (key == HdCameraTokens->verticalAperture) {
        const float result =
            _camera.GetVerticalAperture() * 
            float(GfCamera::APERTURE_UNIT);
        return VtValue(result);
    }

    if (key == HdCameraTokens->horizontalApertureOffset) {
        const float result =
            _camera.GetHorizontalApertureOffset() * 
            float(GfCamera::APERTURE_UNIT);
        return VtValue(result);
    }

    if (key == HdCameraTokens->verticalApertureOffset) {
        const float result =
            _camera.GetVerticalApertureOffset() * 
            float(GfCamera::APERTURE_UNIT);
        return VtValue(result);
    }

    if (key == HdCameraTokens->clippingRange) {
        return VtValue(_camera.GetClippingRange());
    }

    if (key == HdCameraTokens->clipPlanes) {
        const std::vector<GfVec4f> &clipPlanes = _camera.GetClippingPlanes();
        const std::vector<GfVec4d> planes(clipPlanes.begin(), clipPlanes.end());
        return VtValue(planes);
    }

    if (key == HdCameraTokens->fStop) {
        return VtValue(_camera.GetFStop());
    }

    if (key == HdCameraTokens->focusDistance) {
        return VtValue(_camera.GetFocusDistance());
    }

    if (key == HdCameraTokens->windowPolicy) {
        return VtValue(_policy);
    }

    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
