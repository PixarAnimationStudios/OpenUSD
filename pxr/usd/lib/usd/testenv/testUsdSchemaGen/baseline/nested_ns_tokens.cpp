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
#include "pxr/usd/usdContrived/tokens.h"

namespace foo { namespace bar { namespace baz {

UsdContrivedTokensType::UsdContrivedTokensType() :
    binding("binding", TfToken::Immortal),
    cornerIndices("cornerIndices", TfToken::Immortal),
    cornerSharpnesses("cornerSharpnesses", TfToken::Immortal),
    creaseLengths("creaseLengths", TfToken::Immortal),
    holeIndices("holeIndices", TfToken::Immortal),
    justDefault("justDefault", TfToken::Immortal),
    libraryToken1("libraryToken1", TfToken::Immortal),
    libraryToken2("/non-identifier-tokenValue!", TfToken::Immortal),
    myColorFloat("myColorFloat", TfToken::Immortal),
    myDouble("myDouble", TfToken::Immortal),
    myFloat("myFloat", TfToken::Immortal),
    myNormals("myNormals", TfToken::Immortal),
    myPoints("myPoints", TfToken::Immortal),
    myUniformBool("myUniformBool", TfToken::Immortal),
    myVaryingToken("myVaryingToken", TfToken::Immortal),
    myVecfArray("myVecfArray", TfToken::Immortal),
    myVelocities("myVelocities", TfToken::Immortal),
    namespacedProperty("namespaced:property", TfToken::Immortal),
    newToken("newToken", TfToken::Immortal),
    pivotPosition("pivotPosition", TfToken::Immortal),
    relCanShareApiNameWithAttr("relCanShareApiNameWithAttr", TfToken::Immortal),
    riStatementsAttributesUserGofur_GeomOnHairdensity("riStatements:attributes:user:Gofur_GeomOnHairdensity", TfToken::Immortal),
    temp("temp", TfToken::Immortal),
    testingAsset("testingAsset", TfToken::Immortal),
    transform("transform", TfToken::Immortal),
    unsignedChar("unsignedChar", TfToken::Immortal),
    unsignedInt("unsignedInt", TfToken::Immortal),
    unsignedInt64Array("unsignedInt64Array", TfToken::Immortal),
    variableTokenAllowed1("VariableTokenAllowed1", TfToken::Immortal),
    variabletokenAllowed2("VariabletokenAllowed2", TfToken::Immortal),
    variableTokenDefault("VariableTokenDefault", TfToken::Immortal),
    allTokens({
        binding,
        cornerIndices,
        cornerSharpnesses,
        creaseLengths,
        holeIndices,
        justDefault,
        libraryToken1,
        libraryToken2,
        myColorFloat,
        myDouble,
        myFloat,
        myNormals,
        myPoints,
        myUniformBool,
        myVaryingToken,
        myVecfArray,
        myVelocities,
        namespacedProperty,
        newToken,
        pivotPosition,
        relCanShareApiNameWithAttr,
        riStatementsAttributesUserGofur_GeomOnHairdensity,
        temp,
        testingAsset,
        transform,
        unsignedChar,
        unsignedInt,
        unsignedInt64Array,
        variableTokenAllowed1,
        variabletokenAllowed2,
        variableTokenDefault
    })
{
}

TfStaticData<UsdContrivedTokensType> UsdContrivedTokens;

}}}
