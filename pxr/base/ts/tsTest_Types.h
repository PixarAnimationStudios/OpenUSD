//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_TS_TEST_TYPES_H
#define PXR_BASE_TS_TS_TEST_TYPES_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

struct TS_API TsTest_Sample
{
    double time = 0;
    double value = 0;

public:
    TsTest_Sample();
    TsTest_Sample(double time, double value);
    TsTest_Sample(const TsTest_Sample &other);
    TsTest_Sample& operator=(const TsTest_Sample &other);
};

using TsTest_SampleVec = std::vector<TsTest_Sample>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
