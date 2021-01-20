//
// Copyright 2017 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/conditionalAbortDiagnosticDelegate.h"

#include "pxr/base/tf/pyResultConversions.h"

#include <iostream>
#include <string>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/list.hpp>
#include <boost/python/tuple.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;

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
    class_<This, boost::noncopyable>("ConditionalAbortDiagnosticDelegate",
            init<ErrorFilters, ErrorFilters>());
}
