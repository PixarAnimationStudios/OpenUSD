//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/conditionalAbortDiagnosticDelegate.h"

#include "pxr/base/tf/pyResultConversions.h"

#include <iostream>
#include <string>

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/list.hpp"
#include "pxr/external/boost/python/tuple.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapConditionalAbortDiagnosticDelegate()
{
    using ErrorFilters = UsdUtilsConditionalAbortDiagnosticDelegateErrorFilters;
    class_<ErrorFilters>("ConditionalAbortDiagnosticDelegateErrorFilters",
            init<std::vector<std::string>, std::vector<std::string>>())
        .def(init<>())
        .def("GetCodePathFilters", &ErrorFilters::GetCodePathFilters, 
                return_value_policy<TfPySequenceToList>())
        .def("GetStringFilters", &ErrorFilters::GetStringFilters,
                return_value_policy<TfPySequenceToList>())
        .def("SetStringFilters", &ErrorFilters::SetStringFilters,
                args("stringFilters"))
        .def("SetCodePathFilters", &ErrorFilters::SetCodePathFilters,
                args("codePathFilters"));

    using This = UsdUtilsConditionalAbortDiagnosticDelegate;
    class_<This, noncopyable>("ConditionalAbortDiagnosticDelegate",
            init<ErrorFilters, ErrorFilters>());
}
