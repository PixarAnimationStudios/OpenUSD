//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdShade/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdShadeTokensType::UsdShadeTokensType() :
    allPurpose("", TfToken::Immortal),
    bindMaterialAs("bindMaterialAs", TfToken::Immortal),
    coordSys("coordSys", TfToken::Immortal),
    coordSys_MultipleApplyTemplate_Binding("coordSys:__INSTANCE_NAME__:binding", TfToken::Immortal),
    displacement("displacement", TfToken::Immortal),
    fallbackStrength("fallbackStrength", TfToken::Immortal),
    full("full", TfToken::Immortal),
    id("id", TfToken::Immortal),
    infoId("info:id", TfToken::Immortal),
    infoImplementationSource("info:implementationSource", TfToken::Immortal),
    inputs("inputs:", TfToken::Immortal),
    interfaceOnly("interfaceOnly", TfToken::Immortal),
    materialBind("materialBind", TfToken::Immortal),
    materialBinding("material:binding", TfToken::Immortal),
    materialBindingCollection("material:binding:collection", TfToken::Immortal),
    materialVariant("materialVariant", TfToken::Immortal),
    outputs("outputs:", TfToken::Immortal),
    outputsDisplacement("outputs:displacement", TfToken::Immortal),
    outputsSurface("outputs:surface", TfToken::Immortal),
    outputsVolume("outputs:volume", TfToken::Immortal),
    preview("preview", TfToken::Immortal),
    sdrMetadata("sdrMetadata", TfToken::Immortal),
    sourceAsset("sourceAsset", TfToken::Immortal),
    sourceCode("sourceCode", TfToken::Immortal),
    strongerThanDescendants("strongerThanDescendants", TfToken::Immortal),
    subIdentifier("subIdentifier", TfToken::Immortal),
    surface("surface", TfToken::Immortal),
    universalRenderContext("", TfToken::Immortal),
    universalSourceType("", TfToken::Immortal),
    volume("volume", TfToken::Immortal),
    weakerThanDescendants("weakerThanDescendants", TfToken::Immortal),
    ConnectableAPI("ConnectableAPI", TfToken::Immortal),
    CoordSysAPI("CoordSysAPI", TfToken::Immortal),
    Material("Material", TfToken::Immortal),
    MaterialBindingAPI("MaterialBindingAPI", TfToken::Immortal),
    NodeDefAPI("NodeDefAPI", TfToken::Immortal),
    NodeGraph("NodeGraph", TfToken::Immortal),
    Shader("Shader", TfToken::Immortal),
    allTokens({
        allPurpose,
        bindMaterialAs,
        coordSys,
        coordSys_MultipleApplyTemplate_Binding,
        displacement,
        fallbackStrength,
        full,
        id,
        infoId,
        infoImplementationSource,
        inputs,
        interfaceOnly,
        materialBind,
        materialBinding,
        materialBindingCollection,
        materialVariant,
        outputs,
        outputsDisplacement,
        outputsSurface,
        outputsVolume,
        preview,
        sdrMetadata,
        sourceAsset,
        sourceCode,
        strongerThanDescendants,
        subIdentifier,
        surface,
        universalRenderContext,
        universalSourceType,
        volume,
        weakerThanDescendants,
        ConnectableAPI,
        CoordSysAPI,
        Material,
        MaterialBindingAPI,
        NodeDefAPI,
        NodeGraph,
        Shader
    })
{
}

TfStaticData<UsdShadeTokensType> UsdShadeTokens;

PXR_NAMESPACE_CLOSE_SCOPE
