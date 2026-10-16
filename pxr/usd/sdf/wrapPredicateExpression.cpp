//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pySignatureExt.h" // wrap lvalue-ref-qualified mem fns.
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

#include "pxr/usd/sdf/predicateExpression.h"

PXR_NAMESPACE_USING_DIRECTIVE

using PredExpr = SdfPredicateExpression;

static std::string
_Repr(SdfPredicateExpression const &self) {
    if (!self) {
        return TF_PY_REPR_PREFIX + "PredicateExpression()";
    }
    else {
        return std::string(TF_PY_REPR_PREFIX + "PredicateExpression(")
            + TfPyRepr(self.GetText()) + ")";
    }
}
                           
void wrapPredicateExpression(nanobind::module_ &m)
{
    namespace nb = nanobind;

    TfPyFunctionFromPython<void (PredExpr::Op, int)> {};
    TfPyFunctionFromPython<void (PredExpr::FnCall const &)> {};
    
    nb::class_<PredExpr> s(m, "PredicateExpression");
    s
        .def(nb::init<>())
        .def(nb::init<PredExpr const &>())
        .def(nb::init<std::string, std::string>(),
             nb::arg("exprString"), nb::arg("context") = std::string())

        .def_static("MakeNot",
             +[](PredExpr const &r) {
                 return PredExpr::MakeNot(PredExpr(r));
             }, nb::arg("right"))

        .def_static("MakeOp",
             +[](PredExpr::Op op, PredExpr const &r, PredExpr const &l) {
                 return PredExpr::MakeOp(op, PredExpr(l), PredExpr(r));
             }, nb::arg("op"), nb::arg("left"), nb::arg("right"))

        .def_static("MakeCall",
             +[](PredExpr::FnCall const &call) {
                 return PredExpr::MakeCall(PredExpr::FnCall(call));
             }, nb::arg("call"))

        .def("Walk",
             +[](PredExpr const &expr,
                 std::function<void (PredExpr::Op, int)> logic,
                 std::function<void (PredExpr::FnCall const &)> call) {
                 return expr.Walk(logic, call);
             }, nb::arg("logic"), nb::arg("call"))

        .def("GetText", &PredExpr::GetText)

        .def("IsEmpty", &PredExpr::IsEmpty)
        .def("__bool__", &PredExpr::operator bool)
        .def("__repr__", &_Repr)
        .def("__str__", &PredExpr::GetText)
        .def("__hash__", +[](PredExpr const &e) { return TfHash{}(e); })
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def("GetParseError",
             +[](PredExpr const &self) -> std::string const & {
                 return self.GetParseError();
             },
             nb::rv_policy::copy)
        ;

    TfNbWrapEnum<PredExpr::Op>(s);

    {
        nb::class_<PredExpr::FnCall> fnCallCls(s, "FnCall");
        fnCallCls
            .def(nb::init<>())
            .def(nb::init<PredExpr::FnCall const &>())
            .def_rw("kind", &PredExpr::FnCall::kind)
            .def_rw("funcName", &PredExpr::FnCall::funcName)
            .def_rw("args", &PredExpr::FnCall::args)
            .def("__hash__",
                 +[](PredExpr::FnCall const &c) { return TfHash{}(c); })
            .def(nb::self == nb::self)
            .def(nb::self != nb::self)
            ;
        TfNbWrapEnum<PredExpr::FnCall::Kind>(fnCallCls);
    }

    nb::class_<PredExpr::FnArg>(s, "FnArg")
        .def(nb::init<>())
        .def(nb::init<PredExpr::FnArg const &>())
        .def_static("Positional", &PredExpr::FnArg::Positional, nb::arg("value"))
        .def_static("Keyword", &PredExpr::FnArg::Keyword, nb::arg("name"), nb::arg("value"))
        .def_rw("argName", &PredExpr::FnArg::argName)
        .def_prop_rw("value",
                      +[](PredExpr::FnArg const &arg) {
                          return arg.value;
                      },
                      +[](PredExpr::FnArg &arg, VtValue const &val) {
                          arg.value = val;
                      }
            )
        .def("__hash__", +[](PredExpr::FnArg const &a) { return TfHash{}(a); })
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        ;
}
