//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_Evaluator.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;

using This = TsTest_Evaluator;


void wrapTsTest_Evaluator()
{
    // TsTest_Evaluator is an abstract base class, so wrap without constructors.
    // Concrete classes need their own wrapping, declaring this class as a base.
    class_<This, boost::noncopyable>("TsTest_Evaluator", no_init)
        .def("Eval", &This::Eval,
            (arg("splineData"),
             arg("sampleTimes")),
            return_value_policy<TfPySequenceToList>())
        .def("Sample", &This::Sample,
            (arg("splineData"),
             arg("tolerance")),
            return_value_policy<TfPySequenceToList>())
        .def("BakeInnerLoops", &This::BakeInnerLoops,
            (arg("splineData")))
        ;
}
