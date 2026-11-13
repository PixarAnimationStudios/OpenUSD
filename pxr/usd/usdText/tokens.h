//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    /// \brief "bottom"
    /// 
    /// Possible value for UsdTextColumnStyle::GetColumnAlignmentAttr()
    const TfToken bottom;
    /// \brief "bottomToTop"
    /// 
    /// Possible value for UsdTextTextLayoutAPI::GetLayoutBaselineDirectionAttr(), Possible value for UsdTextTextLayoutAPI::GetLayoutLinesStackDirectionAttr()
    const TfToken bottomToTop;
    /// \brief "center"
    /// 
    /// Possible value for UsdTextColumnStyle::GetColumnAlignmentAttr(), Possible value for UsdTextParagraphStyle::GetParagraphAlignmentAttr()
    const TfToken center;
    /// \brief "centerTab"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetTabStopTypesAttr()
    const TfToken centerTab;
    /// \brief "columnAlignment"
    /// 
    /// UsdTextColumnStyle
    const TfToken columnAlignment;
    /// \brief "columnHeight"
    /// 
    /// UsdTextColumnStyle
    const TfToken columnHeight;
    /// \brief "columnOffset"
    /// 
    /// UsdTextColumnStyle
    const TfToken columnOffset;
    /// \brief "columnStyle:binding"
    /// 
    ///  The relationship name to denote a binding to a UsdTextColumnStyle. 
    const TfToken columnStyleBinding;
    /// \brief "columnWidth"
    /// 
    /// UsdTextColumnStyle
    const TfToken columnWidth;
    /// \brief "decimalTab"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetTabStopTypesAttr()
    const TfToken decimalTab;
    /// \brief "distributed"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetParagraphAlignmentAttr()
    const TfToken distributed;
    /// \brief "exactly"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetLineSpaceTypeAttr()
    const TfToken exactly;
    /// \brief "firstLineIndent"
    /// 
    /// UsdTextParagraphStyle
    const TfToken firstLineIndent;
    /// \brief "justify"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetParagraphAlignmentAttr()
    const TfToken justify;
    /// \brief "layout:baselineDirection"
    /// 
    /// UsdTextTextLayoutAPI, UsdTextColumnStyle
    const TfToken layoutBaselineDirection;
    /// \brief "layout:linesStackDirection"
    /// 
    /// UsdTextTextLayoutAPI, UsdTextColumnStyle
    const TfToken layoutLinesStackDirection;
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
    /// Possible value for UsdTextTextLayoutAPI::GetLayoutBaselineDirectionAttr(), Possible value for UsdTextTextLayoutAPI::GetLayoutLinesStackDirectionAttr()
    const TfToken leftToRight;
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
    /// \brief "markup"
    /// 
    /// UsdTextMarkupText
    const TfToken markup;
    /// \brief "markupLanguage"
    /// 
    /// UsdTextMarkupText
    const TfToken markupLanguage;
    /// \brief "markup:plain"
    /// 
    /// UsdTextMarkupText
    const TfToken markupPlain;
    /// \brief "mtext"
    /// 
    /// Possible value for UsdTextMarkupText::GetMarkupLanguageAttr()
    const TfToken mtext;
    /// \brief "multiple"
    /// 
    /// Possible value for UsdTextParagraphStyle::GetLineSpaceTypeAttr()
    const TfToken multiple;
    /// \brief "paragraphAlignment"
    /// 
    /// UsdTextParagraphStyle
    const TfToken paragraphAlignment;
    /// \brief "paragraphSpace"
    /// 
    /// UsdTextParagraphStyle
    const TfToken paragraphSpace;
    /// \brief "paragraphStyle:binding"
    /// 
    ///  The relationship name to denote a binding to a UsdTextParagraphStyle. 
    const TfToken paragraphStyleBinding;
    /// \brief "pixel"
    /// 
    /// Possible value for UsdTextMarkupText::GetTextMetricsUnitAttr()
    const TfToken pixel;
    /// \brief "plain"
    /// 
    /// Fallback value for UsdTextMarkupText::GetMarkupLanguageAttr()
    const TfToken plain;
    /// \brief "primvars:backgroundColor"
    /// 
    /// UsdTextMarkupText
    const TfToken primvarsBackgroundColor;
    /// \brief "primvars:backgroundOpacity"
    /// 
    /// UsdTextMarkupText
    const TfToken primvarsBackgroundOpacity;
    /// \brief "publishingPoint"
    /// 
    /// Possible value for UsdTextMarkupText::GetTextMetricsUnitAttr()
    const TfToken publishingPoint;
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
    /// Possible value for UsdTextTextLayoutAPI::GetLayoutBaselineDirectionAttr(), Possible value for UsdTextTextLayoutAPI::GetLayoutLinesStackDirectionAttr()
    const TfToken rightToLeft;
    /// \brief "tabStopPositions"
    /// 
    /// UsdTextParagraphStyle
    const TfToken tabStopPositions;
    /// \brief "tabStopTypes"
    /// 
    /// UsdTextParagraphStyle
    const TfToken tabStopTypes;
    /// \brief "textMetricsUnit"
    /// 
    /// UsdTextMarkupText
    const TfToken textMetricsUnit;
    /// \brief "top"
    /// 
    /// Fallback value for UsdTextColumnStyle::GetColumnAlignmentAttr()
    const TfToken top;
    /// \brief "topToBottom"
    /// 
    /// Possible value for UsdTextTextLayoutAPI::GetLayoutBaselineDirectionAttr(), Possible value for UsdTextTextLayoutAPI::GetLayoutLinesStackDirectionAttr()
    const TfToken topToBottom;
    /// \brief "upToImpl"
    /// 
    /// Fallback value for UsdTextTextLayoutAPI::GetLayoutBaselineDirectionAttr(), Fallback value for UsdTextTextLayoutAPI::GetLayoutLinesStackDirectionAttr(), Fallback value for UsdTextColumnStyle schema attribute layout:baselineDirection, Fallback value for UsdTextColumnStyle schema attribute layout:linesStackDirection
    const TfToken upToImpl;
    /// \brief "worldUnit"
    /// 
    /// Fallback value for UsdTextMarkupText::GetTextMetricsUnitAttr()
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
    /// \brief "TextLayoutAPI"
    /// 
    /// Schema identifer and family for UsdTextTextLayoutAPI
    const TfToken TextLayoutAPI;
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
