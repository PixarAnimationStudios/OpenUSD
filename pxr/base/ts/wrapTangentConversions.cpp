//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tangentConversions.h"

#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/typeHelpers.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/diagnostic.h"

#include "pxr/external/boost/python.hpp"
#include "pxr/external/boost/python/tuple.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;


static object _WrapConvertToStandardTangent(
    const double widthIn,
    const double slopeOrHeightIn,
    bool convertHeightToSlope,
    bool divideValuesByThree,
    bool negateHeight)
{
    bool ok;
    double widthOut, slopeOut;

    ok = TsConvertToStandardTangent(
        widthIn, slopeOrHeightIn, convertHeightToSlope, divideValuesByThree,
        negateHeight, &widthOut, &slopeOut);

    if (ok) {
        return make_tuple(widthOut, slopeOut);
    } else {
        return object();
    }
}

static object _WrapConvertFromStandardTangent(
    const double widthIn,
    const double slopeIn,
    bool convertSlopeToHeight,
    bool multiplyValuesByThree,
    bool negateHeight)
{
    bool ok;
    double widthOut, slopeOrHeightOut;

    ok = TsConvertFromStandardTangent(
        widthIn, slopeIn, convertSlopeToHeight, multiplyValuesByThree,
        negateHeight, &widthOut, &slopeOrHeightOut);

    if (ok) {
        return make_tuple(widthOut, slopeOrHeightOut);
    } else {
        return object();
    }
}

void wrapTangentConversions()
{
    def("ConvertToStandardTangent", &_WrapConvertToStandardTangent,
        (arg("widthIn"),
         arg("slopeOrHeightIn"),
         arg("convertHeightToSlope")=false,
         arg("divideValuesByThree")=false,
         arg("negateHeight")=false));

    def("ConvertFromStandardTangent", &_WrapConvertFromStandardTangent,
        (arg("widthIn"),
         arg("slopeIn"),
         arg("convertSlopeToHeight")=false,
         arg("multiplyValuesByThree")=false,
         arg("negateHeight")=false));
}
