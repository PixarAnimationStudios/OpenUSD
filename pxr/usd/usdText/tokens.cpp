//
// Copyright 2024 Pixar
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
#include "pxr/usd/usdText/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdTextTokensType::UsdTextTokensType() :
    atLeast("atLeast", TfToken::Immortal),
    blockAlignment("blockAlignment", TfToken::Immortal),
    bold("bold", TfToken::Immortal),
    bottom("bottom", TfToken::Immortal),
    bottomToTop("bottomToTop", TfToken::Immortal),
    center("center", TfToken::Immortal),
    centerTab("centerTab", TfToken::Immortal),
    charSpacing("charSpacing", TfToken::Immortal),
    columnHeight("columnHeight", TfToken::Immortal),
    columnStyle("columnStyle", TfToken::Immortal),
    columnStyleBinding("columnStyle:binding", TfToken::Immortal),
    columnWidth("columnWidth", TfToken::Immortal),
    decimalTab("decimalTab", TfToken::Immortal),
    defaultDir("defaultDir", TfToken::Immortal),
    direction("direction", TfToken::Immortal),
    distributed("distributed", TfToken::Immortal),
    doubleLines("doubleLines", TfToken::Immortal),
    exactly("exactly", TfToken::Immortal),
    firstLineIndent("firstLineIndent", TfToken::Immortal),
    italic("italic", TfToken::Immortal),
    justify("justify", TfToken::Immortal),
    left("left", TfToken::Immortal),
    leftIndent("leftIndent", TfToken::Immortal),
    leftTab("leftTab", TfToken::Immortal),
    leftToRight("leftToRight", TfToken::Immortal),
    linesFlowDirection("linesFlowDirection", TfToken::Immortal),
    lineSpace("lineSpace", TfToken::Immortal),
    lineSpaceType("lineSpaceType", TfToken::Immortal),
    margins("margins", TfToken::Immortal),
    markupLanguage("markupLanguage", TfToken::Immortal),
    markupString("markupString", TfToken::Immortal),
    mtext("mtext", TfToken::Immortal),
    multiple("multiple", TfToken::Immortal),
    noMarkup("noMarkup", TfToken::Immortal),
    none("none", TfToken::Immortal),
    normal("normal", TfToken::Immortal),
    obliqueAngle("obliqueAngle", TfToken::Immortal),
    offset("offset", TfToken::Immortal),
    overlineType("overlineType", TfToken::Immortal),
    paragraphAlignment("paragraphAlignment", TfToken::Immortal),
    paragraphSpace("paragraphSpace", TfToken::Immortal),
    paragraphStyle("paragraphStyle", TfToken::Immortal),
    paragraphStyleBinding("paragraphStyle:binding", TfToken::Immortal),
    pixel("pixel", TfToken::Immortal),
    primvarsBackgroundColor("primvars:backgroundColor", TfToken::Immortal),
    primvarsBackgroundOpacity("primvars:backgroundOpacity", TfToken::Immortal),
    primvarsTextMetricsUnit("primvars:textMetricsUnit", TfToken::Immortal),
    publishingPoint("publishingPoint", TfToken::Immortal),
    renderer("renderer", TfToken::Immortal),
    right("right", TfToken::Immortal),
    rightIndent("rightIndent", TfToken::Immortal),
    rightTab("rightTab", TfToken::Immortal),
    rightToLeft("rightToLeft", TfToken::Immortal),
    strikethroughType("strikethroughType", TfToken::Immortal),
    tabStopPositions("tabStopPositions", TfToken::Immortal),
    tabStopTypes("tabStopTypes", TfToken::Immortal),
    textData("textData", TfToken::Immortal),
    textHeight("textHeight", TfToken::Immortal),
    textStyle("textStyle", TfToken::Immortal),
    textStyleBinding("textStyle:binding", TfToken::Immortal),
    textWidthFactor("textWidthFactor", TfToken::Immortal),
    top("top", TfToken::Immortal),
    topToBottom("topToBottom", TfToken::Immortal),
    typeface("typeface", TfToken::Immortal),
    underlineType("underlineType", TfToken::Immortal),
    weight("weight", TfToken::Immortal),
    worldUnit("worldUnit", TfToken::Immortal),
    ColumnStyle("ColumnStyle", TfToken::Immortal),
    ColumnStyleAPI("ColumnStyleAPI", TfToken::Immortal),
    MarkupText("MarkupText", TfToken::Immortal),
    ParagraphStyle("ParagraphStyle", TfToken::Immortal),
    ParagraphStyleAPI("ParagraphStyleAPI", TfToken::Immortal),
    SimpleText("SimpleText", TfToken::Immortal),
    TextLayout("TextLayout", TfToken::Immortal),
    TextStyle("TextStyle", TfToken::Immortal),
    TextStyleAPI("TextStyleAPI", TfToken::Immortal),
    allTokens({
        atLeast,
        blockAlignment,
        bold,
        bottom,
        bottomToTop,
        center,
        centerTab,
        charSpacing,
        columnHeight,
        columnStyle,
        columnStyleBinding,
        columnWidth,
        decimalTab,
        defaultDir,
        direction,
        distributed,
        doubleLines,
        exactly,
        firstLineIndent,
        italic,
        justify,
        left,
        leftIndent,
        leftTab,
        leftToRight,
        linesFlowDirection,
        lineSpace,
        lineSpaceType,
        margins,
        markupLanguage,
        markupString,
        mtext,
        multiple,
        noMarkup,
        none,
        normal,
        obliqueAngle,
        offset,
        overlineType,
        paragraphAlignment,
        paragraphSpace,
        paragraphStyle,
        paragraphStyleBinding,
        pixel,
        primvarsBackgroundColor,
        primvarsBackgroundOpacity,
        primvarsTextMetricsUnit,
        publishingPoint,
        renderer,
        right,
        rightIndent,
        rightTab,
        rightToLeft,
        strikethroughType,
        tabStopPositions,
        tabStopTypes,
        textData,
        textHeight,
        textStyle,
        textStyleBinding,
        textWidthFactor,
        top,
        topToBottom,
        typeface,
        underlineType,
        weight,
        worldUnit,
        ColumnStyle,
        ColumnStyleAPI,
        MarkupText,
        ParagraphStyle,
        ParagraphStyleAPI,
        SimpleText,
        TextLayout,
        TextStyle,
        TextStyleAPI
    })
{
}

TfStaticData<UsdTextTokensType> UsdTextTokens;

PXR_NAMESPACE_CLOSE_SCOPE
