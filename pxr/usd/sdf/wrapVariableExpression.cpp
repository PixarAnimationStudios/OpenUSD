//
// Copyright 2023 Pixar
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

#include "pxr/usd/sdf/variableExpression.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/tuple.hpp>

#include <string>
#include <vector>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void
wrapVariableExpression()
{
    using This = SdfVariableExpression;

    scope s = class_<This>("VariableExpression")
        .def(init<>())
        .def(init<const std::string&>(arg("expression")))

        .def("__bool__", &This::operator bool)
        .def("__str__", &This::GetString,
            return_value_policy<return_by_value>())
        .def("__repr__", 
            +[](const This& expr) {
                return TfStringPrintf("%sVariableExpression('%s')",
                    TF_PY_REPR_PREFIX.c_str(), expr.GetString().c_str());
            })

        .def("GetErrors", &This::GetErrors,
            return_value_policy<TfPySequenceToList>())

        .def("Evaluate", &This::Evaluate,
            arg("vars"))

        .def("IsExpression", &This::IsExpression)
        .staticmethod("IsExpression")

        .def("IsValidVariableType", &This::IsValidVariableType)
        .staticmethod("IsValidVariableType")
        ;

    class_<This::Result>("Result", no_init)
        .add_property("value", 
            make_getter(
                &This::Result::value,
                return_value_policy<return_by_value>()))
        .add_property("errors", 
            make_getter(
                &This::Result::errors,
                return_value_policy<TfPySequenceToList>()))
        .add_property("usedVariables", 
            make_getter(
                &This::Result::usedVariables,
                return_value_policy<TfPySequenceToSet>()))
        ;
}
