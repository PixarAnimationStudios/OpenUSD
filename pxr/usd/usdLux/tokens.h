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
///     gprim.GetMyTokenValuedAttr().Set(UsdLuxTokens->angular);
/// \endcode
struct UsdLuxTokensType {
    USDLUX_API UsdLuxTokensType();
    /// \brief "angular"
    /// 
    /// Possible value for UsdLuxDomeLight::GetTextureFormatAttr()
    const TfToken angular;
    /// \brief "automatic"
    /// 
    /// Fallback value for UsdLuxDomeLight::GetTextureFormatAttr()
    const TfToken automatic;
    /// \brief "collection:filterLink:includeRoot"
    /// 
    /// UsdLuxLightFilter
    const TfToken collectionFilterLinkIncludeRoot;
    /// \brief "collection:lightLink:includeRoot"
    /// 
    /// UsdLuxLightAPI
    const TfToken collectionLightLinkIncludeRoot;
    /// \brief "collection:shadowLink:includeRoot"
    /// 
    /// UsdLuxLightAPI
    const TfToken collectionShadowLinkIncludeRoot;
    /// \brief "consumeAndContinue"
    /// 
    /// Possible value for UsdLuxLightListAPI::GetLightListCacheBehaviorAttr(), Possible value for UsdLuxListAPI::GetLightListCacheBehaviorAttr()
    const TfToken consumeAndContinue;
    /// \brief "consumeAndHalt"
    /// 
    /// Possible value for UsdLuxLightListAPI::GetLightListCacheBehaviorAttr(), Possible value for UsdLuxListAPI::GetLightListCacheBehaviorAttr()
    const TfToken consumeAndHalt;
    /// \brief "cubeMapVerticalCross"
    /// 
    /// Possible value for UsdLuxDomeLight::GetTextureFormatAttr()
    const TfToken cubeMapVerticalCross;
    /// \brief "extent"
    /// 
    /// UsdLuxPortalLight
    const TfToken extent;
    /// \brief "filterLink"
    /// 
    ///  This token represents the collection name to use with UsdCollectionAPI to represent filter-linking of a UsdLuxLightFilter prim. 
    const TfToken filterLink;
    /// \brief "geometry"
    /// 
    /// UsdLuxGeometryLight
    const TfToken geometry;
    /// \brief "guideRadius"
    /// 
    /// UsdLuxDomeLight
    const TfToken guideRadius;
    /// \brief "ignore"
    /// 
    /// Possible value for UsdLuxLightListAPI::GetLightListCacheBehaviorAttr(), Possible value for UsdLuxListAPI::GetLightListCacheBehaviorAttr()
    const TfToken ignore;
    /// \brief "independent"
    /// 
    /// Possible value for UsdLuxLightAPI::GetMaterialSyncModeAttr()
    const TfToken independent;
    /// \brief "inputs:angle"
    /// 
    /// UsdLuxDistantLight
    const TfToken inputsAngle;
    /// \brief "inputs:color"
    /// 
    /// UsdLuxLightAPI
    const TfToken inputsColor;
    /// \brief "inputs:colorTemperature"
    /// 
    /// UsdLuxLightAPI
    const TfToken inputsColorTemperature;
    /// \brief "inputs:diffuse"
    /// 
    /// UsdLuxLightAPI
    const TfToken inputsDiffuse;
    /// \brief "inputs:enableColorTemperature"
    /// 
    /// UsdLuxLightAPI
    const TfToken inputsEnableColorTemperature;
    /// \brief "inputs:exposure"
    /// 
    /// UsdLuxLightAPI
    const TfToken inputsExposure;
    /// \brief "inputs:height"
    /// 
    /// UsdLuxRectLight
    const TfToken inputsHeight;
    /// \brief "inputs:intensity"
    /// 
    /// UsdLuxLightAPI, UsdLuxDistantLight
    const TfToken inputsIntensity;
    /// \brief "inputs:length"
    /// 
    /// UsdLuxCylinderLight
    const TfToken inputsLength;
    /// \brief "inputs:normalize"
    /// 
    /// UsdLuxLightAPI
    const TfToken inputsNormalize;
    /// \brief "inputs:radius"
    /// 
    /// UsdLuxDiskLight, UsdLuxSphereLight, UsdLuxCylinderLight
    const TfToken inputsRadius;
    /// \brief "inputs:shadow:color"
    /// 
    /// UsdLuxShadowAPI
    const TfToken inputsShadowColor;
    /// \brief "inputs:shadow:distance"
    /// 
    /// UsdLuxShadowAPI
    const TfToken inputsShadowDistance;
    /// \brief "inputs:shadow:enable"
    /// 
    /// UsdLuxShadowAPI
    const TfToken inputsShadowEnable;
    /// \brief "inputs:shadow:falloff"
    /// 
    /// UsdLuxShadowAPI
    const TfToken inputsShadowFalloff;
    /// \brief "inputs:shadow:falloffGamma"
    /// 
    /// UsdLuxShadowAPI
    const TfToken inputsShadowFalloffGamma;
    /// \brief "inputs:shaping:cone:angle"
    /// 
    /// UsdLuxShapingAPI
    const TfToken inputsShapingConeAngle;
    /// \brief "inputs:shaping:cone:softness"
    /// 
    /// UsdLuxShapingAPI
    const TfToken inputsShapingConeSoftness;
    /// \brief "inputs:shaping:focus"
    /// 
    /// UsdLuxShapingAPI
    const TfToken inputsShapingFocus;
    /// \brief "inputs:shaping:focusTint"
    /// 
    /// UsdLuxShapingAPI
    const TfToken inputsShapingFocusTint;
    /// \brief "inputs:shaping:ies:angleScale"
    /// 
    /// UsdLuxShapingAPI
    const TfToken inputsShapingIesAngleScale;
    /// \brief "inputs:shaping:ies:file"
    /// 
    /// UsdLuxShapingAPI
    const TfToken inputsShapingIesFile;
    /// \brief "inputs:shaping:ies:normalize"
    /// 
    /// UsdLuxShapingAPI
    const TfToken inputsShapingIesNormalize;
    /// \brief "inputs:specular"
    /// 
    /// UsdLuxLightAPI
    const TfToken inputsSpecular;
    /// \brief "inputs:texture:file"
    /// 
    /// UsdLuxRectLight, UsdLuxDomeLight
    const TfToken inputsTextureFile;
    /// \brief "inputs:texture:format"
    /// 
    /// UsdLuxDomeLight
    const TfToken inputsTextureFormat;
    /// \brief "inputs:width"
    /// 
    /// UsdLuxRectLight
    const TfToken inputsWidth;
    /// \brief "latlong"
    /// 
    /// Possible value for UsdLuxDomeLight::GetTextureFormatAttr()
    const TfToken latlong;
    /// \brief "light:filters"
    /// 
    /// UsdLuxLightAPI
    const TfToken lightFilters;
    /// \brief "lightFilter:shaderId"
    /// 
    /// UsdLuxLightFilter
    const TfToken lightFilterShaderId;
    /// \brief "lightLink"
    /// 
    ///  This token represents the collection name to use with UsdCollectionAPI to represent light-linking of a prim with an applied UsdLuxLightAPI. 
    const TfToken lightLink;
    /// \brief "lightList"
    /// 
    /// UsdLuxLightListAPI, UsdLuxListAPI
    const TfToken lightList;
    /// \brief "lightList:cacheBehavior"
    /// 
    /// UsdLuxLightListAPI, UsdLuxListAPI
    const TfToken lightListCacheBehavior;
    /// \brief "light:materialSyncMode"
    /// 
    /// UsdLuxLightAPI, UsdLuxMeshLightAPI, UsdLuxVolumeLightAPI
    const TfToken lightMaterialSyncMode;
    /// \brief "light:shaderId"
    /// 
    /// UsdLuxLightAPI, UsdLuxMeshLightAPI, UsdLuxVolumeLightAPI, UsdLuxDistantLight, UsdLuxDiskLight, UsdLuxRectLight, UsdLuxSphereLight, UsdLuxCylinderLight, UsdLuxGeometryLight, UsdLuxDomeLight, UsdLuxPortalLight
    const TfToken lightShaderId;
    /// \brief "materialGlowTintsLight"
    /// 
    /// Possible value for UsdLuxLightAPI::GetMaterialSyncModeAttr(), Fallback value for UsdLuxMeshLightAPI schema attribute light:materialSyncMode, Fallback value for UsdLuxVolumeLightAPI schema attribute light:materialSyncMode
    const TfToken materialGlowTintsLight;
    /// \brief "MeshLight"
    /// 
    /// Fallback value for UsdLuxMeshLightAPI schema attribute light:shaderId
    const TfToken MeshLight;
    /// \brief "mirroredBall"
    /// 
    /// Possible value for UsdLuxDomeLight::GetTextureFormatAttr()
    const TfToken mirroredBall;
    /// \brief "noMaterialResponse"
    /// 
    /// Fallback value for UsdLuxLightAPI::GetMaterialSyncModeAttr()
    const TfToken noMaterialResponse;
    /// \brief "orientToStageUpAxis"
    /// 
    ///  This token represents the suffix for a UsdGeomXformOp used to orient a light with the stage's up axis. 
    const TfToken orientToStageUpAxis;
    /// \brief "portals"
    /// 
    /// UsdLuxDomeLight
    const TfToken portals;
    /// \brief "shadowLink"
    /// 
    ///  This token represents the collection name to use with UsdCollectionAPI to represent shadow-linking of a prim with an applied UsdLuxLightAPI. 
    const TfToken shadowLink;
    /// \brief "treatAsLine"
    /// 
    /// UsdLuxCylinderLight
    const TfToken treatAsLine;
    /// \brief "treatAsPoint"
    /// 
    /// UsdLuxSphereLight
    const TfToken treatAsPoint;
    /// \brief "VolumeLight"
    /// 
    /// Fallback value for UsdLuxVolumeLightAPI schema attribute light:shaderId
    const TfToken VolumeLight;
    /// \brief "BoundableLightBase"
    /// 
    /// Schema identifer and family for UsdLuxBoundableLightBase
    const TfToken BoundableLightBase;
    /// \brief "CylinderLight"
    /// 
    /// Schema identifer and family for UsdLuxCylinderLight, Fallback value for UsdLuxCylinderLight schema attribute light:shaderId
    const TfToken CylinderLight;
    /// \brief "DiskLight"
    /// 
    /// Schema identifer and family for UsdLuxDiskLight, Fallback value for UsdLuxDiskLight schema attribute light:shaderId
    const TfToken DiskLight;
    /// \brief "DistantLight"
    /// 
    /// Schema identifer and family for UsdLuxDistantLight, Fallback value for UsdLuxDistantLight schema attribute light:shaderId
    const TfToken DistantLight;
    /// \brief "DomeLight"
    /// 
    /// Schema identifer and family for UsdLuxDomeLight, Fallback value for UsdLuxDomeLight schema attribute light:shaderId
    const TfToken DomeLight;
    /// \brief "GeometryLight"
    /// 
    /// Schema identifer and family for UsdLuxGeometryLight, Fallback value for UsdLuxGeometryLight schema attribute light:shaderId
    const TfToken GeometryLight;
    /// \brief "LightAPI"
    /// 
    /// Schema identifer and family for UsdLuxLightAPI
    const TfToken LightAPI;
    /// \brief "LightFilter"
    /// 
    /// Schema identifer and family for UsdLuxLightFilter
    const TfToken LightFilter;
    /// \brief "LightListAPI"
    /// 
    /// Schema identifer and family for UsdLuxLightListAPI
    const TfToken LightListAPI;
    /// \brief "ListAPI"
    /// 
    /// Schema identifer and family for UsdLuxListAPI
    const TfToken ListAPI;
    /// \brief "MeshLightAPI"
    /// 
    /// Schema identifer and family for UsdLuxMeshLightAPI
    const TfToken MeshLightAPI;
    /// \brief "NonboundableLightBase"
    /// 
    /// Schema identifer and family for UsdLuxNonboundableLightBase
    const TfToken NonboundableLightBase;
    /// \brief "PluginLight"
    /// 
    /// Schema identifer and family for UsdLuxPluginLight
    const TfToken PluginLight;
    /// \brief "PluginLightFilter"
    /// 
    /// Schema identifer and family for UsdLuxPluginLightFilter
    const TfToken PluginLightFilter;
    /// \brief "PortalLight"
    /// 
    /// Schema identifer and family for UsdLuxPortalLight, Fallback value for UsdLuxPortalLight schema attribute light:shaderId
    const TfToken PortalLight;
    /// \brief "RectLight"
    /// 
    /// Schema identifer and family for UsdLuxRectLight, Fallback value for UsdLuxRectLight schema attribute light:shaderId
    const TfToken RectLight;
    /// \brief "ShadowAPI"
    /// 
    /// Schema identifer and family for UsdLuxShadowAPI
    const TfToken ShadowAPI;
    /// \brief "ShapingAPI"
    /// 
    /// Schema identifer and family for UsdLuxShapingAPI
    const TfToken ShapingAPI;
    /// \brief "SphereLight"
    /// 
    /// Schema identifer and family for UsdLuxSphereLight, Fallback value for UsdLuxSphereLight schema attribute light:shaderId
    const TfToken SphereLight;
    /// \brief "VolumeLightAPI"
    /// 
    /// Schema identifer and family for UsdLuxVolumeLightAPI
    const TfToken VolumeLightAPI;
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
