//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdUI/attributeHints.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <vector>

namespace nb = nanobind;
PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdUIAttributeHints(nanobind::module_ &m)
{
    using This = UsdUIAttributeHints;

    nb::class_<This, UsdUIPropertyHints> clsObj(m, "AttributeHints");
    clsObj
        .def(nb::init<>())
        .def(nb::init<const UsdAttribute &>(), nb::arg("attr"))

        .def("GetAttribute",
             &This::GetAttribute,
             nb::rv_policy::copy)

        .def("GetValueLabels",
             &This::GetValueLabels)
        .def("SetValueLabels",
             &This::SetValueLabels,
             nb::arg("labels"))

        .def("GetValueLabelsOrder",
             &This::GetValueLabelsOrder)
        .def("SetValueLabelsOrder",
             &This::SetValueLabelsOrder,
             nb::arg("order"))

        .def("ApplyValueLabel",
             &This::ApplyValueLabel,
             nb::arg("label"))
        ;

    TfPyRegisterStlSequencesFromPython<This>();
}
