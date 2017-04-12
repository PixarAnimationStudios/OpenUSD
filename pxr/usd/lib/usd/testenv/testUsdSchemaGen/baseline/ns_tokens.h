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
#ifndef USDCONTRIVED_TOKENS_H
#define USDCONTRIVED_TOKENS_H

/// \file usdContrived/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdContrived/api.h"
#include "pxr/base/tf/staticTokens.h"

namespace foo {

/// \hideinitializer
#define USDCONTRIVED_TOKENS \
    (binding) \
    (cornerIndices) \
    (cornerSharpnesses) \
    (creaseLengths) \
    (holeIndices) \
    (justDefault) \
    (libraryToken1) \
    ((libraryToken2, "/non-identifier-tokenValue!")) \
    (myColorFloat) \
    (myDouble) \
    (myFloat) \
    (myNormals) \
    (myPoints) \
    (myUniformBool) \
    (myVaryingToken) \
    (myVecfArray) \
    (myVelocities) \
    ((namespacedProperty, "namespaced:property")) \
    (newToken) \
    (pivotPosition) \
    (relCanShareApiNameWithAttr) \
    ((riStatementsAttributesUserGofur_GeomOnHairdensity, "riStatements:attributes:user:Gofur_GeomOnHairdensity")) \
    (temp) \
    (testingAsset) \
    (transform) \
    (unsignedChar) \
    (unsignedInt) \
    (unsignedInt64Array) \
    ((variableTokenAllowed1, "VariableTokenAllowed1")) \
    ((variabletokenAllowed2, "VariabletokenAllowed2")) \
    ((variableTokenDefault, "VariableTokenDefault"))

/// \anchor UsdContrivedTokens
///
/// <b>UsdContrivedTokens</b> provides static, efficient TfToken's for
/// use in all public USD API
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdContrivedTokens also contains all of the \em allowedTokens values declared
/// for schema builtin attributes of 'token' scene description type.
/// Use UsdContrivedTokens like so:
///
/// \code
///     gprim.GetVisibilityAttr().Set(UsdContrivedTokens->invisible);
/// \endcode
///
/// The tokens are:
/// \li <b>binding</b> - UsdContrivedDerived
/// \li <b>cornerIndices</b> - UsdContrivedDerived
/// \li <b>cornerSharpnesses</b> - UsdContrivedDerived
/// \li <b>creaseLengths</b> - UsdContrivedDerived
/// \li <b>holeIndices</b> - UsdContrivedDerived
/// \li <b>justDefault</b> - UsdContrivedDerived
/// \li <b>libraryToken1</b> - Special token for the usdContrived library.
/// \li <b>libraryToken2</b> - libraryToken2 doc
/// \li <b>myColorFloat</b> - UsdContrivedBase
/// \li <b>myDouble</b> - UsdContrivedBase
/// \li <b>myFloat</b> - UsdContrivedBase
/// \li <b>myNormals</b> - UsdContrivedBase
/// \li <b>myPoints</b> - UsdContrivedBase
/// \li <b>myUniformBool</b> - UsdContrivedBase
/// \li <b>myVaryingToken</b> - UsdContrivedBase
/// \li <b>myVecfArray</b> - UsdContrivedDerived
/// \li <b>myVelocities</b> - UsdContrivedBase
/// \li <b>namespacedProperty</b> - UsdContrivedDerived
/// \li <b>newToken</b> - Default value for UsdContrivedDerived::GetJustDefaultAttr()
/// \li <b>pivotPosition</b> - UsdContrivedDerived
/// \li <b>relCanShareApiNameWithAttr</b> - UsdContrivedTestHairman
/// \li <b>riStatementsAttributesUserGofur_GeomOnHairdensity</b> - UsdContrivedTestHairman
/// \li <b>temp</b> - UsdContrivedTestHairman
/// \li <b>testingAsset</b> - UsdContrivedDerived
/// \li <b>transform</b> - UsdContrivedDerived
/// \li <b>unsignedChar</b> - UsdContrivedBase
/// \li <b>unsignedInt</b> - UsdContrivedBase
/// \li <b>unsignedInt64Array</b> - UsdContrivedBase
/// \li <b>variableTokenAllowed1</b> - Possible value for UsdContrivedBase::GetMyVaryingTokenAttr()
/// \li <b>variabletokenAllowed2</b> - Possible value for UsdContrivedBase::GetMyVaryingTokenAttr()
/// \li <b>variableTokenDefault</b> - Default value for UsdContrivedBase::GetMyVaryingTokenAttr()
TF_DECLARE_PUBLIC_TOKENS(UsdContrivedTokens, USDCONTRIVED_API, USDCONTRIVED_TOKENS);

}

#endif
