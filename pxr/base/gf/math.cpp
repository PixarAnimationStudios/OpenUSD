//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/math.h"

PXR_NAMESPACE_OPEN_SCOPE

double
GfMod(double a, double b)
{
    double c = fmod(a, b);
    if (a < 0)
        return c ? (b + c) : 0;
    else
        return c;
}

float
GfMod(float a, float b)
{
    double c = fmodf(a, b);
    if (a < 0)
        return c ? (b + c) : 0;
    else
        return c;
}

double
GfSmoothStep(double min, double max, double val, double slope0, double slope1)
{
    // Implements standard hermite formulation:
    // p(h) = (2h^3 - 3h^2 + 1)p0 + (h^3 - 2h^2 + h)m0 + 
    //        (-2h^3 + 3h^2)p1 + (h^3 - h^2)m1;
    if (val >= max) return 1.0;
    if (val <  min) return 0.0;

    // Note due to above, if here, max != min
    double dv = max - min;

    double h = ((val - min) / (max - min));

    double h2 = h * h;
    double h3 = h2 * h;

    // p1 term
    double v = -2.0 * h3 + 3.0 * h2;

    // p0 is always zero

    if (slope0 != 0.0) {
        // normalize the slope
        slope0 /= dv;
        v += (h3 - 2 * h2 + h) * slope0;
    }

    if (slope1 != 0.0) {
        // normalize the slope
        slope1 /= dv;
        v += (h3 - h2) * slope1;
    }

    return v;
}

double
GfGSmoothRamp(double t, double a0, double a1, double b0, double  b1, double s0,
    double  s1, double wid0, double wid1)
{
    double dx, dy, x, d0, d1, wnorm, w0, w1, y;
    
    // change variables to x in [0,1] and y in [0,1]
    dy = b1 - b0;
    dx = a1 - a0;
    if (dy == 0 || dx==0) {
        y = 0.0;
    }
    else {
        x = (t - a0)/dx;
        d0 = s0*dx/dy;
        d1 = s1*dx/dy;

        // make sure shoulder widths don't sum to more than 1
        wnorm = 1./ GfMax(1.0, wid0 + wid1);
        w0 = wid0 * wnorm;
        w1 = wid1 * wnorm;

        // compute y
        if (x <= 0.0)
            y  = 0.0;
        else if (x >= 1.0)
            y = 1.0;
        else {
            double xr = 2.0 - w0 - w1;
            double a = (2.0 - w1*d1 + (w1 - 2.0)*d0)/(2.0 * xr);
            double b = (2.0 - w0*d0 + (w0 - 2.0)*d1)/(2.0 * xr);
            if (x < w0)
                y = a*x*x/w0 + d0*x;
            else if (x > 1.0 - w1) {
                double omx = 1.0 - x;
                y = 1.0 - (b * omx * omx/w1) - (d1 * omx);
            }
            else {
                double ya = (a * w0) + (d0 * w0);
                double da = 2.0 * a + d0;
                y = ya + (x - w0)*da;
            }
        }
    }

    // map y back to Y and return.  Note: analytically y is always in
    // [0,1], but numerically it might have noise so clamp it.
    return GfClamp(y, 0.0, 1.0) * dy + b0;
}

PXR_NAMESPACE_CLOSE_SCOPE
