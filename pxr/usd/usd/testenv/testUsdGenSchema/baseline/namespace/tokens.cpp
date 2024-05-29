//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/tokens.h"

namespace foo {

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

}
