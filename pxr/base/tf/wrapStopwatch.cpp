//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/stopwatch.h"

PXR_NAMESPACE_USING_DIRECTIVE

#include "pxr/base/tf/pyNanobindBridge.h"

void wrapStopwatch(nanobind::module_& m) {

    typedef TfStopwatch This;
    namespace nb = nanobind;

    nb::class_<This>(m, "Stopwatch")
        .def(nb::init<>())
        .def(nb::init<const This&>())

        .def("Start", &This::Start)
        .def("Stop", &This::Stop)
        .def("Reset", &This::Reset)
        .def("AddFrom", &This::AddFrom)

        .def_prop_ro("nanoseconds", &This::GetNanoseconds)
        .def_prop_ro("microseconds", &This::GetMicroseconds)
        .def_prop_ro("milliseconds", &This::GetMilliseconds)
        .def_prop_ro("sampleCount", &This::GetSampleCount)
        .def_prop_ro("seconds", &This::GetSeconds)
        ;
}
