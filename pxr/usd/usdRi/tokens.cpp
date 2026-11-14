//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdRi/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdRiTokensType::UsdRiTokensType() :
    renderContext("ri", TfToken::Immortal),
    StatementsAPI("StatementsAPI", TfToken::Immortal),
    allTokens({
        renderContext,
        StatementsAPI
    })
{
}

TfStaticData<UsdRiTokensType> UsdRiTokens;

PXR_NAMESPACE_CLOSE_SCOPE
