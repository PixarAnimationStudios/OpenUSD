//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pySignatureExt.h" // wrap lvalue-ref-qualified mem fns.

#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/usd/sdf/pathExpression.h"
#include "pxr/usd/sdf/pathExpressionEval.h"
#include "pxr/usd/sdf/predicateLibrary.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/scope.hpp"

#include <mutex>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

using PathExpr = SdfPathExpression;
using ExpressionReference = PathExpr::ExpressionReference;
using PathPattern = PathExpr::PathPattern;

static std::string
_Repr(SdfPathExpression const &self) {
    if (!self) {
        return TF_PY_REPR_PREFIX + "PathExpression()";
    }
    else {
        return std::string(TF_PY_REPR_PREFIX + "PathExpression(")
            + TfPyRepr(self.GetText()) + ")";
    }
}

namespace {

struct _PathIdentity {
    SdfPath operator()(SdfPath const &p) const { return p; }
};

static SdfPredicateLibrary<SdfPath const &> const &
_GetBasicPredicateLib() {
    // Super simple predicate library for paths for testing.
    static auto theLib = SdfPredicateLibrary<SdfPath const &>()
        .Define("isPrimPath", [](SdfPath const &p) {
            return p.IsPrimPath();
        })
        .Define("isPropertyPath", [](SdfPath const &p) {
            return p.IsPropertyPath();
        })
        ;
    return theLib;
}

struct _BasicMatchEval
{
    explicit _BasicMatchEval(SdfPathExpression const &expr)
        : _eval(SdfMakePathExpressionEval(expr, _GetBasicPredicateLib())) {}
    explicit _BasicMatchEval(std::string const &expr)
        : _BasicMatchEval(SdfPathExpression(expr)) {}
    SdfPredicateFunctionResult
    Match(SdfPath const &p) {
        return _eval.Match(p, _PathIdentity {});
    }
    SdfPathExpressionEval<SdfPath const &> _eval;
};

std::once_flag wrapMatchEvalFlag;
static _BasicMatchEval
_MakeBasicMatchEval(std::string const &expr)
{
    std::call_once(wrapMatchEvalFlag, []() {
        class_<_BasicMatchEval>("_BasicMatchEval", init<std::string>())
            .def(init<SdfPathExpression>())
            .def("Match", &_BasicMatchEval::Match)
            ;
    });
    return _BasicMatchEval(expr);
}

} // anon

void wrapPathExpression()
{
    // For testing.
    def("_MakeBasicMatchEval", _MakeBasicMatchEval);
    
    // For ResolveReferences.
    TfPyFunctionFromPython<PathExpr (ExpressionReference const &)> {};
        
    // For Walk.
    TfPyFunctionFromPython<void (PathExpr::Op, int)> {};
    TfPyFunctionFromPython<void (ExpressionReference const &)> {};
    TfPyFunctionFromPython<void (PathPattern const &)> {};

    scope s = class_<PathExpr>("PathExpression")
        .def(init<PathExpr const &>())
        .def(init<std::string, optional<std::string>>(
                 args("patternString", "parseContext")))

        .def("Everything", &PathExpr::Everything,
             return_value_policy<return_by_value>())
        .staticmethod("Everything")

        .def("Nothing", &PathExpr::Nothing,
             return_value_policy<return_by_value>())
        .staticmethod("Nothing")

        .def("WeakerRef", &PathExpr::WeakerRef,
             return_value_policy<return_by_value>())
        .staticmethod("WeakerRef")

        .def("MakeComplement",
             +[](PathExpr const &r) {
                 return PathExpr::MakeComplement(r);
             }, arg("right"))
        .staticmethod("MakeComplement")

        .def("MakeOp",
             +[](PathExpr::Op op, PathExpr const &l, PathExpr const &r) {
                 return PathExpr::MakeOp(op, l, r);
             }, (arg("op"), arg("left"), arg("right")))
        .staticmethod("MakeOp")

        .def("MakeAtom",
             +[](SdfPath const &path) {
                 return PathExpr::MakeAtom(path);
             }, arg("path"))
        .def("MakeAtom",
             +[](PathExpr::ExpressionReference const &ref) {
                 return PathExpr::MakeAtom(ref);
             }, (arg("ref")))
        .def("MakeAtom",
             +[](PathExpr::PathPattern const &pat) {
                 return PathExpr::MakeAtom(pat);
             }, (arg("pattern")))
        .staticmethod("MakeAtom")

        .def("ReplacePrefix",
             +[](PathExpr const &self,
                 SdfPath const &oldPrefix, SdfPath const &newPrefix) {
                 return self.ReplacePrefix(oldPrefix, newPrefix);
             }, (arg("oldPrefix"), arg("newPrefix")))

        .def("IsAbsolute", &PathExpr::IsAbsolute)

        .def("MakeAbsolute",
             +[](PathExpr const &self, SdfPath const &anchor) {
                 return self.MakeAbsolute(anchor);
             }, (arg("anchor")))

        .def("ContainsExpressionReferences",
             &PathExpr::ContainsExpressionReferences)

        .def("ContainsWeakerExpressionReference",
             &PathExpr::ContainsWeakerExpressionReference)

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

        .def("IsComplete", &PathExpr::IsComplete)

        .def("Walk",
             +[](PathExpr const &self,
                 std::function<void (PathExpr::Op, int)> logic,
                 std::function<void (ExpressionReference const &)> ref,
                 std::function<void (PathPattern const &)> pattern) {
                 return self.Walk(logic, ref, pattern);
             }, (arg("logic"), arg("ref"), arg("pattern")))

        .def("GetText", &PathExpr::GetText)

        .def("IsEmpty", &PathExpr::IsEmpty)

        .def("__bool__", &PathExpr::operator bool)
        .def("__repr__", _Repr)
        .def("__hash__", +[](PathExpr const &e) { return TfHash{}(e); })
        .def(self == self)
        .def(self != self)
        ;
    VtValueFromPython<SdfPathExpression>();

    TfPyWrapEnum<PathExpr::Op>();

    s.attr("PathPattern") = TfPyGetClassObject<SdfPathPattern>();
    
    class_<ExpressionReference>("ExpressionReference")
        .def_readwrite("path", &ExpressionReference::path)
        .def_readwrite("name", &ExpressionReference::name)
        .def("__hash__",
             +[](ExpressionReference const &r) { return TfHash{}(r); })
        .def(self == self)
        .def(self != self)
        .def("Weaker", &ExpressionReference::Weaker,
             return_value_policy<return_by_value>())
        .staticmethod("Weaker")
        ;        
    VtValueFromPython<ExpressionReference>();

}
