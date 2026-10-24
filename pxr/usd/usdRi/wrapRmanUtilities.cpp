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
#include "pxr/base/tf/pyNanobindBridge.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapUsdRiRmanUtilities(nb::module_ &m)
{
    m.def("ConvertToRManInterpolateBoundary", (int (*)(const TfToken &))
          UsdRiConvertToRManInterpolateBoundary,
          nb::rv_policy::copy);
    m.def("ConvertFromRManInterpolateBoundary", (const TfToken & (*)(int))
          UsdRiConvertFromRManInterpolateBoundary,
          nb::rv_policy::copy);

    m.def("ConvertToRManFaceVaryingLinearInterpolation",
          (int (*)(const TfToken &))
          UsdRiConvertToRManFaceVaryingLinearInterpolation,
          nb::rv_policy::copy);
    m.def("ConvertFromRManFaceVaryingLinearInterpolation",
          (const TfToken & (*)(int))
          UsdRiConvertFromRManFaceVaryingLinearInterpolation,
          nb::rv_policy::copy);

    m.def("ConvertRManSetSpecificationToListOp",
          &UsdRiConvertRManSetSpecificationToListOp);
    m.def("DoesAttributeUseSetSpecification",
          &UsdRiDoesAttributeUseSetSpecification);
}
