//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_Types.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/vt/value.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <sstream>
#include <string>
#include <cstdio>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;


// Return a full-precision python repr for a double value.
static std::string
_HexFloatRepr(const double num)
{
    // XXX: work around std::hexfloat apparently not working in our libstdc++ as
    // of this writing.
    char buf[100];
    sprintf(buf, "float.fromhex('%a')", num);
    return std::string(buf);
}

static std::string
_SampleRepr(const TsTest_Sample &sample)
{
    std::ostringstream result;

    result << "Ts.TsTest_Sample("
           << _HexFloatRepr(sample.time)
           << ", " << _HexFloatRepr(sample.value)
           << ")";

    return result.str();
}

static 
TsSpline
_TestTsSplineToVtValueFromPython(VtValue val) {
    if (!val.IsHolding<TsSpline>()) {
        TF_CODING_ERROR("VtValue did not hold a TsSpline.");
        // Default Spline, but we are interested in the coding error so we
        // don't care.
        return TsSpline();
    }

    return val.UncheckedGet<TsSpline>();
}

void wrapTsTest_Types(nb::module_ &m)
{
    nb::class_<TsTest_Sample>(m, "TsTest_Sample")
        .def(nb::init<>())
        .def(nb::init<double, double>())
        .def(nb::init<const TsTest_Sample&>())
        .def("__repr__", &_SampleRepr)
        .def_rw("time", &TsTest_Sample::time)
        .def_rw("value", &TsTest_Sample::value)
        ;

    m.def("_TestTsSplineToVtValueFromPython", 
        _TestTsSplineToVtValueFromPython);
}
