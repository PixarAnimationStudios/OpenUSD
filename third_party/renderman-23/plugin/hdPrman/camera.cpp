//
// Copyright 2019 Pixar
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
#include "hdPrman/camera.h"
#include "RiTypesHelper.h" // XXX: Shouldn't rixStrings.h include this?
#include "hdPrman/rixStrings.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/camera.h"

#include "RtParamList.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPrmanCamera::HdPrmanCamera(SdfPath const& id)
    : HdCamera(id), _dirtyParams(false)
{
    /* NOTHING */
}

HdPrmanCamera::~HdPrmanCamera() = default;

/* virtual */
void
HdPrmanCamera::Sync(HdSceneDelegate *sceneDelegate,
                    HdRenderParam   *renderParam,
                    HdDirtyBits     *dirtyBits)
{  
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    SdfPath const &id = GetId();
    HdDirtyBits& bits = *dirtyBits;

    if (bits & DirtyViewMatrix) {
        sceneDelegate->SampleTransform(id, &_sampleXforms);
    }

    if (bits & DirtyParams) {
        _dirtyParams = true;
    }

    HdCamera::Sync(sceneDelegate, renderParam, dirtyBits);

    // XXX: Should we flip the proj matrix (RHS vs LHS) as well here?

    // We don't need to clear the dirty bits since HdCamera::Sync always clears
    // all the dirty bits.

}

bool
HdPrmanCamera::GetAndResetHasParamsChanged()
{
    bool paramsChanged = _dirtyParams;
    _dirtyParams = false;
    return paramsChanged;
}

void
HdPrmanCamera::SetRileyCameraParams(RtParamList& camParams,
                                    RtParamList& projParams) const
{
    // Following parameters can be set on the projection shader:
    // fov (currently unhandled)
    // fovEnd (currently unhandled)
    // fStop
    // focalLength
    // focalDistance
    // RenderMan defines disabled DOF as fStop=inf not zero
    const float fStop = GetFStop();
    if (GetFStop() > 0) {
        projParams.SetFloat(RixStr.k_fStop, fStop);
    } else {
        projParams.SetFloat(RixStr.k_fStop, RI_INFINITY);
    }
    
    // Do not use the initial value 0 which we get if the scene delegate
    // did not provide a focal length.
    const float focalLength = GetFocalLength();
    if (focalLength > 0) {
        projParams.SetFloat(RixStr.k_focalLength, focalLength);
    }

    // Similar for focus distance.
    const float focusDistance = GetFocusDistance();
    if (focusDistance > 0) {
        projParams.SetFloat(RixStr.k_focalDistance, focusDistance);
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
    const GfRange1f &clippingRange = GetClippingRange();
    if (clippingRange.GetMin() < clippingRange.GetMax()) {
        camParams.SetFloat(RixStr.k_nearClip, clippingRange.GetMin());
        camParams.SetFloat(RixStr.k_farClip, clippingRange.GetMax());
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

PXR_NAMESPACE_CLOSE_SCOPE

