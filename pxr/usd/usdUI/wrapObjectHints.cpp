//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdUI/objectHints.h"
#include "pxr/usd/usd/pyConversions.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <vector>

namespace nb = nanobind;
PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdUIObjectHints(nanobind::module_ &m)
{
    TF_NB_WRAP_PUBLIC_TOKENS(
        m, "HintKeys", UsdUIHintKeys, USDUI_HINT_KEYS);

    using This = UsdUIObjectHints;

    nb::class_<This> clsObj(m, "ObjectHints");
    clsObj
        .def(nb::init<>())
        .def(nb::init<const UsdObject &>(), nb::arg("obj"))

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__bool__", [](const This &self) { return bool(self); })

        .def("GetObject", &This::GetObject,
             nb::rv_policy::copy)

        .def("GetDisplayName", &This::GetDisplayName)
        .def("SetDisplayName", &This::SetDisplayName,
             nb::arg("name"))

        .def("GetHidden", &This::GetHidden)
        .def("SetHidden", &This::SetHidden,
             nb::arg("hidden"))
        ;

    TfPyRegisterStlSequencesFromPython<This>();
}
