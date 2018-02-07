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
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdLuxTokensType
///
/// \link UsdLuxTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdLuxTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdLuxTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdLuxTokens->angle);
/// \endcode
struct UsdLuxTokensType {
    USDLUX_API UsdLuxTokensType();
    /// \brief "angle"
    /// 
    /// UsdLuxDistantLight
    const TfToken angle;
    /// \brief "angular"
    /// 
    /// Possible value for UsdLuxDomeLight::GetTextureFormatAttr()
    const TfToken angular;
    /// \brief "automatic"
    /// 
    /// Possible value for UsdLuxDomeLight::GetTextureFormatAttr(), Default value for UsdLuxDomeLight::GetTextureFormatAttr()
    const TfToken automatic;
    /// \brief "color"
    /// 
    /// UsdLuxLight
    const TfToken color;
    /// \brief "colorTemperature"
    /// 
    /// UsdLuxLight
    const TfToken colorTemperature;
    /// \brief "consumeAndContinue"
    /// 
    /// Possible value for UsdLuxListAPI::GetLightListCacheBehaviorAttr()
    const TfToken consumeAndContinue;
    /// \brief "consumeAndHalt"
    /// 
    /// Possible value for UsdLuxListAPI::GetLightListCacheBehaviorAttr()
    const TfToken consumeAndHalt;
    /// \brief "cubeMapVerticalCross"
    /// 
    /// Possible value for UsdLuxDomeLight::GetTextureFormatAttr()
    const TfToken cubeMapVerticalCross;
    /// \brief "diffuse"
    /// 
    /// UsdLuxLight
    const TfToken diffuse;
    /// \brief "enableColorTemperature"
    /// 
    /// UsdLuxLight
    const TfToken enableColorTemperature;
    /// \brief "exposure"
    /// 
    /// UsdLuxLight
    const TfToken exposure;
    /// \brief "filters"
    /// 
    /// UsdLuxLight
    const TfToken filters;
    /// \brief "geometry"
    /// 
    /// UsdLuxGeometryLight
    const TfToken geometry;
    /// \brief "height"
    /// 
    /// UsdLuxRectLight
    const TfToken height;
    /// \brief "ignore"
    /// 
    /// Possible value for UsdLuxListAPI::GetLightListCacheBehaviorAttr()
    const TfToken ignore;
    /// \brief "intensity"
    /// 
    /// UsdLuxDistantLight, UsdLuxLight
    const TfToken intensity;
    /// \brief "latlong"
    /// 
    /// Possible value for UsdLuxDomeLight::GetTextureFormatAttr()
    const TfToken latlong;
    /// \brief "lightList"
    /// 
    /// UsdLuxListAPI
    const TfToken lightList;
    /// \brief "lightList:cacheBehavior"
    /// 
    /// UsdLuxListAPI
    const TfToken lightListCacheBehavior;
    /// \brief "mirroredBall"
    /// 
    /// Possible value for UsdLuxDomeLight::GetTextureFormatAttr()
    const TfToken mirroredBall;
    /// \brief "normalize"
    /// 
    /// UsdLuxLight
    const TfToken normalize;
    /// \brief "portals"
    /// 
    /// UsdLuxDomeLight
    const TfToken portals;
    /// \brief "radius"
    /// 
    /// UsdLuxSphereLight, UsdLuxDiskLight
    const TfToken radius;
    /// \brief "shadow:color"
    /// 
    /// UsdLuxShadowAPI
    const TfToken shadowColor;
    /// \brief "shadow:distance"
    /// 
    /// UsdLuxShadowAPI
    const TfToken shadowDistance;
    /// \brief "shadow:enable"
    /// 
    /// UsdLuxShadowAPI
    const TfToken shadowEnable;
    /// \brief "shadow:exclude"
    /// 
    /// UsdLuxShadowAPI
    const TfToken shadowExclude;
    /// \brief "shadow:falloff"
    /// 
    /// UsdLuxShadowAPI
    const TfToken shadowFalloff;
    /// \brief "shadow:falloffGamma"
    /// 
    /// UsdLuxShadowAPI
    const TfToken shadowFalloffGamma;
    /// \brief "shadow:include"
    /// 
    /// UsdLuxShadowAPI
    const TfToken shadowInclude;
    /// \brief "shaping:cone:angle"
    /// 
    /// UsdLuxShapingAPI
    const TfToken shapingConeAngle;
    /// \brief "shaping:cone:softness"
    /// 
    /// UsdLuxShapingAPI
    const TfToken shapingConeSoftness;
    /// \brief "shaping:focus"
    /// 
    /// UsdLuxShapingAPI
    const TfToken shapingFocus;
    /// \brief "shaping:focusTint"
    /// 
    /// UsdLuxShapingAPI
    const TfToken shapingFocusTint;
    /// \brief "shaping:ies:angleScale"
    /// 
    /// UsdLuxShapingAPI
    const TfToken shapingIesAngleScale;
    /// \brief "shaping:ies:file"
    /// 
    /// UsdLuxShapingAPI
    const TfToken shapingIesFile;
    /// \brief "specular"
    /// 
    /// UsdLuxLight
    const TfToken specular;
    /// \brief "texture:file"
    /// 
    /// UsdLuxDomeLight, UsdLuxRectLight
    const TfToken textureFile;
    /// \brief "texture:format"
    /// 
    /// UsdLuxDomeLight
    const TfToken textureFormat;
    /// \brief "width"
    /// 
    /// UsdLuxRectLight
    const TfToken width;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdLuxTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdLuxTokensType
extern USDLUX_API TfStaticData<UsdLuxTokensType> UsdLuxTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
