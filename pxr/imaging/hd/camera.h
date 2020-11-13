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
#include "pxr/base/gf/range1f.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Camera state that can be requested from the scene delegate via
/// GetCameraParamValue(id, token). The parameters below mimic the
/// USD camera schema and GfCamera (with the exception of window
/// policy). All spatial units are in world units though and
/// projection is HdCamera::Projection rather than a token.
#define HD_CAMERA_TOKENS                            \
    /* frustum */                                   \
    (projection)                                    \
    (horizontalAperture)                            \
    (verticalAperture)                              \
    (horizontalApertureOffset)                      \
    (verticalApertureOffset)                        \
    (focalLength)                                   \
    (clippingRange)                                 \
    (clipPlanes)                                    \
                                                    \
    /* depth of field */                            \
    (fStop)                                         \
    (focusDistance)                                 \
                                                    \
    /* shutter/lighting */                          \
    (shutterOpen)                                   \
    (shutterClose)                                  \
    (exposure)                                      \
                                                    \
    /* how to match window with different aspect */ \
    (windowPolicy)                                  \
                                                    \
    /* OpenGL-style matrices, deprecated */         \
    (worldToViewMatrix)                             \
    (projectionMatrix)


TF_DECLARE_PUBLIC_TOKENS(HdCameraTokens, HD_API, HD_CAMERA_TOKENS);

/// \class HdCamera
///
/// Hydra schema for a camera that pulls the params (see above) during
/// Sync.
/// Backends that use additional camera parameters can inherit from HdCamera and
/// pull on them.
///
class HdCamera : public HdSprim
{
public:
    using ClipPlanesVector = std::vector<GfVec4d>;

    HD_API
    HdCamera(SdfPath const & id);
    HD_API
    ~HdCamera() override;

    // change tracking for HdCamera
    enum DirtyBits : HdDirtyBits
    {
        Clean                 = 0,
        DirtyTransform        = 1 << 0,
        DirtyViewMatrix       = DirtyTransform, // deprecated
        DirtyProjMatrix       = 1 << 1,         // deprecated
        DirtyWindowPolicy     = 1 << 2,
        DirtyClipPlanes       = 1 << 3,
        DirtyParams           = 1 << 4,
        AllDirty              = (DirtyTransform
                                |DirtyProjMatrix
                                |DirtyWindowPolicy
                                |DirtyClipPlanes
                                |DirtyParams)
    };

    enum Projection {
        Perspective = 0,
        Orthographic
    };

    // ---------------------------------------------------------------------- //
    /// Sprim API
    // ---------------------------------------------------------------------- //
 
    /// Synchronizes state from the delegate to this object.
    HD_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;
    

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HD_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;
 
    // ---------------------------------------------------------------------- //
    /// Camera parameters accessor API
    // ---------------------------------------------------------------------- //

    /// Returns camera transform
    GfMatrix4d const& GetTransform() const {
        return _transform;
    }

    /// Returns whether camera is orthographic and perspective
    Projection GetProjection() const {
        return _projection;
    }

    /// Returns horizontal aperture in world units.
    float GetHorizontalAperture() const {
        return _horizontalAperture;
    }

    /// Returns vertical aperture in world units.
    float GetVerticalAperture() const {
        return _verticalAperture;
    }

    /// Returns horizontal aperture offset in world units.
    float GetHorizontalApertureOffset() const {
        return _horizontalApertureOffset;
    }

    /// Returns vertical aperture offset in world units.
    float GetVerticalApertureOffset() const {
        return _verticalApertureOffset;
    }

    /// Returns focal length in world units.
    float GetFocalLength() const {
        return _focalLength;
    }

    /// Returns near and far plane in world units
    GfRange1f const &GetClippingRange() const {
        return _clippingRange;
    }
    
    /// Returns any additional clipping planes defined in camera space.
    std::vector<GfVec4d> const& GetClipPlanes() const {
        return _clipPlanes;
    }

    /// Returns fstop of camera
    float GetFStop() const {
        return _fStop;
    }

    /// Returns focus distance in world units.
    float GetFocusDistance() const {
        return _focusDistance;
    }

    double GetShutterOpen() const {
        return _shutterOpen;
    }

    double GetShutterClose() const {
        return _shutterClose;
    }

    float GetExposure() const {
        return _exposure;
    }

    /// Returns the window policy of the camera. If no opinion is authored, we
    /// default to "CameraUtilFit"
    CameraUtilConformWindowPolicy const& GetWindowPolicy() const {
        return _windowPolicy;
    }

    // ---------------------------------------------------------------------- //
    /// Legacy camera parameters accessor API
    // ---------------------------------------------------------------------- //

    /// Returns the matrix transformation from world to camera space.
    /// \deprecated Use GetTransform instead
    HD_API
    GfMatrix4d GetViewMatrix() const;

    /// Returns the matrix transformation from camera to world space.
    /// \deprecated Use GetTransform and invert instead
    HD_API
    GfMatrix4d GetViewInverseMatrix() const;

    /// Returns the projection matrix for the camera.
    /// \deprecated Compute from above physically based attributes
    HD_API
    GfMatrix4d GetProjectionMatrix() const;

protected:
    // frustum
    GfMatrix4d              _transform;
    Projection              _projection;
    float                   _horizontalAperture;
    float                   _verticalAperture;
    float                   _horizontalApertureOffset;
    float                   _verticalApertureOffset;
    float                   _focalLength;
    GfRange1f               _clippingRange;
    std::vector<GfVec4d>    _clipPlanes;

    // focus
    float                   _fStop;
    float                   _focusDistance;

    // shutter/lighting
    double                  _shutterOpen;
    double                  _shutterClose;
    float                   _exposure;

    // Camera's opinion how it display in a window with
    // a different aspect ratio
    CameraUtilConformWindowPolicy _windowPolicy;

    // OpenGL-style matrices
    GfMatrix4d _worldToViewMatrix;
    GfMatrix4d _worldToViewInverseMatrix;
    GfMatrix4d _projectionMatrix;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_CAMERA_H
