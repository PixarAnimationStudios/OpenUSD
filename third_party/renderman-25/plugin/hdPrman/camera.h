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
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "hdPrman/renderParam.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/timeSampleArray.h"

#include "pxr/base/vt/array.h"

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
        float shutterOpenTime;
        float shutterCloseTime;
        VtArray<float> shutterOpening;
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

private:
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
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_H
