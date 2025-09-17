//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    hydraGenerativeProcedural("hydraGenerativeProcedural", TfToken::Immortal),
    infoFilename("inputs:file", TfToken::Immortal),
    infoVarname("inputs:varname", TfToken::Immortal),
    linear("linear", TfToken::Immortal),
    linearMipmapLinear("linearMipmapLinear", TfToken::Immortal),
    linearMipmapNearest("linearMipmapNearest", TfToken::Immortal),
    magFilter("magFilter", TfToken::Immortal),
    minFilter("minFilter", TfToken::Immortal),
    mirror("mirror", TfToken::Immortal),
    nearest("nearest", TfToken::Immortal),
    nearestMipmapLinear("nearestMipmapLinear", TfToken::Immortal),
    nearestMipmapNearest("nearestMipmapNearest", TfToken::Immortal),
    primvarsHdGpProceduralType("primvars:hdGp:proceduralType", TfToken::Immortal),
    proceduralSystem("proceduralSystem", TfToken::Immortal),
    repeat("repeat", TfToken::Immortal),
    textureMemory("textureMemory", TfToken::Immortal),
    useMetadata("useMetadata", TfToken::Immortal),
    uv("uv", TfToken::Immortal),
    wrapS("wrapS", TfToken::Immortal),
    wrapT("wrapT", TfToken::Immortal),
    HydraGenerativeProceduralAPI("HydraGenerativeProceduralAPI", TfToken::Immortal),
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
        hydraGenerativeProcedural,
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
        primvarsHdGpProceduralType,
        proceduralSystem,
        repeat,
        textureMemory,
        useMetadata,
        uv,
        wrapS,
        wrapT,
        HydraGenerativeProceduralAPI
    })
{
}

TfStaticData<UsdHydraTokensType> UsdHydraTokens;

PXR_NAMESPACE_CLOSE_SCOPE
