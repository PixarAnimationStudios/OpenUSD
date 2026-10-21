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
///     gprim.GetMyTokenValuedAttr().Set(UsdTextTokens->bdf);
/// \endcode
struct UsdTextTokensType {
    USDTEXT_API UsdTextTokensType();
    /// \brief "bdf"
    /// 
    /// Possible value for UsdTextTextStyle::GetFontAltFormatAttr(), Possible value for UsdTextTextStyle::GetFontFormatAttr()
    const TfToken bdf;
    /// \brief "bottomToTop"
    /// 
    /// Possible value for UsdTextTextLayoutAPI::GetLayoutBaselineDirectionAttr(), Possible value for UsdTextTextLayoutAPI::GetLayoutLinesStackDirectionAttr()
    const TfToken bottomToTop;
    /// \brief "charHeight"
    /// 
    /// UsdTextTextStyle
    const TfToken charHeight;
    /// \brief "charSpacingFactor"
    /// 
    /// UsdTextTextStyle
    const TfToken charSpacingFactor;
    /// \brief "charWidthFactor"
    /// 
    /// UsdTextTextStyle
    const TfToken charWidthFactor;
    /// \brief "doubleLines"
    /// 
    /// Possible value for UsdTextTextStyle::GetStrikethroughTypeAttr()
    const TfToken doubleLines;
    /// \brief "fon"
    /// 
    /// Possible value for UsdTextTextStyle::GetFontAltFormatAttr(), Possible value for UsdTextTextStyle::GetFontFormatAttr()
    const TfToken fon;
    /// \brief "font:altFormat"
    /// 
    /// UsdTextTextStyle
    const TfToken fontAltFormat;
    /// \brief "font:altTypeface"
    /// 
    /// UsdTextTextStyle
    const TfToken fontAltTypeface;
    /// \brief "font:bold"
    /// 
    /// UsdTextTextStyle
    const TfToken fontBold;
    /// \brief "font:format"
    /// 
    /// UsdTextTextStyle
    const TfToken fontFormat;
    /// \brief "font:italic"
    /// 
    /// UsdTextTextStyle
    const TfToken fontItalic;
    /// \brief "font:typeface"
    /// 
    /// UsdTextTextStyle
    const TfToken fontTypeface;
    /// \brief "font:weight"
    /// 
    /// UsdTextTextStyle
    const TfToken fontWeight;
    /// \brief "layout:baselineDirection"
    /// 
    /// UsdTextTextLayoutAPI
    const TfToken layoutBaselineDirection;
    /// \brief "layout:linesStackDirection"
    /// 
    /// UsdTextTextLayoutAPI
    const TfToken layoutLinesStackDirection;
    /// \brief "leftToRight"
    /// 
    /// Possible value for UsdTextTextLayoutAPI::GetLayoutBaselineDirectionAttr(), Possible value for UsdTextTextLayoutAPI::GetLayoutLinesStackDirectionAttr()
    const TfToken leftToRight;
    /// \brief "none"
    /// 
    /// Possible value for UsdTextTextStyle::GetFontAltFormatAttr(), Possible value for UsdTextTextStyle::GetFontFormatAttr(), Possible value for UsdTextTextStyle::GetOverlineTypeAttr(), Possible value for UsdTextTextStyle::GetStrikethroughTypeAttr(), Possible value for UsdTextTextStyle::GetUnderlineTypeAttr()
    const TfToken none;
    /// \brief "normal"
    /// 
    /// Possible value for UsdTextTextStyle::GetOverlineTypeAttr(), Possible value for UsdTextTextStyle::GetStrikethroughTypeAttr(), Possible value for UsdTextTextStyle::GetUnderlineTypeAttr()
    const TfToken normal;
    /// \brief "obliqueAngle"
    /// 
    /// UsdTextTextStyle
    const TfToken obliqueAngle;
    /// \brief "overlineType"
    /// 
    /// UsdTextTextStyle
    const TfToken overlineType;
    /// \brief "pcf"
    /// 
    /// Possible value for UsdTextTextStyle::GetFontAltFormatAttr(), Possible value for UsdTextTextStyle::GetFontFormatAttr()
    const TfToken pcf;
    /// \brief "pfa/pfb"
    /// 
    /// Possible value for UsdTextTextStyle::GetFontAltFormatAttr(), Possible value for UsdTextTextStyle::GetFontFormatAttr()
    const TfToken pfa_pfb;
    /// \brief "pixel"
    /// 
    /// Possible value for UsdTextSimpleText::GetTextMetricsUnitAttr()
    const TfToken pixel;
    /// \brief "primvars:backgroundColor"
    /// 
    /// UsdTextSimpleText
    const TfToken primvarsBackgroundColor;
    /// \brief "primvars:backgroundOpacity"
    /// 
    /// UsdTextSimpleText
    const TfToken primvarsBackgroundOpacity;
    /// \brief "publishingPoint"
    /// 
    /// Possible value for UsdTextSimpleText::GetTextMetricsUnitAttr()
    const TfToken publishingPoint;
    /// \brief "rightToLeft"
    /// 
    /// Possible value for UsdTextTextLayoutAPI::GetLayoutBaselineDirectionAttr(), Possible value for UsdTextTextLayoutAPI::GetLayoutLinesStackDirectionAttr()
    const TfToken rightToLeft;
    /// \brief "shx"
    /// 
    /// Possible value for UsdTextTextStyle::GetFontAltFormatAttr(), Possible value for UsdTextTextStyle::GetFontFormatAttr()
    const TfToken shx;
    /// \brief "strikethroughType"
    /// 
    /// UsdTextTextStyle
    const TfToken strikethroughType;
    /// \brief "textData"
    /// 
    /// UsdTextSimpleText
    const TfToken textData;
    /// \brief "textMetricsUnit"
    /// 
    /// UsdTextSimpleText
    const TfToken textMetricsUnit;
    /// \brief "textStyle:binding"
    /// 
    ///  The relationship name to denote a binding to a UsdTextTextStyle. 
    const TfToken textStyleBinding;
    /// \brief "topToBottom"
    /// 
    /// Possible value for UsdTextTextLayoutAPI::GetLayoutBaselineDirectionAttr(), Possible value for UsdTextTextLayoutAPI::GetLayoutLinesStackDirectionAttr()
    const TfToken topToBottom;
    /// \brief "ttf/cff/otf"
    /// 
    /// Possible value for UsdTextTextStyle::GetFontAltFormatAttr(), Possible value for UsdTextTextStyle::GetFontFormatAttr()
    const TfToken ttf_cff_otf;
    /// \brief "underlineType"
    /// 
    /// UsdTextTextStyle
    const TfToken underlineType;
    /// \brief "upToImpl"
    /// 
    /// Fallback value for UsdTextTextLayoutAPI::GetLayoutBaselineDirectionAttr(), Fallback value for UsdTextTextLayoutAPI::GetLayoutLinesStackDirectionAttr()
    const TfToken upToImpl;
    /// \brief "worldUnit"
    /// 
    /// Fallback value for UsdTextSimpleText::GetTextMetricsUnitAttr()
    const TfToken worldUnit;
    /// \brief "SimpleText"
    /// 
    /// Schema identifer and family for UsdTextSimpleText
    const TfToken SimpleText;
    /// \brief "TextLayoutAPI"
    /// 
    /// Schema identifer and family for UsdTextTextLayoutAPI
    const TfToken TextLayoutAPI;
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
