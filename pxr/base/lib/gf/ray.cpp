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

#include "pxr/pxr.h"
#include "pxr/base/gf/ray.h"
#include "pxr/base/gf/line.h"
#include "pxr/base/gf/lineSeg.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/ostreamHelpers.h"
#include "pxr/base/gf/plane.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/tf/type.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// Tolerance for GfIsClose
static const double tolerance = 1e-6;

// CODE_COVERAGE_OFF_GCOV_BUG - gcov is finicky
TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfRay>();
}
// CODE_COVERAGE_ON_GCOV_BUG

// Menv 2x uses SetDirection, but it looks like it is only in
// sgu/rayIntersectAction.cpp so I'm just going to change the name
// and let the compiler complain about any problems.
void
GfRay::SetPointAndDirection(const GfVec3d &startPoint,
                            const GfVec3d &direction) {
    _startPoint = startPoint;
    _direction  = direction;
}

void
GfRay::SetEnds(const GfVec3d &startPoint, const GfVec3d &endPoint)
{
    _startPoint = startPoint;
    _direction  = endPoint - startPoint;
}

GfRay &
GfRay::Transform(const GfMatrix4d &matrix)
{
    _startPoint = matrix.Transform(_startPoint);
    _direction = matrix.TransformDir(_direction);
    
    return *this;
}

GfVec3d
GfRay::FindClosestPoint(const GfVec3d &point, double *rayDistance) const
{
    GfLine l;
    double len = l.Set(_startPoint, _direction);
    double lrd;
    (void) l.FindClosestPoint(point, &lrd);

    if (lrd < 0.0) lrd = 0.0;

    if (rayDistance)
	*rayDistance = lrd / len;

    return l.GetPoint(lrd);
}

bool
GfFindClosestPoints(const GfRay &ray, const GfLine &line,
		     GfVec3d *rayPoint,
		     GfVec3d *linePoint,
		     double *rayDistance,
		     double *lineDistance)
{
    GfLine l;
    double len = l.Set(ray._startPoint, ray._direction);

    GfVec3d rp, lp;
    double rd,ld;

    if (!GfFindClosestPoints(l, line, &rp, &lp, &rd, &ld))
	return false;

    if (rd < 0.0) rd = 0.0;

    if (rayPoint)
	*rayPoint = l.GetPoint(rd);

    if (linePoint)
	*linePoint = lp;

    if (rayDistance)
	*rayDistance = rd / len;

    if (lineDistance)
	*lineDistance = ld;

    return true;
}

bool 
GfFindClosestPoints(const GfRay &ray, const GfLineSeg &seg,
			  GfVec3d *rayPoint,
			  GfVec3d *segPoint,
			  double *rayDistance,
			  double *segDistance)
{
    GfLine l;
    double len = l.Set(ray._startPoint, ray._direction);

    GfVec3d rp, sp;
    double rd,sd;

    if (!GfFindClosestPoints(l, seg, &rp, &sp, &rd, &sd))
	return false;

    if (rd < 0.0) rd = 0.0;

    if (rayPoint)
	*rayPoint = l.GetPoint(rd);

    if (segPoint)
	*segPoint = sp;

    if (rayDistance)
	*rayDistance = rd / len;

    if (segDistance)
	*segDistance = sd;

    return true;
}

bool
GfRay::Intersect(const GfVec3d &p0, const GfVec3d &p1, const GfVec3d &p2,
                 double *distance,
                 GfVec3d *barycentricCoords, bool *frontFacing,
                 double maxDist) const
{
    // Intersect the ray with the plane containing the three points.
    GfPlane plane(p0, p1, p2);
    double intersectionDist;
    if (! Intersect(plane, &intersectionDist, frontFacing))
        return false;

    if (intersectionDist > maxDist)
        return false;

    // Find the largest component of the plane normal. The other two
    // dimensions are the axes of the aligned plane we will use to
    // project the triangle.
    double xAbs = GfAbs(plane.GetNormal()[0]);
    double yAbs = GfAbs(plane.GetNormal()[1]);
    double zAbs = GfAbs(plane.GetNormal()[2]);
    unsigned int axis0, axis1;
    if (xAbs > yAbs && xAbs > zAbs) {
	axis0 = 1;
	axis1 = 2;
    }
    else if (yAbs > zAbs) {
	axis0 = 2;
	axis1 = 0;
    }
    else {
	axis0 = 0;
	axis1 = 1;
    }

    // Determine if the projected intersection (of the ray's line and
    // the triangle's plane) lies within the projected triangle.
    // Since we deal with only 2 components, we can avoid the third
    // computation.
    double  inter0 = _startPoint[axis0] + intersectionDist * _direction[axis0];
    double  inter1 = _startPoint[axis1] + intersectionDist * _direction[axis1];
    GfVec2d d0(inter0    - p0[axis0], inter1     - p0[axis1]);
    GfVec2d d1(p1[axis0] - p0[axis0], p1[axis1] - p0[axis1]);
    GfVec2d d2(p2[axis0] - p0[axis0], p2[axis1] - p0[axis1]);

    // XXX This code can miss some intersections on very tiny tris.
    double alpha;
    double beta = ((d0[1] * d1[0] - d0[0] * d1[1]) /
                   (d2[1] * d1[0] - d2[0] * d1[1]));
    // clamp beta to 0 if it is only very slightly less than 0
    if (beta < 0 && beta > -GF_MIN_VECTOR_LENGTH) {
	// CODE_COVERAGE_OFF_NO_REPORT - architecture dependent numerics
        beta = 0;
	// CODE_COVERAGE_ON_NO_REPORT
    }
    if (beta < 0.0 || beta > 1.0) {
        return false;
    }
    alpha = -1.0;
    if (d1[1] < -GF_MIN_VECTOR_LENGTH || d1[1] > GF_MIN_VECTOR_LENGTH) 
	alpha = (d0[1] - beta * d2[1]) / d1[1];
    else
        alpha = (d0[0] - beta * d2[0]) / d1[0];

    // clamp alpha to 0 if it is only very slightly less than 0
    if (alpha < 0 && alpha > -GF_MIN_VECTOR_LENGTH) {
	// CODE_COVERAGE_OFF_NO_REPORT - architecture dependent numerics
        alpha = 0;
	// CODE_COVERAGE_ON_NO_REPORT
    }

    // clamp gamma to 0 if it is only very slightly less than 0
    float gamma = 1.0 - (alpha + beta);
    if (gamma < 0 && gamma > -GF_MIN_VECTOR_LENGTH) {
	// CODE_COVERAGE_OFF_NO_REPORT - architecture dependent numerics
        gamma = 0;
	// CODE_COVERAGE_ON_NO_REPORT
    }
    if (alpha < 0.0 || gamma < 0.0)
        return false;

    if (distance)
        *distance = intersectionDist;
    if (barycentricCoords)
        barycentricCoords->Set(gamma, alpha, beta);

    return true;
}

bool
GfRay::Intersect(const GfPlane &plane,
		 double *distance, bool *frontFacing) const
{
    // The dot product of the ray direction and the plane normal
    // indicates the angle between them. Reject glancing
    // intersections. Note: this also rejects ill-formed planes with
    // zero normals.
    double d = GfDot(_direction, plane.GetNormal());
    if (d < GF_MIN_VECTOR_LENGTH && d > -GF_MIN_VECTOR_LENGTH)
        return false;

    // Get a point on the plane.
    GfVec3d planePoint = plane.GetDistanceFromOrigin() * plane.GetNormal();

    // Compute the parametric distance t to the plane. Reject
    // intersections outside the ray bounds.
    double t = GfDot(planePoint - _startPoint, plane.GetNormal()) / d;
    if (t < 0.0)
	return false;

    if (distance)
	*distance = t;
    if (frontFacing)
	*frontFacing = (d < 0.0);

    return true;
}

bool
GfRay::Intersect(const GfRange3d &box,
		 double *enterDistance, double *exitDistance) const
{
    if (box.IsEmpty())
	return false;

    // Compute the intersection distance of all 6 planes of the
    // box. Save the largest near-plane intersection and the smallest
    // far-plane intersection.
    double maxNearest = -DBL_MAX, minFarthest = DBL_MAX;
    for (size_t i = 0; i < 3; i++) {

        // Skip dimensions almost parallel to the ray.
        double d = GetDirection()[i];
        if (GfAbs(d) < GF_MIN_VECTOR_LENGTH) {
            // ray is parallel to this set of planes.
            // If origin is not between them, no intersection.
            if (GetStartPoint()[i] < box.GetMin()[i] ||
                GetStartPoint()[i] > box.GetMax()[i]) {
                return false;
            } else {
                continue;
            }
        }

        d = 1.0 / d;
        double t1 = d * (box.GetMin()[i] - GetStartPoint()[i]);
        double t2 = d * (box.GetMax()[i] - GetStartPoint()[i]);

        // Make sure t1 is the nearer one
        if (t1 > t2) {
            double tmp = t1;
            t1 = t2;
            t2 = tmp;
        }

        // Update the min and max
        if (t1 > maxNearest)
            maxNearest = t1;
        if (t2 < minFarthest)
            minFarthest = t2;
    }

    // If the largest near-plane intersection is after the smallest
    // far-plane intersection, the ray's line misses the box. Also
    // check if both intersections are completely outside the near/far
    // bounds.
    if (maxNearest  >  minFarthest ||
        minFarthest < 0.0)
        return false;
        
    if (enterDistance)
	*enterDistance = maxNearest;
    if (exitDistance)
        *exitDistance = minFarthest;
    return true;
}

bool
GfRay::Intersect(const GfVec3d& center, double radius, 
                 double* enterDistance, double* exitDistance ) const
{

    GfVec3d p1 = _startPoint;
    GfVec3d p2 = p1 + _direction;

    // Intersection pt: p = p1 + u (p2 -p1)
    // we are solving for u.
    // where p1 = [x1 y1 z1],  p2 = [x2 y2 z2]
    //
    double A, B, C;
    double x1, x2, x3, y1, y2, y3, z1, z2, z3;
    x1 = p1[0];     y1 = p1[1];     z1 = p1[2];
    x2 = p2[0];     y2 = p2[1];     z2 = p2[2];
    x3 = center[0]; y3 = center[1]; z3 = center[2];

    A = (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) + (z2-z1)*(z2-z1);
    B = 2 * ((x2-x1)*(x1-x3) + (y2-y1)*(y1-y3) + (z2-z1)*(z1-z3));
    C = x3*x3 + y3*y3 + z3*z3 + x1*x1 + y1*y1 + z1*z1
        - 2*(x3*x1 +y3*y1 +z3*z1) - radius*radius;

    return _SolveQuadratic(A, B, C, enterDistance, exitDistance) > 0;
}

bool
GfRay::Intersect(const GfVec3d &origin,
                 const GfVec3d &axis,
                 const double radius,
                 double *enterDistance,
                 double *exitDistance) const
{
    GfVec3d unitAxis = axis.GetNormalized();
    
    GfVec3d delta = _startPoint - origin;
    GfVec3d u = _direction - GfDot(_direction, unitAxis) * unitAxis;
    GfVec3d v = delta - GfDot(delta, unitAxis) * unitAxis;
    
    // Quadratic equation for implicit infinite cylinder
    double a = GfDot(u, u);
    double b = 2.0 * GfDot(u, v);
    double c = GfDot(v, v) - GfSqr(radius);
    
    return _SolveQuadratic(a, b, c, enterDistance, exitDistance) > 0;
}

bool
GfRay::Intersect(const GfVec3d &origin,
                 const GfVec3d &axis,
                 const double radius, 
                 const double height,
                 double *enterDistance,
                 double *exitDistance) const
{ 
    GfVec3d unitAxis = axis.GetNormalized();
    
    // Apex of cone
    GfVec3d apex = origin + height * unitAxis;
    
    GfVec3d delta = _startPoint - apex;
    GfVec3d u =_direction - GfDot(_direction, unitAxis) * unitAxis;
    GfVec3d v = delta - GfDot(delta, unitAxis) * unitAxis;
    
    double p = GfDot(_direction, unitAxis);
    double q = GfDot(delta, unitAxis);
    
    double cos2 = GfSqr(height) / (GfSqr(height) + GfSqr(radius));
    double sin2 = 1 - cos2;
    
    double a = cos2 * GfDot(u, u) - sin2 * GfSqr(p);
    double b = 2.0 * (cos2 * GfDot(u, v) - sin2 * p * q);
    double c = cos2 * GfDot(v, v) - sin2 * GfSqr(q);
    
    if (!_SolveQuadratic(a, b, c, enterDistance, exitDistance)) {
        return false;
    }
    
    // Eliminate any solutions on the double cone
    bool enterValid = GfDot(unitAxis, GetPoint(*enterDistance) - apex) <= 0.0;
    bool exitValid = GfDot(unitAxis, GetPoint(*exitDistance) - apex) <= 0.0;
    
    if ((!enterValid) && (!exitValid)) {
        
        // Solutions lie only on double cone
        return false;
    }
    
    if (!enterValid) {
        *enterDistance = *exitDistance;
    }
    else if (!exitValid) {
        *exitDistance = *enterDistance;
    }
        
    return true;
}

bool
GfRay::_SolveQuadratic(const double a, 
                       const double b,
                       const double c,
                       double *enterDistance, 
                       double *exitDistance) const
{
    if (GfIsClose(a, 0.0, tolerance)) {
        if (GfIsClose(b, 0.0, tolerance)) {
            
            // Degenerate solution
            return false;
        }
        
        double t = -c / b;
        
        if (t < 0.0) {
            return false;
        }
        
        if (enterDistance) {
            *enterDistance = t;
        }
        
        if (exitDistance) {
            *exitDistance = t;
        }
        
        return true;
    }
    
    // Discriminant
    double disc = GfSqr(b) - 4.0 * a * c;
    
    if (GfIsClose(disc, 0.0, tolerance)) {
        
        // Tangent
        double t = -b / (2.0 * a);
        
        if (t < 0.0) {
            return false;
        }
        
        if (enterDistance) {
            *enterDistance = t;
        }
        
        if (exitDistance) {
            *exitDistance = t;
        }
        
        return true;
    }
    
    if (disc < 0.0) {

        // No intersection
        return false;
    }

    // Two intersection points
    double q = -0.5 * (b + copysign(1.0, b) * GfSqrt(disc));
    double t0 = q / a;
    double t1 = c / q;

    if (t0 > t1) {
        std::swap(t0, t1);
    }
    
    if (t1 >= 0) {

        if (enterDistance) {
            *enterDistance = t0;
        }
            
        if (exitDistance) {
            *exitDistance  = t1;
        }

        return true;
    }
    
    return false;
}

std::ostream &
operator<<(std::ostream& out, const GfRay& r)
{
    return out << '[' << Gf_OstreamHelperP(r.GetStartPoint()) << " >> " 
               << Gf_OstreamHelperP(r.GetDirection()) << ']';
}

PXR_NAMESPACE_CLOSE_SCOPE
