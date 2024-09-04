//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdMtlx/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdMtlxTokensType::UsdMtlxTokensType() :
    configMtlxVersion("config:mtlx:version", TfToken::Immortal),
    DefaultOutputName("out", TfToken::Immortal),
    MaterialXConfigAPI("MaterialXConfigAPI", TfToken::Immortal),
    allTokens({
        configMtlxVersion,
        DefaultOutputName,
        MaterialXConfigAPI
    })
{
}

TfStaticData<UsdMtlxTokensType> UsdMtlxTokens;

PXR_NAMESPACE_CLOSE_SCOPE
