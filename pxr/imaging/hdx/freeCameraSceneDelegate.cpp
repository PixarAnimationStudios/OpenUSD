//
// Copyright 2021 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
    GfCamera cam;
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
