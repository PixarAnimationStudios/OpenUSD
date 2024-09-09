//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/implicit.hpp"
#include "pxr/external/boost/python/operators.hpp"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
        if (self.IsEarliestTime()) {
            tail = ".EarliestTime()";
        } else {
            tail = self.GetValue() == 0.0 ? string("()") :
                TfStringPrintf("(%s)", TfPyRepr(self.GetValue()).c_str());
        }
    }
    return TF_PY_REPR_PREFIX + "TimeCode" + tail;
}

} // anonymous namespace 

void wrapUsdTimeCode()
{
    scope s = class_<UsdTimeCode>("TimeCode")
        .def(init<double>())
        .def(init<SdfTimeCode>())
        .def(init<UsdTimeCode>())

        .def("EarliestTime", &UsdTimeCode::EarliestTime)
        .staticmethod("EarliestTime")
        
        .def("Default", &UsdTimeCode::Default)
        .staticmethod("Default")
        
        .def("SafeStep", &UsdTimeCode::SafeStep,
             (arg("maxValue")=1e6, arg("maxCompression")=10.0))
        .staticmethod("SafeStep")

        .def("IsEarliestTime", &UsdTimeCode::IsEarliestTime)
        .def("IsDefault", &UsdTimeCode::IsDefault)
        .def("IsNumeric", &UsdTimeCode::IsNumeric)
        .def("GetValue", &UsdTimeCode::GetValue)

        .def(self == self)
        .def(self != self)
        .def(self < self)
        .def(self <= self)
        .def(self > self)
        .def(self >= self)

        .def("__hash__", __hash__)
        .def("__repr__", __repr__)
//        .def(str(self))
        .def("__str__", _Str)
        ;

    TF_PY_WRAP_PUBLIC_TOKENS("Tokens", UsdTimeCodeTokens, USD_TIME_CODE_TOKENS);

    implicitly_convertible<double, UsdTimeCode>();
    implicitly_convertible<SdfTimeCode, UsdTimeCode>();
}
