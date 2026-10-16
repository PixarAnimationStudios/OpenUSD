//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_TsEvaluator.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/tsTest_SampleTimes.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

static nb::object
_wrapSample(const TsTest_TsEvaluator& evaluator,
            const TsSpline& spline,
            const GfInterval& timeInterval,
            const double timeScale,
            const double valueScale,
            const double tolerance,
            bool withSamples = false)
{
    if (withSamples) {
        TsSplineSamplesWithSources<GfVec2d> samples;
        if (evaluator.Sample(spline, timeInterval,
                             timeScale, valueScale, tolerance,
                             &samples)) {
            return nb::cast(samples);
        }
    } else {
        TsSplineSamples<GfVec2d> samples;
        if (evaluator.Sample(spline, timeInterval,
                             timeScale, valueScale, tolerance,
                             &samples)) {
            return nb::cast(samples);
        }
    }

    return nb::none();
}

void wrapTsTest_TsEvaluator(nb::module_ &m)
{
    using This = TsTest_TsEvaluator;

    nb::class_<This>(m, "TsTest_TsEvaluator")
        .def(nb::init<>())

        .def("Eval", &This::Eval,
            nb::arg("spline"),
            nb::arg("sampleTimes"))

        .def("Sample", &_wrapSample,
            nb::arg("spline"),
            nb::arg("timeInterval"),
            nb::arg("timeScale"),
            nb::arg("valueScale"),
            nb::arg("tolerance"),
            nb::arg("withSources") = false)

        /*
        .def("BakeInnerLoops", &This::BakeInnerLoops,
            (arg("splineData")))
        */

        ;
}
