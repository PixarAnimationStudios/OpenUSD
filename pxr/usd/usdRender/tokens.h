//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    /// \brief "collection:cameraVisibility:includeRoot"
    /// 
    /// UsdRenderPass
    const TfToken collectionCameraVisibilityIncludeRoot;
    /// \brief "collection:renderVisibility:includeRoot"
    /// 
    /// UsdRenderPass
    const TfToken collectionRenderVisibilityIncludeRoot;
    /// \brief "color3f"
    /// 
    /// Fallback value for UsdRenderVar::GetDataTypeAttr()
    const TfToken color3f;
    /// \brief "command"
    /// 
    /// UsdRenderPass
    const TfToken command;
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
    /// \brief "deepRaster"
    /// 
    /// Possible value for UsdRenderProduct::GetProductTypeAttr()
    const TfToken deepRaster;
    /// \brief "disableDepthOfField"
    /// 
    /// UsdRenderSettingsBase
    const TfToken disableDepthOfField;
    /// \brief "disableMotionBlur"
    /// 
    /// UsdRenderSettingsBase
    const TfToken disableMotionBlur;
    /// \brief "expandAperture"
    /// 
    /// Fallback value for UsdRenderSettingsBase::GetAspectRatioConformPolicyAttr()
    const TfToken expandAperture;
    /// \brief "fileName"
    /// 
    /// UsdRenderPass
    const TfToken fileName;
    /// \brief "full"
    /// 
    /// Possible value for UsdRenderSettings::GetMaterialBindingPurposesAttr()
    const TfToken full;
    /// \brief "includedPurposes"
    /// 
    /// UsdRenderSettings
    const TfToken includedPurposes;
    /// \brief "inputPasses"
    /// 
    /// UsdRenderPass
    const TfToken inputPasses;
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
    /// \brief "passType"
    /// 
    /// UsdRenderPass
    const TfToken passType;
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
    /// Fallback value for UsdRenderProduct::GetProductTypeAttr()
    const TfToken raster;
    /// \brief "raw"
    /// 
    /// Fallback value for UsdRenderVar::GetSourceTypeAttr()
    const TfToken raw;
    /// \brief "renderingColorSpace"
    /// 
    /// UsdRenderSettings
    const TfToken renderingColorSpace;
    /// \brief "renderSettingsPrimPath"
    /// 
    /// Stage-level metadata that encodes the path to UsdRenderSettingsPrim to use for rendering.
    const TfToken renderSettingsPrimPath;
    /// \brief "renderSource"
    /// 
    /// UsdRenderPass
    const TfToken renderSource;
    /// \brief "renderVisibility"
    /// 
    ///  This token represents the collection name to use with UsdCollectionAPI to represent renderVisibility of a RenderPass prim. 
    const TfToken renderVisibility;
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
    /// \brief "RenderPass"
    /// 
    /// Schema identifer and family for UsdRenderPass
    const TfToken RenderPass;
    /// \brief "RenderProduct"
    /// 
    /// Schema identifer and family for UsdRenderProduct
    const TfToken RenderProduct;
    /// \brief "RenderSettings"
    /// 
    /// Schema identifer and family for UsdRenderSettings
    const TfToken RenderSettings;
    /// \brief "RenderSettingsBase"
    /// 
    /// Schema identifer and family for UsdRenderSettingsBase
    const TfToken RenderSettingsBase;
    /// \brief "RenderVar"
    /// 
    /// Schema identifer and family for UsdRenderVar
    const TfToken RenderVar;
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
