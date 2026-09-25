//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usdText/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdTextTokens->name.GetString(); });

void wrapUsdTextTokens()
{
    boost::python::class_<UsdTextTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _ADD_TOKEN(cls, atLeast);
    _ADD_TOKEN(cls, bottom);
    _ADD_TOKEN(cls, bottomToTop);
    _ADD_TOKEN(cls, center);
    _ADD_TOKEN(cls, centerTab);
    _ADD_TOKEN(cls, columnAlignment);
    _ADD_TOKEN(cls, columnHeight);
    _ADD_TOKEN(cls, columnOffset);
    _ADD_TOKEN(cls, columnStyleBinding);
    _ADD_TOKEN(cls, columnWidth);
    _ADD_TOKEN(cls, decimalTab);
    _ADD_TOKEN(cls, distributed);
    _ADD_TOKEN(cls, exactly);
    _ADD_TOKEN(cls, firstLineIndent);
    _ADD_TOKEN(cls, justify);
    _ADD_TOKEN(cls, layoutBaselineDirection);
    _ADD_TOKEN(cls, layoutLinesStackDirection);
    _ADD_TOKEN(cls, left);
    _ADD_TOKEN(cls, leftIndent);
    _ADD_TOKEN(cls, leftTab);
    _ADD_TOKEN(cls, leftToRight);
    _ADD_TOKEN(cls, lineSpace);
    _ADD_TOKEN(cls, lineSpaceType);
    _ADD_TOKEN(cls, margins);
    _ADD_TOKEN(cls, markup);
    _ADD_TOKEN(cls, markupLanguage);
    _ADD_TOKEN(cls, markupPlain);
    _ADD_TOKEN(cls, mtext);
    _ADD_TOKEN(cls, multiple);
    _ADD_TOKEN(cls, paragraphAlignment);
    _ADD_TOKEN(cls, paragraphSpace);
    _ADD_TOKEN(cls, paragraphStyleBinding);
    _ADD_TOKEN(cls, pixel);
    _ADD_TOKEN(cls, plain);
    _ADD_TOKEN(cls, primvarsBackgroundColor);
    _ADD_TOKEN(cls, primvarsBackgroundOpacity);
    _ADD_TOKEN(cls, publishingPoint);
    _ADD_TOKEN(cls, right);
    _ADD_TOKEN(cls, rightIndent);
    _ADD_TOKEN(cls, rightTab);
    _ADD_TOKEN(cls, rightToLeft);
    _ADD_TOKEN(cls, tabStopPositions);
    _ADD_TOKEN(cls, tabStopTypes);
    _ADD_TOKEN(cls, textMetricsUnit);
    _ADD_TOKEN(cls, top);
    _ADD_TOKEN(cls, topToBottom);
    _ADD_TOKEN(cls, upToImpl);
    _ADD_TOKEN(cls, worldUnit);
    _ADD_TOKEN(cls, ColumnStyle);
    _ADD_TOKEN(cls, ColumnStyleAPI);
    _ADD_TOKEN(cls, MarkupText);
    _ADD_TOKEN(cls, ParagraphStyle);
    _ADD_TOKEN(cls, ParagraphStyleAPI);
    _ADD_TOKEN(cls, TextLayoutAPI);
}
