//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pySignatureExt.h" // wrap lvalue-ref-qualified mem fns.

#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyFunction.h"

#include "pxr/usd/sdf/pathPattern.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/return_arg.hpp"
#include "pxr/external/boost/python/scope.hpp"

#include <mutex>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

static std::string
_Repr(SdfPathPattern const &self) {
    if (!self) {
        return TF_PY_REPR_PREFIX + "PathPattern()";
    }
    else {
        return std::string(TF_PY_REPR_PREFIX + "PathPattern(")
            + TfPyRepr(self.GetText()) + ")";
    }
}

struct Sdf_PathPatternCanAppendResult
    : public TfPyAnnotatedBoolResult<std::string>
{
    Sdf_PathPatternCanAppendResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static
Sdf_PathPatternCanAppendResult
_CanAppendChild(SdfPathPattern const &self, std::string const &text)
{
    std::string errMsg;
    bool valid = self.CanAppendChild(text, &errMsg);
    return Sdf_PathPatternCanAppendResult(valid, errMsg);
}

static
Sdf_PathPatternCanAppendResult
_CanAppendProperty(SdfPathPattern const &self, std::string const &text)
{
    std::string errMsg;
    bool valid = self.CanAppendProperty(text, &errMsg);
    return Sdf_PathPatternCanAppendResult(valid, errMsg);
}

void wrapPathPattern()
{
    class_<SdfPathPattern>("PathPattern")
        .def(init<SdfPath>(arg("prefix")))
        .def("Everything", SdfPathPattern::Everything,
             return_value_policy<return_by_value>())
        .staticmethod("Everything")
        .def("EveryDescendant", SdfPathPattern::EveryDescendant,
            return_value_policy<return_by_value>())
        .staticmethod("EveryDescendant")
        .def("Nothing", SdfPathPattern::Nothing,
            return_value_policy<return_by_value>())
        .staticmethod("Nothing")
        .def("CanAppendChild", _CanAppendChild, arg("text"))
        .def("AppendChild",
             +[](SdfPathPattern &self, std::string text,
                 SdfPredicateExpression const &predExpr) {
                 self.AppendChild(text, predExpr);
             }, (arg("text"), arg("predExpr")=SdfPredicateExpression()),
             return_self<>())
        .def("CanAppendProperty", _CanAppendProperty, arg("text"))
        .def("AppendProperty",
             +[](SdfPathPattern &self, std::string text,
                 SdfPredicateExpression const &predExpr) {
                 self.AppendProperty(text, predExpr);
             }, (arg("text"), arg("predExpr")=SdfPredicateExpression()),
             return_self<>())
        .def("GetPrefix",
             +[](SdfPathPattern const &self) {
                 return self.GetPrefix();
             }, return_value_policy<return_by_value>())
        .def("SetPrefix",
             +[](SdfPathPattern &self, SdfPath const &prefix) {
                 self.SetPrefix(prefix);
             }, (arg("prefix")), return_self<>())
        .def("HasLeadingStretch", &SdfPathPattern::HasLeadingStretch)
        .def("HasTrailingStretch", &SdfPathPattern::HasTrailingStretch)
        .def("AppendStretchIfPossible",
             &SdfPathPattern::AppendStretchIfPossible,
             return_self<>())
        .def("RemoveTrailingStretch",
             &SdfPathPattern::RemoveTrailingStretch,
             return_self<>())
        .def("RemoveTrailingComponent",
             &SdfPathPattern::RemoveTrailingComponent,
             return_self<>())
        .def("GetText", &SdfPathPattern::GetText)
        .def("IsProperty", &SdfPathPattern::IsProperty)
        .def("__bool__", &SdfPathPattern::operator bool)
        .def("__repr__", &_Repr)
        .def("__hash__", +[](SdfPathPattern const &p) { return TfHash{}(p); })
        .def(self == self)
        .def(self != self)
        ;
    VtValueFromPython<SdfPathPattern>();

    Sdf_PathPatternCanAppendResult::
        Wrap<Sdf_PathPatternCanAppendResult>("_PathPatternCanAppendResult",
                                             "reason");
}
