//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "./tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdSchemaExamplesTokensType::UsdSchemaExamplesTokensType() :
    complexString("complexString", TfToken::Immortal),
    intAttr("intAttr", TfToken::Immortal),
    paramsMass("params:mass", TfToken::Immortal),
    paramsVelocity("params:velocity", TfToken::Immortal),
    paramsVolume("params:volume", TfToken::Immortal),
    target("target", TfToken::Immortal),
    ComplexPrim("ComplexPrim", TfToken::Immortal),
    ParamsAPI("ParamsAPI", TfToken::Immortal),
    SimplePrim("SimplePrim", TfToken::Immortal),
    allTokens({
        complexString,
        intAttr,
        paramsMass,
        paramsVelocity,
        paramsVolume,
        target,
        ComplexPrim,
        ParamsAPI,
        SimplePrim
    })
{
}

TfStaticData<UsdSchemaExamplesTokensType> UsdSchemaExamplesTokens;

PXR_NAMESPACE_CLOSE_SCOPE
