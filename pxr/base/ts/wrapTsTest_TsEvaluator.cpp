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


void wrapTsTest_TsEvaluator()
{
    using This = TsTest_TsEvaluator;

    class_<This>("TsTest_TsEvaluator")
        // Default init is not suppressed, so automatically created.

        .def("Eval", &This::Eval,
            (arg("splineData"),
             arg("sampleTimes")),
            return_value_policy<TfPySequenceToList>())

        /*
        .def("Sample", &This::Sample,
            (arg("splineData"),
             arg("interval"),
             arg("tolerance")),
            return_value_policy<TfPySequenceToList>())
        */

        .def("SplineToSplineData", &This::SplineToSplineData,
            (arg("spline")))

        .def("SplineDataToSpline", &This::SplineDataToSpline,
            (arg("splineData"),
             arg("mayaTangentForm") = false))

        /*
        .def("BakeInnerLoops", &This::BakeInnerLoops,
            (arg("splineData")))
        */

        ;
}
