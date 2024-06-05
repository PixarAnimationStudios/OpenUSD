//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    _AddToken(cls, "geometry", UsdLuxTokens->geometry);
    _AddToken(cls, "guideRadius", UsdLuxTokens->guideRadius);
    _AddToken(cls, "ignore", UsdLuxTokens->ignore);
    _AddToken(cls, "independent", UsdLuxTokens->independent);
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
    _AddToken(cls, "lightFilters", UsdLuxTokens->lightFilters);
    _AddToken(cls, "lightFilterShaderId", UsdLuxTokens->lightFilterShaderId);
    _AddToken(cls, "lightLink", UsdLuxTokens->lightLink);
    _AddToken(cls, "lightList", UsdLuxTokens->lightList);
    _AddToken(cls, "lightListCacheBehavior", UsdLuxTokens->lightListCacheBehavior);
    _AddToken(cls, "lightMaterialSyncMode", UsdLuxTokens->lightMaterialSyncMode);
    _AddToken(cls, "lightShaderId", UsdLuxTokens->lightShaderId);
    _AddToken(cls, "materialGlowTintsLight", UsdLuxTokens->materialGlowTintsLight);
    _AddToken(cls, "MeshLight", UsdLuxTokens->MeshLight);
    _AddToken(cls, "mirroredBall", UsdLuxTokens->mirroredBall);
    _AddToken(cls, "noMaterialResponse", UsdLuxTokens->noMaterialResponse);
    _AddToken(cls, "orientToStageUpAxis", UsdLuxTokens->orientToStageUpAxis);
    _AddToken(cls, "poleAxis", UsdLuxTokens->poleAxis);
    _AddToken(cls, "portals", UsdLuxTokens->portals);
    _AddToken(cls, "scene", UsdLuxTokens->scene);
    _AddToken(cls, "shadowLink", UsdLuxTokens->shadowLink);
    _AddToken(cls, "treatAsLine", UsdLuxTokens->treatAsLine);
    _AddToken(cls, "treatAsPoint", UsdLuxTokens->treatAsPoint);
    _AddToken(cls, "VolumeLight", UsdLuxTokens->VolumeLight);
    _AddToken(cls, "Y", UsdLuxTokens->Y);
    _AddToken(cls, "Z", UsdLuxTokens->Z);
    _AddToken(cls, "BoundableLightBase", UsdLuxTokens->BoundableLightBase);
    _AddToken(cls, "CylinderLight", UsdLuxTokens->CylinderLight);
    _AddToken(cls, "DiskLight", UsdLuxTokens->DiskLight);
    _AddToken(cls, "DistantLight", UsdLuxTokens->DistantLight);
    _AddToken(cls, "DomeLight", UsdLuxTokens->DomeLight);
    _AddToken(cls, "DomeLight_1", UsdLuxTokens->DomeLight_1);
    _AddToken(cls, "GeometryLight", UsdLuxTokens->GeometryLight);
    _AddToken(cls, "LightAPI", UsdLuxTokens->LightAPI);
    _AddToken(cls, "LightFilter", UsdLuxTokens->LightFilter);
    _AddToken(cls, "LightListAPI", UsdLuxTokens->LightListAPI);
    _AddToken(cls, "ListAPI", UsdLuxTokens->ListAPI);
    _AddToken(cls, "MeshLightAPI", UsdLuxTokens->MeshLightAPI);
    _AddToken(cls, "NonboundableLightBase", UsdLuxTokens->NonboundableLightBase);
    _AddToken(cls, "PluginLight", UsdLuxTokens->PluginLight);
    _AddToken(cls, "PluginLightFilter", UsdLuxTokens->PluginLightFilter);
    _AddToken(cls, "PortalLight", UsdLuxTokens->PortalLight);
    _AddToken(cls, "RectLight", UsdLuxTokens->RectLight);
    _AddToken(cls, "ShadowAPI", UsdLuxTokens->ShadowAPI);
    _AddToken(cls, "ShapingAPI", UsdLuxTokens->ShapingAPI);
    _AddToken(cls, "SphereLight", UsdLuxTokens->SphereLight);
    _AddToken(cls, "VolumeLightAPI", UsdLuxTokens->VolumeLightAPI);
}
