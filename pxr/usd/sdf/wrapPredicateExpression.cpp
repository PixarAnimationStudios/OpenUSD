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

#include "pxr/base/tf/pySignatureExt.h" // wrap lvalue-ref-qualified mem fns.
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyFunction.h"

#include "pxr/usd/sdf/predicateExpression.h"

#include <boost/python/class.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

using namespace boost::python;

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
                           
void wrapPredicateExpression()
{
    TfPyFunctionFromPython<void (PredExpr::Op, int)> {};
    TfPyFunctionFromPython<void (PredExpr::FnCall const &)> {};
    
    scope s = class_<PredExpr>("PredicateExpression")
        .def(init<PredExpr const &>())
        .def(init<std::string, optional<std::string> >(
                 args("exprString", "context")))

        .def("MakeNot",
             +[](PredExpr const &r) {
                 return PredExpr::MakeNot(PredExpr(r));
             }, arg("right"))
        .staticmethod("MakeNot")

        .def("MakeOp",
             +[](PredExpr::Op op, PredExpr const &r, PredExpr const &l) {
                 return PredExpr::MakeOp(op, PredExpr(l), PredExpr(r));
             }, (arg("op"), arg("left"), arg("right")))
        .staticmethod("MakeOp")

        .def("MakeCall",
             +[](PredExpr::FnCall const &call) {
                 return PredExpr::MakeCall(PredExpr::FnCall(call));
             }, (arg("call")))
        .staticmethod("MakeCall")

        .def("Walk",
             +[](PredExpr const &expr,
                 std::function<void (PredExpr::Op, int)> logic,
                 std::function<void (PredExpr::FnCall const &)> call) {
                 return expr.Walk(logic, call);
             }, (arg("logic"), arg("call")))

        .def("GetText", &PredExpr::GetText)

        .def("IsEmpty", &PredExpr::IsEmpty)
        .def("__bool__", &PredExpr::operator bool)
        .def("__repr__", &_Repr)

        .def("GetParseError",
             static_cast<std::string const &(PredExpr::*)() const &>(
                 &PredExpr::GetParseError),
             return_value_policy<return_by_value>())
        ;

    TfPyWrapEnum<PredExpr::Op>();

    {
        scope s = class_<PredExpr::FnCall>("FnCall")
            .def(init<PredExpr::FnCall const &>())
            .def_readwrite("kind", &PredExpr::FnCall::kind)
            .def_readwrite("funcName", &PredExpr::FnCall::funcName)
            .def_readwrite("args", &PredExpr::FnCall::args)
            ;
        TfPyWrapEnum<PredExpr::FnCall::Kind>();
    }

    class_<PredExpr::FnArg>("FnArg")
        .def(init<PredExpr::FnArg const &>())
        .def("Positional", PredExpr::FnArg::Positional, arg("value"))
        .staticmethod("Positional")
        .def("Keyword", PredExpr::FnArg::Keyword, (arg("name"), arg("value")))
        .staticmethod("Keyword")
        .def_readwrite("argName", &PredExpr::FnArg::argName)
        .add_property("value",
                      +[](PredExpr::FnArg const &arg) {
                          return arg.value;
                      },
                      +[](PredExpr::FnArg &arg, VtValue const &val) {
                          arg.value = val;
                      }
            )
        ;

    class_<std::vector<PredExpr::FnArg>>("_PredicateExpressionFnArgVector")
        .def(vector_indexing_suite<std::vector<PredExpr::FnArg>>())
        ;
}
