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
#ifndef USDLUX_TOKENS_H
#define USDLUX_TOKENS_H

/// \file usdLux/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \hideinitializer
#define USDLUX_TOKENS \
    (angle) \
    (angular) \
    (automatic) \
    (color) \
    (colorTemperature) \
    (consumeAndContinue) \
    (consumeAndHalt) \
    (cubeMapVerticalCross) \
    (diffuse) \
    (enableColorTemperature) \
    (exposure) \
    (filters) \
    (geometry) \
    (height) \
    (ignore) \
    (intensity) \
    (latlong) \
    (lightList) \
    ((lightListCacheBehavior, "lightList:cacheBehavior")) \
    (mirroredBall) \
    (normalize) \
    (portals) \
    (radius) \
    ((shadowColor, "shadow:color")) \
    ((shadowDistance, "shadow:distance")) \
    ((shadowEnable, "shadow:enable")) \
    ((shadowExclude, "shadow:exclude")) \
    ((shadowFalloff, "shadow:falloff")) \
    ((shadowFalloffGamma, "shadow:falloffGamma")) \
    ((shadowInclude, "shadow:include")) \
    ((shapingConeAngle, "shaping:cone:angle")) \
    ((shapingConeSoftness, "shaping:cone:softness")) \
    ((shapingFocus, "shaping:focus")) \
    ((shapingFocusTint, "shaping:focusTint")) \
    ((shapingIesAngleScale, "shaping:ies:angleScale")) \
    ((shapingIesFile, "shaping:ies:file")) \
    (specular) \
    ((textureFile, "texture:file")) \
    ((textureFormat, "texture:format")) \
    (width)

/// \anchor UsdLuxTokens
///
/// <b>UsdLuxTokens</b> provides static, efficient TfToken's for
/// use in all public USD API
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdLuxTokens also contains all of the \em allowedTokens values declared
/// for schema builtin attributes of 'token' scene description type.
/// Use UsdLuxTokens like so:
///
/// \code
///     gprim.GetVisibilityAttr().Set(UsdLuxTokens->invisible);
/// \endcode
///
/// The tokens are:
/// \li <b>angle</b> - UsdLuxDistantLight
/// \li <b>angular</b> - Possible value for UsdLuxDomeLight::GetTextureFormatAttr()
/// \li <b>automatic</b> - Possible value for UsdLuxDomeLight::GetTextureFormatAttr(), Default value for UsdLuxDomeLight::GetTextureFormatAttr()
/// \li <b>color</b> - UsdLuxLight
/// \li <b>colorTemperature</b> - UsdLuxLight
/// \li <b>consumeAndContinue</b> - Possible value for UsdLuxListAPI::GetLightListCacheBehaviorAttr()
/// \li <b>consumeAndHalt</b> - Possible value for UsdLuxListAPI::GetLightListCacheBehaviorAttr()
/// \li <b>cubeMapVerticalCross</b> - Possible value for UsdLuxDomeLight::GetTextureFormatAttr()
/// \li <b>diffuse</b> - UsdLuxLight
/// \li <b>enableColorTemperature</b> - UsdLuxLight
/// \li <b>exposure</b> - UsdLuxLight
/// \li <b>filters</b> - UsdLuxLight
/// \li <b>geometry</b> - UsdLuxGeometryLight
/// \li <b>height</b> - UsdLuxRectLight
/// \li <b>ignore</b> - Possible value for UsdLuxListAPI::GetLightListCacheBehaviorAttr()
/// \li <b>intensity</b> - UsdLuxDistantLight, UsdLuxLight
/// \li <b>latlong</b> - Possible value for UsdLuxDomeLight::GetTextureFormatAttr()
/// \li <b>lightList</b> - UsdLuxListAPI
/// \li <b>lightListCacheBehavior</b> - UsdLuxListAPI
/// \li <b>mirroredBall</b> - Possible value for UsdLuxDomeLight::GetTextureFormatAttr()
/// \li <b>normalize</b> - UsdLuxLight
/// \li <b>portals</b> - UsdLuxDomeLight
/// \li <b>radius</b> - UsdLuxSphereLight, UsdLuxDiskLight
/// \li <b>shadowColor</b> - UsdLuxShadowAPI
/// \li <b>shadowDistance</b> - UsdLuxShadowAPI
/// \li <b>shadowEnable</b> - UsdLuxShadowAPI
/// \li <b>shadowExclude</b> - UsdLuxShadowAPI
/// \li <b>shadowFalloff</b> - UsdLuxShadowAPI
/// \li <b>shadowFalloffGamma</b> - UsdLuxShadowAPI
/// \li <b>shadowInclude</b> - UsdLuxShadowAPI
/// \li <b>shapingConeAngle</b> - UsdLuxShapingAPI
/// \li <b>shapingConeSoftness</b> - UsdLuxShapingAPI
/// \li <b>shapingFocus</b> - UsdLuxShapingAPI
/// \li <b>shapingFocusTint</b> - UsdLuxShapingAPI
/// \li <b>shapingIesAngleScale</b> - UsdLuxShapingAPI
/// \li <b>shapingIesFile</b> - UsdLuxShapingAPI
/// \li <b>specular</b> - UsdLuxLight
/// \li <b>textureFile</b> - UsdLuxDomeLight, UsdLuxRectLight
/// \li <b>textureFormat</b> - UsdLuxDomeLight
/// \li <b>width</b> - UsdLuxRectLight
TF_DECLARE_PUBLIC_TOKENS(UsdLuxTokens, USDLUX_API, USDLUX_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
