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

PXR_NAMESPACE_OPEN_SCOPE


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
///     gprim.GetMyTokenValuedAttr().Set(UsdContrivedTokens->asset);
/// \endcode
struct UsdContrivedTokensType {
    USDCONTRIVED_API UsdContrivedTokensType();
    /// \brief "asset"
    /// 
    /// UsdContrivedBase
    const TfToken asset;
    /// \brief "assetArray"
    /// 
    /// UsdContrivedBase
    const TfToken assetArray;
    /// \brief "attrWithoutGeneratedAccessorAPI"
    /// 
    /// UsdContrivedTestHairman
    const TfToken attrWithoutGeneratedAccessorAPI;
    /// \brief "binding"
    /// 
    /// UsdContrivedDerived
    const TfToken binding;
    /// \brief "bool"
    /// 
    /// UsdContrivedBase
    const TfToken bool;
    /// \brief "boolArray"
    /// 
    /// UsdContrivedBase
    const TfToken boolArray;
    /// \brief "color3d"
    /// 
    /// UsdContrivedBase
    const TfToken color3d;
    /// \brief "color3dArray"
    /// 
    /// UsdContrivedBase
    const TfToken color3dArray;
    /// \brief "color3f"
    /// 
    /// UsdContrivedBase
    const TfToken color3f;
    /// \brief "color3fArray"
    /// 
    /// UsdContrivedBase
    const TfToken color3fArray;
    /// \brief "color3h"
    /// 
    /// UsdContrivedBase
    const TfToken color3h;
    /// \brief "color3hArray"
    /// 
    /// UsdContrivedBase
    const TfToken color3hArray;
    /// \brief "color4d"
    /// 
    /// UsdContrivedBase
    const TfToken color4d;
    /// \brief "color4dArray"
    /// 
    /// UsdContrivedBase
    const TfToken color4dArray;
    /// \brief "color4f"
    /// 
    /// UsdContrivedBase
    const TfToken color4f;
    /// \brief "color4fArray"
    /// 
    /// UsdContrivedBase
    const TfToken color4fArray;
    /// \brief "color4h"
    /// 
    /// UsdContrivedBase
    const TfToken color4h;
    /// \brief "color4hArray"
    /// 
    /// UsdContrivedBase
    const TfToken color4hArray;
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
    /// \brief "double2"
    /// 
    /// UsdContrivedBase
    const TfToken double2;
    /// \brief "double2Array"
    /// 
    /// UsdContrivedBase
    const TfToken double2Array;
    /// \brief "double3"
    /// 
    /// UsdContrivedBase
    const TfToken double3;
    /// \brief "double3Array"
    /// 
    /// UsdContrivedBase
    const TfToken double3Array;
    /// \brief "double4"
    /// 
    /// UsdContrivedBase
    const TfToken double4;
    /// \brief "double4Array"
    /// 
    /// UsdContrivedBase
    const TfToken double4Array;
    /// \brief "double"
    /// 
    /// UsdContrivedBase
    const TfToken double_;
    /// \brief "doubleArray"
    /// 
    /// UsdContrivedBase
    const TfToken doubleArray;
    /// \brief "float2"
    /// 
    /// UsdContrivedBase
    const TfToken float2;
    /// \brief "float2Array"
    /// 
    /// UsdContrivedBase
    const TfToken float2Array;
    /// \brief "float3"
    /// 
    /// UsdContrivedBase
    const TfToken float3;
    /// \brief "float3Array"
    /// 
    /// UsdContrivedBase
    const TfToken float3Array;
    /// \brief "float4"
    /// 
    /// UsdContrivedBase
    const TfToken float4;
    /// \brief "float4Array"
    /// 
    /// UsdContrivedBase
    const TfToken float4Array;
    /// \brief "float"
    /// 
    /// UsdContrivedBase
    const TfToken float_;
    /// \brief "floatArray"
    /// 
    /// UsdContrivedBase
    const TfToken floatArray;
    /// \brief "frame4d"
    /// 
    /// UsdContrivedBase
    const TfToken frame4d;
    /// \brief "frame4dArray"
    /// 
    /// UsdContrivedBase
    const TfToken frame4dArray;
    /// \brief "half"
    /// 
    /// UsdContrivedBase
    const TfToken half;
    /// \brief "half2"
    /// 
    /// UsdContrivedBase
    const TfToken half2;
    /// \brief "half2Array"
    /// 
    /// UsdContrivedBase
    const TfToken half2Array;
    /// \brief "half3"
    /// 
    /// UsdContrivedBase
    const TfToken half3;
    /// \brief "half3Array"
    /// 
    /// UsdContrivedBase
    const TfToken half3Array;
    /// \brief "half4"
    /// 
    /// UsdContrivedBase
    const TfToken half4;
    /// \brief "half4Array"
    /// 
    /// UsdContrivedBase
    const TfToken half4Array;
    /// \brief "halfArray"
    /// 
    /// UsdContrivedBase
    const TfToken halfArray;
    /// \brief "holeIndices"
    /// 
    /// UsdContrivedDerived
    const TfToken holeIndices;
    /// \brief "int2"
    /// 
    /// UsdContrivedBase
    const TfToken int2;
    /// \brief "int2Array"
    /// 
    /// UsdContrivedBase
    const TfToken int2Array;
    /// \brief "int3"
    /// 
    /// UsdContrivedBase
    const TfToken int3;
    /// \brief "int3Array"
    /// 
    /// UsdContrivedBase
    const TfToken int3Array;
    /// \brief "int4"
    /// 
    /// UsdContrivedBase
    const TfToken int4;
    /// \brief "int4Array"
    /// 
    /// UsdContrivedBase
    const TfToken int4Array;
    /// \brief "int64"
    /// 
    /// UsdContrivedBase
    const TfToken int64;
    /// \brief "int64Array"
    /// 
    /// UsdContrivedBase
    const TfToken int64Array;
    /// \brief "int"
    /// 
    /// UsdContrivedBase
    const TfToken int_;
    /// \brief "intArray"
    /// 
    /// UsdContrivedBase
    const TfToken intArray;
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
    /// \brief "matrix2d"
    /// 
    /// UsdContrivedBase
    const TfToken matrix2d;
    /// \brief "matrix2dArray"
    /// 
    /// UsdContrivedBase
    const TfToken matrix2dArray;
    /// \brief "matrix3d"
    /// 
    /// UsdContrivedBase
    const TfToken matrix3d;
    /// \brief "matrix3dArray"
    /// 
    /// UsdContrivedBase
    const TfToken matrix3dArray;
    /// \brief "matrix4d"
    /// 
    /// UsdContrivedBase
    const TfToken matrix4d;
    /// \brief "matrix4dArray"
    /// 
    /// UsdContrivedBase
    const TfToken matrix4dArray;
    /// \brief "myDouble"
    /// 
    /// UsdContrivedBase
    const TfToken myDouble;
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
    /// \brief "namespaced:property"
    /// 
    /// UsdContrivedDerived
    const TfToken namespacedProperty;
    /// \brief "newToken"
    /// 
    /// Default value for UsdContrivedDerived::GetJustDefaultAttr()
    const TfToken newToken;
    /// \brief "normal3d"
    /// 
    /// UsdContrivedBase
    const TfToken normal3d;
    /// \brief "normal3dArray"
    /// 
    /// UsdContrivedBase
    const TfToken normal3dArray;
    /// \brief "normal3f"
    /// 
    /// UsdContrivedBase
    const TfToken normal3f;
    /// \brief "normal3fArray"
    /// 
    /// UsdContrivedBase
    const TfToken normal3fArray;
    /// \brief "normal3h"
    /// 
    /// UsdContrivedBase
    const TfToken normal3h;
    /// \brief "normal3hArray"
    /// 
    /// UsdContrivedBase
    const TfToken normal3hArray;
    /// \brief "pivotPosition"
    /// 
    /// UsdContrivedDerived
    const TfToken pivotPosition;
    /// \brief "point3d"
    /// 
    /// UsdContrivedBase
    const TfToken point3d;
    /// \brief "point3dArray"
    /// 
    /// UsdContrivedBase
    const TfToken point3dArray;
    /// \brief "point3f"
    /// 
    /// UsdContrivedBase
    const TfToken point3f;
    /// \brief "point3fArray"
    /// 
    /// UsdContrivedBase
    const TfToken point3fArray;
    /// \brief "point3h"
    /// 
    /// UsdContrivedBase
    const TfToken point3h;
    /// \brief "point3hArray"
    /// 
    /// UsdContrivedBase
    const TfToken point3hArray;
    /// \brief "quatd"
    /// 
    /// UsdContrivedBase
    const TfToken quatd;
    /// \brief "quatdArray"
    /// 
    /// UsdContrivedBase
    const TfToken quatdArray;
    /// \brief "quatf"
    /// 
    /// UsdContrivedBase
    const TfToken quatf;
    /// \brief "quatfArray"
    /// 
    /// UsdContrivedBase
    const TfToken quatfArray;
    /// \brief "quath"
    /// 
    /// UsdContrivedBase
    const TfToken quath;
    /// \brief "quathArray"
    /// 
    /// UsdContrivedBase
    const TfToken quathArray;
    /// \brief "relCanShareApiNameWithAttr"
    /// 
    /// UsdContrivedTestHairman
    const TfToken relCanShareApiNameWithAttr;
    /// \brief "riStatements:attributes:user:Gofur_GeomOnHairdensity"
    /// 
    /// UsdContrivedTestHairman
    const TfToken riStatementsAttributesUserGofur_GeomOnHairdensity;
    /// \brief "string"
    /// 
    /// UsdContrivedBase
    const TfToken string;
    /// \brief "stringArray"
    /// 
    /// UsdContrivedBase
    const TfToken stringArray;
    /// \brief "temp"
    /// 
    /// UsdContrivedTestHairman
    const TfToken temp;
    /// \brief "testAttrOne"
    /// 
    /// UsdContrivedPublicMultipleApplyAPI, UsdContrivedMultipleApplyAPI
    const TfToken testAttrOne;
    /// \brief "testAttrThree"
    /// 
    /// UsdContrivedDerivedMultipleApplyAPI
    const TfToken testAttrThree;
    /// \brief "testAttrTwo"
    /// 
    /// UsdContrivedPublicMultipleApplyAPI, UsdContrivedMultipleApplyAPI
    const TfToken testAttrTwo;
    /// \brief "testingAsset"
    /// 
    /// UsdContrivedDerived
    const TfToken testingAsset;
    /// \brief "token"
    /// 
    /// Default value for UsdContrivedBase::GetTokenAttr(), UsdContrivedBase
    const TfToken token;
    /// \brief "tokenArray"
    /// 
    /// UsdContrivedBase
    const TfToken tokenArray;
    /// \brief "transform"
    /// 
    /// UsdContrivedDerived
    const TfToken transform;
    /// \brief "uchar"
    /// 
    /// UsdContrivedBase
    const TfToken uchar;
    /// \brief "ucharArray"
    /// 
    /// UsdContrivedBase
    const TfToken ucharArray;
    /// \brief "uint"
    /// 
    /// UsdContrivedBase
    const TfToken uint;
    /// \brief "uint64"
    /// 
    /// UsdContrivedBase
    const TfToken uint64;
    /// \brief "uint64Array"
    /// 
    /// UsdContrivedBase
    const TfToken uint64Array;
    /// \brief "uintArray"
    /// 
    /// UsdContrivedBase
    const TfToken uintArray;
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
    /// \brief "vector3d"
    /// 
    /// UsdContrivedBase
    const TfToken vector3d;
    /// \brief "vector3dArray"
    /// 
    /// UsdContrivedBase
    const TfToken vector3dArray;
    /// \brief "vector3f"
    /// 
    /// UsdContrivedBase
    const TfToken vector3f;
    /// \brief "vector3fArray"
    /// 
    /// UsdContrivedBase
    const TfToken vector3fArray;
    /// \brief "vector3h"
    /// 
    /// UsdContrivedBase
    const TfToken vector3h;
    /// \brief "vector3hArray"
    /// 
    /// UsdContrivedBase
    const TfToken vector3hArray;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdContrivedTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdContrivedTokensType
extern USDCONTRIVED_API TfStaticData<UsdContrivedTokensType> UsdContrivedTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
