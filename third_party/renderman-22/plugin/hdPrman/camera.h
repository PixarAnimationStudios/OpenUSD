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
#ifndef EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_H
#define EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "hdPrman/context.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/timeSampleArray.h"
#include "pxr/base/vt/dictionary.h"

class RixParamList;

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
    HdPrmanCamera(SdfPath const& id);
    virtual ~HdPrmanCamera();

    /// Synchronizes state from the delegate to this object.
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override;
    
    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Returns true if any physical camera parameter was updated during Sync,
    /// and reset the internal tracking state.
    /// This is meant to be called post Sync, and exists only because we don't
    /// hold a handle to the Riley camera to directly update it during Sync.
    HDPRMAN_API
    bool GetAndResetHasParamsChanged();

    /// Returns the time sampled xforms that were queried during Sync.
    HDPRMAN_API
    HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> const&
    GetTimeSampleXforms() const {
        return _sampleXforms;
    }
 
    /// Sets the camera and projection shader parameters as expected by Riley
    /// from the USD physical camera params.
    HDPRMAN_API
    void SetRileyCameraParams(RixParamList *camParams,
                              RixParamList *projParams) const;

private:
    HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> _sampleXforms;
    
    VtDictionary _params;
    bool _dirtyParams;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CAMERA_H
