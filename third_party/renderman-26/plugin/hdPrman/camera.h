//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "hdPrman/renderParam.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/timeSampleArray.h"

#include "pxr/base/vt/array.h"

#include <optional>

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

    /// Synchronizes state from the delegate to this object.
    HDPRMAN_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;
    
    /// Returns the time sampled xforms that were queried during Sync.
    HDPRMAN_API
    HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> const&
    GetTimeSampleXforms() const {
        return _sampleXforms;
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

    float GetApertureAngle() const {
        return _apertureAngle;
    }

    float GetApertureDensity() const {
        return _apertureAngle;
    }

    float GetApertureNSides() const {
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

    float _apertureAngle;
    float _apertureDensity;
    int _apertureNSides;
    float _apertureRoundness;
    float _dofMult;

    VtDictionary _params;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_H
