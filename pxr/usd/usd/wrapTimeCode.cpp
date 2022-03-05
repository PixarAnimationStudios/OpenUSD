//
// Copyright 2016 Pixar
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
#include "pxr/usd/usd/timeCode.h"

#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/operators.hpp>

#include <string>



PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static size_t __hash__(const UsdTimeCode &self) { return hash_value(self); }

static std::string _Str(const UsdTimeCode &self)
{
    return boost::lexical_cast<std::string>(self);
}

static std::string __repr__(const UsdTimeCode &self)
{
    std::string tail = ".Default()";
    if (self.IsNumeric()) {
        if (self.IsEarliestTime()) {
            tail = ".EarliestTime()";
        } else {
            tail = self.GetValue() == 0.0 ? std::string("()") :
                TfStringPrintf("(%s)", TfPyRepr(self.GetValue()).c_str());
        }
    }
    return TF_PY_REPR_PREFIX + "TimeCode" + tail;
}

} // anonymous namespace 

void wrapUsdTimeCode()
{
    boost::python::scope s = boost::python::class_<UsdTimeCode>("TimeCode")
        .def(boost::python::init<double>())
        .def(boost::python::init<SdfTimeCode>())
        .def(boost::python::init<UsdTimeCode>())

        .def("EarliestTime", &UsdTimeCode::EarliestTime)
        .staticmethod("EarliestTime")
        
        .def("Default", &UsdTimeCode::Default)
        .staticmethod("Default")
        
        .def("SafeStep", &UsdTimeCode::SafeStep,
             (boost::python::arg("maxValue")=1e6, boost::python::arg("maxCompression")=10.0))
        .staticmethod("SafeStep")

        .def("IsEarliestTime", &UsdTimeCode::IsEarliestTime)
        .def("IsDefault", &UsdTimeCode::IsDefault)
        .def("IsNumeric", &UsdTimeCode::IsNumeric)
        .def("GetValue", &UsdTimeCode::GetValue)

        .def(boost::python::self == boost::python::self)
        .def(boost::python::self != boost::python::self)
        .def(boost::python::self < boost::python::self)
        .def(boost::python::self <= boost::python::self)
        .def(boost::python::self > boost::python::self)
        .def(boost::python::self >= boost::python::self)

        .def("__hash__", __hash__)
        .def("__repr__", __repr__)
//        .def(str(self))
        .def("__str__", _Str)
        ;

    TF_PY_WRAP_PUBLIC_TOKENS("Tokens", UsdTimeCodeTokens, USD_TIME_CODE_TOKENS);

    boost::python::implicitly_convertible<double, UsdTimeCode>();
    boost::python::implicitly_convertible<SdfTimeCode, UsdTimeCode>();
}
