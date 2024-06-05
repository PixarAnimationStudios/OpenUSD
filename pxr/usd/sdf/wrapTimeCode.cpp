//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/timeCode.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/wrapArray.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/operators.hpp>

#include <sstream>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

TF_REGISTRY_FUNCTION(VtValue)
{
    VtRegisterValueCastsFromPythonSequencesToArray<SdfTimeCode>();
}

namespace {

static std::string _Str(SdfTimeCode const &self)
{
    return TfStringify(self);
}

static std::string
_Repr(SdfTimeCode const &self)
{
    std::ostringstream repr;
    repr << TF_PY_REPR_PREFIX << "TimeCode(" << self << ")";
    return repr.str();
}

static bool _HasNonZeroTimeCode(SdfTimeCode const &self)
{
    return self != SdfTimeCode(0.0);
}

static double _Float(SdfTimeCode const &self)
{
    return double(self);
}

} // anonymous namespace 

void wrapTimeCode()
{
    typedef SdfTimeCode This;

    auto selfCls = class_<This>("TimeCode", init<>())
        .def(init<double>())

        .def("GetValue", &This::GetValue)

        .def("__repr__", _Repr)
        .def("__str__", _Str)
        .def("__bool__", _HasNonZeroTimeCode)
        .def("__hash__", &This::GetHash)
        .def("__float__", _Float)

        .def( self == self )
        .def( double() == self )
        .def( self != self )
        .def( double() != self )
        .def( self < self )
        .def( double() < self )
        .def( self > self )
        .def( double() > self )
        .def( self <= self )
        .def( double() <= self )
        .def( self >= self )
        .def( double() >= self )

        .def( self * self )
        .def( double() * self )
        .def( self / self )
        .def( double() / self )
        .def( self + self )
        .def( double() + self )
        .def( self - self )
        .def( double() - self )
        ;

    implicitly_convertible<double, This>();

    // Let python know about us, to enable assignment from python back to C++
    VtValueFromPython<SdfTimeCode>();
}
