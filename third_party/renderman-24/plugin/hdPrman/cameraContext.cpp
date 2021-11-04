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

void
HdPrmanCameraContext::SetCameraAndCameraNodeParams(
    RtParamList * camParams,
    RtParamList * camNodeParams) const
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
}


void
HdPrmanCameraContext::MarkValid()
{
    _invalid = false;
}

PXR_NAMESPACE_CLOSE_SCOPE
