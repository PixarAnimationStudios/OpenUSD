//
// Copyright 2023 Pixar
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
#include "pxr/base/ts/mathUtils.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/quatd.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////
// Polynomial evaluation & root-solving


// Solve for the roots of a quadratic equation.
//
// From numerical recipes ch. 5.6f, stable quadratic roots.
// Return true if any real roots, false if not.
// Roots are sorted root0 <= root1.
//
bool
Ts_SolveQuadratic( const double poly[3], double *root0, double *root1 )
{
    double a,b,c,disc,q,sq;

    a=poly[2]; b=poly[1]; c=poly[0];

    disc = b*b - 4*a*c;
    sq = sqrt(fabs(disc));

    /* If this is zero, sqrt(disc) is too small for a float... this avoids
     * having an epsilon for size of discriminant... compute in
     * double and then cast to float. If it becomes 0, then we're ok. */

    // Linear case
    if (a == 0.0) {
        if (b == 0.0) {
            // Constant y=0; infinite roots.
            *root1 = *root0 = 0.0;
            return false;
        }
        *root1 = *root0 = -c / b;
        return true;
    }

    /* if the disciminant is positive or it's very close to zero,
       we'll go on */
    if ((disc >= 0.0) || (((float) sq) == 0.0f)) {

        if (b >= 0.0)
            q = -0.5 * (b + sq);
        else
            q = -0.5 * (b - sq);

        *root0 = q / a;

        /* if q is zero, then we avoid the divide by zero.
         * q is zero means that b is zero and c is zero.
         * that implies that the root is zero.
         */
        if (q != 0.0) 
            *root1 = c / q;
        else {
            /* b and c are zero, so this has gotta be zero */
            // I have not been able to construct test data to exercise
            // this case.
            *root1 = 0.0;
        }

        /* order root0 < root1 */
        if (*root0 > *root1) {
            std::swap(*root0, *root1);
        }
        return true;
    }

    // Zero real roots.
    *root1 = *root0 = 0.0;
    return false;
}

// Solve f(x) = y for x where f is a cubic polynomial.
static double
_SolveCubic_RegulaFalsi(
        const TsTime poly[4], TsTime y, const GfInterval& bounds )
{
    static const int NUM_ITERS = 20;
    static const double EPSILON_1 = 1e-4;
    static const double EPSILON_2 = 1e-6;

    double x0 = bounds.GetMin();
    double x1 = bounds.GetMax();
    double y0 = Ts_EvalCubic( poly, x0 ) - y;
    double y1 = Ts_EvalCubic( poly, x1 ) - y;
    double x, yEst;

    if (fabs(y0) < EPSILON_1) {
        return x0;
    }
    if (fabs(y1) < EPSILON_1) {
        // I have not been able to construct test data to exercise
        // this case.
        return x1;
    }
    if (y0 * y1 > 0) {
        // Either no root or 2 roots, so punt
        // I have not been able to construct test data to exercise
        // this case.
        return -1;
    }

    // Regula Falsi iteration
    for (int i = 0; i < NUM_ITERS; ++i ) {
        x = x0 - y0 * (x1 - x0) / (y1 - y0);
        yEst = Ts_EvalCubic(poly, x) - y;
        if (fabs(yEst) < EPSILON_2)
            break;
        if (y0 * yEst <= 0) {
            y1 = yEst;
            x1 = x;
        } else {
            y0 = yEst;
            x0 = x;
        }
    }
    return x;
}

// Solve f(x) = y for x in the given bounds where f is a cubic polynomial.
double
Ts_SolveCubicInInterval(
        const TsTime poly[4], const TsTime polyDeriv[3],
        TsTime y, const GfInterval& bounds)
{
    static const int NUM_ITERS = 20;
    static const double EPSILON = 1e-5;

    double x = (bounds.GetMin() + bounds.GetMax()) * 0.5;
    for (int i = 0; i < NUM_ITERS; ++i ) {
        double dx = (Ts_EvalCubic(poly, x) - y) /
            Ts_EvalQuadratic(polyDeriv, x);
        x -= dx;
        if (!bounds.Contains(x))
            return _SolveCubic_RegulaFalsi( poly, y, bounds );
        if (fabs(dx) < EPSILON)
            break;
    }
    return x;
}

// Solve f(x) = y for x where f is a cubic polynomial.
    double
Ts_SolveCubic(const TsTime poly[4], TsTime y)
{
    double root0 = 0, root1 = 1;
    GfInterval bounds(0, 1);

    // Check to see if the first derivative ever goes to 0 in the interval
    // [0,1].  If it does then the cubic is not monotonically increasing
    // in that interval.
    double polyDeriv[3];
    Ts_CubicDerivative( poly, polyDeriv );
    if (Ts_SolveQuadratic( polyDeriv, &root0, &root1 )) {
        if (root0 >= 0.0 && root1 <= 1.0) {
            // The curve's inverse doubles back on itself in the interval
            // (root0,root1).  In that interval there are 3 solutions
            // for any y.  To disambiguate the solutions we'll use the
            // solution in [0,root0] for y < tmid and the solution in
            // [root1,1] for y >= tmid, where tmid is some value for
            // which there are 3 solutions.  We choose tmid as the
            // average of the values at root0 and root1.
            //
            // If the value at root0 is less than the value at root1 then
            // only the segment of the curve between root0 and root1 is
            // valid (monotonically increasing).  That shouldn't normally
            // happen but it's possible and will happen if Bezier tangent
            // lengths are zero.  In this case we'll use the [root0,root1]
            // interval.
            double t0  = Ts_EvalCubic( poly, 0 );
            double t1  = Ts_EvalCubic( poly, 1 );
            double tlo = Ts_EvalCubic( poly, root0 );
            double thi = Ts_EvalCubic( poly, root1 );
            double tmid = (GfClamp(tlo, t0, t1) + GfClamp(thi, t0, t1)) * 0.5;
            if (tlo < thi) {
                bounds = GfInterval( root0, root1 );
            }
            else if (tmid > y) {
                bounds = GfInterval( 0, root0 );
            } else {
                bounds = GfInterval( root1, 1 );
            }
        }
    }

    return Ts_SolveCubicInInterval(poly, polyDeriv, y, bounds);
}

PXR_NAMESPACE_CLOSE_SCOPE
