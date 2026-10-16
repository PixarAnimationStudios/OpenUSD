//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdUI/propertyHints.h"
#include "pxr/usd/usd/pyConversions.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <vector>

namespace nb = nanobind;
PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdUIPropertyHints(nanobind::module_ &m)
{
    using This = UsdUIPropertyHints;

    nb::class_<This, UsdUIObjectHints> clsObj(m, "PropertyHints");
    clsObj
        .def(nb::init<>())
        .def(nb::init<const UsdProperty &>(), nb::arg("prop"))

        .def("GetProperty", &This::GetProperty,
             nb::rv_policy::copy)

        .def("GetDisplayGroup", &This::GetDisplayGroup)
        .def("SetDisplayGroup", &This::SetDisplayGroup,
             nb::arg("group"))

        .def("GetShownIf", &This::GetShownIf)
        .def("SetShownIf", &This::SetShownIf,
             nb::arg("shownIf"))
        ;

    TfPyRegisterStlSequencesFromPython<This>();
}
