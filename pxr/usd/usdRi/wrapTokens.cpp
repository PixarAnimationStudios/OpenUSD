//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usdRi/tokens.h"

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

void wrapUsdRiTokens()
{
    boost::python::class_<UsdRiTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "bspline", UsdRiTokens->bspline);
    _AddToken(cls, "cameraVisibility", UsdRiTokens->cameraVisibility);
    _AddToken(cls, "catmullRom", UsdRiTokens->catmullRom);
    _AddToken(cls, "collectionCameraVisibilityIncludeRoot", UsdRiTokens->collectionCameraVisibilityIncludeRoot);
    _AddToken(cls, "constant", UsdRiTokens->constant);
    _AddToken(cls, "interpolation", UsdRiTokens->interpolation);
    _AddToken(cls, "linear", UsdRiTokens->linear);
    _AddToken(cls, "matte", UsdRiTokens->matte);
    _AddToken(cls, "outputsRiDisplacement", UsdRiTokens->outputsRiDisplacement);
    _AddToken(cls, "outputsRiSurface", UsdRiTokens->outputsRiSurface);
    _AddToken(cls, "outputsRiVolume", UsdRiTokens->outputsRiVolume);
    _AddToken(cls, "positions", UsdRiTokens->positions);
    _AddToken(cls, "renderContext", UsdRiTokens->renderContext);
    _AddToken(cls, "spline", UsdRiTokens->spline);
    _AddToken(cls, "values", UsdRiTokens->values);
    _AddToken(cls, "RiMaterialAPI", UsdRiTokens->RiMaterialAPI);
    _AddToken(cls, "RiRenderPassAPI", UsdRiTokens->RiRenderPassAPI);
    _AddToken(cls, "RiSplineAPI", UsdRiTokens->RiSplineAPI);
    _AddToken(cls, "StatementsAPI", UsdRiTokens->StatementsAPI);
}
