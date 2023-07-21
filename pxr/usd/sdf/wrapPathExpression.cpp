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

#include "pxr/usd/sdf/pathExpression.h"

#include <boost/python/class.hpp>
#include <boost/python/scope.hpp>

#include <string>

using namespace boost::python;
using std::pair;
using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

using PathExpr = SdfPathExpression;
using ExpressionReference = PathExpr::ExpressionReference;
using PathPattern = PathExpr::PathPattern;

void wrapPathExpression()
{
    // For ResolveReferences.
    TfPyFunctionFromPython<PathExpr (ExpressionReference const &)> {};
        
    // For Walk.
    TfPyFunctionFromPython<void (PathExpr::Op, int)> {};
    TfPyFunctionFromPython<void (ExpressionReference const &, int)> {};
    TfPyFunctionFromPython<void (PathPattern const &, int)> {};

    scope s = class_<PathExpr>("PathExpression")
        .def(init<PathExpr const &>())
        .def(init<std::string, optional<std::string>>(
                 args("patternString", "parseContext")))
        
        .def("GetDebugString", &PathExpr::GetDebugString)

        .def("ReplacePrefix",
             +[](PathExpr const &self,
                 SdfPath const &oldPrefix, SdfPath const &newPrefix) {
                 return self.ReplacePrefix(oldPrefix, newPrefix);
             }, (arg("oldPrefix"), arg("newPrefix")))

        .def("MakeAbsolute",
             +[](PathExpr const &self, SdfPath const &anchor) {
                 return self.MakeAbsolute(anchor);
             }, (arg("anchor")))

        .def("ContainsExpressionReferences",
             &PathExpr::ContainsExpressionReferences)

        .def("ResolveReferences",
             +[](PathExpr const &self,
                 std::function<
                 PathExpr (ExpressionReference const &)> resolve) {
                 return self.ResolveReferences(resolve);
             }, (arg("resolve")))

        .def("ComposeOver",
             +[](PathExpr const &self, SdfPathExpression const &weaker) {
                 return self.ComposeOver(weaker);
             }, (arg("weaker")))

        .def("Walk",
             +[](PathExpr const &self,
                 std::function<void (PathExpr::Op, int)> logic,
                 std::function<void (ExpressionReference const &)> ref,
                 std::function<void (PathPattern const &)> pattern) {
                 return self.Walk(logic, ref, pattern);
             }, (arg("logic"), arg("ref"), arg("pattern")))

        .def("__bool__", &SdfPathExpression::operator bool)
        ;

    TfPyWrapEnum<PathExpr::Op>();

    class_<PathPattern>("PathPattern")
        .def("AppendChild",
             +[](PathPattern &self, std::string text,
                 SdfPredicateExpression const &predExpr) {
                 self.AppendChild(text, predExpr);
             }, (arg("text"), arg("predExpr")=SdfPredicateExpression()))
        .def("AppendProperty",
             +[](PathPattern &self, std::string text,
                 SdfPredicateExpression const &predExpr) {
                 self.AppendProperty(text, predExpr);
             }, (arg("text"), arg("predExpr")=SdfPredicateExpression()))
        .def("GetPrefix",
             +[](PathPattern const &self) {
                 return self.GetPrefix();
             }, return_value_policy<return_by_value>())
        .def("SetPrefix",
             +[](PathPattern &self, SdfPath const &prefix) {
                 self.SetPrefix(prefix);
             }, (arg("prefix")))
        .def("GetDebugString", &PathPattern::GetDebugString)
        .def("__bool__", &PathPattern::operator bool)
        ;

    class_<ExpressionReference>("ExpressionReference")
        .def_readwrite("path", &ExpressionReference::path)
        .def_readwrite("name", &ExpressionReference::name)
        ;        

}
