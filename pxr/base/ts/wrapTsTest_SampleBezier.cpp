//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_SampleBezier.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/return_value_policy.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;


void wrapTsTest_SampleBezier()
{
    def("TsTest_SampleBezier", &TsTest_SampleBezier,
        (arg("splineData"),
         arg("numSamples")),
        return_value_policy<TfPySequenceToList>());
}
