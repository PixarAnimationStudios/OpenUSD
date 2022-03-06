//
// Copyright 2016 Pixar
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
#include "pxr/usd/usdHydra/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdHydraWrapTokens {

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
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "black", UsdHydraTokens->black);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "clamp", UsdHydraTokens->clamp);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "displayLookBxdf", UsdHydraTokens->displayLookBxdf);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "faceIndex", UsdHydraTokens->faceIndex);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "faceOffset", UsdHydraTokens->faceOffset);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "frame", UsdHydraTokens->frame);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "HwPrimvar_1", UsdHydraTokens->HwPrimvar_1);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "HwPtexTexture_1", UsdHydraTokens->HwPtexTexture_1);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "HwUvTexture_1", UsdHydraTokens->HwUvTexture_1);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "infoFilename", UsdHydraTokens->infoFilename);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "infoVarname", UsdHydraTokens->infoVarname);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "linear", UsdHydraTokens->linear);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "linearMipmapLinear", UsdHydraTokens->linearMipmapLinear);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "linearMipmapNearest", UsdHydraTokens->linearMipmapNearest);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "magFilter", UsdHydraTokens->magFilter);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "minFilter", UsdHydraTokens->minFilter);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "mirror", UsdHydraTokens->mirror);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "nearest", UsdHydraTokens->nearest);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "nearestMipmapLinear", UsdHydraTokens->nearestMipmapLinear);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "nearestMipmapNearest", UsdHydraTokens->nearestMipmapNearest);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "repeat", UsdHydraTokens->repeat);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "textureMemory", UsdHydraTokens->textureMemory);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "uv", UsdHydraTokens->uv);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "wrapS", UsdHydraTokens->wrapS);
    pxrUsdUsdHydraWrapTokens::_AddToken(cls, "wrapT", UsdHydraTokens->wrapT);
}
