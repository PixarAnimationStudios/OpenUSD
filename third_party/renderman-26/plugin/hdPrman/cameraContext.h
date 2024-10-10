//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_CONTEXT_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_CONTEXT_H

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
    void CreateRileyCamera(
        riley::Riley * riley,
        const RtUString &cameraName);

    void DeleteRileyCameraAndClipPlanes(riley::Riley * riley);

    /// Get id of riley camera - valid only after Begin.
    const riley::CameraId &GetCameraId() const { return _cameraId; }

    /// Update the given riley options for offline rendering
    /// to an image file.
    ///
    /// Sets the crop window, format resolution and pixel aspect ratio.
    void SetRileyOptions(
        RtParamList * options) const;

    /// Update the given riley options for rendering to AOVs backed by
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

    // A projection that will override the value from the camera setting if
    // it is different from the default perspective.
    void SetProjectionOverride(const RtUString& projection,
                               const RtParamList& projectionParams);
    
    /// Mark that riley camera and options are up to date.
    void MarkValid();

    /// Get resolution from the display window.
    GfVec2i GetResolutionFromDisplayWindow() const;

    // Get resolution from the data window
    // This can be removed once XPU handles under/overscan correctly.
    GfVec2i GetResolutionFromDataWindow() const;

    /// When depth of field is disabled the fstop is set to infinity.
    void SetDisableDepthOfField(bool disableDepthOfField);

    /// Path of current camera in render index.
    const SdfPath &GetCameraPath() const { return _cameraPath; }

    /// Camera name used when creating the riley camera object.
    const RtUString& GetCameraName() const { return _cameraName; }

    /// For convenience, get camera at camera path from render index.
    const HdPrmanCamera * GetCamera(const HdRenderIndex *renderIndex) const;

    /// Get the camera framing.
    const CameraUtilFraming &GetFraming() const;

    static RtUString GetDefaultReferenceCameraName();

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

    void _DeleteClipPlanes(riley::Riley * riley);

    // Hydra Sprim path in the render index.
    SdfPath _cameraPath;
    CameraUtilFraming _framing;
    CameraUtilConformWindowPolicy _policy;
    bool _disableDepthOfField;
    
    // Save ids of riley clip planes so that we can delete them before
    // re-creating them to update the clip planes.
    std::vector<riley::ClippingPlaneId> _clipPlaneIds;
    riley::CameraId _cameraId;
    // Riley camera name provided as an argument to CreateRileyCamera.
    // This needs to be unique across all cameras.
    RtUString _cameraName;
    
    RtUString _projectionNameOverride;
    RtParamList _projectionParamsOverride;

    std::atomic_bool _invalid;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_CONTEXT_H
