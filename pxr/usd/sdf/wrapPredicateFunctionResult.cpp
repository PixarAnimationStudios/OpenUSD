//
// Copyright 2024 Pixar
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

#include "pxr/base/tf/pyEnum.h"

#include "pxr/usd/sdf/predicateLibrary.h"

#include <boost/python/class.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static std::string
_Repr(SdfPredicateFunctionResult const &self) {
    return TF_PY_REPR_PREFIX +
        "PredicateFunctionResult(" +
        TfPyRepr(self.GetValue()) + ", " +
        TfPyRepr(self.GetConstancy()) + ")";
}

static bool
_Bool(SdfPredicateFunctionResult const &self) {
    return self.GetValue();
}
                           
void wrapPredicateFunctionResult()
{
    using FunctionResult = SdfPredicateFunctionResult;
    
    scope s = class_<FunctionResult>("PredicateFunctionResult")
        .def(init<FunctionResult const &>())
        .def(init<bool, optional<FunctionResult::Constancy>>(
                 (arg("value"), arg("constancy"))))

        .def("MakeConstant", &FunctionResult::MakeConstant, arg("value"))
        .staticmethod("MakeConstant")
        .def("MakeVarying", &FunctionResult::MakeVarying, arg("value"))
        .staticmethod("MakeVarying")

        .def("GetValue", &FunctionResult::GetValue)
        .def("GetConstancy", &FunctionResult::GetConstancy)
        .def("IsConstant", &FunctionResult::IsConstant)

        .def("SetAndPropagateConstancy",
             &FunctionResult::SetAndPropagateConstancy)

        .def(!self)
        .def("__bool__", _Bool)
        .def(self == self)
        .def(self != self)
        .def(self == bool{})
        .def(bool{} == self)
        .def(self != bool{})
        .def(bool{} != self)

        .def("__repr__", _Repr)
        ;

    TfPyWrapEnum<FunctionResult::Constancy>();
}
