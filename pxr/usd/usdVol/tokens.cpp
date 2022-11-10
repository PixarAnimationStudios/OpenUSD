//
// Copyright 2016 Pixar
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
#include "pxr/usd/usdVol/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdVolTokensType::UsdVolTokensType() :
    bool_("bool", TfToken::Immortal),
    color("Color", TfToken::Immortal),
    double2("double2", TfToken::Immortal),
    double3("double3", TfToken::Immortal),
    double_("double", TfToken::Immortal),
    field("field", TfToken::Immortal),
    fieldClass("fieldClass", TfToken::Immortal),
    fieldDataType("fieldDataType", TfToken::Immortal),
    fieldIndex("fieldIndex", TfToken::Immortal),
    fieldName("fieldName", TfToken::Immortal),
    fieldPurpose("fieldPurpose", TfToken::Immortal),
    filePath("filePath", TfToken::Immortal),
    float2("float2", TfToken::Immortal),
    float3("float3", TfToken::Immortal),
    float_("float", TfToken::Immortal),
    fogVolume("fogVolume", TfToken::Immortal),
    half("half", TfToken::Immortal),
    half2("half2", TfToken::Immortal),
    half3("half3", TfToken::Immortal),
    int2("int2", TfToken::Immortal),
    int3("int3", TfToken::Immortal),
    int64("int64", TfToken::Immortal),
    int_("int", TfToken::Immortal),
    levelSet("levelSet", TfToken::Immortal),
    mask("mask", TfToken::Immortal),
    matrix3d("matrix3d", TfToken::Immortal),
    matrix4d("matrix4d", TfToken::Immortal),
    none("None", TfToken::Immortal),
    normal("Normal", TfToken::Immortal),
    point("Point", TfToken::Immortal),
    quatd("quatd", TfToken::Immortal),
    staggered("staggered", TfToken::Immortal),
    string("string", TfToken::Immortal),
    uint("uint", TfToken::Immortal),
    unknown("unknown", TfToken::Immortal),
    vector("Vector", TfToken::Immortal),
    vectorDataRoleHint("vectorDataRoleHint", TfToken::Immortal),
    allTokens({
        bool_,
        color,
        double2,
        double3,
        double_,
        field,
        fieldClass,
        fieldDataType,
        fieldIndex,
        fieldName,
        fieldPurpose,
        filePath,
        float2,
        float3,
        float_,
        fogVolume,
        half,
        half2,
        half3,
        int2,
        int3,
        int64,
        int_,
        levelSet,
        mask,
        matrix3d,
        matrix4d,
        none,
        normal,
        point,
        quatd,
        staggered,
        string,
        uint,
        unknown,
        vector,
        vectorDataRoleHint
    })
{
}

TfStaticData<UsdVolTokensType> UsdVolTokens;

PXR_NAMESPACE_CLOSE_SCOPE
