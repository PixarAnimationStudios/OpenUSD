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
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

namespace foo { namespace bar { namespace baz {


/// \class UsdContrivedTokensType
///
/// \link UsdContrivedTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdContrivedTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdContrivedTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdContrivedTokens->binding);
/// \endcode
struct UsdContrivedTokensType {
    USDCONTRIVED_API UsdContrivedTokensType();
    /// \brief "binding"
    /// 
    /// UsdContrivedDerived
    const TfToken binding;
    /// \brief "cornerIndices"
    /// 
    /// UsdContrivedDerived
    const TfToken cornerIndices;
    /// \brief "cornerSharpnesses"
    /// 
    /// UsdContrivedDerived
    const TfToken cornerSharpnesses;
    /// \brief "creaseLengths"
    /// 
    /// UsdContrivedDerived
    const TfToken creaseLengths;
    /// \brief "holeIndices"
    /// 
    /// UsdContrivedDerived
    const TfToken holeIndices;
    /// \brief "justDefault"
    /// 
    /// UsdContrivedDerived
    const TfToken justDefault;
    /// \brief "libraryToken1"
    /// 
    /// Special token for the usdContrived library.
    const TfToken libraryToken1;
    /// \brief "/non-identifier-tokenValue!"
    /// 
    /// libraryToken2 doc
    const TfToken libraryToken2;
    /// \brief "myColorFloat"
    /// 
    /// UsdContrivedBase
    const TfToken myColorFloat;
    /// \brief "myDouble"
    /// 
    /// UsdContrivedBase
    const TfToken myDouble;
    /// \brief "myFloat"
    /// 
    /// UsdContrivedBase
    const TfToken myFloat;
    /// \brief "myNormals"
    /// 
    /// UsdContrivedBase
    const TfToken myNormals;
    /// \brief "myPoints"
    /// 
    /// UsdContrivedBase
    const TfToken myPoints;
    /// \brief "myUniformBool"
    /// 
    /// UsdContrivedBase
    const TfToken myUniformBool;
    /// \brief "myVaryingToken"
    /// 
    /// UsdContrivedBase
    const TfToken myVaryingToken;
    /// \brief "myVecfArray"
    /// 
    /// UsdContrivedDerived
    const TfToken myVecfArray;
    /// \brief "myVelocities"
    /// 
    /// UsdContrivedBase
    const TfToken myVelocities;
    /// \brief "namespaced:property"
    /// 
    /// UsdContrivedDerived
    const TfToken namespacedProperty;
    /// \brief "newToken"
    /// 
    /// Default value for UsdContrivedDerived::GetJustDefaultAttr()
    const TfToken newToken;
    /// \brief "pivotPosition"
    /// 
    /// UsdContrivedDerived
    const TfToken pivotPosition;
    /// \brief "relCanShareApiNameWithAttr"
    /// 
    /// UsdContrivedTestHairman
    const TfToken relCanShareApiNameWithAttr;
    /// \brief "riStatements:attributes:user:Gofur_GeomOnHairdensity"
    /// 
    /// UsdContrivedTestHairman
    const TfToken riStatementsAttributesUserGofur_GeomOnHairdensity;
    /// \brief "temp"
    /// 
    /// UsdContrivedTestHairman
    const TfToken temp;
    /// \brief "testingAsset"
    /// 
    /// UsdContrivedDerived
    const TfToken testingAsset;
    /// \brief "transform"
    /// 
    /// UsdContrivedDerived
    const TfToken transform;
    /// \brief "unsignedChar"
    /// 
    /// UsdContrivedBase
    const TfToken unsignedChar;
    /// \brief "unsignedInt"
    /// 
    /// UsdContrivedBase
    const TfToken unsignedInt;
    /// \brief "unsignedInt64Array"
    /// 
    /// UsdContrivedBase
    const TfToken unsignedInt64Array;
    /// \brief "VariableTokenAllowed1"
    /// 
    /// Possible value for UsdContrivedBase::GetMyVaryingTokenAttr()
    const TfToken variableTokenAllowed1;
    /// \brief "VariabletokenAllowed2"
    /// 
    /// Possible value for UsdContrivedBase::GetMyVaryingTokenAttr()
    const TfToken variabletokenAllowed2;
    /// \brief "VariableTokenDefault"
    /// 
    /// Default value for UsdContrivedBase::GetMyVaryingTokenAttr()
    const TfToken variableTokenDefault;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdContrivedTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdContrivedTokensType
extern USDCONTRIVED_API TfStaticData<UsdContrivedTokensType> UsdContrivedTokens;

}}}

#endif
