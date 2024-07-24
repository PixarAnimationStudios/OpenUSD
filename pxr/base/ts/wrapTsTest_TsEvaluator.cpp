//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_TsEvaluator.h"

#include <boost/python.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;


void wrapTsTest_TsEvaluator()
{
    class_<TsTest_TsEvaluator, bases<TsTest_Evaluator>>("TsTest_TsEvaluator")
        // Default init is not suppressed, so automatically created.
        ;
}
