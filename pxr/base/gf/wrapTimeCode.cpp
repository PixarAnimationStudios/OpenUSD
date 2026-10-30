//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/gf/timeCode.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stringUtils.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>

#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

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

void wrapTimeCode(nanobind::module_& m)
{
    typedef GfTimeCode This;

    nb::class_<This>(m, "TimeCode")
        .def(nb::init<>())
        .def(nb::init_implicit<double>())

        .def("GetValue", &This::GetValue)

        .def("__repr__", _Repr)
        .def("__str__", _Str)
        .def("__bool__", _HasNonZeroTimeCode)
        .def("__hash__", &This::GetHash)
        .def("__float__", _Float)

        .def( double() == nb::self )
        .def( nb::self == nb::self )
        .def( double() != nb::self )
        .def( nb::self != nb::self )
        .def( double() < nb::self )
        .def( nb::self < nb::self )
        .def( double() > nb::self )
        .def( nb::self > nb::self )
        .def( double() <= nb::self )
        .def( nb::self <= nb::self )
        .def( double() >= nb::self )
        .def( nb::self >= nb::self )

        .def( double() * nb::self )
        .def( nb::self * double() )
        .def("__mul__", [](This const &, This const &) {
            throw nb::type_error();
        }, nb::is_operator())

        .def( nb::self / double() )
        .def( nb::self / nb::self )

        .def( nb::self + double() )
        .def( double() + nb::self )
        .def( nb::self + GfDuration() )
        .def( GfDuration() + nb::self )
        .def( nb::self + nb::self )

        .def( nb::self - double() )
        .def( double() - nb::self )
        .def( nb::self - GfDuration() )
        .def( GfDuration() - nb::self )
        .def( nb::self - nb::self )

        .def( -nb::self )
        ;

}
