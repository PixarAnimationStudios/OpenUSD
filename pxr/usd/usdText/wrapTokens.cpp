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
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usdText/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Helper to return a static token as a string.  We wrap tokens as Python
// strings and for some reason simply wrapping the token using def_readonly
// bypasses to-Python conversion, leading to the error that there's no
// Python type for the C++ TfToken type.  So we wrap this functor instead.
class _WrapStaticToken {
public:
    _WrapStaticToken(const TfToken* token) : _token(token) { }

    std::string operator()() const
    {
        return _token->GetString();
    }

private:
    const TfToken* _token;
};

template <typename T>
void
_AddToken(T& cls, const char* name, const TfToken& token)
{
    cls.add_static_property(name,
                            boost::python::make_function(
                                _WrapStaticToken(&token),
                                boost::python::return_value_policy<
                                    boost::python::return_by_value>(),
                                boost::mpl::vector1<std::string>()));
}

} // anonymous

void wrapUsdTextTokens()
{
    boost::python::class_<UsdTextTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "atLeast", UsdTextTokens->atLeast);
    _AddToken(cls, "blockAlignment", UsdTextTokens->blockAlignment);
    _AddToken(cls, "bold", UsdTextTokens->bold);
    _AddToken(cls, "bottom", UsdTextTokens->bottom);
    _AddToken(cls, "bottomToTop", UsdTextTokens->bottomToTop);
    _AddToken(cls, "center", UsdTextTokens->center);
    _AddToken(cls, "centerTab", UsdTextTokens->centerTab);
    _AddToken(cls, "charSpacing", UsdTextTokens->charSpacing);
    _AddToken(cls, "columnHeight", UsdTextTokens->columnHeight);
    _AddToken(cls, "columnStyle", UsdTextTokens->columnStyle);
    _AddToken(cls, "columnStyleBinding", UsdTextTokens->columnStyleBinding);
    _AddToken(cls, "columnWidth", UsdTextTokens->columnWidth);
    _AddToken(cls, "decimalTab", UsdTextTokens->decimalTab);
    _AddToken(cls, "defaultDir", UsdTextTokens->defaultDir);
    _AddToken(cls, "direction", UsdTextTokens->direction);
    _AddToken(cls, "distributed", UsdTextTokens->distributed);
    _AddToken(cls, "doubleLines", UsdTextTokens->doubleLines);
    _AddToken(cls, "exactly", UsdTextTokens->exactly);
    _AddToken(cls, "firstLineIndent", UsdTextTokens->firstLineIndent);
    _AddToken(cls, "italic", UsdTextTokens->italic);
    _AddToken(cls, "justify", UsdTextTokens->justify);
    _AddToken(cls, "left", UsdTextTokens->left);
    _AddToken(cls, "leftIndent", UsdTextTokens->leftIndent);
    _AddToken(cls, "leftTab", UsdTextTokens->leftTab);
    _AddToken(cls, "leftToRight", UsdTextTokens->leftToRight);
    _AddToken(cls, "linesFlowDirection", UsdTextTokens->linesFlowDirection);
    _AddToken(cls, "lineSpace", UsdTextTokens->lineSpace);
    _AddToken(cls, "lineSpaceType", UsdTextTokens->lineSpaceType);
    _AddToken(cls, "margins", UsdTextTokens->margins);
    _AddToken(cls, "markupLanguage", UsdTextTokens->markupLanguage);
    _AddToken(cls, "markupString", UsdTextTokens->markupString);
    _AddToken(cls, "mtext", UsdTextTokens->mtext);
    _AddToken(cls, "multiple", UsdTextTokens->multiple);
    _AddToken(cls, "noMarkup", UsdTextTokens->noMarkup);
    _AddToken(cls, "none", UsdTextTokens->none);
    _AddToken(cls, "normal", UsdTextTokens->normal);
    _AddToken(cls, "obliqueAngle", UsdTextTokens->obliqueAngle);
    _AddToken(cls, "offset", UsdTextTokens->offset);
    _AddToken(cls, "overlineType", UsdTextTokens->overlineType);
    _AddToken(cls, "paragraphAlignment", UsdTextTokens->paragraphAlignment);
    _AddToken(cls, "paragraphSpace", UsdTextTokens->paragraphSpace);
    _AddToken(cls, "paragraphStyle", UsdTextTokens->paragraphStyle);
    _AddToken(cls, "paragraphStyleBinding", UsdTextTokens->paragraphStyleBinding);
    _AddToken(cls, "pixel", UsdTextTokens->pixel);
    _AddToken(cls, "primvarsBackgroundColor", UsdTextTokens->primvarsBackgroundColor);
    _AddToken(cls, "primvarsBackgroundOpacity", UsdTextTokens->primvarsBackgroundOpacity);
    _AddToken(cls, "primvarsTextMetricsUnit", UsdTextTokens->primvarsTextMetricsUnit);
    _AddToken(cls, "publishingPoint", UsdTextTokens->publishingPoint);
    _AddToken(cls, "renderer", UsdTextTokens->renderer);
    _AddToken(cls, "right", UsdTextTokens->right);
    _AddToken(cls, "rightIndent", UsdTextTokens->rightIndent);
    _AddToken(cls, "rightTab", UsdTextTokens->rightTab);
    _AddToken(cls, "rightToLeft", UsdTextTokens->rightToLeft);
    _AddToken(cls, "strikethroughType", UsdTextTokens->strikethroughType);
    _AddToken(cls, "tabStopPositions", UsdTextTokens->tabStopPositions);
    _AddToken(cls, "tabStopTypes", UsdTextTokens->tabStopTypes);
    _AddToken(cls, "textData", UsdTextTokens->textData);
    _AddToken(cls, "textHeight", UsdTextTokens->textHeight);
    _AddToken(cls, "textStyle", UsdTextTokens->textStyle);
    _AddToken(cls, "textStyleBinding", UsdTextTokens->textStyleBinding);
    _AddToken(cls, "textWidthFactor", UsdTextTokens->textWidthFactor);
    _AddToken(cls, "top", UsdTextTokens->top);
    _AddToken(cls, "topToBottom", UsdTextTokens->topToBottom);
    _AddToken(cls, "typeface", UsdTextTokens->typeface);
    _AddToken(cls, "underlineType", UsdTextTokens->underlineType);
    _AddToken(cls, "weight", UsdTextTokens->weight);
    _AddToken(cls, "worldUnit", UsdTextTokens->worldUnit);
    _AddToken(cls, "ColumnStyle", UsdTextTokens->ColumnStyle);
    _AddToken(cls, "ColumnStyleAPI", UsdTextTokens->ColumnStyleAPI);
    _AddToken(cls, "MarkupText", UsdTextTokens->MarkupText);
    _AddToken(cls, "ParagraphStyle", UsdTextTokens->ParagraphStyle);
    _AddToken(cls, "ParagraphStyleAPI", UsdTextTokens->ParagraphStyleAPI);
    _AddToken(cls, "SimpleText", UsdTextTokens->SimpleText);
    _AddToken(cls, "TextLayout", UsdTextTokens->TextLayout);
    _AddToken(cls, "TextStyle", UsdTextTokens->TextStyle);
    _AddToken(cls, "TextStyleAPI", UsdTextTokens->TextStyleAPI);
}
