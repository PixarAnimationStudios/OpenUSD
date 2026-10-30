//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/tsTest_SampleTimes.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/diagnostic.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <sstream>
#include <string>
#include <cstdio>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

using This = TsTest_SampleTimes;


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
_SampleTimeRepr(const This::SampleTime &st)
{
    std::ostringstream result;

    result << "Ts.TsTest_SampleTimes.SampleTime("
           << _HexFloatRepr(st.time)
           << ", " << st.pre
           << ")";

    return result.str();
}

static void
_ConstructSampleTimes(
    This *self,
    const nb::object &times)
{
    new (self) This();

    std::vector<This::SampleTime> sts;
    if (!times.is_none())
    {
        if (!nb::try_cast<std::vector<This::SampleTime>>(times, sts))
        {
            TF_CODING_ERROR("Unexpected type for times");
        }
    }
    self->AddTimes(sts);
}

static std::string
_SampleTimesRepr(const This &times)
{
    std::ostringstream result;

    std::vector<std::string> stStrs;
    for (const This::SampleTime &st : times.GetTimes())
        stStrs.push_back(_SampleTimeRepr(st));

    result << "Ts.TsTest_SampleTimes([" << TfStringJoin(stStrs, ", ") << "])";

    return result.str();
}


void wrapTsTest_SampleTimes(nb::module_ &m)
{
    nb::class_<This> classObj(m, "TsTest_SampleTimes");

    nb::class_<This::SampleTime>(classObj, "SampleTime")
        .def(nb::init<>())
        .def(nb::init_implicit<double>())
        .def(nb::init<double, bool>())
        .def(nb::init<const This::SampleTime&>())
        .def("__repr__", &_SampleTimeRepr)
        .def(nb::self < nb::self)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def_rw("time", &This::SampleTime::time)
        .def_rw("pre", &This::SampleTime::pre)
        ;

    classObj
        .def(nb::init<const TsSpline&>())

        .def("__init__", &_ConstructSampleTimes,
             nb::arg("times") = nb::none())

        .def("__repr__", &_SampleTimesRepr)

        .def("AddTimes",
            static_cast<void (This::*)(const std::vector<double>&)>(
                &This::AddTimes))

        .def("AddTimes",
            static_cast<void (This::*)(const std::vector<This::SampleTime>&)>(
                &This::AddTimes))

        .def("AddKnotTimes", &This::AddKnotTimes)

        .def("AddUniformInterpolationTimes",
            &This::AddUniformInterpolationTimes,
            nb::arg("numSamples"))

        .def("AddExtrapolationTimes", &This::AddExtrapolationTimes,
            nb::arg("extrapolationFactor"))

        .def("AddExtrapolatingLoopTimes", &This::AddExtrapolatingLoopTimes,
            nb::arg("numIterations"),
            nb::arg("numSamplesPerIteration"))

        .def("AddStandardTimes", &This::AddStandardTimes)

        .def("GetTimes", &This::GetTimes)

        .def("GetMinTime", &This::GetMinTime)
        .def("GetMaxTime", &This::GetMaxTime)

        ;
}
