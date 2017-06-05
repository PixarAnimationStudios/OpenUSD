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
#ifndef USDRI_TOKENS_H
#define USDRI_TOKENS_H

/// \file usdRi/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdRi/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \hideinitializer
#define USDRI_TOKENS \
    (analytic) \
    ((analyticApex, "analytic:apex")) \
    ((analyticBlurAmount, "analytic:blur:amount")) \
    ((analyticBlurExponent, "analytic:blur:exponent")) \
    ((analyticBlurFarDistance, "analytic:blur:farDistance")) \
    ((analyticBlurFarValue, "analytic:blur:farValue")) \
    ((analyticBlurMidpoint, "analytic:blur:midpoint")) \
    ((analyticBlurMidValue, "analytic:blur:midValue")) \
    ((analyticBlurNearDistance, "analytic:blur:nearDistance")) \
    ((analyticBlurNearValue, "analytic:blur:nearValue")) \
    ((analyticBlurSMult, "analytic:blur:sMult")) \
    ((analyticBlurTMult, "analytic:blur:tMult")) \
    ((analyticDensityExponent, "analytic:density:exponent")) \
    ((analyticDensityFarDistance, "analytic:density:farDistance")) \
    ((analyticDensityFarValue, "analytic:density:farValue")) \
    ((analyticDensityMidpoint, "analytic:density:midpoint")) \
    ((analyticDensityMidValue, "analytic:density:midValue")) \
    ((analyticDensityNearDistance, "analytic:density:nearDistance")) \
    ((analyticDensityNearValue, "analytic:density:nearValue")) \
    ((analyticDirectional, "analytic:directional")) \
    ((analyticShearX, "analytic:shearX")) \
    ((analyticShearY, "analytic:shearY")) \
    ((analyticUseLightDirection, "analytic:useLightDirection")) \
    (aovName) \
    (argsPath) \
    (barnMode) \
    (clamp) \
    ((colorContrast, "color:contrast")) \
    ((colorMidpoint, "color:midpoint")) \
    ((colorSaturation, "color:saturation")) \
    ((colorTint, "color:tint")) \
    ((colorWhitepoint, "color:whitepoint")) \
    (cone) \
    (cookieMode) \
    (day) \
    (depth) \
    (distanceToLight) \
    ((edgeBack, "edge:back")) \
    ((edgeBottom, "edge:bottom")) \
    ((edgeFront, "edge:front")) \
    ((edgeLeft, "edge:left")) \
    ((edgeRight, "edge:right")) \
    (edgeThickness) \
    ((edgeTop, "edge:top")) \
    ((falloffRampBeginDistance, "falloffRamp:beginDistance")) \
    ((falloffRampEndDistance, "falloffRamp:endDistance")) \
    (filePath) \
    (haziness) \
    (height) \
    (hour) \
    ((infoArgsPath, "info:argsPath")) \
    ((infoFilePath, "info:filePath")) \
    ((infoOslPath, "info:oslPath")) \
    ((infoSloPath, "info:sloPath")) \
    (inPrimaryHit) \
    (inReflection) \
    (inRefraction) \
    (invert) \
    (latitude) \
    (linear) \
    (longitude) \
    (max) \
    (min) \
    (month) \
    (multiply) \
    (noEffect) \
    (noLight) \
    (off) \
    (onVolumeBoundaries) \
    ((outputsRiBxdf, "outputs:ri:bxdf")) \
    ((outputsRiDisplacement, "outputs:ri:displacement")) \
    ((outputsRiSurface, "outputs:ri:surface")) \
    ((outputsRiVolume, "outputs:ri:volume")) \
    (physical) \
    (preBarnEffect) \
    (radial) \
    (radius) \
    (rampMode) \
    ((refineBack, "refine:back")) \
    ((refineBottom, "refine:bottom")) \
    ((refineFront, "refine:front")) \
    ((refineLeft, "refine:left")) \
    ((refineRight, "refine:right")) \
    ((refineTop, "refine:top")) \
    (repeat) \
    ((riCombineMode, "ri:combineMode")) \
    ((riDensity, "ri:density")) \
    ((riDiffuse, "ri:diffuse")) \
    ((riExposure, "ri:exposure")) \
    ((riFocusRegion, "ri:focusRegion")) \
    ((riIntensity, "ri:intensity")) \
    ((riIntensityNearDist, "ri:intensityNearDist")) \
    ((riInvert, "ri:invert")) \
    ((riLightGroup, "ri:lightGroup")) \
    ((riLookBxdf, "riLook:bxdf")) \
    ((riLookCoshaders, "riLook:coshaders")) \
    ((riLookDisplacement, "riLook:displacement")) \
    ((riLookPatterns, "riLook:patterns")) \
    ((riLookSurface, "riLook:surface")) \
    ((riLookVolume, "riLook:volume")) \
    ((riPortalIntensity, "ri:portal:intensity")) \
    ((riPortalTint, "ri:portal:tint")) \
    ((riSamplingFixedSampleCount, "ri:sampling:fixedSampleCount")) \
    ((riSamplingImportanceMultiplier, "ri:sampling:importanceMultiplier")) \
    ((riShadowThinShadow, "ri:shadow:thinShadow")) \
    ((riSpecular, "ri:specular")) \
    ((riTextureGamma, "ri:texture:gamma")) \
    ((riTextureSaturation, "ri:texture:saturation")) \
    ((riTraceLightPaths, "ri:trace:lightPaths")) \
    ((scaleDepth, "scale:depth")) \
    ((scaleHeight, "scale:height")) \
    ((scaleWidth, "scale:width")) \
    (screen) \
    (skyTint) \
    (spherical) \
    (sunDirection) \
    (sunSize) \
    (sunTint) \
    ((textureFillColor, "texture:fillColor")) \
    ((textureInvertU, "texture:invertU")) \
    ((textureInvertV, "texture:invertV")) \
    ((textureMap, "texture:map")) \
    ((textureOffsetU, "texture:offsetU")) \
    ((textureOffsetV, "texture:offsetV")) \
    ((textureScaleU, "texture:scaleU")) \
    ((textureScaleV, "texture:scaleV")) \
    ((textureWrapMode, "texture:wrapMode")) \
    (useColor) \
    (useThroughput) \
    (width) \
    (year) \
    (zone)

/// \anchor UsdRiTokens
///
/// <b>UsdRiTokens</b> provides static, efficient TfToken's for
/// use in all public USD API
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdRiTokens also contains all of the \em allowedTokens values declared
/// for schema builtin attributes of 'token' scene description type.
/// Use UsdRiTokens like so:
///
/// \code
///     gprim.GetVisibilityAttr().Set(UsdRiTokens->invisible);
/// \endcode
///
/// The tokens are:
/// \li <b>analytic</b> - Possible value for UsdRiPxrCookieLightFilter::GetCookieModeAttr(), Possible value for UsdRiPxrBarnLightFilter::GetBarnModeAttr()
/// \li <b>analyticApex</b> - UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>analyticBlurAmount</b> - UsdRiPxrCookieLightFilter
/// \li <b>analyticBlurExponent</b> - UsdRiPxrCookieLightFilter
/// \li <b>analyticBlurFarDistance</b> - UsdRiPxrCookieLightFilter
/// \li <b>analyticBlurFarValue</b> - UsdRiPxrCookieLightFilter
/// \li <b>analyticBlurMidpoint</b> - UsdRiPxrCookieLightFilter
/// \li <b>analyticBlurMidValue</b> - UsdRiPxrCookieLightFilter
/// \li <b>analyticBlurNearDistance</b> - UsdRiPxrCookieLightFilter
/// \li <b>analyticBlurNearValue</b> - UsdRiPxrCookieLightFilter
/// \li <b>analyticBlurSMult</b> - UsdRiPxrCookieLightFilter
/// \li <b>analyticBlurTMult</b> - UsdRiPxrCookieLightFilter
/// \li <b>analyticDensityExponent</b> - UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>analyticDensityFarDistance</b> - UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>analyticDensityFarValue</b> - UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>analyticDensityMidpoint</b> - UsdRiPxrCookieLightFilter
/// \li <b>analyticDensityMidValue</b> - UsdRiPxrCookieLightFilter
/// \li <b>analyticDensityNearDistance</b> - UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>analyticDensityNearValue</b> - UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>analyticDirectional</b> - UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>analyticShearX</b> - UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>analyticShearY</b> - UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>analyticUseLightDirection</b> - UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>aovName</b> - UsdRiPxrAovLight
/// \li <b>argsPath</b> - UsdRiRisIntegrator
/// \li <b>barnMode</b> - UsdRiPxrBarnLightFilter
/// \li <b>clamp</b> - Possible value for UsdRiPxrCookieLightFilter::GetTextureWrapModeAttr()
/// \li <b>colorContrast</b> - UsdRiPxrCookieLightFilter
/// \li <b>colorMidpoint</b> - UsdRiPxrCookieLightFilter
/// \li <b>colorSaturation</b> - UsdRiPxrRodLightFilter, UsdRiPxrCookieLightFilter
/// \li <b>colorTint</b> - UsdRiPxrCookieLightFilter
/// \li <b>colorWhitepoint</b> - UsdRiPxrCookieLightFilter
/// \li <b>cone</b> - Possible value for UsdRiPxrBarnLightFilter::GetPreBarnEffectAttr()
/// \li <b>cookieMode</b> - UsdRiPxrCookieLightFilter
/// \li <b>day</b> - UsdRiPxrEnvDayLight
/// \li <b>depth</b> - UsdRiPxrRodLightFilter
/// \li <b>distanceToLight</b> - Possible value for UsdRiPxrRampLightFilter::GetRampModeAttr(), Default value for UsdRiPxrRampLightFilter::GetRampModeAttr()
/// \li <b>edgeBack</b> - UsdRiPxrRodLightFilter
/// \li <b>edgeBottom</b> - UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>edgeFront</b> - UsdRiPxrRodLightFilter
/// \li <b>edgeLeft</b> - UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>edgeRight</b> - UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>edgeThickness</b> - UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>edgeTop</b> - UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>falloffRampBeginDistance</b> - UsdRiPxrRampLightFilter
/// \li <b>falloffRampEndDistance</b> - UsdRiPxrRampLightFilter
/// \li <b>filePath</b> - UsdRiRisIntegrator
/// \li <b>haziness</b> - UsdRiPxrEnvDayLight
/// \li <b>height</b> - UsdRiPxrRodLightFilter, UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>hour</b> - UsdRiPxrEnvDayLight
/// \li <b>infoArgsPath</b> - UsdRiRisObject
/// \li <b>infoFilePath</b> - UsdRiRisOslPattern, UsdRiRisObject
/// \li <b>infoOslPath</b> - UsdRiRisOslPattern
/// \li <b>infoSloPath</b> - UsdRiRslShader
/// \li <b>inPrimaryHit</b> - UsdRiPxrAovLight
/// \li <b>inReflection</b> - UsdRiPxrAovLight
/// \li <b>inRefraction</b> - UsdRiPxrAovLight
/// \li <b>invert</b> - UsdRiPxrAovLight
/// \li <b>latitude</b> - UsdRiPxrEnvDayLight
/// \li <b>linear</b> - Possible value for UsdRiPxrRampLightFilter::GetRampModeAttr()
/// \li <b>longitude</b> - UsdRiPxrEnvDayLight
/// \li <b>max</b> - Possible value for UsdRiLightFilterAPI::GetRiCombineModeAttr()
/// \li <b>min</b> - Possible value for UsdRiLightFilterAPI::GetRiCombineModeAttr()
/// \li <b>month</b> - UsdRiPxrEnvDayLight
/// \li <b>multiply</b> - Possible value for UsdRiLightFilterAPI::GetRiCombineModeAttr()
/// \li <b>noEffect</b> - Possible value for UsdRiPxrBarnLightFilter::GetPreBarnEffectAttr(), Default value for UsdRiPxrBarnLightFilter::GetPreBarnEffectAttr()
/// \li <b>noLight</b> - Possible value for UsdRiPxrBarnLightFilter::GetPreBarnEffectAttr()
/// \li <b>off</b> - Possible value for UsdRiPxrCookieLightFilter::GetTextureWrapModeAttr(), Default value for UsdRiPxrCookieLightFilter::GetTextureWrapModeAttr()
/// \li <b>onVolumeBoundaries</b> - UsdRiPxrAovLight
/// \li <b>outputsRiBxdf</b> - UsdRiMaterialAPI
/// \li <b>outputsRiDisplacement</b> - UsdRiMaterialAPI
/// \li <b>outputsRiSurface</b> - UsdRiMaterialAPI
/// \li <b>outputsRiVolume</b> - UsdRiMaterialAPI
/// \li <b>physical</b> - Possible value for UsdRiPxrCookieLightFilter::GetCookieModeAttr(), Default value for UsdRiPxrCookieLightFilter::GetCookieModeAttr(), Possible value for UsdRiPxrBarnLightFilter::GetBarnModeAttr(), Default value for UsdRiPxrBarnLightFilter::GetBarnModeAttr()
/// \li <b>preBarnEffect</b> - UsdRiPxrBarnLightFilter
/// \li <b>radial</b> - Possible value for UsdRiPxrRampLightFilter::GetRampModeAttr()
/// \li <b>radius</b> - UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>rampMode</b> - UsdRiPxrRampLightFilter
/// \li <b>refineBack</b> - UsdRiPxrRodLightFilter
/// \li <b>refineBottom</b> - UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>refineFront</b> - UsdRiPxrRodLightFilter
/// \li <b>refineLeft</b> - UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>refineRight</b> - UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>refineTop</b> - UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>repeat</b> - Possible value for UsdRiPxrCookieLightFilter::GetTextureWrapModeAttr()
/// \li <b>riCombineMode</b> - UsdRiLightFilterAPI
/// \li <b>riDensity</b> - UsdRiLightFilterAPI
/// \li <b>riDiffuse</b> - UsdRiLightFilterAPI
/// \li <b>riExposure</b> - UsdRiLightFilterAPI
/// \li <b>riFocusRegion</b> - UsdRiStatements
/// \li <b>riIntensity</b> - UsdRiLightFilterAPI
/// \li <b>riIntensityNearDist</b> - UsdRiLightAPI
/// \li <b>riInvert</b> - UsdRiLightFilterAPI
/// \li <b>riLightGroup</b> - UsdRiLightAPI
/// \li <b>riLookBxdf</b> - UsdRiLookAPI
/// \li <b>riLookCoshaders</b> - UsdRiLookAPI
/// \li <b>riLookDisplacement</b> - UsdRiLookAPI
/// \li <b>riLookPatterns</b> - UsdRiLookAPI
/// \li <b>riLookSurface</b> - UsdRiLookAPI
/// \li <b>riLookVolume</b> - UsdRiLookAPI
/// \li <b>riPortalIntensity</b> - UsdRiLightPortalAPI
/// \li <b>riPortalTint</b> - UsdRiLightPortalAPI
/// \li <b>riSamplingFixedSampleCount</b> - UsdRiLightAPI
/// \li <b>riSamplingImportanceMultiplier</b> - UsdRiLightAPI
/// \li <b>riShadowThinShadow</b> - UsdRiLightAPI
/// \li <b>riSpecular</b> - UsdRiLightFilterAPI
/// \li <b>riTextureGamma</b> - UsdRiTextureAPI
/// \li <b>riTextureSaturation</b> - UsdRiTextureAPI
/// \li <b>riTraceLightPaths</b> - UsdRiLightAPI
/// \li <b>scaleDepth</b> - UsdRiPxrRodLightFilter
/// \li <b>scaleHeight</b> - UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>scaleWidth</b> - UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>screen</b> - Possible value for UsdRiLightFilterAPI::GetRiCombineModeAttr()
/// \li <b>skyTint</b> - UsdRiPxrEnvDayLight
/// \li <b>spherical</b> - Possible value for UsdRiPxrRampLightFilter::GetRampModeAttr()
/// \li <b>sunDirection</b> - UsdRiPxrEnvDayLight
/// \li <b>sunSize</b> - UsdRiPxrEnvDayLight
/// \li <b>sunTint</b> - UsdRiPxrEnvDayLight
/// \li <b>textureFillColor</b> - UsdRiPxrCookieLightFilter
/// \li <b>textureInvertU</b> - UsdRiPxrCookieLightFilter
/// \li <b>textureInvertV</b> - UsdRiPxrCookieLightFilter
/// \li <b>textureMap</b> - UsdRiPxrCookieLightFilter
/// \li <b>textureOffsetU</b> - UsdRiPxrCookieLightFilter
/// \li <b>textureOffsetV</b> - UsdRiPxrCookieLightFilter
/// \li <b>textureScaleU</b> - UsdRiPxrCookieLightFilter
/// \li <b>textureScaleV</b> - UsdRiPxrCookieLightFilter
/// \li <b>textureWrapMode</b> - UsdRiPxrCookieLightFilter
/// \li <b>useColor</b> - UsdRiPxrAovLight
/// \li <b>useThroughput</b> - UsdRiPxrAovLight
/// \li <b>width</b> - UsdRiPxrRodLightFilter, UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
/// \li <b>year</b> - UsdRiPxrEnvDayLight
/// \li <b>zone</b> - UsdRiPxrEnvDayLight
TF_DECLARE_PUBLIC_TOKENS(UsdRiTokens, USDRI_API, USDRI_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
