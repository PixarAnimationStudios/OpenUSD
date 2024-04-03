//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#ifndef PXR_BASE_TS_TS_TEST_EVALUATOR_H
#define PXR_BASE_TS_TS_TEST_EVALUATOR_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/tsTest_SplineData.h"
#include "pxr/base/ts/tsTest_SampleTimes.h"
#include "pxr/base/ts/tsTest_Types.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class TS_API TsTest_Evaluator
{
public:
    // Required.  Evaluates a spline at the specified times.
    virtual TsTest_SampleVec Eval(
        const TsTest_SplineData &splineData,
        const TsTest_SampleTimes &sampleTimes) const = 0;

    // Optional.  Produces samples at implementation-determined times,
    // sufficient to produce a piecewise linear approximation of the spline with
    // an absolute value error less than the specified tolerance.  Default
    // implementation returns no samples.
    virtual TsTest_SampleVec Sample(
        const TsTest_SplineData &splineData,
        double tolerance) const;

    // Optional.  Produce a copy of splineData with inner loops, if any, baked
    // out into ordinary knots.  Default implementation returns the input data
    // unmodified.
    virtual TsTest_SplineData BakeInnerLoops(
        const TsTest_SplineData &splineData) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
