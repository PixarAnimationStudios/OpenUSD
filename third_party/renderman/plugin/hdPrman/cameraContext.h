//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_CONTEXT_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_CONTEXT_H

#include "hdPrman/api.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"
#include "pxr/imaging/cameraUtil/framing.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/vec2i.h"

#include "pxr/pxr.h"

#include <Riley.h>
#include <RileyIds.h>
#include <RiTypesHelper.h>

#include <atomic>
#include <cstdint>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderIndex;
class HdCamera;
class HdPrmanCamera;
class HdPrman_RenderParam;

/// HdPrman_CameraContext holds all the data necessary to populate the
/// riley camera and other camera-related riley options. It also keeps
/// track whether the camera or camera-related settings such as the
/// framing have changed so that updating riley is necessary.
///
class HdPrman_CameraContext final
{
public:
    HDPRMAN_API
    explicit HdPrman_CameraContext(HdPrman_RenderParam* renderParam);

    /// Call when hydra changed the transform or parameters of a camera.
    HDPRMAN_API
    void MarkCameraInvalidIfActive(const SdfPath& path);

    /// Set the camera framing. Context is only marked invalid if framing
    /// is different from what it used to be.
    HDPRMAN_API
    void SetFraming(const CameraUtilFraming &framing);

    /// Set window policy. Same comments as for SetFraming apply.
    HDPRMAN_API
    void SetWindowPolicy(CameraUtilConformWindowPolicy policy);

    /// If true, some aspect of the camera or related state has changed
    /// and the riley camera or options need to be updated.
    HDPRMAN_API
    bool IsInvalid() const;

    // ------------------------------------------------------------------------
    // Fallback camera lifecycle management & accessors
    // ------------------------------------------------------------------------

    /// Create the reserved fallback riley camera (with default settings), used
    /// as the active camera when there is no scene camera.
    HDPRMAN_API
    void CreateFallbackCamera();

    /// Delete the reserved fallback riley camera.
    HDPRMAN_API
    void DeleteFallbackCamera();

    /// The reserved riley name given to the fallback camera. Chosen so it
    /// cannot collide with a scene camera's path-derived name.
    HDPRMAN_API
    static RtUString GetFallbackCameraName();

    /// Sets the fallback camera as the active camera
    HDPRMAN_API
    void ActivateFallbackCamera();

    // ------------------------------------------------------------------------
    // Active camera tracking & accessors
    // ------------------------------------------------------------------------

    /// Set the active camera. If camera is the same as it used to be,
    /// context is not marked invalid.
    HDPRMAN_API
    void SetActiveCameraPath(const SdfPath& path);

    /// Path of the camera that has been *requested* as the active camera.
    /// Empty when the fallback camera is to be used. May be valid before
    /// camera's first Sync.
    const SdfPath& GetActiveCameraPath() const
    {
        return _requestedActiveCameraPath;
    }

    /// Id of the active camera's riley camera. Valid only after the camera's
    /// first Sync.
    const riley::CameraId& GetActiveCameraId() const
    {
        return _activeCamera.id != riley::CameraId::InvalidId()
          ? _activeCamera.id
          : _fallbackCamera.id;
    }

    /// Riley name of current active camera. Valid only after the camera's
    /// first Sync.
    const RtUString& GetActiveCameraName() const
    {
        return _activeCamera.id != riley::CameraId::InvalidId()
          ? _activeCamera.name
          : _fallbackCamera.name;
    }

    /// For convenience, get camera at active camera path from render index.
    HDPRMAN_API
    HdPrmanCamera* GetActiveCamera(const HdRenderIndex* renderIndex) const;

    /// The set of "overlay" opinions applied to whichever camera is active, on
    /// top of that camera's scene-derived params
    struct ActiveCameraOverlay
    {
        CameraUtilFraming framing;
        CameraUtilConformWindowPolicy windowPolicy;
        RtUString projectionNameOverride;
        RtParamList projectionParamsOverride;
        bool disableDepthOfField;
        std::optional<GfVec2i> renderBufferSize;
    };

    /// Assemble the overlay opinions the active camera should apply.
    /// \p renderBufferSize is set for interactive rendering and empty for
    /// offline rendering.
    HDPRMAN_API
    ActiveCameraOverlay GetActiveCameraOverlay(
        std::optional<GfVec2i> renderBufferSize = std::nullopt) const;

    /// Re-resolve the active camera and re-apply the overlay to its riley
    /// camera. \p renderBufferSize should be set for interactive rendering and
    /// empty for offline rendering.
    HDPRMAN_API
    void UpdateActiveCamera(
        const HdRenderIndex* renderIndex,
        std::optional<GfVec2i> renderBufferSize = std::nullopt);

    /// Commit \p camera to riley as the active camera from within its own Sprim
    /// Sync, and bind it as the scene's default dicing camera.
    HDPRMAN_API
    void CommitActiveCameraDuringSync(
        const HdRenderIndex* renderIndex,
        HdPrmanCamera* camera);

    /// Re-commit the active camera to riley using the overlay most recently
    /// applied by UpdateActiveCamera / ActivateFallbackCamera. We use this to
    /// reassert that the active camera is the last modified camera in Riley,
    /// which we do right after SetOptions and right before Render.
    HDPRMAN_API
    void ReapplyActiveCamera(const HdRenderIndex* renderIndex);

    // ------------------------------------------------------------------------
    // Camera name registry
    // ------------------------------------------------------------------------

    /// Register a scene camera in the camera name registry
    HDPRMAN_API
    void RegisterCameraName(const SdfPath& cameraPath,
                            const RtUString& rileyName);

    /// Remove a scene camera from the camera name registry
    HDPRMAN_API
    void UnregisterCameraName(const SdfPath& cameraPath);

    /// Resolve a camera name to the name of the Riley camera. Tries to match
    /// \p authored against the full path of a camera in the registry. If no
    /// such match is found, it will try to match \p authored against the
    /// terminal path token of a camera in the registry, provided it is unique
    /// across the registry. Multiple matches on terminal path token are treated
    /// as not found (with warning).
    HDPRMAN_API
    RtUString ResolveCameraName(const std::string& authored) const;

    // ------------------------------------------------------------------------

    /// Update the given riley options for offline rendering
    /// to an image file.
    ///
    /// Sets the crop window, format resolution and pixel aspect ratio.
    HDPRMAN_API
    void SetRileyOptions(
        RtParamList * options) const;

    /// Update the given riley options for rendering to AOVs backed by
    /// render buffers of the given size.
    ///
    /// Sets the crop window and pixel aspect ratio.
    HDPRMAN_API
    void SetRileyOptionsInteractive(
        RtParamList * options,
        const GfVec2i &renderBufferSize) const;

    // A projection that will override the value from the camera setting if
    // it is different from the default perspective.
    HDPRMAN_API
    void SetProjectionOverride(const RtUString& projection,
                               const RtParamList& projectionParams);

    /// Mark that riley camera and options are up to date.
    HDPRMAN_API
    void MarkValid();

    /// Get resolution from the display window.
    HDPRMAN_API
    GfVec2i GetResolutionFromDisplayWindow() const;

    // Get resolution from the data window
    // This can be removed once XPU handles under/overscan correctly.
    HDPRMAN_API
    GfVec2i GetResolutionFromDataWindow() const;

    /// When depth of field is disabled the fstop is set to infinity.
    HDPRMAN_API
    void SetDisableDepthOfField(bool disableDepthOfField);

    /// Get the camera framing.
    HDPRMAN_API
    const CameraUtilFraming &GetFraming() const;

private:
    struct _CameraIdentity
    {
        riley::CameraId id;
        SdfPath path;
        RtUString name;
    };

    void _UpdateActiveCameraIds(const HdPrmanCamera* camera);

    void _BindActiveCamera(const HdRenderIndex* renderIndex);

    void _RevertPrevious(
        const HdRenderIndex* renderIndex,
        const SdfPath& newActivePath);

    void _ApplyFallbackOverlay(const ActiveCameraOverlay& overlay);

    void _RevertFallback();

    HdPrman_RenderParam* _renderParam;

    SdfPath _requestedActiveCameraPath;

    _CameraIdentity _activeCamera;
    _CameraIdentity _fallbackCamera;
    _CameraIdentity _overlayCamera;

    CameraUtilFraming _framing;
    CameraUtilConformWindowPolicy _policy;
    bool _disableDepthOfField;

    std::optional<ActiveCameraOverlay> _lastAppliedOverlay;

    RtUString _projectionNameOverride;
    RtParamList _projectionParamsOverride;
    uint32_t _projectionParamsOverrideHash = 0;

    mutable std::shared_mutex _cameraNameTableMutex;
    std::unordered_map<SdfPath, RtUString, SdfPath::Hash> _cameraNameTable;

    std::atomic_bool _invalid;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_CONTEXT_H
