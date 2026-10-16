//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/usd/usd/tokens.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

PXR_NAMESPACE_USING_DIRECTIVE

using UsdTokens_StaticTokenType = UsdTokensType;

#define USD_TOKENS \
    (apiSchemas) \
    (automatic) \
    (clips) \
    (clipSets) \
    (collection) \
    (collection_MultipleApplyTemplate_) \
    (collection_MultipleApplyTemplate_Excludes) \
    (collection_MultipleApplyTemplate_ExpansionRule) \
    (collection_MultipleApplyTemplate_IncludeRoot) \
    (collection_MultipleApplyTemplate_Includes) \
    (collection_MultipleApplyTemplate_MembershipExpression) \
    (collection_MultipleApplyTemplate_Mode) \
    (colorSpaceDefinition) \
    (colorSpaceDefinition_MultipleApplyTemplate_BlueChroma) \
    (colorSpaceDefinition_MultipleApplyTemplate_Gamma) \
    (colorSpaceDefinition_MultipleApplyTemplate_GreenChroma) \
    (colorSpaceDefinition_MultipleApplyTemplate_LinearBias) \
    (colorSpaceDefinition_MultipleApplyTemplate_Name) \
    (colorSpaceDefinition_MultipleApplyTemplate_RedChroma) \
    (colorSpaceDefinition_MultipleApplyTemplate_WhitePoint) \
    (colorSpaceName) \
    (custom) \
    (exclude) \
    (expandPrims) \
    (expandPrimsAndProperties) \
    (explicitOnly) \
    (expression) \
    (fallbackPrimTypes) \
    (relationship) \
    (APISchemaBase) \
    (ClipsAPI) \
    (CollectionAPI) \
    (ColorSpaceAPI) \
    (ColorSpaceDefinitionAPI) \
    (ModelAPI) \
    (Typed)

void wrapUsdTokens(nanobind::module_ &m)
{
    TF_NB_WRAP_PUBLIC_TOKENS(m, "Tokens", UsdTokens, USD_TOKENS);
}

