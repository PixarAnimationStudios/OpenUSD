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
#include "pxr/usd/usdShade/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdShadeWrapTokens {

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

void wrapUsdShadeTokens()
{
    boost::python::class_<UsdShadeTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "allPurpose", UsdShadeTokens->allPurpose);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "bindMaterialAs", UsdShadeTokens->bindMaterialAs);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "coordSys", UsdShadeTokens->coordSys);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "displacement", UsdShadeTokens->displacement);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "fallbackStrength", UsdShadeTokens->fallbackStrength);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "full", UsdShadeTokens->full);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "id", UsdShadeTokens->id);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "infoId", UsdShadeTokens->infoId);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "infoImplementationSource", UsdShadeTokens->infoImplementationSource);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "inputs", UsdShadeTokens->inputs);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "interfaceOnly", UsdShadeTokens->interfaceOnly);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "materialBind", UsdShadeTokens->materialBind);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "materialBinding", UsdShadeTokens->materialBinding);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "materialBindingCollection", UsdShadeTokens->materialBindingCollection);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "materialVariant", UsdShadeTokens->materialVariant);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "outputs", UsdShadeTokens->outputs);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "outputsDisplacement", UsdShadeTokens->outputsDisplacement);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "outputsSurface", UsdShadeTokens->outputsSurface);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "outputsVolume", UsdShadeTokens->outputsVolume);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "preview", UsdShadeTokens->preview);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "sdrMetadata", UsdShadeTokens->sdrMetadata);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "sourceAsset", UsdShadeTokens->sourceAsset);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "sourceCode", UsdShadeTokens->sourceCode);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "strongerThanDescendants", UsdShadeTokens->strongerThanDescendants);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "subIdentifier", UsdShadeTokens->subIdentifier);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "surface", UsdShadeTokens->surface);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "universalRenderContext", UsdShadeTokens->universalRenderContext);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "universalSourceType", UsdShadeTokens->universalSourceType);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "volume", UsdShadeTokens->volume);
    pxrUsdUsdShadeWrapTokens::_AddToken(cls, "weakerThanDescendants", UsdShadeTokens->weakerThanDescendants);
}
