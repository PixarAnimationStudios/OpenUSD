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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/diagnostic.h"

#include <nanobind/nanobind.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;


static nb::object _WrapConvertToStandardTangent(
    const double widthIn,
    const double slopeOrHeightIn,
    bool convertHeightToSlope,
    bool divideValuesByThree,
    bool negateHeight)
{
    double widthOut, slopeOut;

    if (TsConvertToStandardTangent(
            widthIn, slopeOrHeightIn, convertHeightToSlope, divideValuesByThree,
            negateHeight, &widthOut, &slopeOut)) {
        return nb::make_tuple(widthOut, slopeOut);
    }
    return nb::none();
}

static nb::object _WrapConvertFromStandardTangent(
    const double widthIn,
    const double slopeIn,
    bool convertSlopeToHeight,
    bool multiplyValuesByThree,
    bool negateHeight)
{
    double widthOut, slopeOrHeightOut;

    if (TsConvertFromStandardTangent(
            widthIn, slopeIn, convertSlopeToHeight, multiplyValuesByThree,
            negateHeight, &widthOut, &slopeOrHeightOut)) {
        return nb::make_tuple(widthOut, slopeOrHeightOut);
    }
    return nb::none();
}

void wrapTangentConversions(nanobind::module_& m)
{
    m.def("ConvertToStandardTangent", &_WrapConvertToStandardTangent,
          nb::arg("widthIn"),
          nb::arg("slopeOrHeightIn"),
          nb::arg("convertHeightToSlope") = false,
          nb::arg("divideValuesByThree") = false,
          nb::arg("negateHeight") = false);

    m.def("ConvertFromStandardTangent", &_WrapConvertFromStandardTangent,
          nb::arg("widthIn"),
          nb::arg("slopeIn"),
          nb::arg("convertSlopeToHeight") = false,
          nb::arg("multiplyValuesByThree") = false,
          nb::arg("negateHeight") = false);
}
