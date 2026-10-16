//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/conditionalAbortDiagnosticDelegate.h"

#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <iostream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapConditionalAbortDiagnosticDelegate(nanobind::module_ &m)
{
    using ErrorFilters = UsdUtilsConditionalAbortDiagnosticDelegateErrorFilters;
    nb::class_<ErrorFilters>(m, "ConditionalAbortDiagnosticDelegateErrorFilters")
        .def(nb::init<std::vector<std::string>, std::vector<std::string>>())
        .def(nb::init<>())
        .def("GetCodePathFilters", [](const ErrorFilters& self) {
            return TfNbSequenceToList(self.GetCodePathFilters());
        })
        .def("GetStringFilters", [](const ErrorFilters& self) {
            return TfNbSequenceToList(self.GetStringFilters());
        })
        .def("SetStringFilters", &ErrorFilters::SetStringFilters,
                nb::arg("stringFilters"))
        .def("SetCodePathFilters", &ErrorFilters::SetCodePathFilters,
                nb::arg("codePathFilters"));

    using This = UsdUtilsConditionalAbortDiagnosticDelegate;
    nb::class_<This>(m, "ConditionalAbortDiagnosticDelegate")
        .def(nb::init<ErrorFilters, ErrorFilters>());
}
