//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_TsEvaluator.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/tsTest_SplineData.h"
#include "pxr/base/ts/tsTest_SampleTimes.h"

#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

static object
_wrapSample(const TsTest_TsEvaluator& evaluator,
            const TsTest_SplineData &splineData,
            const GfInterval& timeInterval,
            const double timeScale,
            const double valueScale,
            const double tolerance,
            bool withSamples = false)
{
    if (withSamples) {
        TsSplineSamplesWithSources<GfVec2d> samples;
        if (evaluator.Sample(splineData, timeInterval,
                             timeScale, valueScale, tolerance,
                             &samples)) {
            return object(samples);
        }
    } else {
        TsSplineSamples<GfVec2d> samples;
        if (evaluator.Sample(splineData, timeInterval,
                             timeScale, valueScale, tolerance,
                             &samples)) {
            return object(samples);
        }
    }

    return object();
}

static TsSpline
_wrapSplineDataToSpline(const TsTest_TsEvaluator& evaluator,
                        const TsTest_SplineData &splineData,
                        const std::string& valueTypeName = "double")
{
    const TfType valueType = Ts_GetTypeFromTypeName(valueTypeName);
    if (!valueType) {
        TfPyThrowTypeError("Invalid spline type name '" + valueTypeName + "'");
        return TsSpline();
    }

    return evaluator.SplineDataToSpline(splineData, valueType);
}

void wrapTsTest_TsEvaluator()
{
    using This = TsTest_TsEvaluator;

    class_<This>("TsTest_TsEvaluator")
        // Default init is not suppressed, so automatically created.

        .def("Eval", &This::Eval,
            (arg("splineData"),
             arg("sampleTimes")),
            return_value_policy<TfPySequenceToList>())

        .def("Sample", &_wrapSample,
            (arg("splineData"),
             arg("timeInterval"),
             arg("timeScale"),
             arg("valueScale"),
             arg("tolerance"),
             arg("withSources") = false))

        .def("SplineToSplineData", &This::SplineToSplineData,
            (arg("spline")))

        .def("SplineDataToSpline", &_wrapSplineDataToSpline,
             (arg("splineData"),
              arg("valueType") = "double"))

        /*
        .def("BakeInnerLoops", &This::BakeInnerLoops,
            (arg("splineData")))
        */

        ;
}
