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
#include "pxr/usd/usdLux/tokens.h"

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

void wrapUsdLuxTokens()
{
    boost::python::class_<UsdLuxTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "angular", UsdLuxTokens->angular);
    _AddToken(cls, "automatic", UsdLuxTokens->automatic);
    _AddToken(cls, "collectionFilterLinkIncludeRoot", UsdLuxTokens->collectionFilterLinkIncludeRoot);
    _AddToken(cls, "collectionLightLinkIncludeRoot", UsdLuxTokens->collectionLightLinkIncludeRoot);
    _AddToken(cls, "collectionShadowLinkIncludeRoot", UsdLuxTokens->collectionShadowLinkIncludeRoot);
    _AddToken(cls, "consumeAndContinue", UsdLuxTokens->consumeAndContinue);
    _AddToken(cls, "consumeAndHalt", UsdLuxTokens->consumeAndHalt);
    _AddToken(cls, "cubeMapVerticalCross", UsdLuxTokens->cubeMapVerticalCross);
    _AddToken(cls, "filterLink", UsdLuxTokens->filterLink);
    _AddToken(cls, "filters", UsdLuxTokens->filters);
    _AddToken(cls, "geometry", UsdLuxTokens->geometry);
    _AddToken(cls, "ignore", UsdLuxTokens->ignore);
    _AddToken(cls, "inputsAngle", UsdLuxTokens->inputsAngle);
    _AddToken(cls, "inputsColor", UsdLuxTokens->inputsColor);
    _AddToken(cls, "inputsColorTemperature", UsdLuxTokens->inputsColorTemperature);
    _AddToken(cls, "inputsDiffuse", UsdLuxTokens->inputsDiffuse);
    _AddToken(cls, "inputsEnableColorTemperature", UsdLuxTokens->inputsEnableColorTemperature);
    _AddToken(cls, "inputsExposure", UsdLuxTokens->inputsExposure);
    _AddToken(cls, "inputsHeight", UsdLuxTokens->inputsHeight);
    _AddToken(cls, "inputsIntensity", UsdLuxTokens->inputsIntensity);
    _AddToken(cls, "inputsLength", UsdLuxTokens->inputsLength);
    _AddToken(cls, "inputsNormalize", UsdLuxTokens->inputsNormalize);
    _AddToken(cls, "inputsRadius", UsdLuxTokens->inputsRadius);
    _AddToken(cls, "inputsShadowColor", UsdLuxTokens->inputsShadowColor);
    _AddToken(cls, "inputsShadowDistance", UsdLuxTokens->inputsShadowDistance);
    _AddToken(cls, "inputsShadowEnable", UsdLuxTokens->inputsShadowEnable);
    _AddToken(cls, "inputsShadowFalloff", UsdLuxTokens->inputsShadowFalloff);
    _AddToken(cls, "inputsShadowFalloffGamma", UsdLuxTokens->inputsShadowFalloffGamma);
    _AddToken(cls, "inputsShapingConeAngle", UsdLuxTokens->inputsShapingConeAngle);
    _AddToken(cls, "inputsShapingConeSoftness", UsdLuxTokens->inputsShapingConeSoftness);
    _AddToken(cls, "inputsShapingFocus", UsdLuxTokens->inputsShapingFocus);
    _AddToken(cls, "inputsShapingFocusTint", UsdLuxTokens->inputsShapingFocusTint);
    _AddToken(cls, "inputsShapingIesAngleScale", UsdLuxTokens->inputsShapingIesAngleScale);
    _AddToken(cls, "inputsShapingIesFile", UsdLuxTokens->inputsShapingIesFile);
    _AddToken(cls, "inputsShapingIesNormalize", UsdLuxTokens->inputsShapingIesNormalize);
    _AddToken(cls, "inputsSpecular", UsdLuxTokens->inputsSpecular);
    _AddToken(cls, "inputsTextureFile", UsdLuxTokens->inputsTextureFile);
    _AddToken(cls, "inputsTextureFormat", UsdLuxTokens->inputsTextureFormat);
    _AddToken(cls, "inputsWidth", UsdLuxTokens->inputsWidth);
    _AddToken(cls, "latlong", UsdLuxTokens->latlong);
    _AddToken(cls, "lightLink", UsdLuxTokens->lightLink);
    _AddToken(cls, "lightList", UsdLuxTokens->lightList);
    _AddToken(cls, "lightListCacheBehavior", UsdLuxTokens->lightListCacheBehavior);
    _AddToken(cls, "mirroredBall", UsdLuxTokens->mirroredBall);
    _AddToken(cls, "orientToStageUpAxis", UsdLuxTokens->orientToStageUpAxis);
    _AddToken(cls, "portals", UsdLuxTokens->portals);
    _AddToken(cls, "shadowLink", UsdLuxTokens->shadowLink);
    _AddToken(cls, "treatAsLine", UsdLuxTokens->treatAsLine);
    _AddToken(cls, "treatAsPoint", UsdLuxTokens->treatAsPoint);
}
