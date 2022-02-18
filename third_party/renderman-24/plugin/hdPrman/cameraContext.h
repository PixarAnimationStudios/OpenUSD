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

class HdRenderIndex;
class HdCamera;
class HdPrmanCamera;

/// HdPrman_CameraContext holds all the data necessary to populate the
/// riley camera and other camera-related riley options. It also keeps
/// track whether the camera or camera-related settings such as the
/// framing have changed so that updating riley is necessary.
///
class HdPrman_CameraContext final
{
public:
    HdPrman_CameraContext();

    /// Call when hydra changed the transform or parameters of a camera.
    void MarkCameraInvalid(const SdfPath &path);

    /// Set the active camera. If camera is the same as it used to be,
    /// context is not marked invalid.
    void SetCameraPath(const SdfPath &path);

    /// Set the camera framing. Context is only marked invalid if framing
    /// is different from what it used to be.
    void SetFraming(const CameraUtilFraming &framing);
    
    /// Set window policy. Same comments as for SetFraming apply.
    void SetWindowPolicy(CameraUtilConformWindowPolicy policy);

    /// If true, some aspect of the camera or related state has changed
    /// and the riley camera or options need to be updated.
    bool IsInvalid() const;

    /// Create riley camera (with default settings).
    void Begin(riley::Riley * riley);

    /// Get id of riley camera - valid only after Begin.
    const riley::CameraId &GetCameraId() const { return _cameraId; }

    /// Update the given riley options for offline rendering
    /// to an image file.
    ///
    /// Sets the crop window, format resolution and pixel aspect ratio.
    void SetRileyOptions(
        RtParamList * options) const;

    /// Update the given riley options for rendering to AOVs baked by
    /// render buffers of the given size.
    ///
    /// Sets the crop window and pixel aspect ratio.
    void SetRileyOptionsInteractive(
        RtParamList * options,
        const GfVec2i &renderBufferSize) const;

    /// Update riley camera and clipping planes for offline rendering
    /// to an image file.
    void UpdateRileyCameraAndClipPlanes(
        riley::Riley * riley,
        const HdRenderIndex * renderIndex);

    /// Update riley camera and clipping planes for rendering to AOVs
    /// baked by render buffers of the given size.
    void UpdateRileyCameraAndClipPlanesInteractive(
        riley::Riley * riley,
        const HdRenderIndex * renderIndex,
        const GfVec2i &renderBufferSize);
    
    /// Mark that riley camera and options are up to date.
    void MarkValid();

    /// Get resolution for offline rendering.
    GfVec2i GetResolutionFromDisplayWindow() const;

    /// Set the shutter curve, i.e., the curve that determines how
    /// transparency of the shutter as a function of (normalized)
    /// time.
    ///
    /// Note that the times given here are relative to the shutter
    /// interval.
    ///
    /// Some more explanation:
    ///
    /// The values given here are passed to the Riley camera as options
    /// RixStr.k_shutterOpenTime, k_shutterCloseTime and k_shutteropening.
    ///
    /// (where as the shutter interval is set through the global Riley options
    /// using Ri:Shutter).
    ///
    /// RenderMan computes the shutter curve using constant pieces and
    /// cubic Bezier interpolation between the following points
    /// 
    /// (0, 0), (t1, y1), (t2,y2), (t3, 1), (t4, 1), (t5, y5), (t6, y6), (1, 0)
    ///
    /// which are encoded as:
    ///    t3 is the shutterOpenTime
    ///    t4 is the shutterCloseTime
    ///    [t1, y1, t2, y2, t5, y5, t6, y6] is shutteropeningPoints array.
    ///
    void SetShutterCurve(const float shutterOpenTime,
                         const float shutterCloseTime,
                         const float shutteropeningPoints[8]);

    /// Path of current camera in render index.
    const SdfPath &GetCameraPath() const { return _cameraPath; }

    /// For convenience, get camera at camera path from render index.
    const HdPrmanCamera * GetCamera(const HdRenderIndex *renderIndex) const;

private:
    /// Computes the screen window for the camera and conforms
    /// it to have the display window's aspect ratio using the
    /// current conform policy.
    GfRange2d _ComputeConformedScreenWindow(const HdCamera * camera) const;

    // Compute parameters for Riley::ModifyCamera
    RtParamList _ComputeCameraParams(
        const GfRange2d &screenWindow,
        const HdCamera * camera) const;

    void _UpdateRileyCamera(
        riley::Riley * const riley,
        const GfRange2d &screenWindow,
        const HdPrmanCamera * camera);
    void _UpdateClipPlanes(
        riley::Riley * riley,
        const HdPrmanCamera * camera);

    SdfPath _cameraPath;
    CameraUtilFraming _framing;
    CameraUtilConformWindowPolicy _policy;

    float _shutterOpenTime;
    float _shutterCloseTime;
    float _shutteropeningPoints[8];
    
    // Save ids of riley clip planes so that we can delete them before
    // re-creating them to update the clip planes.
    std::vector<riley::ClippingPlaneId> _clipPlaneIds;
    riley::CameraId _cameraId;
    
    std::atomic_bool _invalid;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_CONTEXT_H
