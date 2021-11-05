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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_CONTEXT_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_CONTEXT_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/cameraUtil/conformWindow.h"
#include "pxr/imaging/cameraUtil/framing.h"

#include "Riley.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

class HdPrmanCamera;

/// HdPrmanCameraContext holds all the data necessary to populate the
/// riley camera and other camera-related riley options. It also keeps
/// track whether the camera or camera-related settings such as the
/// framing have changed so that updating riley is necessary.
///
/// TODO: Move more camera-related code in interactiveRenderPass.cpp
/// and HdPrmanCamera::SetRileyCameraParams here.
///
class HdPrmanCameraContext final
{
public:
    HdPrmanCameraContext();

    /// Call when hydra changed the transform or parameters of a camera.
    void MarkCameraInvalid(const HdPrmanCamera * camera);

    /// Set the active camera. If camera is the same as it used to be,
    /// context is not marked invalid.
    void SetCamera(const HdPrmanCamera * camera);

    /// Set the camera framing. Context is only marked invalid if framing
    /// is different from what it used to be.
    void SetFraming(const CameraUtilFraming &framing);
    
    /// Set window policy. Same comments as for SetFraming apply.
    void SetWindowPolicy(CameraUtilConformWindowPolicy policy);

    /// If true, some aspect of the camera or related state has changed
    /// and the riley camera or options need to be updated.
    bool IsInvalid() const;

    /// Update the given riley options.
    ///
    /// Sets the crop window - if framing is invalid, crop window will be set
    /// to fill the entire render buffer.
    void SetRileyOptions(
        RtParamList * const options,
        const GfVec2i &renderBufferSize) const;

    /// Update riley camera identified by \p cameraId and clipping planes.
    void UpdateRileyCameraAndClipPlanes(
        riley::Riley * const riley,
        const riley::CameraId &cameraId,
        const GfVec2i &renderBufferSize);
    
    /// Mark that riley camera and options are up to date.
    void MarkValid();

private:
    // Compute parameters for Riley::ModifyCamera
    RtParamList _ComputeCameraParams(
        const GfVec2i &renderBufferSize) const;

    void _UpdateRileyCamera(
        riley::Riley * const riley,
        const riley::CameraId &cameraId,
        const GfVec2i &renderBufferSize);
    void _UpdateClipPlanes(riley::Riley * riley);

    const HdPrmanCamera * _camera;
    SdfPath _cameraPath;
    CameraUtilFraming _framing;
    CameraUtilConformWindowPolicy _policy;
    
    // Save ids of riley clip planes so that we can delete them before
    // re-creating them to update the clip planes.
    std::vector<riley::ClippingPlaneId> _clipPlaneIds;
    
    std::atomic_bool _invalid;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_CONTEXT_H
