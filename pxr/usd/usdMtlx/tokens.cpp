//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdMtlx/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdMtlxTokensType::UsdMtlxTokensType() :
    DefaultOutputName("out", TfToken::Immortal),
    infoMtlxVersion("info:mtlx:version", TfToken::Immortal),
    MaterialXInfoAPI("MaterialXInfoAPI", TfToken::Immortal),
    allTokens({
        DefaultOutputName,
        infoMtlxVersion,
        MaterialXInfoAPI
    })
{
}

TfStaticData<UsdMtlxTokensType> UsdMtlxTokens;

PXR_NAMESPACE_CLOSE_SCOPE
