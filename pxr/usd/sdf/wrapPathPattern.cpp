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

#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyFunction.h"

#include "pxr/usd/sdf/pathPattern.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/scope.hpp>

#include <mutex>
#include <string>

using namespace boost::python;

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
