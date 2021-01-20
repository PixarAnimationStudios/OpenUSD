//
// Copyright 2017 Pixar
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
#include "pxr/imaging/hd/camera.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/base/gf/camera.h"
#include "pxr/base/gf/frustum.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(HdCameraTokens, HD_CAMERA_TOKENS);

HdCamera::HdCamera(SdfPath const &id)
  : HdSprim(id)
  , _transform(1.0)
  , _projection(Perspective)
  , _horizontalAperture(0.0f)
  , _verticalAperture(0.0f)
  , _horizontalApertureOffset(0.0f)
  , _verticalApertureOffset(0.0f)
  , _focalLength(0.0f)
  , _fStop(0.0f)
  , _focusDistance(0.0f)
  , _shutterOpen(0.0)
  , _shutterClose(0.0)
  , _exposure(0.0f)
  , _windowPolicy(CameraUtilFit)
  , _worldToViewMatrix(0.0)
  , _worldToViewInverseMatrix(0.0)
  , _projectionMatrix(0.0)
{
}

HdCamera::~HdCamera() = default;

void
HdCamera::Sync(HdSceneDelegate * sceneDelegate,
               HdRenderParam   * renderParam,
               HdDirtyBits     * dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    SdfPath const &id = GetId();
    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    // HdCamera communicates to the scene graph and caches all interesting
    // values within this class.
    // Later on Get() is called from TaskState (RenderPass) to perform
    // aggregation/pre-computation, in order to make the shader execution
    // efficient.
    HdDirtyBits bits = *dirtyBits;
    
    if (bits & DirtyTransform) {
        _transform = sceneDelegate->GetTransform(id);
    }

    if (bits & DirtyParams) {
        const VtValue vProjection =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->projection);
        if (!vProjection.IsEmpty()) {
            _projection = vProjection.Get<Projection>();
        }

        const VtValue vHorizontalAperture =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->horizontalAperture);
        if (!vHorizontalAperture.IsEmpty()) {
            _horizontalAperture = vHorizontalAperture.Get<float>();
        }

        const VtValue vVerticalAperture =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->verticalAperture);
        if (!vVerticalAperture.IsEmpty()) {
            _verticalAperture = vVerticalAperture.Get<float>();
        }

        const VtValue vHorizontalApertureOffset =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->horizontalApertureOffset);
        if (!vHorizontalApertureOffset.IsEmpty()) {
            _horizontalApertureOffset = vHorizontalApertureOffset.Get<float>();
        }

        const VtValue vVerticalApertureOffset =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->verticalApertureOffset);
        if (!vVerticalApertureOffset.IsEmpty()) {
            _verticalApertureOffset = vVerticalApertureOffset.Get<float>();
        }
        
        const VtValue vFocalLength =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->focalLength);
        if (!vFocalLength.IsEmpty()) {
            _focalLength = vFocalLength.Get<float>();
        }

        const VtValue vClippingRange =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->clippingRange);
        if (!vClippingRange.IsEmpty()) {
            _clippingRange = vClippingRange.Get<GfRange1f>();
        }

        const VtValue vFStop =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->fStop);
        if (!vFStop.IsEmpty()) {
            _fStop = vFStop.Get<float>();
        }

        const VtValue vFocusDistance =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->focusDistance);
        if (!vFocusDistance.IsEmpty()) {
            _focusDistance = vFocusDistance.Get<float>();
        }
        
        const VtValue vShutterOpen =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->shutterOpen);
        if (!vShutterOpen.IsEmpty()) {
            _shutterOpen = vShutterOpen.Get<double>();
        }

        const VtValue vShutterClose =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->shutterClose);
        if (!vShutterClose.IsEmpty()) {
            _shutterClose = vShutterClose.Get<double>();
        }

        const VtValue vExposure =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->exposure);
        if (!vExposure.IsEmpty()) {
            _exposure = vExposure.Get<float>();
        }
    }

    if (bits & DirtyViewMatrix) {
        // extract and store view matrix
        const VtValue vMatrix = sceneDelegate->GetCameraParamValue(id,
            HdCameraTokens->worldToViewMatrix);
        
        if (!vMatrix.IsEmpty()) {
            _worldToViewMatrix = vMatrix.Get<GfMatrix4d>();
            _worldToViewInverseMatrix = _worldToViewMatrix.GetInverse();
        }
    }

    if (bits & DirtyProjMatrix) {
        // extract and store projection matrix
        const VtValue vMatrix = sceneDelegate->GetCameraParamValue(id,
            HdCameraTokens->projectionMatrix);

        if (!vMatrix.IsEmpty()) {
            _projectionMatrix = vMatrix.Get<GfMatrix4d>();
        }
    }

    if (bits & DirtyWindowPolicy) {
        // treat window policy as an optional parameter
        const VtValue vPolicy = sceneDelegate->GetCameraParamValue(id, 
            HdCameraTokens->windowPolicy);
        if (!vPolicy.IsEmpty())  {
            _windowPolicy = vPolicy.Get<CameraUtilConformWindowPolicy>();
        }
    }

    if (bits & DirtyClipPlanes) {
        // treat clip planes as an optional parameter
        const VtValue vClipPlanes = 
            sceneDelegate->GetCameraParamValue(id, HdCameraTokens->clipPlanes);
        if (!vClipPlanes.IsEmpty()) {
            _clipPlanes = vClipPlanes.Get< std::vector<GfVec4d> >();
        }
    }

    // Clear all the dirty bits. This ensures that the sprim doesn't
    // remain in the dirty list always.
    *dirtyBits = Clean;
}

GfMatrix4d
HdCamera::GetViewMatrix() const
{
    if (_worldToViewMatrix == GfMatrix4d(0.0)) {
        return _transform.GetInverse();
    }
    return _worldToViewMatrix;
}

GfMatrix4d
HdCamera::GetViewInverseMatrix() const
{
    if (_worldToViewInverseMatrix == GfMatrix4d(0.0)) {
        return _transform;
    }
    return _worldToViewInverseMatrix;
}

GfMatrix4d
HdCamera::GetProjectionMatrix() const
{
    HD_TRACE_FUNCTION();

    if (GetFocalLength() == 0.0f) {
        return _projectionMatrix;
    }

    GfCamera cam;
    
    // Only set the values needed to compute projection matrix.
    cam.SetProjection(
        GetProjection() == HdCamera::Orthographic
        ? GfCamera::Orthographic
        : GfCamera::Perspective);
    cam.SetHorizontalAperture(
        GetHorizontalAperture()
        / GfCamera::APERTURE_UNIT);
    cam.SetVerticalAperture(
        GetVerticalAperture()
        / GfCamera::APERTURE_UNIT);
    cam.SetHorizontalApertureOffset(
        GetHorizontalApertureOffset()
        / GfCamera::APERTURE_UNIT);
    cam.SetVerticalApertureOffset(
        GetVerticalApertureOffset()
        / GfCamera::APERTURE_UNIT);
    cam.SetFocalLength(
        GetFocalLength()
        / GfCamera::FOCAL_LENGTH_UNIT);
    cam.SetClippingRange(
        GetClippingRange());
    
    return cam.GetFrustum().ComputeProjectionMatrix();
}

/* virtual */
HdDirtyBits
HdCamera::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}

PXR_NAMESPACE_CLOSE_SCOPE

