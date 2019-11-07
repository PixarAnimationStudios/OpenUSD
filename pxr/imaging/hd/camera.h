//
// Copyright 2017 Pixar
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
#ifndef PXR_IMAGING_HD_CAMERA_H
#define PXR_IMAGING_HD_CAMERA_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/sprim.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/gf/matrix4d.h"

#include <boost/shared_ptr.hpp>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Camera state that can be requested from the scene delegate via
/// GetCameraParamValue(id, token).
/// The parameters below mimic the USD camera schema (with the exception of
/// window policy).
#define HD_CAMERA_TOKENS                        \
    /* basic params */                          \
    (worldToViewMatrix)                         \
    (projectionMatrix)                          \
    (clipPlanes)                                \
    (windowPolicy)                              \
    /* additional params (in world units)*/     \
    (horizontalAperture)                        \
    (verticalAperture)                          \
    (horizontalApertureOffset)                  \
    (verticalApertureOffset)                    \
    (focalLength)                               \
    (clippingRange)                             \
                                                \
    (fStop)                                     \
    (focusDistance)                             \
                                                \
    (shutterOpen)                               \
    (shutterClose)


TF_DECLARE_PUBLIC_TOKENS(HdCameraTokens, HD_API, HD_CAMERA_TOKENS);

class HdSceneDelegate;

/// \class HdCamera
///
/// Hydra schema for a camera that pulls the basic params (see above) during
/// Sync.
/// Backends that use additional camera parameters can inherit from HdCamera and
/// pull on them.
///
class HdCamera : public HdSprim {
public:
    typedef std::vector<GfVec4d> ClipPlanesVector;

    HD_API
    HdCamera(SdfPath const & id);
    HD_API
    virtual ~HdCamera();

    // change tracking for HdCamera
    enum DirtyBits : HdDirtyBits {
        Clean                 = 0,
        DirtyViewMatrix       = 1 << 0,
        DirtyProjMatrix       = 1 << 1,
        DirtyWindowPolicy     = 1 << 2,
        DirtyClipPlanes       = 1 << 3,
        DirtyParams           = 1 << 4,
        AllDirty              = (DirtyViewMatrix
                                |DirtyProjMatrix
                                |DirtyWindowPolicy
                                |DirtyClipPlanes
                                |DirtyParams)
    };

    // ---------------------------------------------------------------------- //
    /// Sprim API
    // ---------------------------------------------------------------------- //
 
    /// Synchronizes state from the delegate to this object.
    HD_API
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override;


    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HD_API
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;
 
    // ---------------------------------------------------------------------- //
    /// Camera parameters accessor API
    // ---------------------------------------------------------------------- //

    /// Returns the matrix transformation from world to camera space.
    HD_API
    GfMatrix4d const& GetViewMatrix() const {
        return _worldToViewMatrix;
    }

    /// Returns the matrix transformation from camera to world space.
    HD_API
    GfMatrix4d const& GetViewInverseMatrix() const {
        return _worldToViewInverseMatrix;
    }

    /// Returns the projection matrix for the camera.
    HD_API
    GfMatrix4d const& GetProjectionMatrix() const {
        return _projectionMatrix;
    }

    /// Returns any additional clipping planes defined in camera space.
    HD_API
    std::vector<GfVec4d> const& GetClipPlanes() const {
        return _clipPlanes;
    }

    /// Returns the window policy of the camera. If no opinion is authored, we
    /// default to "CameraUtilFit"
    HD_API
    CameraUtilConformWindowPolicy const& GetWindowPolicy() const {
        return _windowPolicy;
    }

protected:
    GfMatrix4d _worldToViewMatrix;
    GfMatrix4d _worldToViewInverseMatrix;
    GfMatrix4d _projectionMatrix;
    CameraUtilConformWindowPolicy _windowPolicy;
    std::vector<GfVec4d> _clipPlanes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_CAMERA_H
