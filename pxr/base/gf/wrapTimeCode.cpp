//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/gf/timeCode.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/implicit.hpp"
#include "pxr/external/boost/python/operators.hpp"

#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static std::string _Str(GfTimeCode const &self)
{
    return TfStringify(self);
}

static std::string
_Repr(GfTimeCode const &self)
{
    std::ostringstream repr;
    repr << TF_PY_REPR_PREFIX << "TimeCode(" << self << ")";
    return repr.str();
}

static bool _HasNonZeroTimeCode(GfTimeCode const &self)
{
    return self != GfTimeCode(0.0);
}

static double _Float(GfTimeCode const &self)
{
    return double(self);
}

} // anonymous namespace

void wrapTimeCode()
{
    typedef GfTimeCode This;

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
}
