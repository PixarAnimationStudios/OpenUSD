//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "./tokens.h"

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

void wrapUsdSchemaExamplesTokens()
{
    boost::python::class_<UsdSchemaExamplesTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "complexString", UsdSchemaExamplesTokens->complexString);
    _AddToken(cls, "intAttr", UsdSchemaExamplesTokens->intAttr);
    _AddToken(cls, "paramsMass", UsdSchemaExamplesTokens->paramsMass);
    _AddToken(cls, "paramsVelocity", UsdSchemaExamplesTokens->paramsVelocity);
    _AddToken(cls, "paramsVolume", UsdSchemaExamplesTokens->paramsVolume);
    _AddToken(cls, "target", UsdSchemaExamplesTokens->target);
    _AddToken(cls, "ComplexPrim", UsdSchemaExamplesTokens->ComplexPrim);
    _AddToken(cls, "ParamsAPI", UsdSchemaExamplesTokens->ParamsAPI);
    _AddToken(cls, "SimplePrim", UsdSchemaExamplesTokens->SimplePrim);
}
