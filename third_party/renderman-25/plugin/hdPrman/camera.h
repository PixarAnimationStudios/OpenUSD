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
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_H
