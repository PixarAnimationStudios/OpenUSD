//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    const TfToken bool_;
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
    /// \brief "myVaryingTokenArray"
    /// 
    /// UsdContrivedBase
    const TfToken myVaryingTokenArray;
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
    /// Fallback value for UsdContrivedDerived::GetJustDefaultAttr()
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
    /// \brief "overrideBaseFalseDerivedFalse"
    /// 
    /// UsdContrivedBase, UsdContrivedDerived
    const TfToken overrideBaseFalseDerivedFalse;
    /// \brief "overrideBaseFalseDerivedNone"
    /// 
    /// UsdContrivedBase, UsdContrivedDerived
    const TfToken overrideBaseFalseDerivedNone;
    /// \brief "overrideBaseNoneDerivedFalse"
    /// 
    /// UsdContrivedBase, UsdContrivedDerived
    const TfToken overrideBaseNoneDerivedFalse;
    /// \brief "overrideBaseTrueDerivedFalse"
    /// 
    /// UsdContrivedBase, UsdContrivedDerived
    const TfToken overrideBaseTrueDerivedFalse;
    /// \brief "overrideBaseTrueDerivedNone"
    /// 
    /// UsdContrivedBase, UsdContrivedDerived
    const TfToken overrideBaseTrueDerivedNone;
    /// \brief "overrideBaseTrueDerivedTrue"
    /// 
    /// UsdContrivedBase, UsdContrivedDerived
    const TfToken overrideBaseTrueDerivedTrue;
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
    /// \brief "schemaToken1"
    /// 
    /// Special token for the UsdContrivedBase schema.
    const TfToken schemaToken1;
    /// \brief "/non-identifier-tokenValue!"
    /// 
    /// schemaToken2 doc
    const TfToken schemaToken2;
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
    /// UsdContrivedTestHairman, UsdContrivedTestHairman_1, UsdContrivedTestNoVersion0_2
    const TfToken temp;
    /// \brief "test"
    /// 
    /// Property namespace prefix for the UsdContrivedMultipleApplyAPI schema.
    const TfToken test;
    /// \brief "test:__INSTANCE_NAME__"
    /// 
    /// UsdContrivedMultipleApplyAPI
    const TfToken test_MultipleApplyTemplate_;
    /// \brief "test:__INSTANCE_NAME__:testAttrOne"
    /// 
    /// UsdContrivedMultipleApplyAPI
    const TfToken test_MultipleApplyTemplate_TestAttrOne;
    /// \brief "test:__INSTANCE_NAME__:testAttrTwo"
    /// 
    /// UsdContrivedMultipleApplyAPI
    const TfToken test_MultipleApplyTemplate_TestAttrTwo;
    /// \brief "testingAsset"
    /// 
    /// UsdContrivedDerived
    const TfToken testingAsset;
    /// \brief "testNewVersion"
    /// 
    /// Property namespace prefix for the UsdContrivedMultipleApplyAPI_1 schema.
    const TfToken testNewVersion;
    /// \brief "testNewVersion:__INSTANCE_NAME__:testAttrOne"
    /// 
    /// UsdContrivedMultipleApplyAPI_1
    const TfToken testNewVersion_MultipleApplyTemplate_TestAttrOne;
    /// \brief "testNewVersion:__INSTANCE_NAME__:testAttrTwo"
    /// 
    /// UsdContrivedMultipleApplyAPI_1
    const TfToken testNewVersion_MultipleApplyTemplate_TestAttrTwo;
    /// \brief "testo"
    /// 
    /// Property namespace prefix for the UsdContrivedPublicMultipleApplyAPI schema.
    const TfToken testo;
    /// \brief "testo:__INSTANCE_NAME__"
    /// 
    /// UsdContrivedPublicMultipleApplyAPI
    const TfToken testo_MultipleApplyTemplate_;
    /// \brief "testo:__INSTANCE_NAME__:testAttrOne"
    /// 
    /// UsdContrivedPublicMultipleApplyAPI
    const TfToken testo_MultipleApplyTemplate_TestAttrOne;
    /// \brief "testo:__INSTANCE_NAME__:testAttrTwo"
    /// 
    /// UsdContrivedPublicMultipleApplyAPI
    const TfToken testo_MultipleApplyTemplate_TestAttrTwo;
    /// \brief "token"
    /// 
    /// UsdContrivedBase, Fallback value for UsdContrivedBase::GetTokenAttr()
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
    const TfToken VariableTokenAllowed1;
    /// \brief "VariableTokenAllowed2"
    /// 
    /// Possible value for UsdContrivedBase::GetMyVaryingTokenAttr()
    const TfToken VariableTokenAllowed2;
    /// \brief "VariableTokenAllowed<3>"
    /// 
    /// Possible value for UsdContrivedBase::GetMyVaryingTokenAttr()
    const TfToken VariableTokenAllowed_3_;
    /// \brief "VariableTokenArrayAllowed1"
    /// 
    /// Possible value for UsdContrivedBase::GetMyVaryingTokenArrayAttr()
    const TfToken VariableTokenArrayAllowed1;
    /// \brief "VariableTokenArrayAllowed2"
    /// 
    /// Possible value for UsdContrivedBase::GetMyVaryingTokenArrayAttr()
    const TfToken VariableTokenArrayAllowed2;
    /// \brief "VariableTokenArrayAllowed<3>"
    /// 
    /// Possible value for UsdContrivedBase::GetMyVaryingTokenArrayAttr()
    const TfToken VariableTokenArrayAllowed_3_;
    /// \brief "VariableTokenDefault"
    /// 
    /// Fallback value for UsdContrivedBase::GetMyVaryingTokenAttr()
    const TfToken VariableTokenDefault;
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
    /// \brief "Base"
    /// 
    /// Schema identifer and family for UsdContrivedBase
    const TfToken Base;
    /// \brief "Derived"
    /// 
    /// Schema identifer and family for UsdContrivedDerived
    const TfToken Derived;
    /// \brief "DerivedNonAppliedAPI"
    /// 
    /// Schema identifer and family for UsdContrivedDerivedNonAppliedAPI
    const TfToken DerivedNonAppliedAPI;
    /// \brief "EmptyMultipleApplyAPI"
    /// 
    /// Schema identifer and family for UsdContrivedEmptyMultipleApplyAPI
    const TfToken EmptyMultipleApplyAPI;
    /// \brief "MultipleApplyAPI"
    /// 
    /// Schema identifer and family for UsdContrivedMultipleApplyAPI, Schema family for UsdContrivedMultipleApplyAPI_1
    const TfToken MultipleApplyAPI;
    /// \brief "MultipleApplyAPI_1"
    /// 
    /// Schema identifer for UsdContrivedMultipleApplyAPI_1
    const TfToken MultipleApplyAPI_1;
    /// \brief "NonAppliedAPI"
    /// 
    /// Schema identifer and family for UsdContrivedNonAppliedAPI
    const TfToken NonAppliedAPI;
    /// \brief "PublicMultipleApplyAPI"
    /// 
    /// Schema identifer and family for UsdContrivedPublicMultipleApplyAPI
    const TfToken PublicMultipleApplyAPI;
    /// \brief "SingleApplyAPI"
    /// 
    /// Schema identifer and family for UsdContrivedSingleApplyAPI, Schema family for UsdContrivedSingleApplyAPI_1
    const TfToken SingleApplyAPI;
    /// \brief "SingleApplyAPI_1"
    /// 
    /// Schema identifer for UsdContrivedSingleApplyAPI_1
    const TfToken SingleApplyAPI_1;
    /// \brief "TestNoVersion0"
    /// 
    /// Schema family for UsdContrivedTestNoVersion0_2
    const TfToken TestNoVersion0;
    /// \brief "TestNoVersion0_2"
    /// 
    /// Schema identifer for UsdContrivedTestNoVersion0_2
    const TfToken TestNoVersion0_2;
    /// \brief "TestPxHairman"
    /// 
    /// Schema identifer and family for UsdContrivedTestHairman, Schema family for UsdContrivedTestHairman_1
    const TfToken TestPxHairman;
    /// \brief "TestPxHairman_1"
    /// 
    /// Schema identifer for UsdContrivedTestHairman_1
    const TfToken TestPxHairman_1;
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
