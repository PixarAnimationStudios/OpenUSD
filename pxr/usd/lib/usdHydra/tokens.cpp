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
#include "pxr/usd/usdHydra/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdHydraTokensType::UsdHydraTokensType() :
    black("black", TfToken::Immortal),
    clamp("clamp", TfToken::Immortal),
    displayLookBxdf("displayLook:bxdf", TfToken::Immortal),
    faceIndex("faceIndex", TfToken::Immortal),
    faceOffset("faceOffset", TfToken::Immortal),
    frame("frame", TfToken::Immortal),
    HwPrimvar_1("HwPrimvar_1", TfToken::Immortal),
    HwPtexTexture_1("HwPtexTexture_1", TfToken::Immortal),
    HwUvTexture_1("HwUvTexture_1", TfToken::Immortal),
    infoFilename("info:filename", TfToken::Immortal),
    infoVarname("info:varname", TfToken::Immortal),
    linear("linear", TfToken::Immortal),
    linearMipmapLinear("linearMipmapLinear", TfToken::Immortal),
    linearMipmapNearest("linearMipmapNearest", TfToken::Immortal),
    magFilter("magFilter", TfToken::Immortal),
    minFilter("minFilter", TfToken::Immortal),
    mirror("mirror", TfToken::Immortal),
    nearest("nearest", TfToken::Immortal),
    nearestMipmapLinear("nearestMipmapLinear", TfToken::Immortal),
    nearestMipmapNearest("nearestMipmapNearest", TfToken::Immortal),
    repeat("repeat", TfToken::Immortal),
    textureMemory("textureMemory", TfToken::Immortal),
    uv("uv", TfToken::Immortal),
    wrapS("wrapS", TfToken::Immortal),
    wrapT("wrapT", TfToken::Immortal),
    allTokens({
        black,
        clamp,
        displayLookBxdf,
        faceIndex,
        faceOffset,
        frame,
        HwPrimvar_1,
        HwPtexTexture_1,
        HwUvTexture_1,
        infoFilename,
        infoVarname,
        linear,
        linearMipmapLinear,
        linearMipmapNearest,
        magFilter,
        minFilter,
        mirror,
        nearest,
        nearestMipmapLinear,
        nearestMipmapNearest,
        repeat,
        textureMemory,
        uv,
        wrapS,
        wrapT
    })
{
}

TfStaticData<UsdHydraTokensType> UsdHydraTokens;

PXR_NAMESPACE_CLOSE_SCOPE
