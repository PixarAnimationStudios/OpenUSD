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
    _AddToken(cls, "analytic", UsdRiTokens->analytic);
    _AddToken(cls, "analyticApex", UsdRiTokens->analyticApex);
    _AddToken(cls, "analyticBlurAmount", UsdRiTokens->analyticBlurAmount);
    _AddToken(cls, "analyticBlurExponent", UsdRiTokens->analyticBlurExponent);
    _AddToken(cls, "analyticBlurFarDistance", UsdRiTokens->analyticBlurFarDistance);
    _AddToken(cls, "analyticBlurFarValue", UsdRiTokens->analyticBlurFarValue);
    _AddToken(cls, "analyticBlurMidpoint", UsdRiTokens->analyticBlurMidpoint);
    _AddToken(cls, "analyticBlurMidValue", UsdRiTokens->analyticBlurMidValue);
    _AddToken(cls, "analyticBlurNearDistance", UsdRiTokens->analyticBlurNearDistance);
    _AddToken(cls, "analyticBlurNearValue", UsdRiTokens->analyticBlurNearValue);
    _AddToken(cls, "analyticBlurSMult", UsdRiTokens->analyticBlurSMult);
    _AddToken(cls, "analyticBlurTMult", UsdRiTokens->analyticBlurTMult);
    _AddToken(cls, "analyticDensityExponent", UsdRiTokens->analyticDensityExponent);
    _AddToken(cls, "analyticDensityFarDistance", UsdRiTokens->analyticDensityFarDistance);
    _AddToken(cls, "analyticDensityFarValue", UsdRiTokens->analyticDensityFarValue);
    _AddToken(cls, "analyticDensityMidpoint", UsdRiTokens->analyticDensityMidpoint);
    _AddToken(cls, "analyticDensityMidValue", UsdRiTokens->analyticDensityMidValue);
    _AddToken(cls, "analyticDensityNearDistance", UsdRiTokens->analyticDensityNearDistance);
    _AddToken(cls, "analyticDensityNearValue", UsdRiTokens->analyticDensityNearValue);
    _AddToken(cls, "analyticDirectional", UsdRiTokens->analyticDirectional);
    _AddToken(cls, "analyticShearX", UsdRiTokens->analyticShearX);
    _AddToken(cls, "analyticShearY", UsdRiTokens->analyticShearY);
    _AddToken(cls, "analyticUseLightDirection", UsdRiTokens->analyticUseLightDirection);
    _AddToken(cls, "aovName", UsdRiTokens->aovName);
    _AddToken(cls, "argsPath", UsdRiTokens->argsPath);
    _AddToken(cls, "barnMode", UsdRiTokens->barnMode);
    _AddToken(cls, "bspline", UsdRiTokens->bspline);
    _AddToken(cls, "catmullRom", UsdRiTokens->catmullRom);
    _AddToken(cls, "clamp", UsdRiTokens->clamp);
    _AddToken(cls, "colorContrast", UsdRiTokens->colorContrast);
    _AddToken(cls, "colorMidpoint", UsdRiTokens->colorMidpoint);
    _AddToken(cls, "colorSaturation", UsdRiTokens->colorSaturation);
    _AddToken(cls, "colorTint", UsdRiTokens->colorTint);
    _AddToken(cls, "colorWhitepoint", UsdRiTokens->colorWhitepoint);
    _AddToken(cls, "cone", UsdRiTokens->cone);
    _AddToken(cls, "constant", UsdRiTokens->constant);
    _AddToken(cls, "cookieMode", UsdRiTokens->cookieMode);
    _AddToken(cls, "day", UsdRiTokens->day);
    _AddToken(cls, "depth", UsdRiTokens->depth);
    _AddToken(cls, "distanceToLight", UsdRiTokens->distanceToLight);
    _AddToken(cls, "edgeBack", UsdRiTokens->edgeBack);
    _AddToken(cls, "edgeBottom", UsdRiTokens->edgeBottom);
    _AddToken(cls, "edgeFront", UsdRiTokens->edgeFront);
    _AddToken(cls, "edgeLeft", UsdRiTokens->edgeLeft);
    _AddToken(cls, "edgeRight", UsdRiTokens->edgeRight);
    _AddToken(cls, "edgeThickness", UsdRiTokens->edgeThickness);
    _AddToken(cls, "edgeTop", UsdRiTokens->edgeTop);
    _AddToken(cls, "falloffRampBeginDistance", UsdRiTokens->falloffRampBeginDistance);
    _AddToken(cls, "falloffRampEndDistance", UsdRiTokens->falloffRampEndDistance);
    _AddToken(cls, "filePath", UsdRiTokens->filePath);
    _AddToken(cls, "haziness", UsdRiTokens->haziness);
    _AddToken(cls, "height", UsdRiTokens->height);
    _AddToken(cls, "hour", UsdRiTokens->hour);
    _AddToken(cls, "infoArgsPath", UsdRiTokens->infoArgsPath);
    _AddToken(cls, "infoFilePath", UsdRiTokens->infoFilePath);
    _AddToken(cls, "infoOslPath", UsdRiTokens->infoOslPath);
    _AddToken(cls, "infoSloPath", UsdRiTokens->infoSloPath);
    _AddToken(cls, "inPrimaryHit", UsdRiTokens->inPrimaryHit);
    _AddToken(cls, "inReflection", UsdRiTokens->inReflection);
    _AddToken(cls, "inRefraction", UsdRiTokens->inRefraction);
    _AddToken(cls, "interpolation", UsdRiTokens->interpolation);
    _AddToken(cls, "invert", UsdRiTokens->invert);
    _AddToken(cls, "latitude", UsdRiTokens->latitude);
    _AddToken(cls, "linear", UsdRiTokens->linear);
    _AddToken(cls, "longitude", UsdRiTokens->longitude);
    _AddToken(cls, "max", UsdRiTokens->max);
    _AddToken(cls, "min", UsdRiTokens->min);
    _AddToken(cls, "month", UsdRiTokens->month);
    _AddToken(cls, "multiply", UsdRiTokens->multiply);
    _AddToken(cls, "noEffect", UsdRiTokens->noEffect);
    _AddToken(cls, "noLight", UsdRiTokens->noLight);
    _AddToken(cls, "off", UsdRiTokens->off);
    _AddToken(cls, "onVolumeBoundaries", UsdRiTokens->onVolumeBoundaries);
    _AddToken(cls, "outputsRiBxdf", UsdRiTokens->outputsRiBxdf);
    _AddToken(cls, "outputsRiDisplacement", UsdRiTokens->outputsRiDisplacement);
    _AddToken(cls, "outputsRiSurface", UsdRiTokens->outputsRiSurface);
    _AddToken(cls, "outputsRiVolume", UsdRiTokens->outputsRiVolume);
    _AddToken(cls, "physical", UsdRiTokens->physical);
    _AddToken(cls, "positions", UsdRiTokens->positions);
    _AddToken(cls, "preBarnEffect", UsdRiTokens->preBarnEffect);
    _AddToken(cls, "radial", UsdRiTokens->radial);
    _AddToken(cls, "radius", UsdRiTokens->radius);
    _AddToken(cls, "rampMode", UsdRiTokens->rampMode);
    _AddToken(cls, "refineBack", UsdRiTokens->refineBack);
    _AddToken(cls, "refineBottom", UsdRiTokens->refineBottom);
    _AddToken(cls, "refineFront", UsdRiTokens->refineFront);
    _AddToken(cls, "refineLeft", UsdRiTokens->refineLeft);
    _AddToken(cls, "refineRight", UsdRiTokens->refineRight);
    _AddToken(cls, "refineTop", UsdRiTokens->refineTop);
    _AddToken(cls, "repeat", UsdRiTokens->repeat);
    _AddToken(cls, "riCombineMode", UsdRiTokens->riCombineMode);
    _AddToken(cls, "riDensity", UsdRiTokens->riDensity);
    _AddToken(cls, "riDiffuse", UsdRiTokens->riDiffuse);
    _AddToken(cls, "riExposure", UsdRiTokens->riExposure);
    _AddToken(cls, "riFocusRegion", UsdRiTokens->riFocusRegion);
    _AddToken(cls, "riIntensity", UsdRiTokens->riIntensity);
    _AddToken(cls, "riIntensityNearDist", UsdRiTokens->riIntensityNearDist);
    _AddToken(cls, "riInvert", UsdRiTokens->riInvert);
    _AddToken(cls, "riLightGroup", UsdRiTokens->riLightGroup);
    _AddToken(cls, "riPortalIntensity", UsdRiTokens->riPortalIntensity);
    _AddToken(cls, "riPortalTint", UsdRiTokens->riPortalTint);
    _AddToken(cls, "riSamplingFixedSampleCount", UsdRiTokens->riSamplingFixedSampleCount);
    _AddToken(cls, "riSamplingImportanceMultiplier", UsdRiTokens->riSamplingImportanceMultiplier);
    _AddToken(cls, "riShadowThinShadow", UsdRiTokens->riShadowThinShadow);
    _AddToken(cls, "riSpecular", UsdRiTokens->riSpecular);
    _AddToken(cls, "riTextureGamma", UsdRiTokens->riTextureGamma);
    _AddToken(cls, "riTextureSaturation", UsdRiTokens->riTextureSaturation);
    _AddToken(cls, "riTraceLightPaths", UsdRiTokens->riTraceLightPaths);
    _AddToken(cls, "scaleDepth", UsdRiTokens->scaleDepth);
    _AddToken(cls, "scaleHeight", UsdRiTokens->scaleHeight);
    _AddToken(cls, "scaleWidth", UsdRiTokens->scaleWidth);
    _AddToken(cls, "screen", UsdRiTokens->screen);
    _AddToken(cls, "skyTint", UsdRiTokens->skyTint);
    _AddToken(cls, "spherical", UsdRiTokens->spherical);
    _AddToken(cls, "spline", UsdRiTokens->spline);
    _AddToken(cls, "sunDirection", UsdRiTokens->sunDirection);
    _AddToken(cls, "sunSize", UsdRiTokens->sunSize);
    _AddToken(cls, "sunTint", UsdRiTokens->sunTint);
    _AddToken(cls, "textureFillColor", UsdRiTokens->textureFillColor);
    _AddToken(cls, "textureInvertU", UsdRiTokens->textureInvertU);
    _AddToken(cls, "textureInvertV", UsdRiTokens->textureInvertV);
    _AddToken(cls, "textureMap", UsdRiTokens->textureMap);
    _AddToken(cls, "textureOffsetU", UsdRiTokens->textureOffsetU);
    _AddToken(cls, "textureOffsetV", UsdRiTokens->textureOffsetV);
    _AddToken(cls, "textureScaleU", UsdRiTokens->textureScaleU);
    _AddToken(cls, "textureScaleV", UsdRiTokens->textureScaleV);
    _AddToken(cls, "textureWrapMode", UsdRiTokens->textureWrapMode);
    _AddToken(cls, "useColor", UsdRiTokens->useColor);
    _AddToken(cls, "useThroughput", UsdRiTokens->useThroughput);
    _AddToken(cls, "values", UsdRiTokens->values);
    _AddToken(cls, "width", UsdRiTokens->width);
    _AddToken(cls, "year", UsdRiTokens->year);
    _AddToken(cls, "zone", UsdRiTokens->zone);
}
