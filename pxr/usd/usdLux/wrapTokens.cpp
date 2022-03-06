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

namespace pxrUsdUsdLuxWrapTokens {

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
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "angular", UsdLuxTokens->angular);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "automatic", UsdLuxTokens->automatic);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "collectionFilterLinkIncludeRoot", UsdLuxTokens->collectionFilterLinkIncludeRoot);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "collectionLightLinkIncludeRoot", UsdLuxTokens->collectionLightLinkIncludeRoot);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "collectionShadowLinkIncludeRoot", UsdLuxTokens->collectionShadowLinkIncludeRoot);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "consumeAndContinue", UsdLuxTokens->consumeAndContinue);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "consumeAndHalt", UsdLuxTokens->consumeAndHalt);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "cubeMapVerticalCross", UsdLuxTokens->cubeMapVerticalCross);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "cylinderLight", UsdLuxTokens->cylinderLight);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "diskLight", UsdLuxTokens->diskLight);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "distantLight", UsdLuxTokens->distantLight);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "domeLight", UsdLuxTokens->domeLight);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "extent", UsdLuxTokens->extent);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "filterLink", UsdLuxTokens->filterLink);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "geometry", UsdLuxTokens->geometry);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "geometryLight", UsdLuxTokens->geometryLight);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "guideRadius", UsdLuxTokens->guideRadius);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "ignore", UsdLuxTokens->ignore);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "independent", UsdLuxTokens->independent);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsAngle", UsdLuxTokens->inputsAngle);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsColor", UsdLuxTokens->inputsColor);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsColorTemperature", UsdLuxTokens->inputsColorTemperature);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsDiffuse", UsdLuxTokens->inputsDiffuse);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsEnableColorTemperature", UsdLuxTokens->inputsEnableColorTemperature);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsExposure", UsdLuxTokens->inputsExposure);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsHeight", UsdLuxTokens->inputsHeight);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsIntensity", UsdLuxTokens->inputsIntensity);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsLength", UsdLuxTokens->inputsLength);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsNormalize", UsdLuxTokens->inputsNormalize);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsRadius", UsdLuxTokens->inputsRadius);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsShadowColor", UsdLuxTokens->inputsShadowColor);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsShadowDistance", UsdLuxTokens->inputsShadowDistance);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsShadowEnable", UsdLuxTokens->inputsShadowEnable);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsShadowFalloff", UsdLuxTokens->inputsShadowFalloff);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsShadowFalloffGamma", UsdLuxTokens->inputsShadowFalloffGamma);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsShapingConeAngle", UsdLuxTokens->inputsShapingConeAngle);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsShapingConeSoftness", UsdLuxTokens->inputsShapingConeSoftness);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsShapingFocus", UsdLuxTokens->inputsShapingFocus);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsShapingFocusTint", UsdLuxTokens->inputsShapingFocusTint);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsShapingIesAngleScale", UsdLuxTokens->inputsShapingIesAngleScale);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsShapingIesFile", UsdLuxTokens->inputsShapingIesFile);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsShapingIesNormalize", UsdLuxTokens->inputsShapingIesNormalize);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsSpecular", UsdLuxTokens->inputsSpecular);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsTextureFile", UsdLuxTokens->inputsTextureFile);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsTextureFormat", UsdLuxTokens->inputsTextureFormat);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "inputsWidth", UsdLuxTokens->inputsWidth);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "latlong", UsdLuxTokens->latlong);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "lightFilters", UsdLuxTokens->lightFilters);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "lightFilterShaderId", UsdLuxTokens->lightFilterShaderId);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "lightLink", UsdLuxTokens->lightLink);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "lightList", UsdLuxTokens->lightList);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "lightListCacheBehavior", UsdLuxTokens->lightListCacheBehavior);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "lightMaterialSyncMode", UsdLuxTokens->lightMaterialSyncMode);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "lightShaderId", UsdLuxTokens->lightShaderId);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "materialGlowTintsLight", UsdLuxTokens->materialGlowTintsLight);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "meshLight", UsdLuxTokens->meshLight);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "mirroredBall", UsdLuxTokens->mirroredBall);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "noMaterialResponse", UsdLuxTokens->noMaterialResponse);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "orientToStageUpAxis", UsdLuxTokens->orientToStageUpAxis);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "portalLight", UsdLuxTokens->portalLight);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "portals", UsdLuxTokens->portals);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "rectLight", UsdLuxTokens->rectLight);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "shadowLink", UsdLuxTokens->shadowLink);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "sphereLight", UsdLuxTokens->sphereLight);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "treatAsLine", UsdLuxTokens->treatAsLine);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "treatAsPoint", UsdLuxTokens->treatAsPoint);
    pxrUsdUsdLuxWrapTokens::_AddToken(cls, "volumeLight", UsdLuxTokens->volumeLight);
}
