//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include "pxr/usd/sdf/predicateLibrary.h"

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
                           
void wrapPredicateFunctionResult(nanobind::module_ &m)
{
    namespace nb = nanobind;
    using FunctionResult = SdfPredicateFunctionResult;
    
    nb::class_<FunctionResult> s(m, "PredicateFunctionResult");
    TfNbWrapEnum<FunctionResult::Constancy>(s);
    s
        .def(nb::init<>())
        .def(nb::init<FunctionResult const &>())
        .def(nb::init<bool, FunctionResult::Constancy>(),
             nb::arg("value"),
             nb::arg("constancy") = FunctionResult::MayVaryOverDescendants)

        .def_static("MakeConstant", &FunctionResult::MakeConstant, nb::arg("value"))
        .def_static("MakeVarying", &FunctionResult::MakeVarying, nb::arg("value"))

        .def_static("And", &FunctionResult::And, nb::arg("lhs"), nb::arg("rhs"))
        .def_static("Or", &FunctionResult::Or, nb::arg("lhs"), nb::arg("rhs"))
        
        .def("GetValue", &FunctionResult::GetValue)
        .def("GetConstancy", &FunctionResult::GetConstancy)
        .def("IsConstant", &FunctionResult::IsConstant)

        .def("SetAndPropagateConstancy",
             &FunctionResult::SetAndPropagateConstancy)

        .def("__bool__", _Bool)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def(nb::self == bool{})
        .def(bool{} == nb::self)
        .def(nb::self != bool{})
        .def(bool{} != nb::self)

        .def("__repr__", _Repr)
        ;

}
