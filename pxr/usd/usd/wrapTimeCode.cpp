//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <string>

using std::string;

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static size_t __hash__(const UsdTimeCode &self) { return hash_value(self); }

static std::string _Str(const UsdTimeCode &self)
{
    return TfStringify(self);
}

static string __repr__(const UsdTimeCode &self)
{
    string tail = ".Default()";
    if (self.IsNumeric()) {
        if (self.IsPreTime()) {
            tail = self.IsEarliestTime() ?
                TfStringPrintf(".PreTime(%sTimeCode.EarliestTime().GetValue())",
                               TF_PY_REPR_PREFIX.c_str()) :
                TfStringPrintf(".PreTime(%s)", TfPyRepr(self.GetValue()).c_str());
        } else {
            if (self.IsEarliestTime()) {
                tail = ".EarliestTime()";
            } else {
                tail = self.GetValue() == 0.0 ? "()" :
                    TfStringPrintf("(%s)", TfPyRepr(self.GetValue()).c_str());
            }
        }
    }
    return TF_PY_REPR_PREFIX + "TimeCode" + tail;
}

} // anonymous namespace 

void wrapUsdTimeCode(nb::module_ &m)
{
    nb::class_<UsdTimeCode> s(m, "TimeCode");
    s
        .def(nb::init<>())
        .def(nb::init<double>())
        .def(nb::init<GfTimeCode>())
        .def(nb::init<UsdTimeCode>())

        .def_static("PreTime", static_cast<UsdTimeCode(*)(double)>(
            &UsdTimeCode::PreTime), nb::arg("value"))
        .def_static("PreTime", static_cast<UsdTimeCode(*)(const GfTimeCode&)>(
            &UsdTimeCode::PreTime), nb::arg("timeCode"))

        .def_static("EarliestTime", &UsdTimeCode::EarliestTime)
        
        .def_static("Default", &UsdTimeCode::Default)
        
        .def_static("SafeStep", &UsdTimeCode::SafeStep,
             nb::arg("maxValue")=1e6, nb::arg("maxCompression")=10.0)

        .def("IsPreTime", &UsdTimeCode::IsPreTime)
        .def("IsEarliestTime", &UsdTimeCode::IsEarliestTime)
        .def("IsDefault", &UsdTimeCode::IsDefault)
        .def("IsNumeric", &UsdTimeCode::IsNumeric)
        .def("GetValue", &UsdTimeCode::GetValue)

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def(nb::self < nb::self)
        .def(nb::self <= nb::self)
        .def(nb::self > nb::self)
        .def(nb::self >= nb::self)

        .def(SdfLayerOffset() * nb::self)

        .def("__hash__", __hash__)
        .def("__repr__", __repr__)
//        .def(str(self))
        .def("__str__", _Str)
        ;

    TfPyRegisterStlSequencesFromPython<UsdTimeCode>();

    // Following is only to test that we can pass a vector of UsdTimeCode
    // objects to and from Python.
    s.def_static("Test_TimeCodeSequenceRoundTrip", 
        +[](const std::vector<UsdTimeCode> &times) {
            return times;  
        },
        nb::arg("times"));

    TF_NB_WRAP_PUBLIC_TOKENS(s, "Tokens", UsdTimeCodeTokens, USD_TIME_CODE_TOKENS)
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        });

    nb::implicitly_convertible<double, UsdTimeCode>();
    nb::implicitly_convertible<GfTimeCode, UsdTimeCode>();
}
