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
#ifndef USDSHADE_TOKENS_H
#define USDSHADE_TOKENS_H

/// \file usdShade/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdShadeTokensType
///
/// \link UsdShadeTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdShadeTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdShadeTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdShadeTokens->allPurpose);
/// \endcode
struct UsdShadeTokensType {
    USDSHADE_API UsdShadeTokensType();
    /// \brief ""
    /// 
    /// Possible value for the 'materialPurpose' parameter in the various methods available in UsdShadeMaterialBindingAPI. Its value is empty and its purpose is to represent a general  purpose material-binding that applies in the absence of a  specific-purpose binding. 
    const TfToken allPurpose;
    /// \brief "bindMaterialAs"
    /// 
    /// Token valued metadata key authored on a material  binding relationship to indicate the strength of the binding  relative to bindings authored on descendants. 
    const TfToken bindMaterialAs;
    /// \brief "connectedSourceFor:"
    /// 
    /// The prefix on UsdShadeShader relationships associated with a Parameter.  This prefixed relationship has a suffix matching the associated attribute name, and denotes a logical shader connection between UsdShadeShaders. 
    const TfToken connectedSourceFor;
    /// \brief "derivesFrom"
    /// 
    /// A legacy relationship name specifying a specializes composition on a UsdShadeMaterial. 
    const TfToken derivesFrom;
    /// \brief "displacement"
    /// 
    /// Describes the <i>displacement</i> output  terminal on a UsdShadeMaterial. It is used to define the  terminal UsdShadeShader describing the displacement of a  UsdShadeMaterial. 
    const TfToken displacement;
    /// \brief "fallbackStrength"
    /// 
    /// Sentinal value to be used for 'bindMaterialAs'  metadata's default value. Clients should pass this in for the  'bindingStrength' argument to UsdShadeMaterialBindingAPI::Bind(), if they want to author the default value (weakerThanDescendants) sparsely. The value "fallbackStrength" never gets authored  into scene description.
    const TfToken fallbackStrength;
    /// \brief "full"
    /// 
    /// Possible value for the 'materialPurpose'  parameter in UsdShadeMaterialBindingAPI, to be used when the  purpose of the render is entirely about visualizing the truest representation of a scene, considering all lighting and material information, at highest fidelity.  Also a possible value for 'connectability' metadata on  a UsdShadeInput. When connectability of an input is set to  "full", it implies that it can be connected to any input or  output. 
    const TfToken full;
    /// \brief "id"
    /// 
    /// Possible value for UsdShadeShader::GetInfoImplementationSourceAttr(), Default value for UsdShadeShader::GetInfoImplementationSourceAttr()
    const TfToken id;
    /// \brief "info:id"
    /// 
    /// UsdShadeShader
    const TfToken infoId;
    /// \brief "info:implementationSource"
    /// 
    /// UsdShadeShader
    const TfToken infoImplementationSource;
    /// \brief "inputs:"
    /// 
    /// The prefix on shading attributes denoting an input. 
    const TfToken inputs;
    /// \brief "interface:"
    /// 
    /// (DEPRECATED) The prefix on UsdShadeNodeGraph  attributes denoting an interface attribute. 
    const TfToken interface_;
    /// \brief "interfaceOnly"
    /// 
    /// Possible value for 'connectability' metadata on  a UsdShadeInput. It implies that the input can only connect to  a NodeGraph Input (which represents an interface override, not  a render-time dataflow connection), or another Input whose  connectability is also 'interfaceOnly'. 
    const TfToken interfaceOnly;
    /// \brief "interfaceRecipientsOf:"
    /// 
    /// (DEPRECATED) The prefix on UsdShadeNodeGraph relationships denoting the target of an interface attribute. 
    const TfToken interfaceRecipientsOf;
    /// \brief "materialBind"
    /// 
    /// The name of the GeomSubset family used to  identify face subsets defined for the purpose of binding  materials to facesets. 
    const TfToken materialBind;
    /// \brief "material:binding"
    /// 
    ///  The relationship name on non-shading prims to denote a binding to a UsdShadeMaterial. 
    const TfToken materialBinding;
    /// \brief "material:binding:collection"
    /// 
    ///  The relationship name on non-shading prims to denote a collection-based binding to a UsdShadeMaterial. 
    const TfToken materialBindingCollection;
    /// \brief "materialVariant"
    /// 
    /// The variant name of material variation described on a UsdShadeMaterial. 
    const TfToken materialVariant;
    /// \brief "outputs:"
    /// 
    /// The prefix on shading attributes denoting an output. 
    const TfToken outputs;
    /// \brief "outputs:displacement"
    /// 
    /// UsdShadeMaterial
    const TfToken outputsDisplacement;
    /// \brief "outputs:surface"
    /// 
    /// UsdShadeMaterial
    const TfToken outputsSurface;
    /// \brief "outputs:volume"
    /// 
    /// UsdShadeMaterial
    const TfToken outputsVolume;
    /// \brief "preview"
    /// 
    /// Possible value for the 'materialPurpose'  parameter in UsdShadeMaterialBindingAPI, to be used when the  render is in service of a goal other than a high fidelity "full" render (such as scene manipulation, modeling, or realtime  playback). Latency and speed are generally of greater concern  for preview renders, therefore preview materials are generally  designed to be "lighterweight" compared to full materials. 
    const TfToken preview;
    /// \brief "shaderMetadata"
    /// 
    /// Dictionary valued metadata key authored on a  Shader prim with implementationSource value of sourceAsset or  sourceCode to pass along metadata to the shader parser or  compiler. 
    const TfToken shaderMetadata;
    /// \brief "sourceAsset"
    /// 
    /// Possible value for UsdShadeShader::GetInfoImplementationSourceAttr()
    const TfToken sourceAsset;
    /// \brief "sourceCode"
    /// 
    /// Possible value for UsdShadeShader::GetInfoImplementationSourceAttr()
    const TfToken sourceCode;
    /// \brief "strongerThanDescendants"
    /// 
    /// Possible value for 'bindMaterialAs' metadata on the  collection-based material binding relationship. Indicates  that the binding represented by the relationship is weaker than  any bindings authored on the descendants.
    const TfToken strongerThanDescendants;
    /// \brief "surface"
    /// 
    /// Describes the <i>surface</i> output  terminal on a UsdShadeMaterial. It is used to define the  terminal UsdShadeShader describing the surface of a  UsdShadeMaterial. 
    const TfToken surface;
    /// \brief ""
    /// 
    /// Possible value for the "renderContext" parameter in \ref UsdShadeMaterial_Outputs API. Represents the universal renderContext. An output with a universal renderContext is  applicable to all possible rendering contexts. 
    const TfToken universalRenderContext;
    /// \brief ""
    /// 
    /// Possible value for the "sourceType" parameter  in \ref UsdShadeShader_ImplementationSource API. Represents  the universal or fallback source type. 
    const TfToken universalSourceType;
    /// \brief "volume"
    /// 
    /// Describes the <i>volume</i> output  terminal on a UsdShadeMaterial. It is used to define the  terminal UsdShadeShader describing the volume of a  UsdShadeMaterial. 
    const TfToken volume;
    /// \brief "weakerThanDescendants"
    /// 
    /// Possible value for 'bindMaterialAs' metadata on the  collection-based material binding relationship. Indicates  that the binding represented by the relationship is weaker than  any bindings authored on the descendants.
    const TfToken weakerThanDescendants;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdShadeTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdShadeTokensType
extern USDSHADE_API TfStaticData<UsdShadeTokensType> UsdShadeTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
