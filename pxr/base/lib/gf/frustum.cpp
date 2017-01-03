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
#include "pxr/base/gf/frustum.h"

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/ostreamHelpers.h"
#include "pxr/base/gf/plane.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/type.h"

#include <algorithm>
#include <iostream>

using namespace std;

// CODE_COVERAGE_OFF_GCOV_BUG
TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfFrustum>();
}
// CODE_COVERAGE_ON_GCOV_BUG

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(GfFrustum::Orthographic);
    TF_ADD_ENUM_NAME(GfFrustum::Perspective);
}


GfFrustum::GfFrustum() :
    _position(0),
    _window(GfVec2d(-1.0, -1.0), GfVec2d(1.0, 1.0)),
    _nearFar(1.0, 10.0),
    _viewDistance(5.0),
    _projectionType(GfFrustum::Perspective)
{
    _rotation.SetIdentity();
}

GfFrustum::GfFrustum(const GfVec3d &position, const GfRotation &rotation,
                     const GfRange2d &window, const GfRange1d &nearFar,
                     GfFrustum::ProjectionType projectionType,
                     double viewDistance) :
    _position(position),
    _rotation(rotation),
    _window(window),
    _nearFar(nearFar),
    _viewDistance(viewDistance),
    _projectionType(projectionType)
{
}

GfFrustum::GfFrustum(const GfMatrix4d &camToWorldXf,
                     const GfRange2d &window, const GfRange1d &nearFar,
                     GfFrustum::ProjectionType projectionType,
                     double viewDistance) :
    _window(window),
    _nearFar(nearFar),
    _viewDistance(viewDistance),
    _projectionType(projectionType)
{
    SetPositionAndRotationFromMatrix(camToWorldXf);
}

GfFrustum::~GfFrustum()
{
}

void
GfFrustum::SetPerspective(double fieldOfViewHeight, double aspectRatio,
                          double nearDistance, double farDistance)
{
    SetPerspective(fieldOfViewHeight, true, aspectRatio,
                   nearDistance, farDistance);
}

void
GfFrustum::SetPerspective(double fieldOfView, bool isFovVertical,
                          double aspectRatio,
                          double nearDistance, double farDistance)
{
    _projectionType = GfFrustum::Perspective;

    double yDist = 1.0;
    double xDist = 1.0;

    // Check for 0, use 1 in that case
    if (aspectRatio == 0.0) {
        aspectRatio = 1.0;
    }

    if (isFovVertical) {
        // vertical is taken from the given field of view  
        yDist = tan(GfDegreesToRadians(fieldOfView / 2.0))
                        * GetReferencePlaneDepth();
        // horizontal is determined by aspect ratio
        xDist = yDist * aspectRatio;
    } else {
        // horizontal is taken from the given field of view  
        xDist = tan(GfDegreesToRadians(fieldOfView / 2.0))
                        * GetReferencePlaneDepth();
        // vertical is determined by aspect ratio
        yDist = xDist / aspectRatio;
    }

    _window.SetMin(GfVec2d(-xDist, -yDist));
    _window.SetMax(GfVec2d(xDist, yDist));
    _nearFar.SetMin(nearDistance);
    _nearFar.SetMax(farDistance);

    _DirtyFrustumPlanes();
}

bool
GfFrustum::GetPerspective(double *fieldOfViewHeight, double *aspectRatio,
                          double *nearDistance, double *farDistance) const
{
    return GetPerspective(true, fieldOfViewHeight, aspectRatio,
                          nearDistance, farDistance);
}

bool
GfFrustum::GetPerspective(bool isFovVertical,
                          double *fieldOfView, double *aspectRatio,
                          double *nearDistance, double *farDistance) const
{
    if (_projectionType != GfFrustum::Perspective)
        return false;

    GfVec2d winSize = _window.GetSize();

    if (isFovVertical) {
        *fieldOfView = 
                2.0 * GfRadiansToDegrees( atan(winSize[1] / 2.0) 
                                      / GetReferencePlaneDepth() );
    } else {
        *fieldOfView = 
                2.0 * GfRadiansToDegrees( atan(winSize[0] / 2.0) 
                                      / GetReferencePlaneDepth() );
    }
    *aspectRatio       = winSize[0] / winSize[1];

    *nearDistance = _nearFar.GetMin();
    *farDistance  = _nearFar.GetMax();

    return true;
}

double
GfFrustum::GetFOV(bool isFovVertical /* = false */)
{
    double result = 0.0;

    if (GetProjectionType() == GfFrustum::Perspective) {
        double aspectRatio;
        double nearDistance;
        double farDistance;

        GetPerspective(isFovVertical,
                       &result,
                       &aspectRatio,
                       &nearDistance,
                       &farDistance);
    }

    return result;
}

void
GfFrustum::SetOrthographic(double left, double right,
                           double bottom, double top,
                           double near, double far)
{
    _projectionType = GfFrustum::Orthographic;

    _window.SetMin(GfVec2d(left, bottom));
    _window.SetMax(GfVec2d(right, top));
    _nearFar.SetMin(near);
    _nearFar.SetMax(far);

    _DirtyFrustumPlanes();
}

bool
GfFrustum::GetOrthographic(double *left, double *right,
                           double *bottom, double *top,
                           double *near, double *far) const
{
    if (_projectionType != GfFrustum::Orthographic)
        return false;

    *left   = _window.GetMin()[0];
    *right  = _window.GetMax()[0];
    *bottom = _window.GetMin()[1];
    *top    = _window.GetMax()[1];

    *near   = _nearFar.GetMin();
    *far    = _nearFar.GetMax();

    return true;
}

void
GfFrustum::FitToSphere(const GfVec3d &center, double radius, double slack)
{
    //
    // The first part of this computes a good value for
    // _viewDistance and modifies the side (left, right, bottom,
    // and top) coordinates of the frustum as necessary.
    //

    if (_projectionType == GfFrustum::Orthographic) {
        // Set the distance so the viewpoint is outside the sphere.
        _viewDistance = radius + slack;
        // Set the camera window to enclose the sphere.
        _window = GfRange2d(GfVec2d(-radius, -radius),
                            GfVec2d(radius, radius));
    }
    else {
        // Find the plane coordinate to use to compute the view.
        // Assuming symmetry, it should be the half-size of the
        // smaller of the two viewing angles. If asymmetric in a
        // dimension, use the larger size in that dimension.
        size_t whichDim = ComputeAspectRatio() > 1.0 ? 1 : 0;

        // XXX-doc
        double min = _window.GetMin()[whichDim];
        double max = _window.GetMax()[whichDim];
        double halfSize;
        if (min > 0.0) {
            halfSize = max;
        } else if (max < 0.0) {
            // CODE_COVERAGE_OFF_GCOV_BUG - seems to be hit but gcov disagrees
            halfSize = min;
            // CODE_COVERAGE_ON_GCOV_BUG
        } else if (-min > max) {
            halfSize = min;
        } else {
            halfSize = max;
        }

        if (halfSize < 0.0) {
            halfSize = -halfSize;
        } else if (halfSize == 0.0) {
            halfSize = 1.0;     // Why not?
        }

        // Determine the distance of the viewpoint from the center of
        // the sphere to make the frustum tangent to the sphere. Use
        // similar triangles: the right triangle formed by the
        // viewpoint and the half-size on the plane is similar to the
        // right triangle formed by the viewpoint and the radius of
        // the sphere at the point of tangency.
        _viewDistance =
            radius * (1.0/halfSize) *
             sqrt(GfSqr(halfSize) + GfSqr(_nearFar.GetMin()));

        // XXX.
        // Hmmm. This is not really used anywhere but in tests, so 
        // not gonna fix right now but it seems to me the above equation is 
        // off.
        // In the diagram below, similar triangles yield the following 
        // equal ratios:
        //    halfSize / referencePlaneDepth = radius / tanDist
        // So tanDist = (radius * referencePlaneDepth) / halfSize
        // Then, because it's a right triangle:
        // viewDistance = sqrt( GfSqr(radius) + GfSqr(tanDist))

        /*

          -----    |\                  /
            ^      |  \ ra            /
            |      |    \ di         /
            |      |      \ us      /
            |      |        \      /
            |      |          \   /
            |      |            \/      <---- make believe this is a right angle
            |      |------------/ ------
            |      |           /     ^
            |      |          /      |
      viewDistance |         /       |
            |      |        /        |
            |      |       /t        |
            |      |      /s        referencePlaneDepth 
            |      |     /i          |
            |      |    /d           |
            |      |   /n            |
            |      |  /a             |
            |      | /t              v
            v      |/            ------
         ------
                   |            |
                   |<-halfSize->|
                   |            |
                   |            |
        */
    }

    // Adjust the camera so the near plane touches the sphere and the
    // far plane encloses the sphere.
    _nearFar.SetMin(_viewDistance - (radius + slack));
    _nearFar.SetMax(_nearFar.GetMin() + 2.0 * (radius + slack));

    // Set the camera to use the new position. The view direction
    // should not have changed.
    _position = center - _viewDistance * ComputeViewDirection();
}

GfFrustum &
GfFrustum::Transform(const GfMatrix4d &matrix)
{
    // We'll need the old parameters as we build up the new ones, so, work
    // on a newly instantiated frustum. We'll replace the contents of
    // this frustum with it once we are done. Note that _dirty is true
    // by default, so, there is no need to initialize it here.
    GfFrustum frustum;

    // Copy the projection type
    frustum._projectionType = _projectionType;

    // Transform the position of the frustum
    frustum._position = matrix.Transform(_position);

    // Transform the rotation as follows:
    //   1. build view and direction vectors
    //   2. transform them with the given matrix
    //   3. normalize the vectors and cross them to build an orthonormal frame
    //   4. construct a rotation matrix
    //   5. extract the new rotation from the matrix
    
    // Generate view direction and up vector
    GfVec3d viewDir = ComputeViewDirection();
    GfVec3d upVec   = ComputeUpVector();

    // Transform by matrix
    GfVec3d viewDirPrime = matrix.TransformDir(viewDir);
    GfVec3d upVecPrime = matrix.TransformDir(upVec);

    // Normalize. Save the vec size since it will be used to scale near/far.
    double scale = viewDirPrime.Normalize();
    upVecPrime.Normalize();

    // Cross them to get the third axis. Voila. We have an orthonormal frame.
    GfVec3d viewRightPrime = GfCross(viewDirPrime, upVecPrime);
    viewRightPrime.Normalize();

    // Construct a rotation matrix using the axes.
    //
    //  [ right     0 ]
    //  [ up        1 ]
    //  [ -viewDir  0 ]
    //  [ 0  0   0  1 ]
    GfMatrix4d rotMatrix;
    rotMatrix.SetIdentity();
    // first row
    rotMatrix[0][0] = viewRightPrime[0];
    rotMatrix[0][1] = viewRightPrime[1];
    rotMatrix[0][2] = viewRightPrime[2];

    // second row
    rotMatrix[1][0] = upVecPrime[0];
    rotMatrix[1][1] = upVecPrime[1];
    rotMatrix[1][2] = upVecPrime[2];

    // third row
    rotMatrix[2][0] = -viewDirPrime[0];
    rotMatrix[2][1] = -viewDirPrime[1];
    rotMatrix[2][2] = -viewDirPrime[2];

    // Extract rotation
    frustum._rotation = rotMatrix.ExtractRotation();

    // Since we applied the matrix to the direction vector, we can use
    // its length to find out the scaling that needs to applied to the
    // near and far plane. 
    frustum._nearFar = _nearFar * scale;

    // Use the same length to scale the view distance
    frustum._viewDistance = _viewDistance * scale;

    // Transform the reference plane as follows:
    //
    //   - construct two 3D points that are on the reference plane 
    //     (left/bottom and right/top corner of the reference window) 
    //   - transform the points with the given matrix
    //   - move the window back to one unit from the viewpoint and
    //     extract the 2D coordinates that would form the new reference
    //     window
    //
    //     A note on how we do the last "move" of the reference window:
    //     Using similar triangles and the fact that the reference window
    //     is one unit away from the viewpoint, one can show that it's 
    //     sufficient to divide the x and y components of the transformed
    //     corners by the length of the transformed direction vector.
    //
    //     A 2D diagram helps:
    //
    //                            |
    //                            |
    //               |            |
    //       * ------+------------+
    //      vp       |y1          |
    //                            |
    //       \--d1--/             |y2
    //
    //       \-------d2----------/
    //
    //     So, y1/y2 = d1/d2 ==> y1 = y2 * d1/d2 
    //     Since d1 = 1 ==> y1 = y2 / d2
    //     The same argument applies to the x coordinate.
    //
    // NOTE: In an orthographic projection, the last step (division by
    // the length of the vector) is skipped.
    //
    // XXX NOTE2:  The above derivation relies on the
    // fact that GetReferecePlaneDepth() is 1.0.
    // If we ever allow this to NOT be 1, we'll need to fix this up.

    const GfVec2d &min = _window.GetMin();
    const GfVec2d &max = _window.GetMax();

    // Construct the corner points in 3D as follows: construct a starting 
    // point by using the x and y coordinates of the reference plane and 
    // -1 as the z coordinate. Add the position of the frustum to generate 
    // the actual points in world-space coordinates.
    GfVec3d leftBottom = 
        _position + _rotation.TransformDir(GfVec3d(min[0], min[1], -1.0));
    GfVec3d rightTop = 
        _position + _rotation.TransformDir(GfVec3d(max[0], max[1], -1.0));

    // Now, transform the corner points by the given matrix
    leftBottom = matrix.Transform(leftBottom);
    rightTop   = matrix.Transform(rightTop);

    // Subtract the transformed frustum position from the transformed
    // corner points. Then, rotate the points using the rotation that would
    // transform the view direction vector back to (0, 0, -1). This brings 
    // the corner points from the woorld coordinate system into the local 
    // frustum one.
    leftBottom -= frustum._position;
    rightTop   -= frustum._position;
    leftBottom = frustum._rotation.GetInverse().TransformDir(leftBottom);
    rightTop   = frustum._rotation.GetInverse().TransformDir(rightTop);

    // Finally, use the similar triangles trick to bring the corner
    // points back at one unit away from the point. These scaled x and
    // y coordinates can be directly used to construct the new
    // transformed reference plane.  Skip the scaling step for an
    // orthographic projection, though.
    if (_projectionType == Perspective) {
        leftBottom /= scale;
        rightTop   /= scale;
    }

    frustum._window.SetMin(GfVec2d(leftBottom[0], leftBottom[1]));
    frustum._window.SetMax(GfVec2d(rightTop[0],   rightTop[1]));

    // Note that negative scales in the transform have the potential
    // to flip the window.  Fix it if necessary.
    GfVec2d wMin = frustum._window.GetMin();
    GfVec2d wMax = frustum._window.GetMax();
    // Make sure left < right
    if ( wMin[0] > wMax[0] ) {
        swap( wMin[0], wMax[0] );
    }
    // Make sure bottom < top
    if ( wMin[1] > wMax[1] ) {
        swap( wMin[1], wMax[1] );
    }
    frustum._window.SetMin( wMin );
    frustum._window.SetMax( wMax );

    *this = frustum;

    return *this;
}

GfVec3d
GfFrustum::ComputeViewDirection() const
{
    return _rotation.TransformDir(-GfVec3d::ZAxis());
}

GfVec3d
GfFrustum::ComputeUpVector() const
{
    return _rotation.TransformDir(GfVec3d::YAxis());
}

void
GfFrustum::ComputeViewFrame(GfVec3d *side, 
                            GfVec3d *up, 
                            GfVec3d *view) const
{
    *up   = ComputeUpVector();
    *view = ComputeViewDirection();
    *side = GfCross(*view, *up);
}

GfVec3d
GfFrustum::ComputeLookAtPoint() const
{
    return _position + _viewDistance * ComputeViewDirection();
}

GfMatrix4d
GfFrustum::ComputeViewMatrix() const
{
    GfMatrix4d m;
    m.SetLookAt(_position, _rotation);
    return m;
}

GfMatrix4d
GfFrustum::ComputeViewInverse() const
{
    return ComputeViewMatrix().GetInverse();
}

GfMatrix4d
GfFrustum::ComputeProjectionMatrix() const
{
    // Build the projection matrix per Section 2.11 of
    // The OpenGL Specification: Coordinate Transforms.
    GfMatrix4d matrix;
    matrix.SetIdentity();

    const double l = _window.GetMin()[0];
    const double r = _window.GetMax()[0];
    const double b = _window.GetMin()[1];
    const double t = _window.GetMax()[1];
    const double n = _nearFar.GetMin();
    const double f = _nearFar.GetMax();

    const double rl = r - l;
    const double tb = t - b;
    const double fn = f - n;

    if (_projectionType == GfFrustum::Orthographic) {
        matrix[0][0] =  2.0 / rl;
        matrix[1][1] =  2.0 / tb;
        matrix[2][2] = -2.0 / fn;
        matrix[3][0] = -(r + l) / rl;
        matrix[3][1] = -(t + b) / tb;
        matrix[3][2] = -(f + n) / fn;
    }
    else {
        // Perspective:
        // The window coordinates are specified with respect to the
        // reference plane (near == 1).
        // XXX Note: If we ever allow reference plane depth to be other 
        // than 1.0, we'll need to revisit this.
        matrix[0][0] = 2.0 / rl;
        matrix[1][1] = 2.0 / tb;
        matrix[2][2] = -(f + n) / fn;
        matrix[2][0] =  (r + l) / rl;
        matrix[2][1] =  (t + b) / tb;
        matrix[3][2] = -2.0 * n * f / fn;
        matrix[2][3] = -1.0;
        matrix[3][3] =  0.0;
    }

    return matrix;
}

double
GfFrustum::ComputeAspectRatio() const
{
    GfVec2d winSize = _window.GetSize();
    double aspectRatio = 0.0;

    if (winSize[1] != 0.0)
        // Negative winsize is used for envcubes, believe it or not.
        aspectRatio = fabs(winSize[0] / winSize[1]);

    return aspectRatio;
}

vector<GfVec3d>
GfFrustum::ComputeCorners() const
{
    const GfVec2d &winMin = _window.GetMin();
    const GfVec2d &winMax = _window.GetMax();
    double near           = _nearFar.GetMin();
    double far            = _nearFar.GetMax();

    vector<GfVec3d> corners;
    corners.reserve(8);

    if (_projectionType == Perspective) {
        // Compute the eye-space corners of the near-plane and
        // far-plane frustum rectangles using similar triangles. The
        // reference plane in which the window rectangle is defined is
        // a distance of 1 from the eyepoint. By similar triangles,
        // just multiply the window points by near and far to get the
        // near and far rectangles.
        // XXX Note: If we ever allow reference plane depth to be other 
        // than 1.0, we'll need to revisit this.
        corners.push_back(GfVec3d(near * winMin[0], near * winMin[1], -near));
        corners.push_back(GfVec3d(near * winMax[0], near * winMin[1], -near));
        corners.push_back(GfVec3d(near * winMin[0], near * winMax[1], -near));
        corners.push_back(GfVec3d(near * winMax[0], near * winMax[1], -near));
        corners.push_back(GfVec3d(far  * winMin[0], far  * winMin[1], -far));
        corners.push_back(GfVec3d(far  * winMax[0], far  * winMin[1], -far));
        corners.push_back(GfVec3d(far  * winMin[0], far  * winMax[1], -far));
        corners.push_back(GfVec3d(far  * winMax[0], far  * winMax[1], -far));
    }
    else {
        // Just use the reference plane rectangle as is, translated to
        // the near and far planes.
        corners.push_back(GfVec3d(winMin[0], winMin[1], -near));
        corners.push_back(GfVec3d(winMax[0], winMin[1], -near));
        corners.push_back(GfVec3d(winMin[0], winMax[1], -near));
        corners.push_back(GfVec3d(winMax[0], winMax[1], -near));
        corners.push_back(GfVec3d(winMin[0], winMin[1], -far));
        corners.push_back(GfVec3d(winMax[0], winMin[1], -far));
        corners.push_back(GfVec3d(winMin[0], winMax[1], -far));
        corners.push_back(GfVec3d(winMax[0], winMax[1], -far));
    }

    // Each corner is then transformed into world space by the inverse
    // of the view matrix.
    GfMatrix4d m = ComputeViewInverse();
    for (int i = 0; i < 8; i++)
        corners[i] = m.Transform(corners[i]);

    return corners;
}

vector<GfVec3d>
GfFrustum::ComputeCornersAtDistance(double d) const
{
    const GfVec2d &winMin = _window.GetMin();
    const GfVec2d &winMax = _window.GetMax();

    vector<GfVec3d> corners;
    corners.reserve(4);

    if (_projectionType == Perspective) {
        // Similar to ComputeCorners
        corners.push_back(GfVec3d(d * winMin[0], d * winMin[1], -d));
        corners.push_back(GfVec3d(d * winMax[0], d * winMin[1], -d));
        corners.push_back(GfVec3d(d * winMin[0], d * winMax[1], -d));
        corners.push_back(GfVec3d(d * winMax[0], d * winMax[1], -d));
    }
    else {
        corners.push_back(GfVec3d(winMin[0], winMin[1], -d));
        corners.push_back(GfVec3d(winMax[0], winMin[1], -d));
        corners.push_back(GfVec3d(winMin[0], winMax[1], -d));
        corners.push_back(GfVec3d(winMax[0], winMax[1], -d));
    }

    // Each corner is then transformed into world space by the inverse
    // of the view matrix.
    const GfMatrix4d m = ComputeViewInverse();
    for (int i = 0; i < 4; i++)
        corners[i] = m.Transform(corners[i]);

    return corners;
}

GfFrustum
GfFrustum::ComputeNarrowedFrustum(const GfVec2d &point,
                                  const GfVec2d &halfSize) const
{
    // Map the point from normalized space (-1 to 1) onto the frustum's
    // window. First, convert the point into the range from 0 to 1,
    // then interpolate in the window rectangle.
    GfVec2d scaledPoint = .5 * (GfVec2d(1.0, 1.0) + point);
    GfVec2d windowPoint = _window.GetMin() + GfCompMult(scaledPoint,
                                                        _window.GetSize());

    return _ComputeNarrowedFrustumSub(windowPoint, halfSize);
}

GfFrustum
GfFrustum::ComputeNarrowedFrustum(const GfVec3d &worldPoint,
                                  const GfVec2d &halfSize) const
{
    // Map the point from worldspace onto the frustum's window
    GfVec3d lclPt = ComputeViewMatrix().Transform(worldPoint);
    if (lclPt[2] >= 0) {
        TF_WARN("Given worldPoint is behind or at the eye");
        // Start with this frustum
        return *this;
    }
    double scaleFactor = _nearFar.GetMin() / -lclPt[2]; 
    GfVec2d windowPoint(lclPt[0] * scaleFactor, lclPt[1] * scaleFactor);

    return _ComputeNarrowedFrustumSub(windowPoint, halfSize);
}

GfFrustum
GfFrustum::_ComputeNarrowedFrustumSub(const GfVec2d windowPoint, 
                                  const GfVec2d &halfSize) const
{
    // Start with this frustum
    GfFrustum narrowedFrustum = *this;

    // Also convert the sizes.
    GfVec2d halfSizeOnRefPlane = .5 * GfCompMult(halfSize, _window.GetSize());

    // Shrink the narrowed frustum's window to surround the point.
    GfVec2d min = windowPoint - halfSizeOnRefPlane;
    GfVec2d max = windowPoint + halfSizeOnRefPlane;

    // Make sure the new bounds are within the old window.
    if (min[0] < _window.GetMin()[0])
        min[0] = _window.GetMin()[0];
    if (min[1] < _window.GetMin()[1])
        min[1] = _window.GetMin()[1];
    if (max[0] > _window.GetMax()[0])
        max[0] = _window.GetMax()[0];
    if (max[1] > _window.GetMax()[1])
        max[1] = _window.GetMax()[1];

    // Set the window to the result.
    narrowedFrustum.SetWindow(GfRange2d(min, max));

    return narrowedFrustum;
}

// Utility function for mapping an input value from
// one range to another.
static double
_Rescale(double in,
        double inA, double inB,
        double outA, double outB )
{
    double factor = (inA==inB) ? 0.0 : ((inA-in) / (inA-inB));
    return outA + ((outB-outA)*factor);
}

static GfRay _ComputeUntransformedRay(GfFrustum::ProjectionType projectionType,
                                      const GfRange2d &window,
                                      const GfVec2d &windowPos)
{
    // Compute position on window, from provided normalized
    // (-1 to 1) coordinates.
    double winX = _Rescale(windowPos[0], -1.0, 1.0,
                           window.GetMin()[0], window.GetMax()[0]);
    double winY = _Rescale(windowPos[1], -1.0, 1.0,
                           window.GetMin()[1], window.GetMax()[1]);

    // Compute the camera-space starting point (the viewpoint) and
    // direction (toward the point on the window).
    GfVec3d pos;
    GfVec3d dir;
    if (projectionType == GfFrustum::Perspective) {
        pos = GfVec3d(0);
        dir = GfVec3d(winX, winY, -1.0).GetNormalized();
    }
    else {
        pos.Set(winX, winY, 0.0);
        dir = -GfVec3d::ZAxis();
    }

    // Build and return the ray
    return GfRay(pos, dir);
}

GfRay
GfFrustum::ComputeRay(const GfVec2d &windowPos) const
{
    GfRay ray = _ComputeUntransformedRay(_projectionType, _window, windowPos);

    // Transform these by the inverse of the view matrix.
    const GfMatrix4d &viewInverse = ComputeViewInverse();
    GfVec3d rayFrom = viewInverse.Transform(ray.GetStartPoint());
    GfVec3d rayDir = viewInverse.TransformDir(ray.GetDirection());

    // Build and return the ray
    return GfRay(rayFrom, rayDir);
}

GfRay
GfFrustum::ComputePickRay(const GfVec2d &windowPos) const
{
    GfRay ray = _ComputeUntransformedRay(_projectionType, _window, windowPos);
    return _ComputePickRayOffsetToNearPlane(ray.GetStartPoint(), 
                                            ray.GetDirection());
}

GfRay               
GfFrustum::ComputeRay(const GfVec3d &worldSpacePos) const
{
    GfVec3d camSpaceToPos = ComputeViewMatrix().Transform(worldSpacePos);

    // Compute the camera-space starting point (the viewpoint) and
    // direction (toward the point camSpaceToPos).
    GfVec3d pos;
    GfVec3d dir;
    if (_projectionType == Perspective) {
        pos = GfVec3d(0);
        dir = camSpaceToPos.GetNormalized();
    }
    else {
        pos.Set(camSpaceToPos[0], camSpaceToPos[1], 0.0);
        dir = -GfVec3d::ZAxis();
    }

    // Transform these by the inverse of the view matrix.
    const GfMatrix4d &viewInverse = ComputeViewInverse();
    GfVec3d rayFrom = viewInverse.Transform(pos);
    GfVec3d rayDir = viewInverse.TransformDir(dir);

    // Build and return the ray
    return GfRay(rayFrom, rayDir);
}

GfRay               
GfFrustum::ComputePickRay(const GfVec3d &worldSpacePos) const
{
    GfVec3d camSpaceToPos = ComputeViewMatrix().Transform(worldSpacePos);

    // Compute the camera-space starting point (the viewpoint) and
    // direction (toward the point camSpaceToPos).
    GfVec3d pos;
    GfVec3d dir;
    if (_projectionType == Perspective) {
        pos = GfVec3d(0);
        dir = camSpaceToPos.GetNormalized();
    }
    else {
        pos.Set(camSpaceToPos[0], camSpaceToPos[1], 0.0);
        dir = -GfVec3d::ZAxis();
    }

    return _ComputePickRayOffsetToNearPlane(pos, dir);
}

GfRay               
GfFrustum::_ComputePickRayOffsetToNearPlane(const GfVec3d &camSpaceFrom, 
                          const GfVec3d &camSpaceDir) const
{
    // Move the starting point to the near plane so we don't pick
    // anything that's clipped out of view.
    GfVec3d rayFrom = camSpaceFrom + _nearFar.GetMin() * camSpaceDir;

    // Transform these by the inverse of the view matrix.
    const GfMatrix4d &viewInverse = ComputeViewInverse();
    rayFrom = viewInverse.Transform(rayFrom);
    GfVec3d rayDir = viewInverse.TransformDir(camSpaceDir);

    // Build and return the ray
    return GfRay(rayFrom, rayDir);
}

bool
GfFrustum::Intersects(const GfBBox3d &bbox) const
{
    if (bbox.GetBox().IsEmpty())
        return false;
    
    // Recalculate frustum planes if necessary
    _CalculateFrustumPlanes();

    // Get the bbox in its local space and the matrix that converts
    // world space to that local space.
    const GfRange3d  &localBBox    = bbox.GetRange();
    const GfMatrix4d &worldToLocal = bbox.GetInverseMatrix();

    // Test the bbox against each of the frustum planes, transforming
    // the plane by the inverse of the matrix to bring it into the
    // bbox's local space.
    for (size_t i = 0; i < _planes.size(); i++) {

        GfPlane localPlane = _planes[i];
        localPlane.Transform(worldToLocal);

        if (! localPlane.IntersectsPositiveHalfSpace(localBBox))
            return false;
    }

    return true;
}

bool
GfFrustum::Intersects(const GfVec3d &point) const
{
    // Recalculate frustum planes if necessary
    _CalculateFrustumPlanes();

    // Determine if the point is inside/intersecting the frustum. Quit early
    // if the point is outside of any of the frustum planes.
    for (size_t i = 0; i < _planes.size(); i++) {
        if (!_planes[i].IntersectsPositiveHalfSpace(point)) {
            return false;
        }
    }

    return true;
}

inline static uint32_t
_CalcIntersectionBitMask( const std::vector<GfPlane> & planes,
                          GfVec3d const &p)
{
    return
        (1 << 0) * planes[0].IntersectsPositiveHalfSpace(p) +
        (1 << 1) * planes[1].IntersectsPositiveHalfSpace(p) +
        (1 << 2) * planes[2].IntersectsPositiveHalfSpace(p) +
        (1 << 3) * planes[3].IntersectsPositiveHalfSpace(p) +
        (1 << 4) * planes[4].IntersectsPositiveHalfSpace(p) +
        (1 << 5) * planes[5].IntersectsPositiveHalfSpace(p);
}

bool
GfFrustum::_SegmentIntersects(GfVec3d const &p0, uint32_t p0Mask,
                              GfVec3d const &p1, uint32_t p1Mask) const
{
    // If any of the 6 bits is 0 in both masks, then both points are
    // on the bad side of the corresponding plane. This means that
    // there can't be any intersection.
    if ((p0Mask | p1Mask) != 0x3F)
        return false;
    
    // If either of the masks has all 6 planes set, the point is
    // inside the frustum, so there's an intersection.
    if ((p0Mask == 0x3F) || (p1Mask == 0x3F))
        return true;

    // If we get here, the 2 points of the segment are both outside
    // the frustum, but not both on the outside of any single plane.

    // Now we can clip the segment against each plane that it
    // straddles to see if the resulting segment has any length.
    // Perform the clipping using parametric coordinates, where t=0
    // represents p0 and t=1 represents p1. Use v = the vector from p0
    // to p1.
    double t0 = 0.0;
    double t1 = 1.0;
    GfVec3d v = p1 - p0;

    for (size_t i=0; i < _planes.size(); ++i) {
        const GfPlane & plane = _planes[i];
        const uint32_t planeBit = 1 << i;

        uint32_t p0Bit = p0Mask & planeBit;
        uint32_t p1Bit = p1Mask & planeBit;

        // Do this only if the points straddle the plane, meaning they
        // have different values for the bit.
        if (p0Bit == p1Bit)
            continue;

        // To find the parametric distance t at the intersection of a
        // plane and the line defined by (p0 + t * v):
        //
        //   Substitute the intersection point (p0 + t * v) into the
        //   plane equation to get   n . (p0 + t * v) - d = 0
        //
        //   Solve for t:  t = - (n . p0 - d) / (n . v)
        //      But (n . p0 - d) is the distance of p0 from the plane.
        double t = -plane.GetDistance(p0) / (plane.GetNormal() * v);

        // If p0 is inside and p1 is outside, replace t1. Otherwise,
        // replace t0.
        if (p0Bit) {
            if (t < t1)
                t1 = t;
        } else {
            if (t > t0)
                t0 = t;
        }

        // If there is no line segment left, there's no intersection.
        if (t0 > t1)
            return false;
    }    

    // If we get here, there's an intersection
    return true;
}    

bool
GfFrustum::Intersects(const GfVec3d &p0, const GfVec3d &p1) const
{
    // Recalculate frustum planes if necessary
    _CalculateFrustumPlanes();

    // Compute the intersection masks for each point. There is one bit
    // in each mask for each of the 6 planes.
    return _SegmentIntersects(p0, _CalcIntersectionBitMask(_planes, p0),
                              p1, _CalcIntersectionBitMask(_planes, p1));

}

bool
GfFrustum::Intersects(const GfVec3d &p0,
                      const GfVec3d &p1,
                      const GfVec3d &p2) const
{
    // Recalculate frustum planes if necessary
    _CalculateFrustumPlanes();

    // Compute the intersection masks for each point. There is one bit
    // in each mask for each of the 6 planes.
    uint32_t p0Mask = _CalcIntersectionBitMask(_planes, p0);
    uint32_t p1Mask = _CalcIntersectionBitMask(_planes, p1);
    uint32_t p2Mask = _CalcIntersectionBitMask(_planes, p2);

    // If any of the 6 bits is 0 in all masks, then all 3 points are
    // on the bad side of the corresponding plane. This means that
    // there can't be any intersection.
    if ((p0Mask | p1Mask | p2Mask) != 0x3F)
        return false;
    
    // If any of the masks has all 6 planes set, the point is inside
    // the frustum, so there's an intersection.
    if ((p0Mask == 0x3F) || (p1Mask == 0x3F) || (p2Mask == 0x3F))
        return true;

    // If we get here, the 3 points of the triangle are all outside
    // the frustum, but not all on the outside of any single plane.
    // There are now 3 remaining possibilities:
    //
    //  (1) At least one edge of the triangle intersects the frustum.
    //  (2) The triangle completely encloses the frustum.
    //  (3) Neither of the above is true, so there is no intersection.

    // Test case (1) by intersecting all three edges with the
    // frustum.
    if (_SegmentIntersects(p0, p0Mask, p1, p1Mask) ||
        _SegmentIntersects(p1, p1Mask, p2, p2Mask) ||
        _SegmentIntersects(p2, p2Mask, p0, p0Mask))
        return true;


    // That leaves cases (2) and (3). 

    // Test for case 2 by computing rays from the viewpoint
    // to the far corners, and doing a ray-triangle
    // intersection test.  
    // If all 3 points of the triangle lie between the near/far planes, 
    // then we only need to test intersection of 1 corner's ray.
    // Otherwise, we test all 4 corners and if any hit, the frustum is inside
    // the triangle.  If all miss, then the frustum is outside.
    // If the points don't lie between near/far, then  we have to test all 
    // 4 corners to catch the case when the triangle is being partially 
    // clipped by the near/far plane.
    size_t numCornersToCheck = 4;
    // XXX Note: 4 & 5 below are highly dependent on 
    // _CalculateFrustumPlanes implementation 
    uint32_t nearBit = (1 << 4);  
    uint32_t farBit  = (1 << 5);
    if ( (p0Mask & nearBit) && (p1Mask & nearBit) && (p2Mask & nearBit) &&
         (p0Mask & farBit)  && (p1Mask & farBit)  && (p2Mask & farBit) ) {
        numCornersToCheck = 1;
    }

    for (size_t i=0; i<numCornersToCheck; ++i) {
        GfVec2d pickPoint = (i==0) ? GfVec2d(-1.0, -1.0) :
                            (i==1) ? GfVec2d(-1.0,  1.0) :
                            (i==2) ? GfVec2d( 1.0,  1.0) :
                                     GfVec2d( 1.0, -1.0);
        GfRay pickRay = ComputePickRay(pickPoint);
        double distance;
        if ( pickRay.Intersect(p0, p1, p2, &distance, NULL, NULL) ) {
            return true;
        }
    }


    // Must be case 3.
    return false;
}

void
GfFrustum::_DirtyFrustumPlanes()
{
    _planes.clear();
}

void
GfFrustum::_CalculateFrustumPlanes() const
{
    if (!_planes.empty())
        return;

    _planes.reserve(6);

    // These are values we need to construct the planes.
    const GfVec2d &winMin = _window.GetMin();
    const GfVec2d &winMax = _window.GetMax();
    double near           = _nearFar.GetMin();
    double far            = _nearFar.GetMax();
    GfMatrix4d m          = ComputeViewInverse();

    // For a perspective frustum, we use the viewpoint and four
    // corners of the near-plane frustum rectangle to define the 4
    // planes forming the left, right, top, and bottom sides of the
    // frustum.
    if (_projectionType == GfFrustum::Perspective) {

        //
        // Get the eye-space viewpoint (the origin) and the four corners
        // of the near-plane frustum rectangle using similar triangles.
        //
        // This picture may help:   
        //                 
        //                  top of near plane
        //                  frustum rectangle
        //
        //                  + --
        //                / |  | 
        //              /   |  |
        //            /     |  | h
        //          /       |  |
        //        /         |  |                 
        //   vp +-----------+ --               
        //                    center of near plane frustum rectangle
        //      |___________|
        //           near    
        //
        // The height (h) of this triangle is found by the following
        // equation, based on the definition of the _window member
        // variable, which is the size of the image rectangle in the
        // reference plane (a distance of 1 from the viewpoint):
        //
        //      h       _window.GetMax()[1]
        //    ------ = --------------------
        //     near             1
        //
        // Solving for h gets the height of the triangle. Doing the
        // similar math for the other 3 sizes of the near-plane
        // rectangle is left as an exercise for the reader.
        //
        // XXX Note: If we ever allow reference plane depth to be other 
        // than 1.0, we'll need to revisit this.

        GfVec3d vp(0.0, 0.0, 0.0);
        GfVec3d lb(near * winMin[0], near * winMin[1], -near);
        GfVec3d rb(near * winMax[0], near * winMin[1], -near);
        GfVec3d lt(near * winMin[0], near * winMax[1], -near);
        GfVec3d rt(near * winMax[0], near * winMax[1], -near);

        // Transform all 5 points into world space by the inverse of the
        // view matrix (which converts from world space to eye space).
        vp = m.Transform(vp);
        lb = m.Transform(lb);
        rb = m.Transform(rb);
        lt = m.Transform(lt);
        rt = m.Transform(rt);

        // Construct the 6 planes. The three points defining each plane
        // should obey the right-hand-rule; they should be in counter-clockwise 
        // order on the inside of the frustum. This makes the intersection of 
        // the half-spaces defined by the planes the contents of the frustum.
        _planes.push_back( GfPlane(vp, lb, lt) );     // Left
        _planes.push_back( GfPlane(vp, rt, rb) );     // Right
        _planes.push_back( GfPlane(vp, rb, lb) );     // Bottom
        _planes.push_back( GfPlane(vp, lt, rt) );     // Top
        _planes.push_back( GfPlane(rb, lb, lt) );     // Near
    }

    // For an orthographic projection, we need only the four corners
    // of the near-plane frustum rectangle and the view direction to
    // define the 4 planes forming the left, right, top, and bottom
    // sides of the frustum.
    else {

        //
        // The math here is much easier than in the perspective case,
        // because we have parallel lines instead of triangles. Just
        // use the size of the image rectangle in the reference plane,
        // which is the same in the near plane.
        //
        GfVec3d lb(winMin[0], winMin[1], -near);
        GfVec3d rb(winMax[0], winMin[1], -near);
        GfVec3d lt(winMin[0], winMax[1], -near);
        GfVec3d rt(winMax[0], winMax[1], -near);

        // Transform the 4 points into world space by the inverse of
        // the view matrix (which converts from world space to eye
        // space).
        lb = m.Transform(lb);
        rb = m.Transform(rb);
        lt = m.Transform(lt);
        rt = m.Transform(rt);

        // Transform the canonical view direction (-z axis) into world
        // space.
        GfVec3d dir = m.TransformDir(-GfVec3d::ZAxis());

        // Construct the 5 planes from these 4 points and the
        // eye-space view direction.
        _planes.push_back( GfPlane(lt + dir, lt, lb) );       // Left
        _planes.push_back( GfPlane(rb + dir, rb, rt) );       // Right
        _planes.push_back( GfPlane(lb + dir, lb, rb) );       // Bottom
        _planes.push_back( GfPlane(rt + dir, rt, lt) );       // Top
        _planes.push_back( GfPlane(rb, lb, lt) );             // Near
    }

    // The far plane is the opposite to the near plane. To compute the 
    // distance from the origin for the far plane, we take the distance 
    // for the near plane, add the difference between the far and the near 
    // and then negate that. We do the negation since the far plane
    // faces the opposite direction. A small drawing would help:
    //
    //                               far - near
    //                     /---------------------------\ *
    //
    //        |           |                             |
    //        |           |                             |
    //        |           |                             |
    //   <----|---->      |                             |
    // fnormal|nnormal    |                             |
    //        |           |                             |
    //                near plane                     far plane
    //
    //         \---------/ *
    //          ndistance
    //         
    //         \---------------------------------------/ *
    //                         fdistance
    //
    // So, fdistance = - (ndistance + (far - near))
    _planes.push_back(
        GfPlane(-_planes[4].GetNormal(), 
                -(_planes[4].GetDistanceFromOrigin() + (far - near))) );
}

bool
GfFrustum::IntersectsViewVolume(const GfBBox3d &bbox,
                                const GfMatrix4d &viewProjMat)
{
    // This implementation is a standard technique employed in frustum
    // culling during rendering.  It correctly culls the box even from
    // view volumes that are not representable by a GfFrustum because of
    // skewed near/far planes, such as the ones produced by
    // presto shadowmap cameras.
    //
    // Its principle of operation:  If all 8 points of
    // the box, when transformed into clip coordinates,
    // are on one side or the other of each dimension's
    // clipping interval, then the entire
    // box volume must lie outside the view volume.

    // Compute the 8 points of the bbox in
    // bbox local space.
    GfVec4d points[8];
    const GfVec3d &localMin = bbox.GetRange().GetMin();
    const GfVec3d &localMax = bbox.GetRange().GetMax();
    points[0] = GfVec4d(localMin[0], localMin[1], localMin[2], 1);
    points[1] = GfVec4d(localMin[0], localMin[1], localMax[2], 1);
    points[2] = GfVec4d(localMin[0], localMax[1], localMin[2], 1);
    points[3] = GfVec4d(localMin[0], localMax[1], localMax[2], 1);
    points[4] = GfVec4d(localMax[0], localMin[1], localMin[2], 1);
    points[5] = GfVec4d(localMax[0], localMin[1], localMax[2], 1);
    points[6] = GfVec4d(localMax[0], localMax[1], localMin[2], 1);
    points[7] = GfVec4d(localMax[0], localMax[1], localMax[2], 1);

    // Transform bbox local space points into clip space
    for (int i = 0; i < 8; ++i) {
        points[i] = points[i] * bbox.GetMatrix() * viewProjMat;
    }

    // clipFlags is a 6-bit field with one bit per +/- per x,y,z,
    // or one per frustum plane.  If the points overlap the
    // clip volume in any axis, then clipFlags will be 0x3f (0b111111).
    int clipFlags = 0;
    for (int i = 0; i < 8; ++i) {
        GfVec4d clipPos = points[i];

        // flag is used as a 6-bit shift register, as we append
        // results of plane-side testing.  OR-ing all the flags
        // combines all the records of what plane-side the points
        // have been on.
        int flag = 0;
        for (int j = 0; j < 3; ++j) {
            // We use +/-clipPos[3] as the interval bound instead of 
            // 1,-1 because these coordinates are not normalized.
            flag = (flag << 1) | (clipPos[j] <  clipPos[3]);
            flag = (flag << 1) | (clipPos[j] > -clipPos[3]);
        }
        clipFlags |= flag;
    }

    return clipFlags == 0x3f;
}

void
GfFrustum::SetPositionAndRotationFromMatrix(
    const GfMatrix4d &camToWorldXf)
{
    // First conform matrix to be...
    GfMatrix4d conformedXf = camToWorldXf;
    // ... right handed
    if (!conformedXf.IsRightHanded()) {
        static GfMatrix4d flip(GfVec4d(-1.0, 1.0, 1.0, 1.0));
        conformedXf = flip * conformedXf;
    }

    // ... and orthonormal
    conformedXf.Orthonormalize();

    SetRotation(conformedXf.ExtractRotation());
    SetPosition(conformedXf.ExtractTranslation());
}

std::ostream &
operator<<(std::ostream& out, const GfFrustum& f)
{
    out << '['
            << Gf_OstreamHelperP(f.GetPosition())     << " "
            << Gf_OstreamHelperP(f.GetRotation())     << " "
            << Gf_OstreamHelperP(f.GetWindow())       << " "
            << Gf_OstreamHelperP(f.GetNearFar())      << " "
            << Gf_OstreamHelperP(f.GetViewDistance()) << " "
            << TfEnum::GetName(TfEnum(f.GetProjectionType()))
        << ']';

    return out;
}
