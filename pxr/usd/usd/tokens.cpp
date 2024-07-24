//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdTokensType::UsdTokensType() :
    apiSchemas("apiSchemas", TfToken::Immortal),
    clips("clips", TfToken::Immortal),
    clipSets("clipSets", TfToken::Immortal),
    collection("collection", TfToken::Immortal),
    collection_MultipleApplyTemplate_("collection:__INSTANCE_NAME__", TfToken::Immortal),
    collection_MultipleApplyTemplate_Excludes("collection:__INSTANCE_NAME__:excludes", TfToken::Immortal),
    collection_MultipleApplyTemplate_ExpansionRule("collection:__INSTANCE_NAME__:expansionRule", TfToken::Immortal),
    collection_MultipleApplyTemplate_IncludeRoot("collection:__INSTANCE_NAME__:includeRoot", TfToken::Immortal),
    collection_MultipleApplyTemplate_Includes("collection:__INSTANCE_NAME__:includes", TfToken::Immortal),
    collection_MultipleApplyTemplate_MembershipExpression("collection:__INSTANCE_NAME__:membershipExpression", TfToken::Immortal),
    exclude("exclude", TfToken::Immortal),
    expandPrims("expandPrims", TfToken::Immortal),
    expandPrimsAndProperties("expandPrimsAndProperties", TfToken::Immortal),
    explicitOnly("explicitOnly", TfToken::Immortal),
    fallbackPrimTypes("fallbackPrimTypes", TfToken::Immortal),
    APISchemaBase("APISchemaBase", TfToken::Immortal),
    ClipsAPI("ClipsAPI", TfToken::Immortal),
    CollectionAPI("CollectionAPI", TfToken::Immortal),
    ModelAPI("ModelAPI", TfToken::Immortal),
    Typed("Typed", TfToken::Immortal),
    allTokens({
        apiSchemas,
        clips,
        clipSets,
        collection,
        collection_MultipleApplyTemplate_,
        collection_MultipleApplyTemplate_Excludes,
        collection_MultipleApplyTemplate_ExpansionRule,
        collection_MultipleApplyTemplate_IncludeRoot,
        collection_MultipleApplyTemplate_Includes,
        collection_MultipleApplyTemplate_MembershipExpression,
        exclude,
        expandPrims,
        expandPrimsAndProperties,
        explicitOnly,
        fallbackPrimTypes,
        APISchemaBase,
        ClipsAPI,
        CollectionAPI,
        ModelAPI,
        Typed
    })
{
}

TfStaticData<UsdTokensType> UsdTokens;

PXR_NAMESPACE_CLOSE_SCOPE
