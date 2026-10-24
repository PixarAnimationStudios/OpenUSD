//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP_NB(Types);
    TF_WRAP_NB(Knot);
    TF_WRAP_NB(KnotMap);
    TF_WRAP_NB(Raii);
    TF_WRAP_NB(RegressionPreventer);
    TF_WRAP_NB(Spline);
    TF_WRAP_NB(TangentConversions);

#ifdef TS_BUILD_TEST_FRAMEWORK
    TF_WRAP_NB(TsTest_Museum);
    TF_WRAP_NB(TsTest_SampleBezier);
    TF_WRAP_NB(TsTest_SampleTimes);
    TF_WRAP_NB(TsTest_TsEvaluator);
    TF_WRAP_NB(TsTest_Types);
#endif

}
