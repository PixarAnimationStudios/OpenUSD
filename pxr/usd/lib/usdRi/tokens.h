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
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdRiTokensType
///
/// \link UsdRiTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdRiTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdRiTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdRiTokens->analytic);
/// \endcode
struct UsdRiTokensType {
    USDRI_API UsdRiTokensType();
    /// \brief "analytic"
    /// 
    /// Possible value for UsdRiPxrCookieLightFilter::GetCookieModeAttr(), Possible value for UsdRiPxrBarnLightFilter::GetBarnModeAttr()
    const TfToken analytic;
    /// \brief "analytic:apex"
    /// 
    /// UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
    const TfToken analyticApex;
    /// \brief "analytic:blur:amount"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken analyticBlurAmount;
    /// \brief "analytic:blur:exponent"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken analyticBlurExponent;
    /// \brief "analytic:blur:farDistance"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken analyticBlurFarDistance;
    /// \brief "analytic:blur:farValue"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken analyticBlurFarValue;
    /// \brief "analytic:blur:midpoint"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken analyticBlurMidpoint;
    /// \brief "analytic:blur:midValue"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken analyticBlurMidValue;
    /// \brief "analytic:blur:nearDistance"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken analyticBlurNearDistance;
    /// \brief "analytic:blur:nearValue"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken analyticBlurNearValue;
    /// \brief "analytic:blur:sMult"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken analyticBlurSMult;
    /// \brief "analytic:blur:tMult"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken analyticBlurTMult;
    /// \brief "analytic:density:exponent"
    /// 
    /// UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
    const TfToken analyticDensityExponent;
    /// \brief "analytic:density:farDistance"
    /// 
    /// UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
    const TfToken analyticDensityFarDistance;
    /// \brief "analytic:density:farValue"
    /// 
    /// UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
    const TfToken analyticDensityFarValue;
    /// \brief "analytic:density:midpoint"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken analyticDensityMidpoint;
    /// \brief "analytic:density:midValue"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken analyticDensityMidValue;
    /// \brief "analytic:density:nearDistance"
    /// 
    /// UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
    const TfToken analyticDensityNearDistance;
    /// \brief "analytic:density:nearValue"
    /// 
    /// UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
    const TfToken analyticDensityNearValue;
    /// \brief "analytic:directional"
    /// 
    /// UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
    const TfToken analyticDirectional;
    /// \brief "analytic:shearX"
    /// 
    /// UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
    const TfToken analyticShearX;
    /// \brief "analytic:shearY"
    /// 
    /// UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
    const TfToken analyticShearY;
    /// \brief "analytic:useLightDirection"
    /// 
    /// UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
    const TfToken analyticUseLightDirection;
    /// \brief "aovName"
    /// 
    /// UsdRiPxrAovLight
    const TfToken aovName;
    /// \brief "argsPath"
    /// 
    /// UsdRiRisIntegrator
    const TfToken argsPath;
    /// \brief "barnMode"
    /// 
    /// UsdRiPxrBarnLightFilter
    const TfToken barnMode;
    /// \brief "bspline"
    /// 
    /// UsdSplineAPI - BSpline spline interpolation
    const TfToken bspline;
    /// \brief "catmullRom"
    /// 
    /// UsdSplineAPI - Catmull-Rom spline interpolation
    const TfToken catmullRom;
    /// \brief "clamp"
    /// 
    /// Possible value for UsdRiPxrCookieLightFilter::GetTextureWrapModeAttr()
    const TfToken clamp;
    /// \brief "color:contrast"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken colorContrast;
    /// \brief "color:midpoint"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken colorMidpoint;
    /// \brief "color:saturation"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrCookieLightFilter
    const TfToken colorSaturation;
    /// \brief "color:tint"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken colorTint;
    /// \brief "color:whitepoint"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken colorWhitepoint;
    /// \brief "cone"
    /// 
    /// Possible value for UsdRiPxrBarnLightFilter::GetPreBarnEffectAttr()
    const TfToken cone;
    /// \brief "constant"
    /// 
    /// UsdSplineAPI - Constant-value spline interpolation
    const TfToken constant;
    /// \brief "cookieMode"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken cookieMode;
    /// \brief "day"
    /// 
    /// UsdRiPxrEnvDayLight
    const TfToken day;
    /// \brief "depth"
    /// 
    /// UsdRiPxrRodLightFilter
    const TfToken depth;
    /// \brief "distanceToLight"
    /// 
    /// Possible value for UsdRiPxrRampLightFilter::GetRampModeAttr(), Default value for UsdRiPxrRampLightFilter::GetRampModeAttr()
    const TfToken distanceToLight;
    /// \brief "edge:back"
    /// 
    /// UsdRiPxrRodLightFilter
    const TfToken edgeBack;
    /// \brief "edge:bottom"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
    const TfToken edgeBottom;
    /// \brief "edge:front"
    /// 
    /// UsdRiPxrRodLightFilter
    const TfToken edgeFront;
    /// \brief "edge:left"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
    const TfToken edgeLeft;
    /// \brief "edge:right"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
    const TfToken edgeRight;
    /// \brief "edgeThickness"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
    const TfToken edgeThickness;
    /// \brief "edge:top"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
    const TfToken edgeTop;
    /// \brief "falloffRamp:beginDistance"
    /// 
    /// UsdRiPxrRampLightFilter
    const TfToken falloffRampBeginDistance;
    /// \brief "falloffRamp:endDistance"
    /// 
    /// UsdRiPxrRampLightFilter
    const TfToken falloffRampEndDistance;
    /// \brief "filePath"
    /// 
    /// UsdRiRisIntegrator
    const TfToken filePath;
    /// \brief "haziness"
    /// 
    /// UsdRiPxrEnvDayLight
    const TfToken haziness;
    /// \brief "height"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
    const TfToken height;
    /// \brief "hour"
    /// 
    /// UsdRiPxrEnvDayLight
    const TfToken hour;
    /// \brief "info:argsPath"
    /// 
    /// UsdRiRisObject
    const TfToken infoArgsPath;
    /// \brief "info:filePath"
    /// 
    /// UsdRiRisOslPattern, UsdRiRisObject
    const TfToken infoFilePath;
    /// \brief "info:oslPath"
    /// 
    /// UsdRiRisOslPattern
    const TfToken infoOslPath;
    /// \brief "info:sloPath"
    /// 
    /// UsdRiRslShader
    const TfToken infoSloPath;
    /// \brief "inPrimaryHit"
    /// 
    /// UsdRiPxrAovLight
    const TfToken inPrimaryHit;
    /// \brief "inReflection"
    /// 
    /// UsdRiPxrAovLight
    const TfToken inReflection;
    /// \brief "inRefraction"
    /// 
    /// UsdRiPxrAovLight
    const TfToken inRefraction;
    /// \brief "interpolation"
    /// 
    /// UsdSplineAPI - Interpolation attribute name
    const TfToken interpolation;
    /// \brief "invert"
    /// 
    /// UsdRiPxrAovLight
    const TfToken invert;
    /// \brief "latitude"
    /// 
    /// UsdRiPxrEnvDayLight
    const TfToken latitude;
    /// \brief "linear"
    /// 
    /// UsdSplineAPI - Linear spline interpolation, Possible value for UsdRiPxrRampLightFilter::GetRampModeAttr()
    const TfToken linear;
    /// \brief "longitude"
    /// 
    /// UsdRiPxrEnvDayLight
    const TfToken longitude;
    /// \brief "max"
    /// 
    /// Possible value for UsdRiLightFilterAPI::GetRiCombineModeAttr()
    const TfToken max;
    /// \brief "min"
    /// 
    /// Possible value for UsdRiLightFilterAPI::GetRiCombineModeAttr()
    const TfToken min;
    /// \brief "month"
    /// 
    /// UsdRiPxrEnvDayLight
    const TfToken month;
    /// \brief "multiply"
    /// 
    /// Possible value for UsdRiLightFilterAPI::GetRiCombineModeAttr()
    const TfToken multiply;
    /// \brief "noEffect"
    /// 
    /// Possible value for UsdRiPxrBarnLightFilter::GetPreBarnEffectAttr(), Default value for UsdRiPxrBarnLightFilter::GetPreBarnEffectAttr()
    const TfToken noEffect;
    /// \brief "noLight"
    /// 
    /// Possible value for UsdRiPxrBarnLightFilter::GetPreBarnEffectAttr()
    const TfToken noLight;
    /// \brief "off"
    /// 
    /// Possible value for UsdRiPxrCookieLightFilter::GetTextureWrapModeAttr(), Default value for UsdRiPxrCookieLightFilter::GetTextureWrapModeAttr()
    const TfToken off;
    /// \brief "onVolumeBoundaries"
    /// 
    /// UsdRiPxrAovLight
    const TfToken onVolumeBoundaries;
    /// \brief "outputs:ri:bxdf"
    /// 
    /// UsdRiMaterialAPI
    const TfToken outputsRiBxdf;
    /// \brief "outputs:ri:displacement"
    /// 
    /// UsdRiMaterialAPI
    const TfToken outputsRiDisplacement;
    /// \brief "outputs:ri:surface"
    /// 
    /// UsdRiMaterialAPI
    const TfToken outputsRiSurface;
    /// \brief "outputs:ri:volume"
    /// 
    /// UsdRiMaterialAPI
    const TfToken outputsRiVolume;
    /// \brief "physical"
    /// 
    /// Possible value for UsdRiPxrCookieLightFilter::GetCookieModeAttr(), Default value for UsdRiPxrCookieLightFilter::GetCookieModeAttr(), Possible value for UsdRiPxrBarnLightFilter::GetBarnModeAttr(), Default value for UsdRiPxrBarnLightFilter::GetBarnModeAttr()
    const TfToken physical;
    /// \brief "positions"
    /// 
    /// UsdSplineAPI - Positions attribute name
    const TfToken positions;
    /// \brief "preBarnEffect"
    /// 
    /// UsdRiPxrBarnLightFilter
    const TfToken preBarnEffect;
    /// \brief "radial"
    /// 
    /// Possible value for UsdRiPxrRampLightFilter::GetRampModeAttr()
    const TfToken radial;
    /// \brief "radius"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
    const TfToken radius;
    /// \brief "rampMode"
    /// 
    /// UsdRiPxrRampLightFilter
    const TfToken rampMode;
    /// \brief "refine:back"
    /// 
    /// UsdRiPxrRodLightFilter
    const TfToken refineBack;
    /// \brief "refine:bottom"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
    const TfToken refineBottom;
    /// \brief "refine:front"
    /// 
    /// UsdRiPxrRodLightFilter
    const TfToken refineFront;
    /// \brief "refine:left"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
    const TfToken refineLeft;
    /// \brief "refine:right"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
    const TfToken refineRight;
    /// \brief "refine:top"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
    const TfToken refineTop;
    /// \brief "repeat"
    /// 
    /// Possible value for UsdRiPxrCookieLightFilter::GetTextureWrapModeAttr()
    const TfToken repeat;
    /// \brief "ri:combineMode"
    /// 
    /// UsdRiLightFilterAPI
    const TfToken riCombineMode;
    /// \brief "ri:density"
    /// 
    /// UsdRiLightFilterAPI
    const TfToken riDensity;
    /// \brief "ri:diffuse"
    /// 
    /// UsdRiLightFilterAPI
    const TfToken riDiffuse;
    /// \brief "ri:exposure"
    /// 
    /// UsdRiLightFilterAPI
    const TfToken riExposure;
    /// \brief "ri:focusRegion"
    /// 
    /// UsdRiStatements
    const TfToken riFocusRegion;
    /// \brief "ri:intensity"
    /// 
    /// UsdRiLightFilterAPI
    const TfToken riIntensity;
    /// \brief "ri:intensityNearDist"
    /// 
    /// UsdRiLightAPI
    const TfToken riIntensityNearDist;
    /// \brief "ri:invert"
    /// 
    /// UsdRiLightFilterAPI
    const TfToken riInvert;
    /// \brief "ri:lightGroup"
    /// 
    /// UsdRiLightAPI
    const TfToken riLightGroup;
    /// \brief "ri:portal:intensity"
    /// 
    /// UsdRiLightPortalAPI
    const TfToken riPortalIntensity;
    /// \brief "ri:portal:tint"
    /// 
    /// UsdRiLightPortalAPI
    const TfToken riPortalTint;
    /// \brief "ri:sampling:fixedSampleCount"
    /// 
    /// UsdRiLightAPI
    const TfToken riSamplingFixedSampleCount;
    /// \brief "ri:sampling:importanceMultiplier"
    /// 
    /// UsdRiLightAPI
    const TfToken riSamplingImportanceMultiplier;
    /// \brief "ri:shadow:thinShadow"
    /// 
    /// UsdRiLightAPI
    const TfToken riShadowThinShadow;
    /// \brief "ri:specular"
    /// 
    /// UsdRiLightFilterAPI
    const TfToken riSpecular;
    /// \brief "ri:texture:gamma"
    /// 
    /// UsdRiTextureAPI
    const TfToken riTextureGamma;
    /// \brief "ri:texture:saturation"
    /// 
    /// UsdRiTextureAPI
    const TfToken riTextureSaturation;
    /// \brief "ri:trace:lightPaths"
    /// 
    /// UsdRiLightAPI
    const TfToken riTraceLightPaths;
    /// \brief "scale:depth"
    /// 
    /// UsdRiPxrRodLightFilter
    const TfToken scaleDepth;
    /// \brief "scale:height"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
    const TfToken scaleHeight;
    /// \brief "scale:width"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrBarnLightFilter
    const TfToken scaleWidth;
    /// \brief "screen"
    /// 
    /// Possible value for UsdRiLightFilterAPI::GetRiCombineModeAttr()
    const TfToken screen;
    /// \brief "skyTint"
    /// 
    /// UsdRiPxrEnvDayLight
    const TfToken skyTint;
    /// \brief "spherical"
    /// 
    /// Possible value for UsdRiPxrRampLightFilter::GetRampModeAttr()
    const TfToken spherical;
    /// \brief "spline"
    /// 
    /// UsdSplineAPI - Namespace for spline attributes
    const TfToken spline;
    /// \brief "sunDirection"
    /// 
    /// UsdRiPxrEnvDayLight
    const TfToken sunDirection;
    /// \brief "sunSize"
    /// 
    /// UsdRiPxrEnvDayLight
    const TfToken sunSize;
    /// \brief "sunTint"
    /// 
    /// UsdRiPxrEnvDayLight
    const TfToken sunTint;
    /// \brief "texture:fillColor"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken textureFillColor;
    /// \brief "texture:invertU"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken textureInvertU;
    /// \brief "texture:invertV"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken textureInvertV;
    /// \brief "texture:map"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken textureMap;
    /// \brief "texture:offsetU"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken textureOffsetU;
    /// \brief "texture:offsetV"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken textureOffsetV;
    /// \brief "texture:scaleU"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken textureScaleU;
    /// \brief "texture:scaleV"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken textureScaleV;
    /// \brief "texture:wrapMode"
    /// 
    /// UsdRiPxrCookieLightFilter
    const TfToken textureWrapMode;
    /// \brief "useColor"
    /// 
    /// UsdRiPxrAovLight
    const TfToken useColor;
    /// \brief "useThroughput"
    /// 
    /// UsdRiPxrAovLight
    const TfToken useThroughput;
    /// \brief "values"
    /// 
    /// UsdSplineAPI - values attribute name
    const TfToken values;
    /// \brief "width"
    /// 
    /// UsdRiPxrRodLightFilter, UsdRiPxrCookieLightFilter, UsdRiPxrBarnLightFilter
    const TfToken width;
    /// \brief "year"
    /// 
    /// UsdRiPxrEnvDayLight
    const TfToken year;
    /// \brief "zone"
    /// 
    /// UsdRiPxrEnvDayLight
    const TfToken zone;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdRiTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdRiTokensType
extern USDRI_API TfStaticData<UsdRiTokensType> UsdRiTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
