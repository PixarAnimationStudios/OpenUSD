//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef USDSHADE_VALIDATOR_TOKENS_H
#define USDSHADE_VALIDATOR_TOKENS_H

/// \file

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_SHADE_VALIDATOR_NAME_TOKENS \
    ((encapsulationValidator, "usdShade:EncapsulationRulesValidator")) \
    ((materialBindingApiAppliedValidator, "usdShade:MaterialBindingApiAppliedValidator")) \
    ((materialBindingRelationships, "usdShade:MaterialBindingRelationships")) \
    ((shaderSdrCompliance, "usdShade:ShaderSdrCompliance"))                   \
    ((subsetMaterialBindFamilyName, "usdShade:SubsetMaterialBindFamilyName")) \
    ((subsetsMaterialBindFamily, "usdShade:SubsetsMaterialBindFamily"))

#define USD_SHADE_VALIDATOR_KEYWORD_TOKENS                                    \
    (UsdShadeValidators)

#define USD_SHADE_VALIDATION_ERROR_NAME_TOKENS                          \
    ((connectableInNonContainer, "ConnectableInNonContainer"))          \
    ((invalidConnectableHierarchy, "InvalidConnectableHierarchy"))      \
    ((missingMaterialBindingAPI,  "MissingMaterialBindingAPI"))         \
    ((materialBindingPropNotARel, "MaterialBindingPropNotARel"))        \
    ((invalidImplSource, "InvalidImplementationSrc"))                   \
    ((missingSourceType, "MissingSourceType"))                          \
    ((missingShaderIdInRegistry, "MissingShaderIdInRegistry"))          \
    ((missingSourceTypeInRegistry, "MissingSourceTypeInRegistry"))      \
    ((incompatShaderPropertyWarning, "IncompatShaderPropertyWarning"))  \
    ((mismatchPropertyType, "MismatchedPropertyType"))                  \
    ((missingFamilyNameOnGeomSubset, "MissingFamilyNameOnGeomSubset"))  \
    ((invalidFamilyType, "InvalidFamilyType"))                          \

/// \def USD_SHADE_VALIDATOR_NAME_TOKENS
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdShade:, which is the name of
/// the usdShade plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdShadeValidatorNameTokens, USDSHADE_API, 
                         USD_SHADE_VALIDATOR_NAME_TOKENS);

/// \def USD_SHADE_VALIDATOR_KEYWORD_TOKENS
/// Tokens representing keywords associated with any validator in the usdShade
/// plugin. Cliends can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(UsdShadeValidatorKeywordTokens, USDSHADE_API, 
                         USD_SHADE_VALIDATOR_KEYWORD_TOKENS);

/// \def USD_SHADE_VALIDATION_ERROR_NAME_TOKENS
/// Tokens representing validation error identifier.
TF_DECLARE_PUBLIC_TOKENS(UsdShadeValidationErrorNameTokens, USDSHADE_API, 
                         USD_SHADE_VALIDATION_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
