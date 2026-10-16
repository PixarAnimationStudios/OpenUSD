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
#include "pxr/base/tf/pyNanobindBridge.h"

#include "pxr/usd/sdf/pathPattern.h"

#include <mutex>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

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

void wrapPathPattern(nanobind::module_ &m)
{
    namespace nb = nanobind;

    nb::class_<SdfPathPattern>(m, "PathPattern")
        .def(nb::init<>())
        .def(nb::init<SdfPath>(), nb::arg("prefix"))
        .def_static("Everything", &SdfPathPattern::Everything,
                    nb::rv_policy::copy)
        .def_static("EveryDescendant", &SdfPathPattern::EveryDescendant,
                    nb::rv_policy::copy)
        .def_static("Nothing", &SdfPathPattern::Nothing,
                    nb::rv_policy::copy)
        .def("CanAppendChild", _CanAppendChild, nb::arg("text"))
        .def("AppendChild",
             +[](SdfPathPattern &self, std::string text,
                 SdfPredicateExpression const &predExpr) -> SdfPathPattern & {
                 self.AppendChild(text, predExpr);
                 return self;
             }, nb::arg("text"), nb::arg("predExpr") = SdfPredicateExpression(),
             nb::rv_policy::reference)
        .def("CanAppendProperty", _CanAppendProperty, nb::arg("text"))
        .def("AppendProperty",
             +[](SdfPathPattern &self, std::string text,
                 SdfPredicateExpression const &predExpr) -> SdfPathPattern & {
                 self.AppendProperty(text, predExpr);
                 return self;
             }, nb::arg("text"), nb::arg("predExpr") = SdfPredicateExpression(),
             nb::rv_policy::reference)
        .def("GetPrefix",
             +[](SdfPathPattern const &self) {
                 return self.GetPrefix();
             }, nb::rv_policy::copy)
        .def("SetPrefix",
             +[](SdfPathPattern &self, SdfPath const &prefix) -> SdfPathPattern & {
                 self.SetPrefix(prefix);
                 return self;
             }, nb::arg("prefix"), nb::rv_policy::reference)
        .def("HasLeadingStretch", &SdfPathPattern::HasLeadingStretch)
        .def("HasTrailingStretch", &SdfPathPattern::HasTrailingStretch)
        .def("AppendStretchIfPossible",
             +[](SdfPathPattern &self) -> SdfPathPattern & {
                 self.AppendStretchIfPossible();
                 return self;
             }, nb::rv_policy::reference)
        .def("RemoveTrailingStretch",
             +[](SdfPathPattern &self) -> SdfPathPattern & {
                 self.RemoveTrailingStretch();
                 return self;
             }, nb::rv_policy::reference)
        .def("RemoveTrailingComponent",
             +[](SdfPathPattern &self) -> SdfPathPattern & {
                 self.RemoveTrailingComponent();
                 return self;
             }, nb::rv_policy::reference)
        .def("GetText", &SdfPathPattern::GetText)
        .def("IsProperty", &SdfPathPattern::IsProperty)
        .def("__bool__", &SdfPathPattern::operator bool)
        .def("__repr__", &_Repr)
        .def("__hash__", +[](SdfPathPattern const &p) { return TfHash{}(p); })
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        ;
    VtValueFromPython<SdfPathPattern>();

    Sdf_PathPatternCanAppendResult::
        WrapNb<Sdf_PathPatternCanAppendResult>(m, "_PathPatternCanAppendResult",
                                               "reason");
}
