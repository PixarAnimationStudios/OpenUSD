//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_GF_FRUSTUM_H
#define PXR_BASE_GF_FRUSTUM_H

/// \file gf/frustum.h
/// \ingroup group_gf_BasicGeometry

#include "pxr/pxr.h"
#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/plane.h"
#include "pxr/base/gf/ray.h"
#include "pxr/base/gf/range1d.h"
#include "pxr/base/gf/range2d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/api.h"
#include "pxr/base/tf/hash.h"

#include <array>
#include <atomic>
#include <iosfwd>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class GfFrustum
/// \ingroup group_gf_BasicGeometry
///
/// Basic type: View frustum.
///
/// This class represents a viewing frustum in three dimensional eye space. It
/// may represent either a parallel (orthographic) or perspective projection.
/// One can think of the frustum as being defined by 6 boundary planes.
///
/// The frustum is specified using these parameters:
///  \li The \em position of the viewpoint.
///  \li The \em rotation applied to the default view frame, which is
///      looking along the -z axis with the +y axis as the "up"
///      direction.
///  \li The 2D \em window on the reference plane that defines the left,
///      right, top, and bottom planes of the viewing frustum, as
///      described below.
///  \li The distances to the \em near and \em far planes.
///  \li The \em projection \em type
///  \li The view distance.
///
/// The window and near/far parameters combine to define the view frustum as
/// follows. Transform the -z axis and the +y axis by the frustum rotation to
/// get the world-space \em view \em direction and \em up \em direction. Now
/// consider the \em reference \em plane that is perpendicular to the view
/// direction, a distance of referencePlaneDepth from the viewpoint, and whose
/// y axis corresponds to the up direction.  The window rectangle is specified
/// in a 2D coordinate system embedded in this plane. The origin of the
/// coordinate system is the point at which the view direction vector
/// intersects the plane. Therefore, the point (0,1) in this plane is found by
/// moving 1 unit along the up direction vector in this plane. The vector from
/// the viewpoint to the resulting point will form a 45-degree angle with the
/// view direction.
///
/// The view distance is only useful for interactive applications. It can be
/// used to compute a look at point which is useful when rotating around an
/// object of interest.
///
class GfFrustum {
  public:
    /// This enum is used to determine the type of projection represented by a
    /// frustum.
    enum ProjectionType {
        Orthographic,                   ///< Orthographic projection
        Perspective,                    ///< Perspective projection
    };

    /// This constructor creates an instance with default viewing parameters:
    /// \li The position is the origin.
    /// \li The rotation is the identity rotation. (The view is along
    ///     the -z axis, with the +y axis as "up").
    /// \li The window is -1 to +1 in both dimensions.
    /// \li The near/far interval is (1, 10).
    /// \li The view distance is 5.0.
    /// \li The projection type is \c GfFrustum::Perspective.
    GF_API GfFrustum();

    /// Copy constructor.
    GfFrustum(GfFrustum const &o)
        : _position(o._position)
        , _rotation(o._rotation)
        , _window(o._window)
        , _nearFar(o._nearFar)
        , _viewDistance(o._viewDistance)
        , _projectionType(o._projectionType)
        , _planes(nullptr) {
        if (auto *planes = o._planes.load()) {
            _planes = new std::array<GfPlane, 6>(*planes);
        }
    }

    /// Move constructor.
    GfFrustum(GfFrustum &&o) noexcept
        : _position(o._position)
        , _rotation(o._rotation)
        , _window(o._window)
        , _nearFar(o._nearFar)
        , _viewDistance(o._viewDistance)
        , _projectionType(o._projectionType)
        , _planes(nullptr) {
        if (auto *planes =
            o._planes.exchange(nullptr, std::memory_order_relaxed)) {
            _planes = planes;
        }
    }

    /// This constructor creates an instance with the given viewing
    /// parameters.
    GF_API GfFrustum(const GfVec3d &position, const GfRotation &rotation,
              const GfRange2d &window, const GfRange1d &nearFar,
              GfFrustum::ProjectionType projectionType,
              double viewDistance = 5.0);

    /// This constructor creates an instance from a camera matrix (always of a
    /// y-Up camera, also see SetPositionAndRotationFromMatrix) and the given
    /// viewing parameters.
    GF_API GfFrustum(const GfMatrix4d &camToWorldXf,
              const GfRange2d &window, const GfRange1d &nearFar,
              GfFrustum::ProjectionType projectionType,
              double viewDistance = 5.0);

    /// Copy assignment.
    GfFrustum &operator=(GfFrustum const &o) noexcept {
        if (this == &o) {
            return *this;
        }
        _position = o._position;
        _rotation = o._rotation;
        _window = o._window;
        _nearFar = o._nearFar;
        _viewDistance = o._viewDistance;
        _projectionType = o._projectionType;
        delete _planes.load(std::memory_order_relaxed);
        if (auto *planes = o._planes.load(std::memory_order_relaxed)) {
            _planes.store(new std::array<GfPlane, 6>(*planes),
                          std::memory_order_relaxed);
        }
        else {
            _planes.store(nullptr, std::memory_order_relaxed);
        }
        return *this;
    }

    /// Move assignment.
    GfFrustum &operator=(GfFrustum &&o) noexcept {
        if (this == &o) {
            return *this;
        }
        _position = o._position;
        _rotation = o._rotation;
        _window = o._window;
        _nearFar = o._nearFar;
        _viewDistance = o._viewDistance;
        _projectionType = o._projectionType;
        delete _planes.load(std::memory_order_relaxed);
        _planes.store(o._planes.load(std::memory_order_relaxed),
                      std::memory_order_relaxed);
        o._planes.store(nullptr, std::memory_order_relaxed);
        return *this;
    }        

    friend inline size_t hash_value(const GfFrustum &f) {
        return TfHash::Combine(
            f._position,
            f._rotation,
            f._window,
            f._nearFar,
            f._viewDistance,
            f._projectionType
        );
    }

    // Equality operator. true iff all parts match.
    bool operator ==(const GfFrustum& f) const {
        if (_position       != f._position)        return false;
        if (_rotation       != f._rotation)        return false;
        if (_window         != f._window)          return false;
        if (_nearFar        != f._nearFar)         return false;
        if (_viewDistance   != f._viewDistance)    return false;
        if (_projectionType != f._projectionType)  return false;

        return true;
    }

    // Inequality operator. true iff not equality.
    bool operator !=(const GfFrustum& f) const {
        return !(*this == f);
    }

    /// Destructor.
    GF_API ~GfFrustum();

    /// \name Value setting and access
    /// The methods in this group set and access the values that are used to
    /// define a frustum.
    ///@{

    /// Sets the position of the frustum in world space.
    void                SetPosition(const GfVec3d &position) {
        _position = position;
        _DirtyFrustumPlanes();
    }

    /// Returns the position of the frustum in world space.
    const GfVec3d &     GetPosition() const {
        return _position;
    }

    /// Sets the orientation of the frustum in world space as a rotation to
    /// apply to the default frame: looking along the -z axis with the +y axis
    /// as "up".
    void                SetRotation(const GfRotation &rotation) {
        _rotation = rotation;
        _DirtyFrustumPlanes();
    }

    /// Returns the orientation of the frustum in world space as a rotation to
    /// apply to the -z axis.
    const GfRotation &  GetRotation() const {
        return _rotation;
    }

    /// Sets the position and rotation of the frustum from a camera matrix
    /// (always from a y-Up camera). The resulting frustum's transform will
    /// always represent a right-handed and orthonormal coordinate sytem
    /// (scale, shear, and projection are removed from the given \p
    /// camToWorldXf).
    GF_API void SetPositionAndRotationFromMatrix(const GfMatrix4d &camToWorldXf);

    /// Sets the window rectangle in the reference plane that defines the
    /// left, right, top, and bottom planes of the frustum.
    void                SetWindow(const GfRange2d &window)  {
        _window = window;
        _DirtyFrustumPlanes();
    }

    /// Returns the window rectangle in the reference plane.
    const GfRange2d &   GetWindow() const {
        return _window;
    }

    /// Returns the depth of the reference plane.
    static double GetReferencePlaneDepth() {
        return 1.0;
    }

    /// Sets the near/far interval.
    void                SetNearFar(const GfRange1d &nearFar) {
        _nearFar = nearFar;
        _DirtyFrustumPlanes();
    }

    /// Returns the near/far interval.
    const GfRange1d &   GetNearFar() const {
        return _nearFar;
    }

    /// Sets the view distance.
    void                SetViewDistance(double viewDistance) {
        _viewDistance = viewDistance;
    }

    /// Returns the view distance.
    double              GetViewDistance() const {
        return _viewDistance;
    }

    /// Sets the projection type.
    void        SetProjectionType(GfFrustum::ProjectionType projectionType) {
        _projectionType = projectionType;
        _DirtyFrustumPlanes();
    }

    /// Returns the projection type.
    GfFrustum::ProjectionType   GetProjectionType() const {
        return _projectionType;
    }

    ///@}

    /// \name Convenience methods
    ///
    /// The methods in this group allow the frustum's data to be accessed and
    /// modified in terms of different representations that may be more
    /// convenient for certain applications.
    ///
    ///@{

    /// Sets up the frustum in a manner similar to \c gluPerspective().
    ///
    /// It sets the projection type to \c GfFrustum::Perspective and sets the
    /// window specification so that the resulting symmetric frustum encloses
    /// an angle of \p fieldOfViewHeight degrees in the vertical direction,
    /// with \p aspectRatio used to figure the angle in the horizontal
    /// direction. The near and far distances are specified as well. The
    /// window coordinates are computed as:
    /// \code
    ///     top    = tan(fieldOfViewHeight / 2)
    ///     bottom = -top
    ///     right  = top * aspectRatio
    ///     left   = -right
    ///     near   = nearDistance
    ///     far    = farDistance
    /// \endcode
    ///
    GF_API void         SetPerspective(double fieldOfViewHeight,
                                       double aspectRatio,
                                       double nearDistance, double farDistance);

    /// Sets up the frustum in a manner similar to gluPerspective().
    ///
    /// It sets the projection type to \c GfFrustum::Perspective and
    /// sets the window specification so that:
    ///
    /// If \a isFovVertical is true, the resulting symmetric frustum encloses
    /// an angle of \p fieldOfView degrees in the vertical direction, with \p
    /// aspectRatio used to figure the angle in the horizontal direction.
    ///
    /// If \a isFovVertical is false, the resulting symmetric frustum encloses
    /// an angle of \p fieldOfView degrees in the horizontal direction, with
    /// \p aspectRatio used to figure the angle in the vertical direction.
    ///
    /// The near and far distances are specified as well. The window
    /// coordinates are computed as follows:
    ///
    /// \li if isFovVertical:
    ///     \li top    = tan(fieldOfView / 2)
    ///     \li right  = top * aspectRatio
    /// \li if NOT isFovVertical:
    ///     \li right    = tan(fieldOfView / 2)
    ///     \li top  = right / aspectRation
    /// \li bottom = -top
    /// \li left   = -right
    /// \li near   = nearDistance
    /// \li far    = farDistance
    ///
    GF_API void         SetPerspective(double fieldOfView,
                                       bool   isFovVertical,
                                       double aspectRatio,
                                       double nearDistance, double farDistance);

    /// Returns the current frustum in the format used by \c SetPerspective().
    /// If the current frustum is not a perspective projection, this returns
    /// \c false and leaves the parameters untouched.
    GF_API bool         GetPerspective(double *fieldOfViewHeight,
                                       double *aspectRatio,
                                       double *nearDistance,
                                       double *farDistance) const;

    /// Returns the current frustum in the format used by \c SetPerspective().
    /// If the current frustum is not a perspective projection, this returns
    /// \c false and leaves the parameters untouched.
    GF_API bool         GetPerspective(bool   isFovVertical,
                                       double *fieldOfView,
                                       double *aspectRatio,
                                       double *nearDistance,
                                       double *farDistance) const;

    /// Returns the horizontal or vertical fov of the frustum. The fov of the
    /// frustum is not necessarily the same value as displayed in the viewer.
    /// The displayed fov is a function of the focal length or FOV avar. The
    /// frustum's fov may be different due to things like lens breathing.
    ///
    /// If the frustum is not of type \c GfFrustum::Perspective, the returned
    /// FOV will be 0.0.
    ///
    /// \note The default value for \c isFovVertical is false so calling \c
    /// GetFOV without an argument will return the horizontal field of view
    /// which is compatible with menv2x's old GfFrustum::GetFOV routine.
    GF_API double       GetFOV(bool isFovVertical = false) const;

    /// Sets up the frustum in a manner similar to \c glOrtho().
    ///
    /// Sets the projection to \c GfFrustum::Orthographic and sets the window
    /// and near/far specifications based on the given values.
    GF_API
    void                SetOrthographic(double left, double right,
                                        double bottom, double top,
                                        double nearPlane, double farPlane);

    /// Returns the current frustum in the format used by \c
    /// SetOrthographic(). If the current frustum is not an orthographic
    /// projection, this returns \c false and leaves the parameters untouched.
    GF_API bool         GetOrthographic(double *left, double *right,
                                        double *bottom, double *top,
                                        double *nearPlane, double *farPlane)
                                        const;

    /// Modifies the frustum to tightly enclose a sphere with the given center
    /// and radius, using the current view direction. The planes of the
    /// frustum are adjusted as necessary. The given amount of slack is added
    /// to the sphere's radius is used around the sphere to avoid boundary
    /// problems.
    GF_API void         FitToSphere(const GfVec3d &center, 
                                    double radius,
                                    double slack = 0.0);

    /// Transforms the frustum by the given matrix.
    ///
    /// The transformation matrix is applied as follows: the position and the
    /// direction vector are transformed with the given matrix. Then the
    /// length of the new direction vector is used to rescale the near and far
    /// plane and the view distance. Finally, the points that define the
    /// reference plane are transformed by the matrix. This method assures
    /// that the frustum will not be sheared or perspective-projected.
    ///
    /// \note Note that this definition means that the transformed frustum
    /// does not preserve scales very well. Do \em not use this function to
    /// transform a frustum that is to be used for precise operations such as
    /// intersection testing.
    GF_API GfFrustum&   Transform(const GfMatrix4d &matrix);

    /// Returns the normalized world-space view direction vector, which is
    /// computed by rotating the -z axis by the frustum's rotation.
    GF_API GfVec3d      ComputeViewDirection() const;

    /// Returns the normalized world-space up vector, which is computed by
    /// rotating the y axis by the frustum's rotation.
    GF_API GfVec3d      ComputeUpVector() const;

    /// Computes the view frame defined by this frustum. The frame consists of
    /// the view direction, up vector and side vector, as shown in this
    /// diagram.
    ///
    /// \code
    ///            up
    ///            ^   ^
    ///            |  / 
    ///            | / view
    ///            |/
    ///            +- - - - > side
    /// \endcode
    ///
    GF_API void         ComputeViewFrame(GfVec3d *side, 
                                         GfVec3d *up, 
                                         GfVec3d *view) const;

    /// Computes and returns the world-space look-at point from the eye point
    /// (position), view direction (rotation), and view distance.
    GF_API GfVec3d      ComputeLookAtPoint() const;

    /// Returns a matrix that represents the viewing transformation for this
    /// frustum.  That is, it returns the matrix that converts points from
    /// world space to eye (frustum) space.
    GF_API GfMatrix4d   ComputeViewMatrix() const;

    /// Returns a matrix that represents the inverse viewing transformation
    /// for this frustum.  That is, it returns the matrix that converts points
    /// from eye (frustum) space to world space.
    GF_API GfMatrix4d   ComputeViewInverse() const;

    /// Returns a GL-style projection matrix corresponding to the frustum's
    /// projection.
    GF_API GfMatrix4d   ComputeProjectionMatrix() const;

    /// Returns the aspect ratio of the frustum, defined as the width of the
    /// window divided by the height. If the height is zero or negative, this
    /// returns 0.
    GF_API double       ComputeAspectRatio() const;

    /// Returns the world-space corners of the frustum as a vector of 8
    /// points, ordered as:
    /// \li Left bottom near
    /// \li Right bottom near
    /// \li Left top near
    /// \li Right top near
    /// \li Left bottom far
    /// \li Right bottom far
    /// \li Left top far
    /// \li Right top far
    GF_API
    std::vector<GfVec3d> ComputeCorners() const;

    /// Returns the world-space corners of the intersection of the frustum
    /// with a plane parallel to the near/far plane at distance d from the
    /// apex, ordered as:
    /// \li Left bottom
    /// \li Right bottom
    /// \li Left top
    /// \li Right top
    /// In particular, it gives the partial result of ComputeCorners when given
    /// near or far distance.
    GF_API
    std::vector<GfVec3d> ComputeCornersAtDistance(double d) const;

    /// Returns a frustum that is a narrowed-down version of this frustum. The
    /// new frustum has the same near and far planes, but the other planes are
    /// adjusted to be centered on \p windowPos with the new width and height
    /// obtained from the existing width and height by multiplying by \p size[0]
    /// and \p size[1], respectively.  Finally, the new frustum is clipped
    /// against this frustum so that it is completely contained in the existing
    /// frustum.
    ///
    /// \p windowPos is given in normalized coords (-1 to +1 in both dimensions).
    /// \p size is given as a scalar (0 to 1 in both dimensions).
    ///
    /// If the \p windowPos or \p size given is outside these ranges, it may
    /// result in returning a collapsed frustum.
    ///
    /// This method is useful for computing a volume to use for interactive
    /// picking.
    GF_API GfFrustum    ComputeNarrowedFrustum(const GfVec2d &windowPos,
                                               const GfVec2d &size) const;

    /// Returns a frustum that is a narrowed-down version of this frustum. The
    /// new frustum has the same near and far planes, but the other planes are
    /// adjusted to be centered on \p worldPoint with the new width and height
    /// obtained from the existing width and height by multiplying by \p size[0]
    /// and \p size[1], respectively.  Finally, the new frustum is clipped
    /// against this frustum so that it is completely contained in the existing
    /// frustum.
    ///
    /// \p worldPoint is given in world space coordinates.
    /// \p size is given as a scalar (0 to 1 in both dimensions).
    ///
    /// If the \p size given is outside this range, it may result in returning
    /// a collapsed frustum.
    ///
    /// If the \p worldPoint is at or behind the eye of the frustum, it will
    /// return a frustum equal to this frustum.
    ///
    /// This method is useful for computing a volume to use for interactive
    /// picking.
    GF_API GfFrustum    ComputeNarrowedFrustum(const GfVec3d &worldPoint,
                                               const GfVec2d &size) const;

    /// Builds and returns a \c GfRay that starts at the viewpoint and extends
    /// through the given \a windowPos given in normalized coords (-1 to +1 in
    /// both dimensions) window position.
    /// 
    /// Contrasted with ComputePickRay(), this method returns a ray whose
    /// origin is the eyepoint, while that method returns a ray whose origin
    /// is on the near plane.
    GF_API GfRay        ComputeRay(const GfVec2d &windowPos) const;

    /// Builds and returns a \c GfRay that connects the viewpoint to the given
    /// 3d point in worldspace.
    /// 
    /// Contrasted with ComputePickRay(), this method returns a ray whose
    /// origin is the eyepoint, while that method returns a ray whose origin
    /// is on the near plane.
    GF_API GfRay        ComputeRay(const GfVec3d &worldSpacePos) const;

    /// Builds and returns a \c GfRay that can be used for picking at the
    /// given normalized (-1 to +1 in both dimensions) window position.
    ///
    /// Contrasted with ComputeRay(), that method returns a ray whose origin
    /// is the eyepoint, while this method returns a ray whose origin is on
    /// the near plane.
    GF_API GfRay        ComputePickRay(const GfVec2d &windowPos) const;

    /// Builds and returns a \c GfRay that can be used for picking that
    /// connects the viewpoint to the given 3d point in worldspace.
    GF_API GfRay       ComputePickRay(const GfVec3d &worldSpacePos) const;

    ///@}

    /// \name Intersection methods
    ///
    /// The methods in this group implement intersection operations
    /// between this frustum and a given primitive.
    ///
    ///@{

    /// Returns true if the given axis-aligned bbox is inside or intersecting
    /// the frustum. Otherwise, it returns false. Useful when doing picking or
    /// frustum culling.
    GF_API bool         Intersects(const GfBBox3d &bbox) const;

    /// Returns true if the given point is inside or intersecting the frustum.
    /// Otherwise, it returns false. 
    GF_API bool         Intersects(const GfVec3d &point) const;

    /// Returns \c true if the line segment formed by the given points is
    /// inside or intersecting the frustum.  Otherwise, it returns false.
    GF_API bool         Intersects(const GfVec3d &p0,
                                   const GfVec3d &p1) const;

    /// Returns \c true if the triangle formed by the given points is inside
    /// or intersecting the frustum.  Otherwise, it returns false.
    GF_API bool         Intersects(const GfVec3d &p0,
                                   const GfVec3d &p1,
                                   const GfVec3d &p2) const;

    /// Returns \c true if the bbox volume intersects the view volume given by
    /// the view-projection matrix, erring on the side of false positives for
    /// efficiency.
    ///
    /// This method is intended for cases where a GfFrustum is not available
    /// or when the view-projection matrix yields a view volume that is not
    /// expressable as a GfFrustum.
    ///
    /// Because it errs on the side of false positives, it is suitable for
    /// early-out tests such as draw or intersection culling.
    ///
    GF_API static bool  IntersectsViewVolume(const GfBBox3d &bbox,
                                             const GfMatrix4d &vpMat);

    ///@}

  private:
    // Dirty the result of _CalculateFrustumPlanes.
      GF_API void _DirtyFrustumPlanes();

    // Calculates cached frustum planes used for intersection tests.
      GF_API void       _CalculateFrustumPlanes() const;

    // Builds and returns a \c GfRay that can be used for picking. Given an
    // eye position and direction in camera space, offsets the ray to emanate
    // from the near plane, then transforms into worldspace
      GF_API GfRay      _ComputePickRayOffsetToNearPlane(
                                    const GfVec3d &camSpaceFrom, 
                                    const GfVec3d &camSpaceDir) const;

    // Returns a frustum that is a narrowed-down version of this frustum. The
    // new frustum has the same near and far planes, but the other planes are
    // adjusted to be centered on \p windowPoint with the new width and height
    // obtained from the existing width and height by multiplying by \p size[0]
    // and \p size[1], respectively.  Finally, the new frustum is clipped
    // against this frustum so that it is completely contained in the existing
    // frustum.
    //
    // \p windowPoint is given in window coordinates.
    // \p size is given as a scalar (0 to 1 in both dimensions).
    //
    // If the \p size given is outside this range, it may result in returning
    // a collapsed frustum.
    //
    // This method is useful for computing a volume to use for interactive
    // picking.
    GfFrustum           _ComputeNarrowedFrustumSub(const GfVec2d windowPoint, 
                                    const GfVec2d &size) const;

    bool _SegmentIntersects(GfVec3d const &p0, uint32_t p0Mask,
                            GfVec3d const &p1, uint32_t p1Mask) const;

    // Position of the frustum in world space.
    GfVec3d                     _position;

    // Orientation of the frustum in world space as a rotation to apply to the
    // -z axis.
    GfRotation                  _rotation;

    // Window rectangle in the image plane.
    GfRange2d                   _window;

    // Near/far interval.
    GfRange1d                   _nearFar;

    // View distance.
    double                      _viewDistance;

    // Projection type.
    ProjectionType              _projectionType;

    // Cached planes.
    // If null, the planes have not been calculated.
    mutable std::atomic<std::array<GfPlane, 6> *> _planes;
};

/// Output a GfFrustum using the format [(position) (rotation) [window]
/// [nearFar] viewDistance type]
///
/// The "type" is "perspective", or "orthographic, depending on the
/// projection type of the frustum.
///
/// \ingroup group_gf_DebuggingOutput
GF_API std::ostream& operator<<(std::ostream& out, const GfFrustum& f);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_GF_FRUSTUM_H 
