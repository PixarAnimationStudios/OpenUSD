//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_Museum.h"

#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

using This = TsTest_Museum;

void wrapTsTest_Museum(nb::module_ &m)
{
    nb::class_<This> classObj(m, "TsTest_Museum");

    TfNbWrapEnum<This::DataId>(classObj);

    classObj
        .def_static("GetAllNames", &This::GetAllNames,
            nb::rv_policy::copy)

        .def_static("GetSpline", &This::GetSpline,
            nb::arg("id"), nb::arg("valueType") = Ts_GetType<double>())

        .def_static("GetSplineByName", &This::GetSplineByName,
            nb::arg("name"), nb::arg("valueType") = Ts_GetType<double>())
        ;
}
