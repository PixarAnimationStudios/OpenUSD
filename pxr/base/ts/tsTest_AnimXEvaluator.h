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

#ifndef PXR_EXTRAS_BASE_TS_TEST_ANIM_X
#define PXR_EXTRAS_BASE_TS_TEST_ANIM_X

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/tsTest_Evaluator.h"

PXR_NAMESPACE_OPEN_SCOPE

class TS_API TsTest_AnimXEvaluator : public TsTest_Evaluator
{
public:
    enum AutoTanType
    {
        AutoTanAuto,
        AutoTanSmooth
    };

    TsTest_AnimXEvaluator(
        AutoTanType autoTanType = AutoTanAuto);

    TsTest_SampleVec Eval(
        const TsTest_SplineData &splineData,
        const TsTest_SampleTimes &sampleTimes) const override;

private:
    const AutoTanType _autoTanType;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
