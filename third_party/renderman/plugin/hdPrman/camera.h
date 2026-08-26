//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_H

#include "hdPrman/api.h"
#include "hdPrman/cameraContext.h"
#include "hdPrman/renderParam.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/timeSampleArray.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range2d.h"
#include "pxr/base/vt/dictionary.h"

#include "pxr/pxr.h"

#include <Riley.h>
#include <RileyIds.h>
#include <RiTypesHelper.h>

#include <array>
#include <mutex>
#include <optional>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;

/// \class HdPrmanCamera
///
/// A representation for cameras that pulls on camera parameters used by Riley
/// cameras.
/// Note: We do not create a Riley camera per HdCamera because in PRman 22,
/// it'd require a render target to be created and bound (per camera), which
/// would be prohibitively expensive in Prman 22.
///
class HdPrmanCamera final : public HdCamera
{
public:
    /// See GetShutterCurve() below for a description of what these
    /// values represent.
    ///
    struct ShutterCurve
    {
        std::optional<float> shutterOpenTime;
        std::optional<float> shutterCloseTime;
        std::optional<std::array<float, 8>> shutteropening;
    };

    HDPRMAN_API
    HdPrmanCamera(SdfPath const& id);

    HDPRMAN_API
    ~HdPrmanCamera() override;

    // ------------------------------------------------------------------------
    // Riley projection shading-node identity
    //
    // Shared with HdPrman_CameraContext, which builds the same kind of
    // projection node for the fallback camera.
    // ------------------------------------------------------------------------

    /// The riley projection shader name to use for the given projection. If
    /// \p projectionOverride is set to anything other than the default
    /// perspective, it wins; otherwise the projection decides orthographic vs
    /// perspective.
    HDPRMAN_API
    static const RtUString& ComputeProjectionShader(
        HdCamera::Projection projection,
        const RtUString& projectionOverride);

    /// \p projectionOverride may be returned by reference, so binding it to a
    /// temporary would dangle.
    static const RtUString& ComputeProjectionShader(
        HdCamera::Projection projection,
        const RtUString&& projectionOverride) = delete;

    /// The riley projection shading-node handle name ("cam_projection").
    HDPRMAN_API
    static const RtUString& ProjectionNodeName();

    /// Synchronizes state from the delegate to this object.
    HDPRMAN_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;

    HDPRMAN_API
    void Finalize(HdRenderParam* renderParam) override;

    HDPRMAN_API
    riley::CameraId GetRileyCameraId() const;

    HDPRMAN_API
    RtUString GetRileyCameraName() const;

    /// Apply the active camera opinions overlay to this camera's riley camera.
    /// The framing-conformed screen window, the projection override, depth-of-
    /// field disabling, and the clipping planes are all part of the active
    /// camera overlay.
    HDPRMAN_API
    void UpdateRileyCameraForActive(
        HdPrman_RenderParam* param,
        const HdPrman_CameraContext::ActiveCameraOverlay& overlay);

    /// Remove the active camera opinions overlay from this camera's riley
    /// camera. This reverts the camera to its intrinsic (unconformed) window,
    /// removes the active-camera projection override and depth-of-field
    /// overrides, and deletes the camera's clipping planes.
    HDPRMAN_API
    void UpdateRileyCameraForInactive(HdPrman_RenderParam* param);

    /// Re-commit this camera's riley camera under the given overlay, and
    /// nothing else.
    ///
    /// Unlike UpdateRileyCameraForActive this is purely a republication of the
    /// camera's parameters: it leaves the clipping planes and the camera-name
    /// registry alone. It also assumes the riley camera already exists --
    /// callers must not use this to create one.
    ///
    /// We need this to be able to reassert the active camera as the last
    /// modified camera in Riley, which we do right after SetOptions and right
    /// before Render.
    HDPRMAN_API
    void RecommitRileyCameraForActive(
        HdPrman_RenderParam* param,
        const HdPrman_CameraContext::ActiveCameraOverlay& overlay);

    /// Returns the time sampled xforms that were queried during Sync.
    HDPRMAN_API
    HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> const&
    GetTimeSampleXforms() const {
        return _sampleXforms;
    }

    riley::ShadingNode
    GetProjectionNode() const {
        return _projectionNode;
    }

#if HD_API_VERSION < 52
    float GetLensDistortionK1() const {
        return _lensDistortionK1;
    }

    float GetLensDistortionK2() const {
        return _lensDistortionK2;
    }

    const GfVec2f &GetLensDistortionCenter() const {
        return _lensDistortionCenter;
    }

    float GetLensDistortionAnaSq() const {
        return _lensDistortionAnaSq;
    }

    const GfVec2f &GetLensDistortionAsym() const {
        return _lensDistortionAsym;
    }

    float GetLensDistortionScale() const {
        return _lensDistortionScale;
    }
#endif

    /// Get the shutter curve of the camera. This curve determines the
    /// transparency of the shutter as a function of (normalized)
    /// time.
    ///
    /// Note that the times returned here are relative to the shutter
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
    ///    [t1, y1, t2, y2, t5, y5, t6, y6] is the shutteropening array.
    ///
    /// \note The shutter:open and shutter:close attributes of UsdGeomCamera
    ///       represent the (frame-relative) time the shutter *begins to open*
    ///       and is *fully closed* respectively.
    ///
    ///       The Riley shutterOpenTime and shutterCloseTime represent the
    ///       (riley shutter-interval relative)  time the shutter is *fully
    ///       open* and *begins to close* respectively.
    ///
    const ShutterCurve& GetShutterCurve() const {
        return _shutterCurve;
    }

    /// Sets the camera and projection shader parameters as expected by Riley
    /// from the USD physical camera params.
    HDPRMAN_API
    void SetRileyCameraParams(RtParamList& camParams,
                              RtParamList& camParamsOverride,
                              RtParamList& projParams) const;

    float GetDofAspect() const {
        return _dofAspect;
    }

    float GetApertureAngle() const {
        return _apertureAngle;
    }

    float GetApertureDensity() const {
        return _apertureDensity;
    }

    int GetApertureNSides() const {
        return _apertureNSides;
    }

    float GetApertureRoundness() const {
        return _apertureRoundness;
    }

    float GetDofMult() const {
        return _dofMult;
    }

private:

    void setFov(RtParamList& projParams) const;

    void setScreenWindow(RtParamList& camParams, bool isPerspective) const;

    // Commit the camera state to riley. Caller must acquire lock on
    // _rileyCameraMutex.
    void _CommitToRiley(
        riley::Riley* riley,
        const GfRange2d& screenWindow,
        bool disableDepthOfField,
        const RtUString& projectionNameOverride,
        const RtParamList& projectionParamsOverride);

    // Commit the clip planes state to riley. Caller must acquire lock on
    // _rileyCameraMutex.
    void _UpdateClipPlanes(riley::Riley* riley);

    // Delete the clip planes from riley. Caller must acquire lock on
    // _rileyCameraMutex.
    void _DeleteClipPlanes(riley::Riley* riley);

    GfRange2d
    _GetScreenWindow() const;

    // This camera's intrinsic screen window, conformed to the framing and
    // window policy carried by the active camera overlay. The camera
    // contributes its aperture; the overlay contributes the framing.
    GfRange2d
    _ConformScreenWindow(
        const HdPrman_CameraContext::ActiveCameraOverlay& overlay) const;

    RtParamList
    _ComputeCameraParams(const GfRange2d & screenWindow) const;

    RtParamList
    _ComputeNodeParams(
        bool disableDepthOfField,
        const RtUString & projectionOverride) const;

    RtParamList
    _ComputePerspectiveNodeParams(bool disableDepthOfField) const;

    RtParamList
    _ComputeOrthographicNodeParams() const;

    HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> _sampleXforms;

#if HD_API_VERSION < 52
    float _lensDistortionK1;
    float _lensDistortionK2;
    GfVec2f _lensDistortionCenter;
    float _lensDistortionAnaSq;
    GfVec2f _lensDistortionAsym;
    float _lensDistortionScale;
#endif

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
    ShutterCurve _shutterCurve;

    float _dofAspect;
    float _apertureAngle;
    float _apertureDensity;
    int _apertureNSides;
    float _apertureRoundness;
    float _dofMult;

    VtDictionary _params;

    riley::ShadingNode _projectionNode;

    riley::CameraId _rileyCameraId;
    RtUString _rileyCameraName;
    std::vector<riley::ClippingPlaneId> _rileyClipPlaneIds;

    // Mutex for gating access to _rileyCameraId and _rileyCameraName
    mutable std::mutex _rileyCameraMutex;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_H
