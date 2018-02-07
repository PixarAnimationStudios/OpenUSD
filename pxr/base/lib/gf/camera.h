//
// Copyright 2016 Pixar
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
#ifndef GF_CAMERA_H
#define GF_CAMERA_H

/// \file gf/camera.h
/// \ingroup group_gf_BasicGeometry

#include "pxr/pxr.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range1f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/api.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class GfFrustum;

/// \class GfCamera
/// \ingroup group_gf_BasicGeometry
/// \brief Object-based representation of a camera.
///
/// This class provides a thin wrapper on the camera data model,
/// with a small number of computations.
///
class GfCamera
{
public:
    /// Projection type.
    enum Projection {
        Perspective = 0,
        Orthographic,
    };

    /// Direction used for Field of View or orthographic size
    enum FOVDirection {
        FOVHorizontal = 0,
        FOVVertical
    };

    /// The unit for horizontal and vertical aperture is one tenth of the
    /// world unit. Thus, if the world unit is assumed to be cm, the horizontal
    /// and vertical aperture unit is mm.
    GF_API static const double APERTURE_UNIT;
    /// The unit for focal length. Similar to APERTURE_UNIT.
    GF_API static const double FOCAL_LENGTH_UNIT;
    
    /// Default horizontal and vertical aperture, based on a 35mm
    /// (non-anamorphic) projector aperture (0.825 x 0602 inches, converted to
    /// mm).
    GF_API static const double DEFAULT_HORIZONTAL_APERTURE;
    GF_API static const double DEFAULT_VERTICAL_APERTURE;

public:
    GF_API GfCamera(
        const GfMatrix4d &transform = GfMatrix4d(1.0),
        Projection projection = Perspective,
        float horizontalAperture = DEFAULT_HORIZONTAL_APERTURE,
        float verticalAperture = DEFAULT_VERTICAL_APERTURE,
        float horizontalApertureOffset = 0.0,
        float verticalApertureOffset = 0.0,
        float focalLength = 50.0,
        const GfRange1f &clippingRange = GfRange1f(1, 1000000),
        const std::vector<GfVec4f> &clippingPlanes = std::vector<GfVec4f>(),
        float fStop = 0.0,
        float focusDistance = 0.0);

    /// Sets the transform of the filmback in world space to \p val. 
    GF_API void SetTransform(const GfMatrix4d &val);

    /// Sets the projection type.
    GF_API void SetProjection(const Projection &val);

    /// \name Physics based camera setup

    /// These are the values actually stored in the class and they correspond
    /// to measurements of an actual physical camera (in mm).
    /// Together with the clipping range, they determine the camera frustum.

    /// @{
    
    /// Sets the focal length in tenths of a world unit (e.g., mm if the world
    /// unit is assumed to be cm).
    GF_API void SetFocalLength(const float val);

    /// Sets the width of the projector aperture in tenths of a world unit
    /// (e.g., mm if the world unit is assumed to be cm).
    GF_API void SetHorizontalAperture(const float val);

    /// Sets the height of the projector aperture in tenths of a world unit
    /// (e.g., mm if the world unit is assumed to be cm).
    GF_API void SetVerticalAperture(const float val);

    /// Sets the horizontal offset of the projector aperture in tenths of a
    /// world unit (e.g., mm if the world unit is assumed to be cm).
    GF_API void SetHorizontalApertureOffset(const float val);

    /// Sets the vertical offset of the projector aperture in tenths of a
    /// world unit (e.g., mm if the world unit is assumed to be cm).
    GF_API void SetVerticalApertureOffset(const float val);
    /// @}

    /// \name Frustum geometry setup
    /// @{

    /// Sets the frustum to be projective with the given \p aspectRatio
    /// and horizontal, respectively, vertical field of view \p fieldOfView
    /// (similar to gluPerspective when direction = FOVVertical).
    ///
    /// Do not pass values for \p horionztalAperture unless you care about
    /// DepthOfField.

    GF_API void SetPerspectiveFromAspectRatioAndFieldOfView(
        float aspectRatio,
        float fieldOfView,
        FOVDirection direction,
        float horizontalAperture = DEFAULT_HORIZONTAL_APERTURE);

    /// Sets the frustum to be orthographic such that it has the given
    /// \p aspectRatio and such that the orthographic width, respectively,
    /// orthographic height (in cm) is equal to \p orthographicSize
    /// (depending on direction).

    GF_API void SetOrthographicFromAspectRatioAndSize(
        float aspectRatio, float orthographicSize, FOVDirection direction);

    /// @}

    /// Sets the clipping range in world units.
    GF_API void SetClippingRange(const GfRange1f &val);

    /// Sets additional arbitrarily oriented clipping planes.
    /// A vector (a,b,c,d) encodes a clipping plane that clips off points
    /// (x,y,z) with
    ///
    ///        a * x + b * y + c * z + d * 1 < 0
    ///
    /// where (x,y,z) are the coordinates in the camera's space.
    GF_API void SetClippingPlanes(const std::vector<GfVec4f> &val);

    /// Sets the lens aperture, unitless.
    GF_API void SetFStop(const float val);

    /// Sets the focus distance in world units.
    GF_API void SetFocusDistance(const float val);

    /// Returns the transform of the filmback in world space.  This is
    /// exactly the transform specified via SetTransform().
    GF_API GfMatrix4d GetTransform() const;

    /// Returns the projection type.
    GF_API Projection GetProjection() const;

    /// Returns the width of the projector aperture in tenths of a world unit
    /// (e.g., mm if the world unit is assumed to be cm).
    GF_API float GetHorizontalAperture() const;

    /// Returns the height of the projector aperture in tenths of a world unit
    /// (e.g., mm if the world unit is assumed to be cm).
    GF_API float GetVerticalAperture() const;

    /// Returns the horizontal offset of the projector aperture in tenths of a
    /// world unit (e.g., mm if the world unit is assumed to be cm).
    /// In particular, an offset is necessary when writing out a stereo camera
    /// with finite convergence distance as two cameras.
    GF_API float GetHorizontalApertureOffset() const;

    /// Returns the vertical offset of the projector aperture in tenths of a
    /// world unit (e.g., mm if the world unit is assumed to be cm).
    GF_API float GetVerticalApertureOffset() const;

    /// Returns the projector aperture aspect ratio.
    GF_API float GetAspectRatio() const;

    /// Returns the focal length in tenths of a world unit (e.g., mm if the
    /// world unit is assumed to be cm).
    GF_API float GetFocalLength() const;

    /// Returns the horizontal or vertical field of view in degrees.
    GF_API float GetFieldOfView(FOVDirection direction) const;

    /// Returns the clipping range in world units.
    GF_API GfRange1f GetClippingRange() const;

    /// Returns additional clipping planes.
    GF_API const std::vector<GfVec4f> &GetClippingPlanes() const;

    /// Returns the computed, world-space camera frustum.  The frustum
    /// will always be that of a Y-up, -Z-looking camera.
    GF_API GfFrustum GetFrustum() const;

    /// Returns the lens aperture.
    GF_API float GetFStop() const;

    /// Returns the focus distance in world units.
    GF_API float GetFocusDistance() const;

    /// Equality operator. true iff all parts match.
    GF_API bool operator==(const GfCamera& other) const;

    // Inequality operator. true iff not equality.
    GF_API bool operator!=(const GfCamera& other) const;

private:
    // frustum
    GfMatrix4d              _transform;
    Projection              _projection;
    float                   _horizontalAperture;
    float                   _verticalAperture;
    float                   _horizontalApertureOffset;
    float                   _verticalApertureOffset;
    float                   _focalLength;
    GfRange1f               _clippingRange;
    std::vector<GfVec4f>    _clippingPlanes;

    // focus
    float                   _fStop;
    float                   _focusDistance;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // GF_CAMERA_H
