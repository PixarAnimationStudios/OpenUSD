//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include <locale>

#include "pxr/usd/usdRi/rmanUtilities.h"

#include "pxr/base/tf/token.h"

#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/return_value_policy.hpp"
#include "pxr/external/boost/python/return_by_value.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdRiRmanUtilities()
{
    def("ConvertToRManInterpolateBoundary", (int (*)(const TfToken &))
        UsdRiConvertToRManInterpolateBoundary,
        return_value_policy<return_by_value>());
    def("ConvertFromRManInterpolateBoundary", (const TfToken & (*)(int))
        UsdRiConvertFromRManInterpolateBoundary,
        return_value_policy<return_by_value>());

    def("ConvertToRManFaceVaryingLinearInterpolation",
        (int (*)(const TfToken &))
        UsdRiConvertToRManFaceVaryingLinearInterpolation,
        return_value_policy<return_by_value>());
    def("ConvertFromRManFaceVaryingLinearInterpolation",
        (const TfToken & (*)(int))
        UsdRiConvertFromRManFaceVaryingLinearInterpolation,
        return_value_policy<return_by_value>());
}
