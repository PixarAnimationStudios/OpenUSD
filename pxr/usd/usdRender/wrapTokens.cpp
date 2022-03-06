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
#include "pxr/usd/usdRender/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdRenderWrapTokens {

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

void wrapUsdRenderTokens()
{
    boost::python::class_<UsdRenderTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "adjustApertureHeight", UsdRenderTokens->adjustApertureHeight);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "adjustApertureWidth", UsdRenderTokens->adjustApertureWidth);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "adjustPixelAspectRatio", UsdRenderTokens->adjustPixelAspectRatio);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "aspectRatioConformPolicy", UsdRenderTokens->aspectRatioConformPolicy);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "camera", UsdRenderTokens->camera);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "color3f", UsdRenderTokens->color3f);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "cropAperture", UsdRenderTokens->cropAperture);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "dataType", UsdRenderTokens->dataType);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "dataWindowNDC", UsdRenderTokens->dataWindowNDC);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "expandAperture", UsdRenderTokens->expandAperture);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "full", UsdRenderTokens->full);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "includedPurposes", UsdRenderTokens->includedPurposes);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "instantaneousShutter", UsdRenderTokens->instantaneousShutter);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "intrinsic", UsdRenderTokens->intrinsic);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "lpe", UsdRenderTokens->lpe);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "materialBindingPurposes", UsdRenderTokens->materialBindingPurposes);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "orderedVars", UsdRenderTokens->orderedVars);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "pixelAspectRatio", UsdRenderTokens->pixelAspectRatio);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "preview", UsdRenderTokens->preview);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "primvar", UsdRenderTokens->primvar);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "productName", UsdRenderTokens->productName);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "products", UsdRenderTokens->products);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "productType", UsdRenderTokens->productType);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "raster", UsdRenderTokens->raster);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "raw", UsdRenderTokens->raw);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "renderSettingsPrimPath", UsdRenderTokens->renderSettingsPrimPath);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "resolution", UsdRenderTokens->resolution);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "sourceName", UsdRenderTokens->sourceName);
    pxrUsdUsdRenderWrapTokens::_AddToken(cls, "sourceType", UsdRenderTokens->sourceType);
}
