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
#ifndef USDRENDER_TOKENS_H
#define USDRENDER_TOKENS_H

/// \file usdRender/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdRender/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdRenderTokensType
///
/// \link UsdRenderTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdRenderTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdRenderTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdRenderTokens->adjustApertureHeight);
/// \endcode
struct UsdRenderTokensType {
    USDRENDER_API UsdRenderTokensType();
    /// \brief "adjustApertureHeight"
    /// 
    /// Possible value for UsdRenderSettingsBase::GetAspectRatioConformPolicyAttr()
    const TfToken adjustApertureHeight;
    /// \brief "adjustApertureWidth"
    /// 
    /// Possible value for UsdRenderSettingsBase::GetAspectRatioConformPolicyAttr()
    const TfToken adjustApertureWidth;
    /// \brief "adjustPixelAspectRatio"
    /// 
    /// Possible value for UsdRenderSettingsBase::GetAspectRatioConformPolicyAttr()
    const TfToken adjustPixelAspectRatio;
    /// \brief "aspectRatioConformPolicy"
    /// 
    /// UsdRenderSettingsBase
    const TfToken aspectRatioConformPolicy;
    /// \brief "camera"
    /// 
    /// UsdRenderSettingsBase
    const TfToken camera;
    /// \brief "color3f"
    /// 
    /// Default value for UsdRenderVar::GetDataTypeAttr()
    const TfToken color3f;
    /// \brief "cropAperture"
    /// 
    /// Possible value for UsdRenderSettingsBase::GetAspectRatioConformPolicyAttr()
    const TfToken cropAperture;
    /// \brief "dataType"
    /// 
    /// UsdRenderVar
    const TfToken dataType;
    /// \brief "dataWindowNDC"
    /// 
    /// UsdRenderSettingsBase
    const TfToken dataWindowNDC;
    /// \brief "expandAperture"
    /// 
    /// Possible value for UsdRenderSettingsBase::GetAspectRatioConformPolicyAttr(), Default value for UsdRenderSettingsBase::GetAspectRatioConformPolicyAttr()
    const TfToken expandAperture;
    /// \brief "full"
    /// 
    /// Possible value for UsdRenderSettings::GetMaterialBindingPurposesAttr()
    const TfToken full;
    /// \brief "includedPurposes"
    /// 
    /// UsdRenderSettings
    const TfToken includedPurposes;
    /// \brief "instantaneousShutter"
    /// 
    /// UsdRenderSettingsBase
    const TfToken instantaneousShutter;
    /// \brief "intrinsic"
    /// 
    /// Possible value for UsdRenderVar::GetSourceTypeAttr()
    const TfToken intrinsic;
    /// \brief "lpe"
    /// 
    /// Possible value for UsdRenderVar::GetSourceTypeAttr()
    const TfToken lpe;
    /// \brief "materialBindingPurposes"
    /// 
    /// UsdRenderSettings
    const TfToken materialBindingPurposes;
    /// \brief "orderedVars"
    /// 
    /// UsdRenderProduct
    const TfToken orderedVars;
    /// \brief "pixelAspectRatio"
    /// 
    /// UsdRenderSettingsBase
    const TfToken pixelAspectRatio;
    /// \brief "preview"
    /// 
    /// Possible value for UsdRenderSettings::GetMaterialBindingPurposesAttr()
    const TfToken preview;
    /// \brief "primvar"
    /// 
    /// Possible value for UsdRenderVar::GetSourceTypeAttr()
    const TfToken primvar;
    /// \brief "productName"
    /// 
    /// UsdRenderProduct
    const TfToken productName;
    /// \brief "products"
    /// 
    /// UsdRenderSettings
    const TfToken products;
    /// \brief "productType"
    /// 
    /// UsdRenderProduct
    const TfToken productType;
    /// \brief "raster"
    /// 
    /// RenderProduct productType value that indicates a 2D raster image of pixels., Default value for UsdRenderProduct::GetProductTypeAttr()
    const TfToken raster;
    /// \brief "raw"
    /// 
    /// Possible value for UsdRenderVar::GetSourceTypeAttr(), Default value for UsdRenderVar::GetSourceTypeAttr()
    const TfToken raw;
    /// \brief "renderSettingsPrimPath"
    /// 
    /// Stage-level metadata that encodes the path to UsdRenderSettingsPrim to use for rendering.
    const TfToken renderSettingsPrimPath;
    /// \brief "resolution"
    /// 
    /// UsdRenderSettingsBase
    const TfToken resolution;
    /// \brief "sourceName"
    /// 
    /// UsdRenderVar
    const TfToken sourceName;
    /// \brief "sourceType"
    /// 
    /// UsdRenderVar
    const TfToken sourceType;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdRenderTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdRenderTokensType
extern USDRENDER_API TfStaticData<UsdRenderTokensType> UsdRenderTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
