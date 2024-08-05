//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usdUI/tokens.h"

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

void wrapUsdUITokens()
{
    boost::python::class_<UsdUITokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "closed", UsdUITokens->closed);
    _AddToken(cls, "minimized", UsdUITokens->minimized);
    _AddToken(cls, "open", UsdUITokens->open);
    _AddToken(cls, "uiDescription", UsdUITokens->uiDescription);
    _AddToken(cls, "uiDisplayGroup", UsdUITokens->uiDisplayGroup);
    _AddToken(cls, "uiDisplayName", UsdUITokens->uiDisplayName);
    _AddToken(cls, "uiNodegraphNodeDisplayColor", UsdUITokens->uiNodegraphNodeDisplayColor);
    _AddToken(cls, "uiNodegraphNodeDocURI", UsdUITokens->uiNodegraphNodeDocURI);
    _AddToken(cls, "uiNodegraphNodeExpansionState", UsdUITokens->uiNodegraphNodeExpansionState);
    _AddToken(cls, "uiNodegraphNodeIcon", UsdUITokens->uiNodegraphNodeIcon);
    _AddToken(cls, "uiNodegraphNodePos", UsdUITokens->uiNodegraphNodePos);
    _AddToken(cls, "uiNodegraphNodeSize", UsdUITokens->uiNodegraphNodeSize);
    _AddToken(cls, "uiNodegraphNodeStackingOrder", UsdUITokens->uiNodegraphNodeStackingOrder);
    _AddToken(cls, "Backdrop", UsdUITokens->Backdrop);
    _AddToken(cls, "NodeGraphNodeAPI", UsdUITokens->NodeGraphNodeAPI);
    _AddToken(cls, "SceneGraphPrimAPI", UsdUITokens->SceneGraphPrimAPI);
}
