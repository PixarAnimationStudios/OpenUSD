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

PXR_NAMESPACE_OPEN_SCOPE

UsdContrivedTokensType::UsdContrivedTokensType() :
    binding("binding"),
    cornerIndices("cornerIndices"),
    cornerSharpnesses("cornerSharpnesses"),
    creaseLengths("creaseLengths"),
    holeIndices("holeIndices"),
    justDefault("justDefault"),
    libraryToken1("libraryToken1"),
    libraryToken2("/non-identifier-tokenValue!"),
    myColorFloat("myColorFloat"),
    myDouble("myDouble"),
    myFloat("myFloat"),
    myNormals("myNormals"),
    myPoints("myPoints"),
    myUniformBool("myUniformBool"),
    myVaryingToken("myVaryingToken"),
    myVecfArray("myVecfArray"),
    myVelocities("myVelocities"),
    namespacedProperty("namespaced:property"),
    newToken("newToken"),
    pivotPosition("pivotPosition"),
    relCanShareApiNameWithAttr("relCanShareApiNameWithAttr"),
    riStatementsAttributesUserGofur_GeomOnHairdensity("riStatements:attributes:user:Gofur_GeomOnHairdensity"),
    temp("temp"),
    testingAsset("testingAsset"),
    transform("transform"),
    unsignedChar("unsignedChar"),
    unsignedInt("unsignedInt"),
    unsignedInt64Array("unsignedInt64Array"),
    variableTokenAllowed1("VariableTokenAllowed1"),
    variabletokenAllowed2("VariabletokenAllowed2"),
    variableTokenDefault("VariableTokenDefault"),
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

PXR_NAMESPACE_CLOSE_SCOPE
