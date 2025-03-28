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
GfSmoothRamp(double tmin, double tmax, double t, double w0, double w1)
{
    if (t <= tmin) {
        return 0.0;
    }

    if (t >= tmax) {
        return 1.0;
    }

    double x = (t-tmin)/(tmax-tmin);
    double xr = 2.0 - w0 - w1;

    if (x < w0) {
        return (x*x)/(w0 * xr);
    }

    if (x > (1.0 - w1) ) {
        return (1.0 - ((1.0 - x) *  (1.0 - x))/ (w1 * xr));
    }

    return (2.0 * x - w0)/xr;
}

PXR_NAMESPACE_CLOSE_SCOPE
