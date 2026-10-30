//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdUI/primHints.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <vector>

namespace nb = nanobind;
PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdUIPrimHints(nanobind::module_ &m)
{
    using This = UsdUIPrimHints;

    nb::class_<This, UsdUIObjectHints> clsObj(m, "PrimHints");
    clsObj
        .def(nb::init<>())
        .def(nb::init<const UsdPrim &>(), nb::arg("prim"))

        .def("GetPrim", &This::GetPrim,
             nb::rv_policy::copy)

        .def("GetDisplayGroupsExpanded",
             &This::GetDisplayGroupsExpanded)
        .def("SetDisplayGroupsExpanded",
             &This::SetDisplayGroupsExpanded,
             nb::arg("expanded"))

        .def("GetDisplayGroupExpanded",
             &This::GetDisplayGroupExpanded)
        .def("SetDisplayGroupExpanded",
             &This::SetDisplayGroupExpanded,
             nb::arg("group"),
             nb::arg("expanded"))

        .def("GetDisplayGroupsShownIf",
             &This::GetDisplayGroupsShownIf)
        .def("SetDisplayGroupsShownIf",
             &This::SetDisplayGroupsShownIf,
             nb::arg("expanded"))

        .def("GetDisplayGroupShownIf",
             &This::GetDisplayGroupShownIf)
        .def("SetDisplayGroupShownIf",
             &This::SetDisplayGroupShownIf,
             nb::arg("group"),
             nb::arg("shownIf"))
        ;

    TfPyRegisterStlSequencesFromPython<This>();
}
