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
#include "hdPrman/cameraContext.h"

#include "RiTypesHelper.h" // XXX: Shouldn't rixStrings.h include this?
#include "hdPrman/rixStrings.h"

#include "pxr/imaging/hd/camera.h"


PXR_NAMESPACE_OPEN_SCOPE

HdPrmanCameraContext::HdPrmanCameraContext()
  : _camera(nullptr)
  , _policy(CameraUtilFit)
  , _invalid(false)
{
}

void
HdPrmanCameraContext::MarkCameraInvalid(HdCamera * const camera)
{
    // No need to invalidate if camera that is not the active camera
    // changed.
    if (camera && camera->GetId() == _cameraPath) {
        _invalid = true;
    }
}

void
HdPrmanCameraContext::SetCamera(HdCamera * const camera)
{
    if (camera) {
        if (_cameraPath != camera->GetId()) {
            _invalid = true;
            _cameraPath = camera->GetId();
        }
    } else {
        if (_camera) {
            // If we had a camera and now have it no more, we need to
            // invalidate since we need to return to the default
            // camera.
            _invalid = true;
        }
    }

    _camera = camera;
}

void
HdPrmanCameraContext::SetFraming(const CameraUtilFraming &framing)
{
    if (_framing != framing) {
        _framing = framing;
        _invalid = true;
    }
}

void
HdPrmanCameraContext::SetWindowPolicy(
    const CameraUtilConformWindowPolicy policy)
{
    if (_policy != policy) {
        _policy = policy;
        _invalid = true;
    }
}

bool
HdPrmanCameraContext::IsInvalid() const
{
    return _invalid;
}

///////////////////////////////////////////////////////////////////////////////
//
// Screen window space: imagine a plane at unit distance (*) in front
// of the camera (and parallel to the camera). Coordinates with
// respect to screen window space are measured in this plane with the
// y-Axis pointing up. Such coordinates parameterize rays from the
// camera.
// (*) This is a simplification achieved by fixing RenderMan's FOV to be
// 90 degrees.
//
// Image space: coordinates of the pixels in the rendered image with the top
// left pixel having coordinate (0,0), i.e., y-down.
// The display window from the camera framing is in image space as well
// as the width and height of the render buffer.
//
// We want to map the screen window space to the image space such that the
// conformed camera frustum from the scene delegate maps to the display window
// of the CameraUtilFraming. This is achieved by the following code.
//
//
// Compute screen window for given camera.
//
static
GfRange2d
_GetScreenWindow(const HdCamera * const cam)
{
    const GfVec2d size(
        cam->GetHorizontalAperture(),       cam->GetVerticalAperture());
    const GfVec2d offset(
        cam->GetHorizontalApertureOffset(), cam->GetVerticalApertureOffset());
        
    const GfRange2d filmbackPlane(-0.5 * size + offset, +0.5 * size + offset);

    if (cam->GetProjection() == HdCamera::Orthographic) {
        return filmbackPlane;
    }

    if (cam->GetFocalLength() == 0.0f) {
        return filmbackPlane;
    }

    return filmbackPlane / double(cam->GetFocalLength());
}

// Compute the screen window we need to give to RenderMan. This screen
// window is mapped to the entire render buffer (in image space) by
// RenderMan.
//
// The input is the screenWindowForDisplayWindow: the screen window
// corresponding to the camera from the scene delegate conformed to match
// the aspect ratio of the display window.
//
// Together with the displayWindow, this input establishes how screen
// window space is mapped to image space. We know need to take the
// render buffer rect in image space and convert it to screen window
// space.
// 
static
GfRange2d
_ConvertScreenWindowForDisplayWindowToRenderBuffer(
    const GfRange2d &screenWindowForDisplayWindow,
    const GfRange2f &displayWindow,
    const GfVec2i &renderBufferSize)
{
    // Scaling factors to go from image space to screen window space.
    const double screenWindowWidthPerPixel =
        screenWindowForDisplayWindow.GetSize()[0] /
        displayWindow.GetSize()[0];
        
    const double screenWindowHeightPerPixel =
        screenWindowForDisplayWindow.GetSize()[1] /
        displayWindow.GetSize()[1];

    // Assuming an affine mapping between screen window space
    // and image space, compute what (0,0) corresponds to in
    // screen window space.
    const GfVec2d screenWindowMin(
        screenWindowForDisplayWindow.GetMin()[0]
        - screenWindowWidthPerPixel * displayWindow.GetMin()[0],
        // Note that image space is y-Down and screen window
        // space is y-Up, so this is a bit tricky...
        screenWindowForDisplayWindow.GetMax()[1]
        + screenWindowHeightPerPixel * (
            displayWindow.GetMin()[1] - renderBufferSize[1]));
        
    const GfVec2d screenWindowSize(
        screenWindowWidthPerPixel * renderBufferSize[0],
        screenWindowHeightPerPixel * renderBufferSize[1]);
    
    return GfRange2d(screenWindowMin, screenWindowMin + screenWindowSize);
}

static
double
_SafeDiv(const double a, const double b)
{
    if (b == 0) {
        TF_CODING_ERROR(
            "Invalid display window in render pass state for hdPrman");
        return 1.0;
    }
    return a / b;
}

// Compute the aspect ratio of the display window taking the
// pixel aspect ratio into account.
static
double
_GetDisplayWindowAspect(const CameraUtilFraming &framing)
{
    const GfVec2f &size = framing.displayWindow.GetSize();
    return framing.pixelAspectRatio * _SafeDiv(size[0], size[1]);
}

// Convert a window into the format expected by RenderMan
// (xmin, xmax, ymin, ymax).
static
GfVec4f
_ToVec4f(const GfRange2d &window)
{
    return { float(window.GetMin()[0]), float(window.GetMax()[0]),
             float(window.GetMin()[1]), float(window.GetMax()[1]) };
}

// Compute the screen window we need to give to RenderMan.
// 
// See above comments. This also conforms the camera frustum using
// the window policy specified by the application or the HdCamera.
//
static
GfVec4f
_ComputeScreenWindow(
    const HdCamera * const camera,
    const CameraUtilFraming &framing,
    const CameraUtilConformWindowPolicy policy,
    const GfVec2i &renderBufferSize)
{
    // Screen window from camera.
    const GfRange2d screenWindowForCamera = _GetScreenWindow(camera);

    // Conform to match display window's aspect ratio.
    const GfRange2d screenWindowForDisplayWindow =
        CameraUtilConformedWindow(
            screenWindowForCamera,
            policy,
            _GetDisplayWindowAspect(framing));
    
    // Compute screen window we need to send to RenderMan.
    const GfRange2d screenWindowForRenderBuffer =
        _ConvertScreenWindowForDisplayWindowToRenderBuffer(
            screenWindowForDisplayWindow,
            framing.displayWindow,
            renderBufferSize);
    
    return _ToVec4f(screenWindowForRenderBuffer);
}

void
HdPrmanCameraContext::SetCameraAndCameraNodeParams(
    RtParamList * camParams,
    RtParamList * camNodeParams,
    const GfVec2i &renderBufferSize) const
{
    // Following parameters can be set on the projection shader:
    // fov (currently unhandled)
    // fovEnd (currently unhandled)
    // fStop
    // focalLength
    // focalDistance
    // RenderMan defines disabled DOF as fStop=inf not zero
    float fStop = RI_INFINITY;
    if (_camera) {
        const float cameraFStop = _camera->GetFStop();
        if (cameraFStop > 0.0) {
            fStop = cameraFStop;
        }
    }
    camNodeParams->SetFloat(RixStr.k_fStop, fStop);

    if (_camera) {
        // Do not use the initial value 0 which we get if the scene delegate
        // did not provide a focal length.
        const float focalLength = _camera->GetFocalLength();
        if (focalLength > 0) {
            camNodeParams->SetFloat(RixStr.k_focalLength, focalLength);
        }
    }

    if (_camera) {
        // Similar for focus distance.
        const float focusDistance = _camera->GetFocusDistance();
        if (focusDistance > 0) {
            camNodeParams->SetFloat(RixStr.k_focalDistance, focusDistance);
        }
    }
        
    // Following parameters are currently set on the Riley camera:
    // 'nearClip' (float): near clipping distance
    // 'farClip' (float): near clipping distance
    // 'shutterOpenTime' (float): beginning of normalized shutter interval
    // 'shutterCloseTime' (float): end of normalized shutter interval

    // Parameters that are not handled (and use their defaults):
    // 'focusregion' (float):
    // 'dofaspect' (float): dof aspect ratio
    // 'apertureNSides' (int):
    // 'apertureAngle' (float):
    // 'apertureRoundness' (float):
    // 'apertureDensity' (float):

    // Parameter that is handled during Riley camera creation:
    // Rix::k_shutteropening (float[8] [c1 c2 d1 d2 e1 e2 f1 f2): additional
    // control points

    // Do not use clipping range if scene delegate did not provide one.
    // Note that we do a sanity check slightly stronger than
    // GfRange1f::IsEmpty() in that we do not allow the range to contain
    // only exactly one point.
    if (_camera) {
        const GfRange1f &clippingRange = _camera->GetClippingRange();
        if (clippingRange.GetMin() < clippingRange.GetMax()) {
            camParams->SetFloat(RixStr.k_nearClip, clippingRange.GetMin());
            camParams->SetFloat(RixStr.k_farClip, clippingRange.GetMax());
        }
    }

    // XXX : Ideally we would want to set the proper shutter open and close,
    // however we can not fully change the shutter without restarting
    // Riley.
    
    // double const *shutterOpen =
    //     _GetDictItem<double>(_params, HdCameraTokens->shutterOpen);
    // if (shutterOpen) {
    //     camParams->SetFloat(RixStr.k_shutterOpenTime, *shutterOpen);
    // }
    
    // double const *shutterClose =
    //     _GetDictItem<double>(_params, HdCameraTokens->shutterClose);
    // if (shutterClose) {
    //     camParams->SetFloat(RixStr.k_shutterCloseTime, *shutterClose);
    // }

    // All subsequent code requires a valid camera and framing.

    if (!_camera) {
        return;
    }

    if (_camera->GetProjection() == HdCamera::Perspective) {
        // TODO: For lens distortion to be correct, we might
        // need to set a different FOV and adjust the screenwindow
        // accordingly.
        // For now, lens distortion parameters are not passed through
        // hdPrman anyway.
        //
        camNodeParams->SetFloat(
            RixStr.k_fov, 90.0f);
    }

    const GfVec4f screenWindow = _ComputeScreenWindow(
        _camera, _framing, _policy, renderBufferSize);
    
    camParams->SetFloatArray(
        RixStr.k_Ri_ScreenWindow, screenWindow.data(), 4);
}


// The crop window for RenderMan.
//
// Computed from data window and render buffer size.
//
// Recall from the RenderMan API:
// Only the pixels within the crop window are rendered. Has no
// affect on how pixels in the image map into the filmback plane.
// The crop window is relative to the render buffer size, e.g.,
// the crop window of (0,0,1,1) corresponds to the entire render
// buffer. The coordinates of the crop window are y-down.
// Format is (xmin, xmax, ymin, ymax).
//
// The limits for the integer locations corresponding to the above crop
// window are:
//
//   rxmin = clamp(ceil( renderbufferwidth*xmin    ), 0, renderbufferwidth - 1)
//   rxmax = clamp(ceil( renderbufferwidth*xmax - 1), 0, renderbufferwidth - 1)
//   similar for y
//
static
float
_DivRoundDown(const int a, const int b)
{
    // Note that if the division (performed here)
    //    float(a) / b
    // rounds up, then the result (by RenderMan) of
    //    ceil(b * (float(a) / b))
    // might be a+1 instead of a.
    //
    // We add a slight negative bias to a to avoid this (we could also
    // set the floating point rounding mode but: how to do this in a
    // portable way - and on x86 switching the rounding is slow).

    return GfClamp((a - 0.0078125f) / b, 0.0f, 1.0f);
}

static
GfVec4f
_ComputeCropWindow(
    const GfRect2i &dataWindow,
    const GfVec2i &renderBufferSize)
{
    return GfVec4f(
        _DivRoundDown(dataWindow.GetMinX(),     renderBufferSize[0]),
        _DivRoundDown(dataWindow.GetMaxX() + 1, renderBufferSize[0]),
        _DivRoundDown(dataWindow.GetMinY(),     renderBufferSize[1]),
        _DivRoundDown(dataWindow.GetMaxY() + 1, renderBufferSize[1]));
}

void
HdPrmanCameraContext::SetRileyOptions(
    RtParamList * const options,
    const GfVec2i &renderBufferSize) const
{
    const GfVec4f cropWindow =
        _ComputeCropWindow(_framing.dataWindow, renderBufferSize);

    options->SetFloatArray(
        RixStr.k_Ri_CropWindow,
        cropWindow.data(), 4);
}

void
HdPrmanCameraContext::MarkValid()
{
    _invalid = false;
}

PXR_NAMESPACE_CLOSE_SCOPE
