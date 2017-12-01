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
    _AddToken(cls, "angle", UsdLuxTokens->angle);
    _AddToken(cls, "angular", UsdLuxTokens->angular);
    _AddToken(cls, "automatic", UsdLuxTokens->automatic);
    _AddToken(cls, "color", UsdLuxTokens->color);
    _AddToken(cls, "colorTemperature", UsdLuxTokens->colorTemperature);
    _AddToken(cls, "consumeAndContinue", UsdLuxTokens->consumeAndContinue);
    _AddToken(cls, "consumeAndHalt", UsdLuxTokens->consumeAndHalt);
    _AddToken(cls, "cubeMapVerticalCross", UsdLuxTokens->cubeMapVerticalCross);
    _AddToken(cls, "diffuse", UsdLuxTokens->diffuse);
    _AddToken(cls, "enableColorTemperature", UsdLuxTokens->enableColorTemperature);
    _AddToken(cls, "exposure", UsdLuxTokens->exposure);
    _AddToken(cls, "filters", UsdLuxTokens->filters);
    _AddToken(cls, "geometry", UsdLuxTokens->geometry);
    _AddToken(cls, "height", UsdLuxTokens->height);
    _AddToken(cls, "ignore", UsdLuxTokens->ignore);
    _AddToken(cls, "intensity", UsdLuxTokens->intensity);
    _AddToken(cls, "latlong", UsdLuxTokens->latlong);
    _AddToken(cls, "lightList", UsdLuxTokens->lightList);
    _AddToken(cls, "lightListCacheBehavior", UsdLuxTokens->lightListCacheBehavior);
    _AddToken(cls, "mirroredBall", UsdLuxTokens->mirroredBall);
    _AddToken(cls, "normalize", UsdLuxTokens->normalize);
    _AddToken(cls, "portals", UsdLuxTokens->portals);
    _AddToken(cls, "radius", UsdLuxTokens->radius);
    _AddToken(cls, "shadowColor", UsdLuxTokens->shadowColor);
    _AddToken(cls, "shadowDistance", UsdLuxTokens->shadowDistance);
    _AddToken(cls, "shadowEnable", UsdLuxTokens->shadowEnable);
    _AddToken(cls, "shadowExclude", UsdLuxTokens->shadowExclude);
    _AddToken(cls, "shadowFalloff", UsdLuxTokens->shadowFalloff);
    _AddToken(cls, "shadowFalloffGamma", UsdLuxTokens->shadowFalloffGamma);
    _AddToken(cls, "shadowInclude", UsdLuxTokens->shadowInclude);
    _AddToken(cls, "shapingConeAngle", UsdLuxTokens->shapingConeAngle);
    _AddToken(cls, "shapingConeSoftness", UsdLuxTokens->shapingConeSoftness);
    _AddToken(cls, "shapingFocus", UsdLuxTokens->shapingFocus);
    _AddToken(cls, "shapingFocusTint", UsdLuxTokens->shapingFocusTint);
    _AddToken(cls, "shapingIesAngleScale", UsdLuxTokens->shapingIesAngleScale);
    _AddToken(cls, "shapingIesFile", UsdLuxTokens->shapingIesFile);
    _AddToken(cls, "specular", UsdLuxTokens->specular);
    _AddToken(cls, "textureFile", UsdLuxTokens->textureFile);
    _AddToken(cls, "textureFormat", UsdLuxTokens->textureFormat);
    _AddToken(cls, "width", UsdLuxTokens->width);
}
