//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdProfiles/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdProfilesTokensType::UsdProfilesTokensType() :
    ClaimsAPI("ClaimsAPI", TfToken::Immortal),
    allTokens({
        ClaimsAPI
    })
{
}

TfStaticData<UsdProfilesTokensType> UsdProfilesTokens;

PXR_NAMESPACE_CLOSE_SCOPE
