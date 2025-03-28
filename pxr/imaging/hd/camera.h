//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_CAMERA_H
#define PXR_IMAGING_HD_CAMERA_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/sprim.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range1f.h"
#include "pxr/base/gf/vec2f.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Camera state that can be requested from the scene delegate via
/// GetCameraParamValue(id, token). The parameters below mimic the
/// USD camera schema and GfCamera (with the exception of window
/// policy). All spatial units are in world units though and
/// projection is HdCamera::Projection rather than a token.
#define HD_CAMERA_TOKENS                                          \
    /* frustum */                                                 \
    (projection)                                                  \
    (horizontalAperture)                                          \
    (verticalAperture)                                            \
    (horizontalApertureOffset)                                    \
    (verticalApertureOffset)                                      \
    (focalLength)                                                 \
    (clippingRange)                                               \
    (clipPlanes)                                                  \
                                                                  \
    /* depth of field */                                          \
    (fStop)                                                       \
    (focusDistance)                                               \
    (focusOn)                                                     \
    (dofAspect)                                                   \
    ((splitDiopterCount,          "splitDiopter:count"))          \
    ((splitDiopterAngle,          "splitDiopter:angle"))          \
    ((splitDiopterOffset1,        "splitDiopter:offset1"))        \
    ((splitDiopterWidth1,         "splitDiopter:width1"))         \
    ((splitDiopterFocusDistance1, "splitDiopter:focusDistance1")) \
    ((splitDiopterOffset2,        "splitDiopter:offset2"))        \
    ((splitDiopterWidth2,         "splitDiopter:width2"))         \
    ((splitDiopterFocusDistance2, "splitDiopter:focusDistance2")) \
                                                                  \
    /* shutter/lighting */                                        \
    (shutterOpen)                                                 \
    (shutterClose)                                                \
    (exposure)                                                    \
    (exposureTime)                                                \
    (exposureIso)                                                 \
    (exposureFStop)                                               \
    (exposureResponsivity)                                        \
    (linearExposureScale)                                         \
                                                                  \
    /* how to match window with different aspect */               \
    (windowPolicy)                                                \
                                                                  \
    /* lens distortion */                                         \
    (standard)                                                    \
    (fisheye)                                                     \
    ((lensDistortionType,   "lensDistortion:type"))               \
    ((lensDistortionK1,     "lensDistortion:k1"))                 \
    ((lensDistortionK2,     "lensDistortion:k2"))                 \
    ((lensDistortionCenter, "lensDistortion:center"))             \
    ((lensDistortionAnaSq,  "lensDistortion:anaSq"))              \
    ((lensDistortionAsym,   "lensDistortion:asym"))               \
    ((lensDistortionScale,  "lensDistortion:scale"))              \
    ((lensDistortionIor,    "lensDistortion:ior"))


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
        DirtyParams           = 1 << 1,
        DirtyClipPlanes       = 1 << 2,
        DirtyWindowPolicy     = 1 << 3,
        AllDirty              = (DirtyTransform
                                |DirtyParams
                                |DirtyClipPlanes
                                |DirtyWindowPolicy)
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

    bool GetFocusOn() const {
        return _focusOn;
    }

    float GetDofAspect() const {
        return _dofAspect;
    }

    int GetSplitDiopterCount() const {
        return _splitDiopterCount;
    }

    float GetSplitDiopterAngle() const {
        return _splitDiopterAngle;
    }

    float GetSplitDiopterOffset1() const {
        return _splitDiopterOffset1;
    }

    float GetSplitDiopterWidth1() const {
        return _splitDiopterWidth1;
    }

    float GetSplitDiopterFocusDistance1() const {
        return _splitDiopterFocusDistance1;
    }

    float GetSplitDiopterOffset2() const {
        return _splitDiopterOffset2;
    }

    float GetSplitDiopterWidth2() const {
        return _splitDiopterWidth2;
    }

    float GetSplitDiopterFocusDistance2() const {
        return _splitDiopterFocusDistance2;
    }

    double GetShutterOpen() const {
        return _shutterOpen;
    }

    double GetShutterClose() const {
        return _shutterClose;
    }

    /// Get the raw exposure exponent value.
    ///
    /// This the same as the value stored in the exposure attribute on the
    /// underlying camera.  Note that in most cases, you will want to use
    /// GetLinearExposureScale() instead of this method, as it is the computed
    /// end result of all related exposure attributes.
    /// GetExposure() is retained as-is for backward compatibility.
    float GetExposure() const {
        return _exposure;
    }

    /// Get the computed linear exposure scale from the underlying camera.
    ///
    /// Scaling the image brightness by this value will cause the various
    /// exposure controls on \ref UsdGeomCamera to behave like those of a real
    /// camera to control the exposure of the image.
    float GetLinearExposureScale() const {
        return _linearExposureScale;
    }

    TfToken GetLensDistortionType() const {
        return _lensDistortionType;
    }

    float GetLensDistortionK1() const {
        return _lensDistortionK1;
    }

    float GetLensDistortionK2() const {
        return _lensDistortionK2;
    }

    const GfVec2f& GetLensDistortionCenter() const {
        return _lensDistortionCenter;
    }

    float GetLensDistortionAnaSq() const {
        return _lensDistortionAnaSq;
    }

    const GfVec2f& GetLensDistortionAsym() const {
        return _lensDistortionAsym;
    }

    float GetLensDistortionScale() const {
        return _lensDistortionScale;
    }

    float GetLensDistortionIor() const {
        return _lensDistortionIor;
    }

    /// Returns the window policy of the camera. If no opinion is authored, we
    /// default to "CameraUtilFit"
    const CameraUtilConformWindowPolicy& GetWindowPolicy() const {
        return _windowPolicy;
    }

    // ---------------------------------------------------------------------- //
    /// Convenience API for rasterizers
    // ---------------------------------------------------------------------- //

    /// Computes the projection matrix for a camera from its physical
    /// properties.
    HD_API
    GfMatrix4d ComputeProjectionMatrix() const;

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
    bool                    _focusOn;
    float                   _dofAspect;
    int                     _splitDiopterCount;
    float                   _splitDiopterAngle;
    float                   _splitDiopterOffset1;
    float                   _splitDiopterWidth1;
    float                   _splitDiopterFocusDistance1;
    float                   _splitDiopterOffset2;
    float                   _splitDiopterWidth2;
    float                   _splitDiopterFocusDistance2;

    // shutter
    double                  _shutterOpen;
    double                  _shutterClose;

    // exposure
    float                   _exposure;
    float                   _exposureTime;
    float                   _exposureIso;
    float                   _exposureFStop;
    float                   _exposureResponsivity;
    float                   _linearExposureScale;

    // lens distortion
    TfToken                 _lensDistortionType;
    float                   _lensDistortionK1;
    float                   _lensDistortionK2;
    GfVec2f                 _lensDistortionCenter;
    float                   _lensDistortionAnaSq;
    GfVec2f                 _lensDistortionAsym;
    float                   _lensDistortionScale;
    float                   _lensDistortionIor;

    // Camera's opinion how it display in a window with
    // a different aspect ratio
    CameraUtilConformWindowPolicy _windowPolicy;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_CAMERA_H
