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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/vt/wrapArray.h"

#include "pxr/usd/sdf/pathExpression.h"
#include "pxr/usd/sdf/pathExpressionEval.h"
#include "pxr/usd/sdf/predicateLibrary.h"

#include <mutex>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using PathExpr = SdfPathExpression;
using ExpressionReference = PathExpr::ExpressionReference;
using PathPattern = PathExpr::PathPattern;

TF_REGISTRY_FUNCTION(VtValue)
{
    VtRegisterValueCastsFromPythonSequencesToArray<SdfPathExpression>();
}

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
        .Define("capital", [](SdfPath const &p) {
            std::string const &name = p.GetName();
            auto isCap = [](char l) { return 'A' <= l && l <= 'Z'; };
            return !name.empty() && isCap(name[0]);
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

static _BasicMatchEval
_MakeBasicMatchEval(std::string const &expr)
{
    return _BasicMatchEval(expr);
}

} // anon

void wrapPathExpression(nanobind::module_ &m)
{
    namespace nb = nanobind;

    // For testing.
    nb::class_<_BasicMatchEval>(m, "_BasicMatchEval")
        .def(nb::init<std::string>())
        .def(nb::init<SdfPathExpression>())
        .def("Match", &_BasicMatchEval::Match)
        ;
    m.def("_MakeBasicMatchEval", _MakeBasicMatchEval);
    
    // For ResolveReferences.
    TfPyFunctionFromPython<PathExpr (ExpressionReference const &)> {};
        
    // For Walk.
    TfPyFunctionFromPython<void (PathExpr::Op, int)> {};
    TfPyFunctionFromPython<void (ExpressionReference const &)> {};
    TfPyFunctionFromPython<void (PathPattern const &)> {};

    nb::class_<PathExpr> s(m, "PathExpression");
    s
        .def(nb::init<>())
        .def(nb::init<PathExpr const &>())
        .def(nb::init<std::string, std::string>(),
             nb::arg("patternString"), nb::arg("parseContext") = std::string())

        .def_static("Everything", &PathExpr::Everything,
                    nb::rv_policy::copy)

        .def_static("Nothing", &PathExpr::Nothing,
                    nb::rv_policy::copy)

        .def_static("WeakerRef", &PathExpr::WeakerRef,
                    nb::rv_policy::copy)

        .def_static("MakeComplement",
             +[](PathExpr const &r) {
                 return PathExpr::MakeComplement(r);
             }, nb::arg("right"))

        .def_static("MakeOp",
             +[](PathExpr::Op op, PathExpr const &l, PathExpr const &r) {
                 return PathExpr::MakeOp(op, l, r);
             }, nb::arg("op"), nb::arg("left"), nb::arg("right"))

        .def_static("MakeAtom",
             +[](SdfPath const &path) {
                 return PathExpr::MakeAtom(path);
             }, nb::arg("path"))
        .def_static("MakeAtom",
             +[](PathExpr::ExpressionReference const &ref) {
                 return PathExpr::MakeAtom(ref);
             }, nb::arg("ref"))
        .def_static("MakeAtom",
             +[](PathExpr::PathPattern const &pat) {
                 return PathExpr::MakeAtom(pat);
             }, nb::arg("pattern"))

        .def("ReplacePrefix",
             +[](PathExpr const &self,
                 SdfPath const &oldPrefix, SdfPath const &newPrefix) {
                 return self.ReplacePrefix(oldPrefix, newPrefix);
             }, nb::arg("oldPrefix"), nb::arg("newPrefix"))

        .def("IsAbsolute", &PathExpr::IsAbsolute)

        .def("MakeAbsolute",
             +[](PathExpr const &self, SdfPath const &anchor) {
                 return self.MakeAbsolute(anchor);
             }, nb::arg("anchor"))

        .def("ContainsExpressionReferences",
             &PathExpr::ContainsExpressionReferences)

        .def("ContainsWeakerExpressionReference",
             &PathExpr::ContainsWeakerExpressionReference)

        .def("ResolveReferences",
             +[](PathExpr const &self,
                 std::function<
                 PathExpr (ExpressionReference const &)> resolve) {
                 return self.ResolveReferences(resolve);
             }, nb::arg("resolve"))

        .def("ComposeOver",
             +[](PathExpr const &self, SdfPathExpression const &weaker) {
                 return self.ComposeOver(weaker);
             }, nb::arg("weaker"))

        .def("IsComplete", &PathExpr::IsComplete)

        .def("Walk",
             +[](PathExpr const &self,
                 std::function<void (PathExpr::Op, int)> logic,
                 std::function<void (ExpressionReference const &)> ref,
                 std::function<void (PathPattern const &)> pattern) {
                 return self.Walk(logic, ref, pattern);
             }, nb::arg("logic"), nb::arg("ref"), nb::arg("pattern"))

        .def("GetText", &PathExpr::GetText)

        .def("IsEmpty", &PathExpr::IsEmpty)

        .def("__bool__", &PathExpr::operator bool)
        .def("__repr__", _Repr)
        .def("__hash__", +[](PathExpr const &e) { return TfHash{}(e); })
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        ;

    VtValueFromPython<SdfPathExpression>();

    TfNbWrapEnum<PathExpr::Op>(s);

    s.attr("PathPattern") = nb::type<SdfPathPattern>();
    
    nb::class_<ExpressionReference>(s, "ExpressionReference")
        .def(nb::init<>())
        .def_rw("path", &ExpressionReference::path)
        .def_rw("name", &ExpressionReference::name)
        .def("__hash__",
             +[](ExpressionReference const &r) { return TfHash{}(r); })
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def_static("Weaker", &ExpressionReference::Weaker,
                    nb::rv_policy::copy)
        ;        
    VtValueFromPython<ExpressionReference>();

}
