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
    libraryToken1("libraryToken1", TfToken::Immortal),
    libraryToken2("/non-identifier-tokenValue!", TfToken::Immortal),
    myColorFloat("myColorFloat", TfToken::Immortal),
    myDouble("myDouble", TfToken::Immortal),
    myFloat("myFloat", TfToken::Immortal),
    myNormals("myNormals", TfToken::Immortal),
    myPoints("myPoints", TfToken::Immortal),
    myVaryingToken("myVaryingToken", TfToken::Immortal),
    myVaryingTokenArray("myVaryingTokenArray", TfToken::Immortal),
    myVelocities("myVelocities", TfToken::Immortal),
    testAttrOne("testAttrOne", TfToken::Immortal),
    testAttrTwo("testAttrTwo", TfToken::Immortal),
    unsignedChar("unsignedChar", TfToken::Immortal),
    unsignedInt("unsignedInt", TfToken::Immortal),
    unsignedInt64Array("unsignedInt64Array", TfToken::Immortal),
    VariableTokenAllowed1("VariableTokenAllowed1", TfToken::Immortal),
    VariableTokenAllowed2("VariableTokenAllowed2", TfToken::Immortal),
    VariableTokenAllowed_3_("VariableTokenAllowed<3>", TfToken::Immortal),
    VariableTokenArrayAllowed1("VariableTokenArrayAllowed1", TfToken::Immortal),
    VariableTokenArrayAllowed2("VariableTokenArrayAllowed2", TfToken::Immortal),
    VariableTokenArrayAllowed_3_("VariableTokenArrayAllowed<3>", TfToken::Immortal),
    VariableTokenDefault("VariableTokenDefault", TfToken::Immortal),
    Base("Base", TfToken::Immortal),
    SingleApplyAPI("SingleApplyAPI", TfToken::Immortal),
    allTokens({
        libraryToken1,
        libraryToken2,
        myColorFloat,
        myDouble,
        myFloat,
        myNormals,
        myPoints,
        myVaryingToken,
        myVaryingTokenArray,
        myVelocities,
        testAttrOne,
        testAttrTwo,
        unsignedChar,
        unsignedInt,
        unsignedInt64Array,
        VariableTokenAllowed1,
        VariableTokenAllowed2,
        VariableTokenAllowed_3_,
        VariableTokenArrayAllowed1,
        VariableTokenArrayAllowed2,
        VariableTokenArrayAllowed_3_,
        VariableTokenDefault,
        Base,
        SingleApplyAPI
    })
{
}

TfStaticData<UsdContrivedTokensType> UsdContrivedTokens;

PXR_NAMESPACE_CLOSE_SCOPE
