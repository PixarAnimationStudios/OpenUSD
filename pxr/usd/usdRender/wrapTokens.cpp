//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/external/boost/python/class.hpp"
#include "pxr/usd/usdRender/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdRenderTokens->name.GetString(); });

void wrapUsdRenderTokens()
{
    pxr_boost::python::class_<UsdRenderTokensType, pxr_boost::python::noncopyable>
        cls("Tokens", pxr_boost::python::no_init);
    _ADD_TOKEN(cls, adjustApertureHeight);
    _ADD_TOKEN(cls, adjustApertureWidth);
    _ADD_TOKEN(cls, adjustPixelAspectRatio);
    _ADD_TOKEN(cls, aspectRatioConformPolicy);
    _ADD_TOKEN(cls, camera);
    _ADD_TOKEN(cls, collectionRenderVisibilityIncludeRoot);
    _ADD_TOKEN(cls, color3f);
    _ADD_TOKEN(cls, command);
    _ADD_TOKEN(cls, cropAperture);
    _ADD_TOKEN(cls, dataType);
    _ADD_TOKEN(cls, dataWindowNDC);
    _ADD_TOKEN(cls, deepRaster);
    _ADD_TOKEN(cls, disableDepthOfField);
    _ADD_TOKEN(cls, disableMotionBlur);
    _ADD_TOKEN(cls, expandAperture);
    _ADD_TOKEN(cls, fileName);
    _ADD_TOKEN(cls, full);
    _ADD_TOKEN(cls, includedPurposes);
    _ADD_TOKEN(cls, inputPasses);
    _ADD_TOKEN(cls, instantaneousShutter);
    _ADD_TOKEN(cls, intrinsic);
    _ADD_TOKEN(cls, lpe);
    _ADD_TOKEN(cls, materialBindingPurposes);
    _ADD_TOKEN(cls, orderedVars);
    _ADD_TOKEN(cls, passType);
    _ADD_TOKEN(cls, pixelAspectRatio);
    _ADD_TOKEN(cls, preview);
    _ADD_TOKEN(cls, primvar);
    _ADD_TOKEN(cls, productName);
    _ADD_TOKEN(cls, products);
    _ADD_TOKEN(cls, productType);
    _ADD_TOKEN(cls, raster);
    _ADD_TOKEN(cls, raw);
    _ADD_TOKEN(cls, renderingColorSpace);
    _ADD_TOKEN(cls, renderSettingsPrimPath);
    _ADD_TOKEN(cls, renderSource);
    _ADD_TOKEN(cls, renderVisibility);
    _ADD_TOKEN(cls, resolution);
    _ADD_TOKEN(cls, sourceName);
    _ADD_TOKEN(cls, sourceType);
    _ADD_TOKEN(cls, RenderPass);
    _ADD_TOKEN(cls, RenderProduct);
    _ADD_TOKEN(cls, RenderSettings);
    _ADD_TOKEN(cls, RenderSettingsBase);
    _ADD_TOKEN(cls, RenderVar);
}
