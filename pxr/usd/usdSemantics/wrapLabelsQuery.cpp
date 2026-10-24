//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdSemantics/labelsQuery.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;
PXR_NAMESPACE_USING_DIRECTIVE

namespace {    

std::string _Repr(const UsdSemanticsLabelsQuery& query) {
     auto timeRepr = std::visit(
          [](auto&& queryTime) -> std::string { return TfPyRepr(queryTime); },
          query.GetTime());
     return TfStringPrintf(
          "%sLabelsQuery('%s', %s)",
          TF_PY_REPR_PREFIX.c_str(),
          query.GetTaxonomy().GetText(),
          timeRepr.c_str()
     );
}

}

void wrapUsdSemanticsLabelsQuery(nanobind::module_ &m)
{
    using This = UsdSemanticsLabelsQuery;
    nb::class_<This>(m, "LabelsQuery")
        .def(nb::init<const TfToken&, UsdTimeCode>(),
             nb::arg("taxonomy"), nb::arg("timeCode"))
        .def(nb::init<const TfToken&, const GfInterval&>(),
             nb::arg("taxonomy"), nb::arg("timeInterval"))
        .def("__repr__", &::_Repr)
        .def("ComputeUniqueDirectLabels", &This::ComputeUniqueDirectLabels)
        .def("ComputeUniqueInheritedLabels",
             &This::ComputeUniqueInheritedLabels)
        .def("HasDirectLabel", &This::HasDirectLabel)
        .def("HasInheritedLabel", &This::HasInheritedLabel)
        .def("GetTaxonomy", [](const This& self) { return self.GetTaxonomy(); });

}
