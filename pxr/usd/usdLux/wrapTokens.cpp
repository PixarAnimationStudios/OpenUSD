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

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdLuxTokens->name.GetString(); });

void wrapUsdLuxTokens()
{
    boost::python::class_<UsdLuxTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _ADD_TOKEN(cls, angular);
    _ADD_TOKEN(cls, automatic);
    _ADD_TOKEN(cls, collectionFilterLinkIncludeRoot);
    _ADD_TOKEN(cls, collectionLightLinkIncludeRoot);
    _ADD_TOKEN(cls, collectionShadowLinkIncludeRoot);
    _ADD_TOKEN(cls, consumeAndContinue);
    _ADD_TOKEN(cls, consumeAndHalt);
    _ADD_TOKEN(cls, cubeMapVerticalCross);
    _ADD_TOKEN(cls, filterLink);
    _ADD_TOKEN(cls, geometry);
    _ADD_TOKEN(cls, guideRadius);
    _ADD_TOKEN(cls, ignore);
    _ADD_TOKEN(cls, independent);
    _ADD_TOKEN(cls, inputsAngle);
    _ADD_TOKEN(cls, inputsColor);
    _ADD_TOKEN(cls, inputsColorTemperature);
    _ADD_TOKEN(cls, inputsDiffuse);
    _ADD_TOKEN(cls, inputsEnableColorTemperature);
    _ADD_TOKEN(cls, inputsExposure);
    _ADD_TOKEN(cls, inputsHeight);
    _ADD_TOKEN(cls, inputsIntensity);
    _ADD_TOKEN(cls, inputsLength);
    _ADD_TOKEN(cls, inputsNormalize);
    _ADD_TOKEN(cls, inputsRadius);
    _ADD_TOKEN(cls, inputsShadowColor);
    _ADD_TOKEN(cls, inputsShadowDistance);
    _ADD_TOKEN(cls, inputsShadowEnable);
    _ADD_TOKEN(cls, inputsShadowFalloff);
    _ADD_TOKEN(cls, inputsShadowFalloffGamma);
    _ADD_TOKEN(cls, inputsShapingConeAngle);
    _ADD_TOKEN(cls, inputsShapingConeSoftness);
    _ADD_TOKEN(cls, inputsShapingFocus);
    _ADD_TOKEN(cls, inputsShapingFocusTint);
    _ADD_TOKEN(cls, inputsShapingIesAngleScale);
    _ADD_TOKEN(cls, inputsShapingIesFile);
    _ADD_TOKEN(cls, inputsShapingIesNormalize);
    _ADD_TOKEN(cls, inputsSpecular);
    _ADD_TOKEN(cls, inputsTextureFile);
    _ADD_TOKEN(cls, inputsTextureFormat);
    _ADD_TOKEN(cls, inputsWidth);
    _ADD_TOKEN(cls, latlong);
    _ADD_TOKEN(cls, lightFilters);
    _ADD_TOKEN(cls, lightFilterShaderId);
    _ADD_TOKEN(cls, lightLink);
    _ADD_TOKEN(cls, lightList);
    _ADD_TOKEN(cls, lightListCacheBehavior);
    _ADD_TOKEN(cls, lightMaterialSyncMode);
    _ADD_TOKEN(cls, lightShaderId);
    _ADD_TOKEN(cls, materialGlowTintsLight);
    _ADD_TOKEN(cls, MeshLight);
    _ADD_TOKEN(cls, mirroredBall);
    _ADD_TOKEN(cls, noMaterialResponse);
    _ADD_TOKEN(cls, orientToStageUpAxis);
    _ADD_TOKEN(cls, poleAxis);
    _ADD_TOKEN(cls, portals);
    _ADD_TOKEN(cls, scene);
    _ADD_TOKEN(cls, shadowLink);
    _ADD_TOKEN(cls, treatAsLine);
    _ADD_TOKEN(cls, treatAsPoint);
    _ADD_TOKEN(cls, VolumeLight);
    _ADD_TOKEN(cls, Y);
    _ADD_TOKEN(cls, Z);
    _ADD_TOKEN(cls, BoundableLightBase);
    _ADD_TOKEN(cls, CylinderLight);
    _ADD_TOKEN(cls, DiskLight);
    _ADD_TOKEN(cls, DistantLight);
    _ADD_TOKEN(cls, DomeLight);
    _ADD_TOKEN(cls, DomeLight_1);
    _ADD_TOKEN(cls, GeometryLight);
    _ADD_TOKEN(cls, LightAPI);
    _ADD_TOKEN(cls, LightFilter);
    _ADD_TOKEN(cls, LightListAPI);
    _ADD_TOKEN(cls, ListAPI);
    _ADD_TOKEN(cls, MeshLightAPI);
    _ADD_TOKEN(cls, NonboundableLightBase);
    _ADD_TOKEN(cls, PluginLight);
    _ADD_TOKEN(cls, PluginLightFilter);
    _ADD_TOKEN(cls, PortalLight);
    _ADD_TOKEN(cls, RectLight);
    _ADD_TOKEN(cls, ShadowAPI);
    _ADD_TOKEN(cls, ShapingAPI);
    _ADD_TOKEN(cls, SphereLight);
    _ADD_TOKEN(cls, VolumeLightAPI);
}
