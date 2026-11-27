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
    _AddToken(cls, "bdf", UsdTextTokens->bdf);
    _AddToken(cls, "bottomToTop", UsdTextTokens->bottomToTop);
    _AddToken(cls, "charHeight", UsdTextTokens->charHeight);
    _AddToken(cls, "charSpacingFactor", UsdTextTokens->charSpacingFactor);
    _AddToken(cls, "charWidthFactor", UsdTextTokens->charWidthFactor);
    _AddToken(cls, "doubleLines", UsdTextTokens->doubleLines);
    _AddToken(cls, "fon", UsdTextTokens->fon);
    _AddToken(cls, "fontAltFormat", UsdTextTokens->fontAltFormat);
    _AddToken(cls, "fontAltTypeface", UsdTextTokens->fontAltTypeface);
    _AddToken(cls, "fontBold", UsdTextTokens->fontBold);
    _AddToken(cls, "fontFormat", UsdTextTokens->fontFormat);
    _AddToken(cls, "fontItalic", UsdTextTokens->fontItalic);
    _AddToken(cls, "fontTypeface", UsdTextTokens->fontTypeface);
    _AddToken(cls, "fontWeight", UsdTextTokens->fontWeight);
    _AddToken(cls, "layoutBaselineDirection", UsdTextTokens->layoutBaselineDirection);
    _AddToken(cls, "layoutLinesStackDirection", UsdTextTokens->layoutLinesStackDirection);
    _AddToken(cls, "leftToRight", UsdTextTokens->leftToRight);
    _AddToken(cls, "none", UsdTextTokens->none);
    _AddToken(cls, "normal", UsdTextTokens->normal);
    _AddToken(cls, "obliqueAngle", UsdTextTokens->obliqueAngle);
    _AddToken(cls, "overlineType", UsdTextTokens->overlineType);
    _AddToken(cls, "pcf", UsdTextTokens->pcf);
    _AddToken(cls, "pfa_pfb", UsdTextTokens->pfa_pfb);
    _AddToken(cls, "pixel", UsdTextTokens->pixel);
    _AddToken(cls, "primvarsBackgroundColor", UsdTextTokens->primvarsBackgroundColor);
    _AddToken(cls, "primvarsBackgroundOpacity", UsdTextTokens->primvarsBackgroundOpacity);
    _AddToken(cls, "publishingPoint", UsdTextTokens->publishingPoint);
    _AddToken(cls, "rightToLeft", UsdTextTokens->rightToLeft);
    _AddToken(cls, "shx", UsdTextTokens->shx);
    _AddToken(cls, "strikethroughType", UsdTextTokens->strikethroughType);
    _AddToken(cls, "textData", UsdTextTokens->textData);
    _AddToken(cls, "textMetricsUnit", UsdTextTokens->textMetricsUnit);
    _AddToken(cls, "textStyleBinding", UsdTextTokens->textStyleBinding);
    _AddToken(cls, "topToBottom", UsdTextTokens->topToBottom);
    _AddToken(cls, "ttf_cff_otf", UsdTextTokens->ttf_cff_otf);
    _AddToken(cls, "underlineType", UsdTextTokens->underlineType);
    _AddToken(cls, "upToImpl", UsdTextTokens->upToImpl);
    _AddToken(cls, "worldUnit", UsdTextTokens->worldUnit);
    _AddToken(cls, "SimpleText", UsdTextTokens->SimpleText);
    _AddToken(cls, "TextLayoutAPI", UsdTextTokens->TextLayoutAPI);
    _AddToken(cls, "TextStyle", UsdTextTokens->TextStyle);
    _AddToken(cls, "TextStyleAPI", UsdTextTokens->TextStyleAPI);
}
