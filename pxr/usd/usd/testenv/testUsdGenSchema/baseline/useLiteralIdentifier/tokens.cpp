//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    unsignedChar("unsignedChar", TfToken::Immortal),
    unsignedInt("unsignedInt", TfToken::Immortal),
    unsignedInt64Array("unsignedInt64Array", TfToken::Immortal),
    variableTokenAllowed1("VariableTokenAllowed1", TfToken::Immortal),
    variableTokenAllowed2("VariableTokenAllowed2", TfToken::Immortal),
    variableTokenAllowed3("VariableTokenAllowed<3>", TfToken::Immortal),
    variableTokenArrayAllowed1("VariableTokenArrayAllowed1", TfToken::Immortal),
    variableTokenArrayAllowed2("VariableTokenArrayAllowed2", TfToken::Immortal),
    variableTokenArrayAllowed3("VariableTokenArrayAllowed<3>", TfToken::Immortal),
    variableTokenDefault("VariableTokenDefault", TfToken::Immortal),
    Base("Base", TfToken::Immortal),
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
        unsignedChar,
        unsignedInt,
        unsignedInt64Array,
        variableTokenAllowed1,
        variableTokenAllowed2,
        variableTokenAllowed3,
        variableTokenArrayAllowed1,
        variableTokenArrayAllowed2,
        variableTokenArrayAllowed3,
        variableTokenDefault,
        Base
    })
{
}

TfStaticData<UsdContrivedTokensType> UsdContrivedTokens;

PXR_NAMESPACE_CLOSE_SCOPE
