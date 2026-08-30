//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdText/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdTextTokensType::UsdTextTokensType() :
    bdf("bdf", TfToken::Immortal),
    bottomToTop("bottomToTop", TfToken::Immortal),
    charHeight("charHeight", TfToken::Immortal),
    charSpacingFactor("charSpacingFactor", TfToken::Immortal),
    charWidthFactor("charWidthFactor", TfToken::Immortal),
    doubleLines("doubleLines", TfToken::Immortal),
    fon("fon", TfToken::Immortal),
    fontAltFormat("font:altFormat", TfToken::Immortal),
    fontAltTypeface("font:altTypeface", TfToken::Immortal),
    fontBold("font:bold", TfToken::Immortal),
    fontFormat("font:format", TfToken::Immortal),
    fontItalic("font:italic", TfToken::Immortal),
    fontTypeface("font:typeface", TfToken::Immortal),
    fontWeight("font:weight", TfToken::Immortal),
    layoutBaselineDirection("layout:baselineDirection", TfToken::Immortal),
    layoutLinesStackDirection("layout:linesStackDirection", TfToken::Immortal),
    leftToRight("leftToRight", TfToken::Immortal),
    none("none", TfToken::Immortal),
    normal("normal", TfToken::Immortal),
    obliqueAngle("obliqueAngle", TfToken::Immortal),
    overlineType("overlineType", TfToken::Immortal),
    pcf("pcf", TfToken::Immortal),
    pfa_pfb("pfa/pfb", TfToken::Immortal),
    pixel("pixel", TfToken::Immortal),
    primvarsBackgroundColor("primvars:backgroundColor", TfToken::Immortal),
    primvarsBackgroundOpacity("primvars:backgroundOpacity", TfToken::Immortal),
    publishingPoint("publishingPoint", TfToken::Immortal),
    rightToLeft("rightToLeft", TfToken::Immortal),
    shx("shx", TfToken::Immortal),
    strikethroughType("strikethroughType", TfToken::Immortal),
    textData("textData", TfToken::Immortal),
    textMetricsUnit("textMetricsUnit", TfToken::Immortal),
    textStyleBinding("textStyle:binding", TfToken::Immortal),
    topToBottom("topToBottom", TfToken::Immortal),
    ttf_cff_otf("ttf/cff/otf", TfToken::Immortal),
    underlineType("underlineType", TfToken::Immortal),
    upToImpl("upToImpl", TfToken::Immortal),
    worldUnit("worldUnit", TfToken::Immortal),
    SimpleText("SimpleText", TfToken::Immortal),
    TextLayoutAPI("TextLayoutAPI", TfToken::Immortal),
    TextStyle("TextStyle", TfToken::Immortal),
    TextStyleAPI("TextStyleAPI", TfToken::Immortal),
    allTokens({
        bdf,
        bottomToTop,
        charHeight,
        charSpacingFactor,
        charWidthFactor,
        doubleLines,
        fon,
        fontAltFormat,
        fontAltTypeface,
        fontBold,
        fontFormat,
        fontItalic,
        fontTypeface,
        fontWeight,
        layoutBaselineDirection,
        layoutLinesStackDirection,
        leftToRight,
        none,
        normal,
        obliqueAngle,
        overlineType,
        pcf,
        pfa_pfb,
        pixel,
        primvarsBackgroundColor,
        primvarsBackgroundOpacity,
        publishingPoint,
        rightToLeft,
        shx,
        strikethroughType,
        textData,
        textMetricsUnit,
        textStyleBinding,
        topToBottom,
        ttf_cff_otf,
        underlineType,
        upToImpl,
        worldUnit,
        SimpleText,
        TextLayoutAPI,
        TextStyle,
        TextStyleAPI
    })
{
}

TfStaticData<UsdTextTokensType> UsdTextTokens;

PXR_NAMESPACE_CLOSE_SCOPE
