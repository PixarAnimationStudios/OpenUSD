//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usd/tokens.h"

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

void wrapUsdTokens()
{
    boost::python::class_<UsdTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "apiSchemas", UsdTokens->apiSchemas);
    _AddToken(cls, "clips", UsdTokens->clips);
    _AddToken(cls, "clipSets", UsdTokens->clipSets);
    _AddToken(cls, "collection", UsdTokens->collection);
    _AddToken(cls, "collection_MultipleApplyTemplate_", UsdTokens->collection_MultipleApplyTemplate_);
    _AddToken(cls, "collection_MultipleApplyTemplate_Excludes", UsdTokens->collection_MultipleApplyTemplate_Excludes);
    _AddToken(cls, "collection_MultipleApplyTemplate_ExpansionRule", UsdTokens->collection_MultipleApplyTemplate_ExpansionRule);
    _AddToken(cls, "collection_MultipleApplyTemplate_IncludeRoot", UsdTokens->collection_MultipleApplyTemplate_IncludeRoot);
    _AddToken(cls, "collection_MultipleApplyTemplate_Includes", UsdTokens->collection_MultipleApplyTemplate_Includes);
    _AddToken(cls, "collection_MultipleApplyTemplate_MembershipExpression", UsdTokens->collection_MultipleApplyTemplate_MembershipExpression);
    _AddToken(cls, "exclude", UsdTokens->exclude);
    _AddToken(cls, "expandPrims", UsdTokens->expandPrims);
    _AddToken(cls, "expandPrimsAndProperties", UsdTokens->expandPrimsAndProperties);
    _AddToken(cls, "explicitOnly", UsdTokens->explicitOnly);
    _AddToken(cls, "fallbackPrimTypes", UsdTokens->fallbackPrimTypes);
    _AddToken(cls, "APISchemaBase", UsdTokens->APISchemaBase);
    _AddToken(cls, "ClipsAPI", UsdTokens->ClipsAPI);
    _AddToken(cls, "CollectionAPI", UsdTokens->CollectionAPI);
    _AddToken(cls, "ModelAPI", UsdTokens->ModelAPI);
    _AddToken(cls, "Typed", UsdTokens->Typed);
}
