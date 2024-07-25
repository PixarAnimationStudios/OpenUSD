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
    TF_WRAP(Types);
    TF_WRAP(Knot);
    TF_WRAP(KnotMap);
    TF_WRAP(Raii);
    TF_WRAP(RegressionPreventer);
    TF_WRAP(Spline);

#ifdef TS_BUILD_TEST_FRAMEWORK
    TF_WRAP(TsTest_Museum);
    TF_WRAP(TsTest_SampleBezier);
    TF_WRAP(TsTest_SampleTimes);
    TF_WRAP(TsTest_SplineData);
    TF_WRAP(TsTest_TsEvaluator);
    TF_WRAP(TsTest_Types);
#endif

#ifdef TS_BUILD_ANIMX_TEST_FRAMEWORK
    TF_WRAP(TsTest_AnimXEvaluator);
#endif
}
