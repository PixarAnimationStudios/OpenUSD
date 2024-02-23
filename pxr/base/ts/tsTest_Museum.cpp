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

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_Museum.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

using SData = TsTest_SplineData;

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(TsTest_Museum::TwoKnotBezier);
    TF_ADD_ENUM_NAME(TsTest_Museum::TwoKnotLinear);
    TF_ADD_ENUM_NAME(TsTest_Museum::SimpleInnerLoop);
    TF_ADD_ENUM_NAME(TsTest_Museum::Recurve);
    TF_ADD_ENUM_NAME(TsTest_Museum::Crossover);
}


static TsTest_SplineData _TwoKnotBezier()
{
    SData::Knot knot1;
    knot1.time = 1.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 1.0;
    knot1.postSlope = 1.0;
    knot1.postLen = 0.5;

    SData::Knot knot2;
    knot2.time = 5.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 2.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 0.5;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _TwoKnotLinear()
{
    SData::Knot knot1;
    knot1.time = 1.0;
    knot1.nextSegInterpMethod = SData::InterpLinear;
    knot1.value = 1.0;

    SData::Knot knot2;
    knot2.time = 5.0;
    knot2.nextSegInterpMethod = SData::InterpLinear;
    knot2.value = 2.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _SimpleInnerLoop()
{
    SData::Knot knot1;
    knot1.time = 112.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 8.8;
    knot1.postSlope = 15.0;
    knot1.postLen = 0.9;

    SData::Knot knot2;
    knot2.time = 137.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 0.0;
    knot2.preSlope = -5.3;
    knot2.postSlope = -5.3;
    knot2.preLen = 1.3;
    knot2.postLen = 1.8;

    SData::Knot knot3;
    knot3.time = 145.0;
    knot3.nextSegInterpMethod = SData::InterpCurve;
    knot3.value = 8.5;
    knot3.preSlope = 12.5;
    knot3.postSlope = 12.5;
    knot3.preLen = 1.0;
    knot3.postLen = 1.2;

    SData::Knot knot4;
    knot4.time = 155.0;
    knot4.nextSegInterpMethod = SData::InterpCurve;
    knot4.value = 20.2;
    knot4.preSlope = -15.7;
    knot4.postSlope = -15.7;
    knot4.preLen = 0.7;
    knot4.postLen = 0.8;

    SData::Knot knot5;
    knot5.time = 181.0;
    knot5.nextSegInterpMethod = SData::InterpCurve;
    knot5.value = 38.2;
    knot5.preSlope = -9.0;
    knot5.preLen = 2.0;

    SData::InnerLoopParams lp;
    lp.enabled = true;
    lp.protoStart = 137.0;
    lp.protoEnd = 155.0;
    lp.preLoopStart = 119.0;
    lp.postLoopEnd = 173.0;
    lp.valueOffset = 20.2;

    SData data;
    data.SetKnots({knot1, knot2, knot3, knot4, knot5});
    data.SetInnerLoopParams(lp);
    return data;
}

static TsTest_SplineData _Recurve()
{
    SData::Knot knot1;
    knot1.time = 145.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = -5.6;
    knot1.postSlope = -1.3;
    knot1.postLen = 3.8;

    SData::Knot knot2;
    knot2.time = 156.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 0.0;
    knot2.preSlope = -1.3;
    knot2.postSlope = -1.3;
    knot2.preLen = 6.2;
    knot2.postLen = 15.8;

    SData::Knot knot3;
    knot3.time = 167.0;
    knot3.nextSegInterpMethod = SData::InterpCurve;
    knot3.value = 28.8;
    knot3.preSlope = 0.4;
    knot3.postSlope = 0.4;
    knot3.preLen = 16.8;
    knot3.postLen = 6.0;

    SData::Knot knot4;
    knot4.time = 185.0;
    knot4.nextSegInterpMethod = SData::InterpCurve;
    knot4.value = 0.0;
    knot4.preSlope = 3.6;
    knot4.postLen = 5.0;

    SData data;
    data.SetKnots({knot1, knot2, knot3, knot4});
    return data;
}

static TsTest_SplineData _Crossover()
{
    SData::Knot knot1;
    knot1.time = 145.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = -5.6;
    knot1.postSlope = -1.3;
    knot1.postLen = 3.8;

    SData::Knot knot2;
    knot2.time = 156.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 0.0;
    knot2.preSlope = -1.3;
    knot2.postSlope = -1.3;
    knot2.preLen = 6.2;
    knot2.postLen = 15.8;

    SData::Knot knot3;
    knot3.time = 167.0;
    knot3.nextSegInterpMethod = SData::InterpCurve;
    knot3.value = 28.8;
    knot3.preSlope = 2.4;
    knot3.postSlope = 2.4;
    knot3.preLen = 21.7;
    knot3.postLen = 5.5;

    SData::Knot knot4;
    knot4.time = 185.0;
    knot4.nextSegInterpMethod = SData::InterpCurve;
    knot4.value = 0.0;
    knot4.preSlope = 3.6;
    knot4.postLen = 5.0;

    SData data;
    data.SetKnots({knot1, knot2, knot3, knot4});
    return data;
}

TsTest_SplineData
TsTest_Museum::GetData(const DataId id)
{
    switch (id)
    {
        case TwoKnotBezier: return _TwoKnotBezier();
        case TwoKnotLinear: return _TwoKnotLinear();
        case SimpleInnerLoop: return _SimpleInnerLoop();
        case Recurve: return _Recurve();
        case Crossover: return _Crossover();
    }

    return {};
}


PXR_NAMESPACE_CLOSE_SCOPE
