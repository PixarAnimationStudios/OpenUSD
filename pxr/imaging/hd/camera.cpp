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
  , _focusOn(false)
  , _dofAspect(1.0f)
  , _splitDiopterCount(0)
  , _splitDiopterAngle(0.0f)
  , _splitDiopterOffset1(0.0f)
  , _splitDiopterWidth1(0.0f)
  , _splitDiopterFocusDistance1(0.0f)
  , _splitDiopterOffset2(0.0f)
  , _splitDiopterWidth2(0.0f)
  , _splitDiopterFocusDistance2(0.0f)
  , _shutterOpen(0.0)
  , _shutterClose(0.0)
  , _exposure(0.0f)
  , _lensDistortionType(HdCameraTokens->standard)
  , _lensDistortionK1(0.0f)
  , _lensDistortionK2(0.0f)
  , _lensDistortionCenter(0.0f)
  , _lensDistortionAnaSq(1.0f)
  , _lensDistortionAsym(0.0f)
  , _lensDistortionScale(1.0f)
  , _lensDistortionIor(0.0f)
  , _windowPolicy(CameraUtilFit)
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

        const VtValue vFocusOn =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->focusOn);
        if (!vFocusOn.IsEmpty()) {
            _focusOn = vFocusOn.Get<bool>();
        }

        const VtValue vDofAspect =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->dofAspect);
        if (!vDofAspect.IsEmpty()) {
            _dofAspect = vDofAspect.Get<float>();
        }

        const VtValue vSplitDiopterCount =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->splitDiopterCount);
        if (!vSplitDiopterCount.IsEmpty()) {
            _splitDiopterCount = vSplitDiopterCount.Get<int>();
        }

        const VtValue vSplitDiopterAngle =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->splitDiopterAngle);
        if (!vSplitDiopterAngle.IsEmpty()) {
            _splitDiopterAngle = vSplitDiopterAngle.Get<float>();
        }

        const VtValue vSplitDiopterOffset1 =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->splitDiopterOffset1);
        if (!vSplitDiopterOffset1.IsEmpty()) {
            _splitDiopterOffset1 = vSplitDiopterOffset1.Get<float>();
        }

        const VtValue vSplitDiopterWidth1 =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->splitDiopterWidth1);
        if (!vSplitDiopterWidth1.IsEmpty()) {
            _splitDiopterWidth1 = vSplitDiopterWidth1.Get<float>();
        }

        const VtValue vSplitDiopterFocusDistance1 =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->splitDiopterFocusDistance1);
        if (!vSplitDiopterFocusDistance1.IsEmpty()) {
            _splitDiopterFocusDistance1 =
                vSplitDiopterFocusDistance1.Get<float>();
        }

        const VtValue vSplitDiopterOffset2 =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->splitDiopterOffset2);
        if (!vSplitDiopterOffset2.IsEmpty()) {
            _splitDiopterOffset2 = vSplitDiopterOffset2.Get<float>();
        }

        const VtValue vSplitDiopterWidth2 =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->splitDiopterWidth2);
        if (!vSplitDiopterWidth2.IsEmpty()) {
            _splitDiopterWidth2 = vSplitDiopterWidth2.Get<float>();
        }

        const VtValue vSplitDiopterFocusDistance2 =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->splitDiopterFocusDistance2);
        if (!vSplitDiopterFocusDistance2.IsEmpty()) {
            _splitDiopterFocusDistance2 =
                vSplitDiopterFocusDistance2.Get<float>();
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

        const VtValue vLensDistortionType =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->lensDistortionType);
        if (!vLensDistortionType.IsEmpty()) {
            _lensDistortionType = vLensDistortionType.Get<TfToken>();
        }

        const VtValue vLensDistortionK1 =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->lensDistortionK1);
        if (!vLensDistortionK1.IsEmpty()) {
            _lensDistortionK1 = vLensDistortionK1.Get<float>();
        }

        const VtValue vLensDistortionK2 =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->lensDistortionK2);
        if (!vLensDistortionK2.IsEmpty()) {
            _lensDistortionK2 = vLensDistortionK2.Get<float>();
        }

        const VtValue vLensDistortionCenter =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->lensDistortionCenter);
        if (!vLensDistortionCenter.IsEmpty()) {
            _lensDistortionCenter = vLensDistortionCenter.Get<GfVec2f>();
        }

        const VtValue vLensDistortionAnaSq =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->lensDistortionAnaSq);
        if (!vLensDistortionAnaSq.IsEmpty()) {
            _lensDistortionAnaSq = vLensDistortionAnaSq.Get<float>();
        }

        const VtValue vLensDistortionAsym =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->lensDistortionAsym);
        if (!vLensDistortionAsym.IsEmpty()) {
            _lensDistortionAsym = vLensDistortionAsym.Get<GfVec2f>();
        }

        const VtValue vLensDistortionScale =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->lensDistortionScale);
        if (!vLensDistortionScale.IsEmpty()) {
            _lensDistortionScale = vLensDistortionScale.Get<float>();
        }

        const VtValue vLensDistortionIor =
            sceneDelegate->GetCameraParamValue(
                id, HdCameraTokens->lensDistortionIor);
        if (!vLensDistortionIor.IsEmpty()) {
            _lensDistortionIor = vLensDistortionIor.Get<float>();
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
HdCamera::ComputeProjectionMatrix() const
{
    HD_TRACE_FUNCTION();

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

