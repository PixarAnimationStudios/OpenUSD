//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usdRender/tokens.h"

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

void wrapUsdRenderTokens()
{
    boost::python::class_<UsdRenderTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "adjustApertureHeight", UsdRenderTokens->adjustApertureHeight);
    _AddToken(cls, "adjustApertureWidth", UsdRenderTokens->adjustApertureWidth);
    _AddToken(cls, "adjustPixelAspectRatio", UsdRenderTokens->adjustPixelAspectRatio);
    _AddToken(cls, "aspectRatioConformPolicy", UsdRenderTokens->aspectRatioConformPolicy);
    _AddToken(cls, "camera", UsdRenderTokens->camera);
    _AddToken(cls, "collectionRenderVisibilityIncludeRoot", UsdRenderTokens->collectionRenderVisibilityIncludeRoot);
    _AddToken(cls, "color3f", UsdRenderTokens->color3f);
    _AddToken(cls, "command", UsdRenderTokens->command);
    _AddToken(cls, "cropAperture", UsdRenderTokens->cropAperture);
    _AddToken(cls, "dataType", UsdRenderTokens->dataType);
    _AddToken(cls, "dataWindowNDC", UsdRenderTokens->dataWindowNDC);
    _AddToken(cls, "deepRaster", UsdRenderTokens->deepRaster);
    _AddToken(cls, "denoiseEnable", UsdRenderTokens->denoiseEnable);
    _AddToken(cls, "denoisePass", UsdRenderTokens->denoisePass);
    _AddToken(cls, "disableDepthOfField", UsdRenderTokens->disableDepthOfField);
    _AddToken(cls, "disableMotionBlur", UsdRenderTokens->disableMotionBlur);
    _AddToken(cls, "expandAperture", UsdRenderTokens->expandAperture);
    _AddToken(cls, "fileName", UsdRenderTokens->fileName);
    _AddToken(cls, "full", UsdRenderTokens->full);
    _AddToken(cls, "includedPurposes", UsdRenderTokens->includedPurposes);
    _AddToken(cls, "inputPasses", UsdRenderTokens->inputPasses);
    _AddToken(cls, "instantaneousShutter", UsdRenderTokens->instantaneousShutter);
    _AddToken(cls, "intrinsic", UsdRenderTokens->intrinsic);
    _AddToken(cls, "lpe", UsdRenderTokens->lpe);
    _AddToken(cls, "materialBindingPurposes", UsdRenderTokens->materialBindingPurposes);
    _AddToken(cls, "orderedVars", UsdRenderTokens->orderedVars);
    _AddToken(cls, "passType", UsdRenderTokens->passType);
    _AddToken(cls, "pixelAspectRatio", UsdRenderTokens->pixelAspectRatio);
    _AddToken(cls, "preview", UsdRenderTokens->preview);
    _AddToken(cls, "primvar", UsdRenderTokens->primvar);
    _AddToken(cls, "productName", UsdRenderTokens->productName);
    _AddToken(cls, "products", UsdRenderTokens->products);
    _AddToken(cls, "productType", UsdRenderTokens->productType);
    _AddToken(cls, "raster", UsdRenderTokens->raster);
    _AddToken(cls, "raw", UsdRenderTokens->raw);
    _AddToken(cls, "renderingColorSpace", UsdRenderTokens->renderingColorSpace);
    _AddToken(cls, "renderSettingsPrimPath", UsdRenderTokens->renderSettingsPrimPath);
    _AddToken(cls, "renderSource", UsdRenderTokens->renderSource);
    _AddToken(cls, "renderVisibility", UsdRenderTokens->renderVisibility);
    _AddToken(cls, "resolution", UsdRenderTokens->resolution);
    _AddToken(cls, "sourceName", UsdRenderTokens->sourceName);
    _AddToken(cls, "sourceType", UsdRenderTokens->sourceType);
    _AddToken(cls, "RenderDenoisePass", UsdRenderTokens->RenderDenoisePass);
    _AddToken(cls, "RenderPass", UsdRenderTokens->RenderPass);
    _AddToken(cls, "RenderProduct", UsdRenderTokens->RenderProduct);
    _AddToken(cls, "RenderSettings", UsdRenderTokens->RenderSettings);
    _AddToken(cls, "RenderSettingsBase", UsdRenderTokens->RenderSettingsBase);
    _AddToken(cls, "RenderVar", UsdRenderTokens->RenderVar);
}
