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
#ifndef USDTEXT_TOKENS_H
#define USDTEXT_TOKENS_H

/// \file usdText/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdText/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdTextTokensType
///
/// \link UsdTextTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdTextTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdTextTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdTextTokens->atLeast);
/// \endcode
struct UsdTextTokensType {
    USDTEXT_API UsdTextTokensType();
    /// \brief "atLeast"
    /// 
    /// Fallback value for UsdTextParagraphStyle::GetLineSpaceTypeAttr()
    const TfToken atLeast;
    /// \brief "blockAlignment"
    /// 
    /// UsdTextColumnStyle
    const TfToken blockAlignment;
    /// \brief "bold"
    /// 
    /// UsdTextTextStyle
    const TfToken bold;
    /// \brief "bottom"
    /// 
    /// Possible value for UsdTextColumnStyle::GetBlockAlignmentAttr()
    const TfToken bottom;
    /// \brief "bottomToTop"
    /// 
    /// Possible value for UsdTextTextLayout::GetDirectionAttr(), Possible value for UsdTextColumnStyle::GetDirectionAttr(), Possible value for UsdTextColumnStyle::GetLinesFlowDirectionAttr()
    const TfToken bottomToTop;
    /// \brief "center"
    /// 
    /// Possible value for UsdTextColumnStyle::GetBlockAlignmentAttr(), Possible value for UsdTextParagraphStyle::GetParagraphAlignmentAttr()
    const TfToken center;
    /// \brief "centerTab"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetTabStopTypesAttr()
    const TfToken centerTab;
    /// \brief "charSpacing"
    /// 
    /// UsdTextTextStyle
    const TfToken charSpacing;
    /// \brief "columnHeight"
    /// 
    /// UsdTextColumnStyle
    const TfToken columnHeight;
    /// \brief "columnStyle"
    /// 
    /// . 
    const TfToken columnStyle;
    /// \brief "columnStyle:binding"
    /// 
    /// . 
    const TfToken columnStyleBinding;
    /// \brief "columnWidth"
    /// 
    /// UsdTextColumnStyle
    const TfToken columnWidth;
    /// \brief "decimalTab"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetTabStopTypesAttr()
    const TfToken decimalTab;
    /// \brief "defaultDir"
    /// 
    /// Fallback value for UsdTextTextLayout::GetDirectionAttr(), Fallback value for UsdTextColumnStyle::GetDirectionAttr()
    const TfToken defaultDir;
    /// \brief "direction"
    /// 
    /// UsdTextTextLayout, UsdTextColumnStyle
    const TfToken direction;
    /// \brief "distributed"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetParagraphAlignmentAttr()
    const TfToken distributed;
    /// \brief "doubleLines"
    /// 
    /// Possible value for UsdTextTextStyle::GetStrikethroughTypeAttr()
    const TfToken doubleLines;
    /// \brief "exactly"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetLineSpaceTypeAttr()
    const TfToken exactly;
    /// \brief "firstLineIndent"
    /// 
    /// UsdTextParagraphStyle
    const TfToken firstLineIndent;
    /// \brief "italic"
    /// 
    /// UsdTextTextStyle
    const TfToken italic;
    /// \brief "justify"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetParagraphAlignmentAttr()
    const TfToken justify;
    /// \brief "left"
    /// 
    /// Fallback value for UsdTextParagraphStyle::GetParagraphAlignmentAttr()
    const TfToken left;
    /// \brief "leftIndent"
    /// 
    /// UsdTextParagraphStyle
    const TfToken leftIndent;
    /// \brief "leftTab"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetTabStopTypesAttr()
    const TfToken leftTab;
    /// \brief "leftToRight"
    /// 
    /// Possible value for UsdTextTextLayout::GetDirectionAttr(), Possible value for UsdTextColumnStyle::GetDirectionAttr(), Possible value for UsdTextColumnStyle::GetLinesFlowDirectionAttr()
    const TfToken leftToRight;
    /// \brief "linesFlowDirection"
    /// 
    /// UsdTextColumnStyle
    const TfToken linesFlowDirection;
    /// \brief "lineSpace"
    /// 
    /// UsdTextParagraphStyle
    const TfToken lineSpace;
    /// \brief "lineSpaceType"
    /// 
    /// UsdTextParagraphStyle
    const TfToken lineSpaceType;
    /// \brief "margins"
    /// 
    /// UsdTextColumnStyle
    const TfToken margins;
    /// \brief "markupLanguage"
    /// 
    /// UsdTextMarkupText
    const TfToken markupLanguage;
    /// \brief "markupString"
    /// 
    /// UsdTextMarkupText
    const TfToken markupString;
    /// \brief "mtext"
    /// 
    /// Possible value for UsdTextMarkupText::GetMarkupLanguageAttr()
    const TfToken mtext;
    /// \brief "multiple"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetLineSpaceTypeAttr()
    const TfToken multiple;
    /// \brief "noMarkup"
    /// 
    /// Fallback value for UsdTextMarkupText::GetMarkupLanguageAttr()
    const TfToken noMarkup;
    /// \brief "none"
    /// 
    /// Possible value for UsdTextTextStyle::GetOverlineTypeAttr(), Possible value for UsdTextTextStyle::GetStrikethroughTypeAttr(), Possible value for UsdTextTextStyle::GetUnderlineTypeAttr()
    const TfToken none;
    /// \brief "normal"
    /// 
    /// Possible value for UsdTextTextStyle::GetOverlineTypeAttr(), Possible value for UsdTextTextStyle::GetStrikethroughTypeAttr(), Possible value for UsdTextTextStyle::GetUnderlineTypeAttr()
    const TfToken normal;
    /// \brief "obliqueAngle"
    /// 
    /// UsdTextTextStyle
    const TfToken obliqueAngle;
    /// \brief "offset"
    /// 
    /// UsdTextColumnStyle
    const TfToken offset;
    /// \brief "overlineType"
    /// 
    /// UsdTextTextStyle
    const TfToken overlineType;
    /// \brief "paragraphAlignment"
    /// 
    /// UsdTextParagraphStyle
    const TfToken paragraphAlignment;
    /// \brief "paragraphSpace"
    /// 
    /// UsdTextParagraphStyle
    const TfToken paragraphSpace;
    /// \brief "paragraphStyle"
    /// 
    /// . 
    const TfToken paragraphStyle;
    /// \brief "paragraphStyle:binding"
    /// 
    /// . 
    const TfToken paragraphStyleBinding;
    /// \brief "pixel"
    /// 
    /// Possible value for UsdTextSimpleText::GetTextMetricsUnitAttr(), Possible value for UsdTextMarkupText::GetTextMetricsUnitAttr()
    const TfToken pixel;
    /// \brief "primvars:backgroundColor"
    /// 
    /// UsdTextSimpleText, UsdTextMarkupText
    const TfToken primvarsBackgroundColor;
    /// \brief "primvars:backgroundOpacity"
    /// 
    /// UsdTextSimpleText, UsdTextMarkupText
    const TfToken primvarsBackgroundOpacity;
    /// \brief "primvars:textMetricsUnit"
    /// 
    /// UsdTextSimpleText, UsdTextMarkupText
    const TfToken primvarsTextMetricsUnit;
    /// \brief "publishingPoint"
    /// 
    /// Possible value for UsdTextSimpleText::GetTextMetricsUnitAttr(), Possible value for UsdTextMarkupText::GetTextMetricsUnitAttr()
    const TfToken publishingPoint;
    /// \brief "renderer"
    /// 
    /// UsdTextSimpleText, UsdTextMarkupText
    const TfToken renderer;
    /// \brief "right"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetParagraphAlignmentAttr()
    const TfToken right;
    /// \brief "rightIndent"
    /// 
    /// UsdTextParagraphStyle
    const TfToken rightIndent;
    /// \brief "rightTab"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetTabStopTypesAttr()
    const TfToken rightTab;
    /// \brief "rightToLeft"
    /// 
    /// Possible value for UsdTextTextLayout::GetDirectionAttr(), Possible value for UsdTextColumnStyle::GetDirectionAttr(), Possible value for UsdTextColumnStyle::GetLinesFlowDirectionAttr()
    const TfToken rightToLeft;
    /// \brief "strikethroughType"
    /// 
    /// UsdTextTextStyle
    const TfToken strikethroughType;
    /// \brief "tabStopPositions"
    /// 
    /// UsdTextParagraphStyle
    const TfToken tabStopPositions;
    /// \brief "tabStopTypes"
    /// 
    /// UsdTextParagraphStyle
    const TfToken tabStopTypes;
    /// \brief "textData"
    /// 
    /// UsdTextSimpleText
    const TfToken textData;
    /// \brief "textHeight"
    /// 
    /// UsdTextTextStyle
    const TfToken textHeight;
    /// \brief "textStyle"
    /// 
    /// . 
    const TfToken textStyle;
    /// \brief "textStyle:binding"
    /// 
    /// . 
    const TfToken textStyleBinding;
    /// \brief "textWidthFactor"
    /// 
    /// UsdTextTextStyle
    const TfToken textWidthFactor;
    /// \brief "top"
    /// 
    /// Fallback value for UsdTextColumnStyle::GetBlockAlignmentAttr()
    const TfToken top;
    /// \brief "topToBottom"
    /// 
    /// Possible value for UsdTextTextLayout::GetDirectionAttr(), Possible value for UsdTextColumnStyle::GetDirectionAttr(), Fallback value for UsdTextColumnStyle::GetLinesFlowDirectionAttr()
    const TfToken topToBottom;
    /// \brief "typeface"
    /// 
    /// UsdTextTextStyle
    const TfToken typeface;
    /// \brief "underlineType"
    /// 
    /// UsdTextTextStyle
    const TfToken underlineType;
    /// \brief "weight"
    /// 
    /// UsdTextTextStyle
    const TfToken weight;
    /// \brief "worldUnit"
    /// 
    /// Fallback value for UsdTextSimpleText::GetTextMetricsUnitAttr(), Fallback value for UsdTextMarkupText::GetTextMetricsUnitAttr()
    const TfToken worldUnit;
    /// \brief "ColumnStyle"
    /// 
    /// Schema identifer and family for UsdTextColumnStyle
    const TfToken ColumnStyle;
    /// \brief "ColumnStyleAPI"
    /// 
    /// Schema identifer and family for UsdTextColumnStyleAPI
    const TfToken ColumnStyleAPI;
    /// \brief "MarkupText"
    /// 
    /// Schema identifer and family for UsdTextMarkupText
    const TfToken MarkupText;
    /// \brief "ParagraphStyle"
    /// 
    /// Schema identifer and family for UsdTextParagraphStyle
    const TfToken ParagraphStyle;
    /// \brief "ParagraphStyleAPI"
    /// 
    /// Schema identifer and family for UsdTextParagraphStyleAPI
    const TfToken ParagraphStyleAPI;
    /// \brief "SimpleText"
    /// 
    /// Schema identifer and family for UsdTextSimpleText
    const TfToken SimpleText;
    /// \brief "TextLayout"
    /// 
    /// Schema identifer and family for UsdTextTextLayout
    const TfToken TextLayout;
    /// \brief "TextStyle"
    /// 
    /// Schema identifer and family for UsdTextTextStyle
    const TfToken TextStyle;
    /// \brief "TextStyleAPI"
    /// 
    /// Schema identifer and family for UsdTextTextStyleAPI
    const TfToken TextStyleAPI;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdTextTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdTextTokensType
extern USDTEXT_API TfStaticData<UsdTextTokensType> UsdTextTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
