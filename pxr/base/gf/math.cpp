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

PXR_NAMESPACE_CLOSE_SCOPE
