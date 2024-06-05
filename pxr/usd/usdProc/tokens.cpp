//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdProc/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdProcTokensType::UsdProcTokensType() :
    proceduralSystem("proceduralSystem", TfToken::Immortal),
    GenerativeProcedural("GenerativeProcedural", TfToken::Immortal),
    allTokens({
        proceduralSystem,
        GenerativeProcedural
    })
{
}

TfStaticData<UsdProcTokensType> UsdProcTokens;

PXR_NAMESPACE_CLOSE_SCOPE
