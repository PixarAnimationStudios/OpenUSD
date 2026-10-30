//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/primTypeInfo.h"
#include "pxr/usd/usd/pyConversions.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdPrimTypeInfo(nb::module_ &m)
{
    typedef UsdPrimTypeInfo This;
    nb::class_<This>(m, "PrimTypeInfo")
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        })
        .def("GetTypeName", &This::GetTypeName,
             nb::rv_policy::copy)
        .def("GetAppliedAPISchemas", &This::GetAppliedAPISchemas,
             nb::rv_policy::copy)
        .def("GetSchemaType", &This::GetSchemaType,
             nb::rv_policy::copy)
        .def("GetSchemaTypeName", &This::GetSchemaTypeName,
             nb::rv_policy::copy)
        .def("GetPrimDefinition", &This::GetPrimDefinition,
             nb::rv_policy::reference_internal)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def_static("GetEmptyPrimType", &This::GetEmptyPrimType,
             nb::rv_policy::reference)
        ;
}
