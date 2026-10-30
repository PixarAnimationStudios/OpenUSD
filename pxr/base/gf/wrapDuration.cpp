//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/gf/duration.h"
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

static std::string _Str(GfDuration const &self)
{
    return TfStringify(self);
}

static std::string
_Repr(GfDuration const &self)
{
    std::ostringstream repr;
    repr << TF_PY_REPR_PREFIX << "Duration(" << self << ")";
    return repr.str();
}

static bool _HasNonZeroDuration(GfDuration const &self)
{
    return self != GfDuration(0.0);
}

static double _Float(GfDuration const &self)
{
    return double(self);
}

} // anonymous namespace

void wrapDuration(nanobind::module_& m)
{
    typedef GfDuration This;

    nb::class_<This>(m, "Duration")
        .def(nb::init<>())
        .def(nb::init_implicit<double>())

        .def("GetValue", &This::GetValue)

        .def("__repr__", _Repr)
        .def("__str__", _Str)
        .def("__bool__", _HasNonZeroDuration)
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

        .def( nb::self * double() )
        .def( double() * nb::self )
        .def("__mul__", [](This const &, This const &) {
            throw nb::type_error();
        }, nb::is_operator())
        .def( nb::self / double() )
        .def( nb::self / nb::self )
        .def( nb::self + double() )
        .def( double() + nb::self )
        .def( nb::self + nb::self )
        .def( nb::self - double() )
        .def( double() - nb::self )
        .def( nb::self - nb::self )
        .def( -nb::self )
        ;

}
