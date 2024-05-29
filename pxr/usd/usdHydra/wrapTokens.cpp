//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usdHydra/tokens.h"

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

void wrapUsdHydraTokens()
{
    boost::python::class_<UsdHydraTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "black", UsdHydraTokens->black);
    _AddToken(cls, "clamp", UsdHydraTokens->clamp);
    _AddToken(cls, "displayLookBxdf", UsdHydraTokens->displayLookBxdf);
    _AddToken(cls, "faceIndex", UsdHydraTokens->faceIndex);
    _AddToken(cls, "faceOffset", UsdHydraTokens->faceOffset);
    _AddToken(cls, "frame", UsdHydraTokens->frame);
    _AddToken(cls, "HwPrimvar_1", UsdHydraTokens->HwPrimvar_1);
    _AddToken(cls, "HwPtexTexture_1", UsdHydraTokens->HwPtexTexture_1);
    _AddToken(cls, "HwUvTexture_1", UsdHydraTokens->HwUvTexture_1);
    _AddToken(cls, "hydraGenerativeProcedural", UsdHydraTokens->hydraGenerativeProcedural);
    _AddToken(cls, "infoFilename", UsdHydraTokens->infoFilename);
    _AddToken(cls, "infoVarname", UsdHydraTokens->infoVarname);
    _AddToken(cls, "linear", UsdHydraTokens->linear);
    _AddToken(cls, "linearMipmapLinear", UsdHydraTokens->linearMipmapLinear);
    _AddToken(cls, "linearMipmapNearest", UsdHydraTokens->linearMipmapNearest);
    _AddToken(cls, "magFilter", UsdHydraTokens->magFilter);
    _AddToken(cls, "minFilter", UsdHydraTokens->minFilter);
    _AddToken(cls, "mirror", UsdHydraTokens->mirror);
    _AddToken(cls, "nearest", UsdHydraTokens->nearest);
    _AddToken(cls, "nearestMipmapLinear", UsdHydraTokens->nearestMipmapLinear);
    _AddToken(cls, "nearestMipmapNearest", UsdHydraTokens->nearestMipmapNearest);
    _AddToken(cls, "primvarsHdGpProceduralType", UsdHydraTokens->primvarsHdGpProceduralType);
    _AddToken(cls, "proceduralSystem", UsdHydraTokens->proceduralSystem);
    _AddToken(cls, "repeat", UsdHydraTokens->repeat);
    _AddToken(cls, "textureMemory", UsdHydraTokens->textureMemory);
    _AddToken(cls, "useMetadata", UsdHydraTokens->useMetadata);
    _AddToken(cls, "uv", UsdHydraTokens->uv);
    _AddToken(cls, "wrapS", UsdHydraTokens->wrapS);
    _AddToken(cls, "wrapT", UsdHydraTokens->wrapT);
    _AddToken(cls, "HydraGenerativeProceduralAPI", UsdHydraTokens->HydraGenerativeProceduralAPI);
}
