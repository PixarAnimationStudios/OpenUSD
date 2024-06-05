//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP(KeyFrame);
    TF_WRAP(LoopParams);
    TF_WRAP(Simplify);
    TF_WRAP(Spline);
    TF_WRAP(TsTest_Evaluator);
    TF_WRAP(TsTest_Museum);
    TF_WRAP(TsTest_SampleBezier);
    TF_WRAP(TsTest_SampleTimes);
    TF_WRAP(TsTest_SplineData);
    TF_WRAP(TsTest_TsEvaluator);
    TF_WRAP(TsTest_Types);
    TF_WRAP(Types);

#ifdef PXR_BUILD_ANIMX_TESTS
    TF_WRAP(TsTest_AnimXEvaluator);
#endif
}
