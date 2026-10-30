//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/inherits.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

using std::string;

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdInherits(nb::module_ &m)
{
    nb::class_<UsdInherits>(m, "Inherits")
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        })
        .def("AddInherit", &UsdInherits::AddInherit,
             nb::arg("primPath"),
             nb::arg("position")=UsdListPositionBackOfPrependList)
        .def("RemoveInherit", &UsdInherits::RemoveInherit, nb::arg("primPath"))
        .def("ClearInherits", &UsdInherits::ClearInherits)
        .def("SetInherits", &UsdInherits::SetInherits)
        .def("GetAllDirectInherits", &UsdInherits::GetAllDirectInherits)
        .def("GetPrim", (UsdPrim (UsdInherits::*)()) &UsdInherits::GetPrim)
        .def("__bool__", [](UsdInherits &self) { return bool(self); })
        ;
}
