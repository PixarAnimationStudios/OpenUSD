//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/specializes.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

using std::string;

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdSpecializes(nb::module_ &m)
{
    nb::class_<UsdSpecializes>(m, "Specializes")
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        })
        .def("AddSpecialize", &UsdSpecializes::AddSpecialize,
             nb::arg("primPath"),
             nb::arg("position")=UsdListPositionBackOfPrependList)
        .def("RemoveSpecialize", &UsdSpecializes::RemoveSpecialize,
             nb::arg("primPath"))
        .def("ClearSpecializes", &UsdSpecializes::ClearSpecializes)
        .def("SetSpecializes", &UsdSpecializes::SetSpecializes)
        .def("GetPrim", 
             (UsdPrim (UsdSpecializes::*)()) &UsdSpecializes::GetPrim)
        .def("__bool__", [](UsdSpecializes &self) { return bool(self); })
        ;
}
