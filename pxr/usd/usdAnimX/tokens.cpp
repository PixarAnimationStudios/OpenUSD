//
// Copyright 2020 benmalartre
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
#include "tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdAnimXTokensType::UsdAnimXTokensType() :
    prim("prim", TfToken::Immortal),
    op("op", TfToken::Immortal),
    curve("curve", TfToken::Immortal),
    keyframes("keyframes", TfToken::Immortal),
    target("target", TfToken::Immortal),
    dataType("dataType", TfToken::Immortal),
    defaultValue("defaultValue", TfToken::Immortal),
    curveInterpolationMethod("curveInterpolationMethod", TfToken::Immortal),
    curveRotationInterpolationMethod("curveRotationInterpolationMethod", TfToken::Immortal),
    constant("constant", TfToken::Immortal),
    linear("linear", TfToken::Immortal),
    cycle("cycle", TfToken::Immortal),
    cycleRelative("cycleRelative", TfToken::Immortal),
    oscillate("oscillate", TfToken::Immortal),
    postInfinityType("postInfinityType", TfToken::Immortal),
    preInfinityType("preInfinityType", TfToken::Immortal),
    tangentType("tangentType", TfToken::Immortal),
    global("global", TfToken::Immortal),
    fixed("fixed", TfToken::Immortal),
    flat("flat", TfToken::Immortal),
    step("step", TfToken::Immortal),
    slow("slow", TfToken::Immortal),
    fast("fast", TfToken::Immortal),
    smooth("smooth", TfToken::Immortal),
    clamped("clamped", TfToken::Immortal),
    automatic("automatic", TfToken::Immortal),
    sine("sine", TfToken::Immortal),
    parabolic("parabolic", TfToken::Immortal),
    logarithmic("logarithmic", TfToken::Immortal),
    plateau("plateau", TfToken::Immortal),
    stepNext("stepNext", TfToken::Immortal),
    allTokens({
        target,
        dataType,
        defaultValue,
        curveInterpolationMethod,
        curveRotationInterpolationMethod,
        constant,
        linear,
        cycle,
        cycleRelative,
        oscillate,
        postInfinityType,
        preInfinityType,
        tangentType,
        global,
        fixed,
        flat,
        step,
        slow,
        fast,
        smooth,
        clamped,
        automatic,
        sine,
        parabolic,
        logarithmic,
        plateau,
        stepNext,
    })
{
}

TfStaticData<UsdAnimXTokensType> UsdAnimXTokens;

PXR_NAMESPACE_CLOSE_SCOPE
