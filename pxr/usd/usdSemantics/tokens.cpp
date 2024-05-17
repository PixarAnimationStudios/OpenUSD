//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSemantics/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdSemanticsTokensType::UsdSemanticsTokensType() :
    semanticsLabels("semantics:labels", TfToken::Immortal),
    semanticsLabels_MultipleApplyTemplate_("semantics:labels:__INSTANCE_NAME__", TfToken::Immortal),
    SemanticsLabelsAPI("SemanticsLabelsAPI", TfToken::Immortal),
    allTokens({
        semanticsLabels,
        semanticsLabels_MultipleApplyTemplate_,
        SemanticsLabelsAPI
    })
{
}

TfStaticData<UsdSemanticsTokensType> UsdSemanticsTokens;

PXR_NAMESPACE_CLOSE_SCOPE
