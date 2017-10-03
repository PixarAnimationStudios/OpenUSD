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
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \hideinitializer
#define USDSHADE_TOKENS \
    ((connectedSourceFor, "connectedSourceFor:")) \
    (derivesFrom) \
    (displacement) \
    (full) \
    ((infoId, "info:id")) \
    ((inputs, "inputs:")) \
    ((interface_, "interface:")) \
    (interfaceOnly) \
    ((interfaceRecipientsOf, "interfaceRecipientsOf:")) \
    ((lookBinding, "look:binding")) \
    (materialBind) \
    ((materialBinding, "material:binding")) \
    (materialVariant) \
    ((outputs, "outputs:")) \
    (surface)

/// \anchor UsdShadeTokens
///
/// <b>UsdShadeTokens</b> provides static, efficient TfToken's for
/// use in all public USD API
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdShadeTokens also contains all of the \em allowedTokens values declared
/// for schema builtin attributes of 'token' scene description type.
/// Use UsdShadeTokens like so:
///
/// \code
///     gprim.GetVisibilityAttr().Set(UsdShadeTokens->invisible);
/// \endcode
///
/// The tokens are:
/// \li <b>connectedSourceFor</b> - The prefix on UsdShadeShader relationships associated with a Parameter.  This prefixed relationship has a suffix matching the associated attribute name, and denotes a logical shader connection between UsdShadeShaders. 
/// \li <b>derivesFrom</b> - A legacy relationship name specifying a specializes composition on a UsdShadeMaterial. 
/// \li <b>displacement</b> - Describes the displacement relationship terminal on a UsdShadeMaterial. Used to find the terminal UsdShadeShader describing the displacement of a UsdShadeMaterial. 
/// \li <b>full</b> - Possible value for 'connectability' metadata on  a UsdShadeInput. When connectability of an input is set to  "full", it implies that it can be connected to any input or  output. 
/// \li <b>infoId</b> - UsdShadeShader
/// \li <b>inputs</b> - The prefix on shading attributes denoting an input. 
/// \li <b>interface_</b> - (DEPRECATED) The prefix on UsdShadeNodeGraph  attributes denoting an interface attribute. 
/// \li <b>interfaceOnly</b> - Possible value for 'connectability' metadata on  a UsdShadeInput. It implies that the input can only connect to  a NodeGraph Input (which represents an interface override, not  a render-time dataflow connection), or another Input whose  connectability is also 'interfaceOnly'. 
/// \li <b>interfaceRecipientsOf</b> - (DEPRECATED) The prefix on UsdShadeNodeGraph relationships denoting the target of an interface attribute. 
/// \li <b>lookBinding</b> - The relationship name on non shading prims to denote a binding to a UsdShadeLook. This is a deprecated relationship and is superceded by material:binding. 
/// \li <b>materialBind</b> - The name of the GeomSubset family used to  identify face subsets defined for the purpose of binding  materials to facesets. 
/// \li <b>materialBinding</b> -  The relationship name on non-shading prims to denote a binding to a UsdShadeMaterial. 
/// \li <b>materialVariant</b> - The variant name of material variation described on a UsdShadeMaterial. 
/// \li <b>outputs</b> - The prefix on shading attributes denoting an output. 
/// \li <b>surface</b> - Describes the surface relationship terminal on a UsdShadeMaterial. Used to find the terminal UsdShadeShader describing the surface of a UsdShadeMaterial. 
TF_DECLARE_PUBLIC_TOKENS(UsdShadeTokens, USDSHADE_API, USDSHADE_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
