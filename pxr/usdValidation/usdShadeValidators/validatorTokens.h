//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_VALIDATION_USD_SHADE_VALIDATOR_TOKENS_H
#define PXR_USD_VALIDATION_USD_SHADE_VALIDATOR_TOKENS_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usdValidation/usdShadeValidators/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_SHADE_VALIDATOR_NAME_TOKENS                                       \
    ((encapsulationValidator,                                                 \
      "usdShadeValidators:EncapsulationRulesValidator"))                      \
    ((materialBindingApiAppliedValidator,                                     \
      "usdShadeValidators:MaterialBindingApiAppliedValidator"))               \
    ((materialBindingRelationships,                                           \
      "usdShadeValidators:MaterialBindingRelationships"))                     \
    ((materialBindingCollectionValidator,                                     \
      "usdShadeValidators:MaterialBindingCollectionValidator"))               \
    ((normalMapTextureValidator,                                              \
      "usdShadeValidators:NormalMapTextureValidator"))                        \
    ((shaderSdrCompliance, "usdShadeValidators:ShaderSdrCompliance"))         \
    ((subsetMaterialBindFamilyName,                                           \
      "usdShadeValidators:SubsetMaterialBindFamilyName"))                     \
    ((subsetsMaterialBindFamily,                                              \
      "usdShadeValidators:SubsetsMaterialBindFamily"))

#define USD_SHADE_VALIDATOR_KEYWORD_TOKENS (UsdShadeValidators)

#define USD_SHADE_VALIDATION_ERROR_NAME_TOKENS                                 \
    ((connectableInNonContainer, "ConnectableInNonContainer"))                 \
    ((invalidConnectableHierarchy, "InvalidConnectableHierarchy"))             \
    ((missingMaterialBindingAPI, "MissingMaterialBindingAPI"))                 \
    ((materialBindingPropNotARel, "MaterialBindingPropNotARel"))               \
    ((invalidMaterialCollection, "InvalidMaterialCollection"))                 \
    ((invalidResourcePath, "InvalidResourcePath"))                             \
    ((invalidImplSource, "InvalidImplementationSrc"))                          \
    ((missingSourceType, "MissingSourceType"))                                 \
    ((missingShaderIdInRegistry, "MissingShaderIdInRegistry"))                 \
    ((missingSourceTypeInRegistry, "MissingSourceTypeInRegistry"))             \
    ((incompatShaderPropertyWarning, "IncompatShaderPropertyWarning"))         \
    ((mismatchPropertyType, "MismatchedPropertyType"))                         \
    ((missingFamilyNameOnGeomSubset, "MissingFamilyNameOnGeomSubset"))         \
    ((nonShaderConnection, "NonShaderConnection"))                             \
    ((invalidFile, "InvalidFile"))                                             \
    ((invalidShaderPrim, "InvalidShaderPrim"))                                 \
    ((invalidSourceColorSpace, "InvalidSourceColorSpace"))                     \
    ((nonCompliantBiasAndScale, "NonCompliantBiasAndScale"))                   \
    ((nonCompliantScale, "NonCompliantScaleValues"))                           \
    ((nonCompliantBias, "NonCompliantBiasValues"))                             \
    ((invalidFamilyType, "InvalidFamilyType"))

/// \def USD_SHADE_VALIDATOR_NAME_TOKENS
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdShadeValidators:, which is the
/// name of the usdShadeValidators plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdShadeValidatorNameTokens, USDSHADEVALIDATORS_API,
                         USD_SHADE_VALIDATOR_NAME_TOKENS);

/// \def USD_SHADE_VALIDATOR_KEYWORD_TOKENS
/// Tokens representing keywords associated with any validator in the usdShade
/// plugin. Cliends can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(UsdShadeValidatorKeywordTokens, USDSHADEVALIDATORS_API,
                         USD_SHADE_VALIDATOR_KEYWORD_TOKENS);

/// \def USD_SHADE_VALIDATION_ERROR_NAME_TOKENS
/// Tokens representing validation error identifier.
TF_DECLARE_PUBLIC_TOKENS(UsdShadeValidationErrorNameTokens,
                         USDSHADEVALIDATORS_API,
                         USD_SHADE_VALIDATION_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
