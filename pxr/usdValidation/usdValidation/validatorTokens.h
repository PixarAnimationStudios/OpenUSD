//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_VALIDATION_USD_VALIDATION_VALIDATOR_TOKENS_H
#define PXR_USD_VALIDATION_USD_VALIDATION_VALIDATOR_TOKENS_H

/// \file usdValidation/validatorTokens.h

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usdValidation/usdValidation/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_VALIDATOR_NAME_TOKENS                                              \
    ((compositionErrorTest, "usdValidation:CompositionErrorTest"))             \
    ((stageMetadataChecker, "usdValidation:StageMetadataChecker"))

#define USD_VALIDATOR_KEYWORD_TOKENS                                           \
    (UsdCoreValidators)

#define USD_VALIDATION_ERROR_NAME_TOKENS                                       \
    ((compositionError, "CompositionError"))                                   \
    ((missingDefaultPrim, "MissingDefaultPrim"))

/// \def USD_VALIDATOR_NAME_TOKENS
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdValidation:, which is the name
/// of the usdValidation plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdValidatorNameTokens, USDVALIDATION_API,
                         USD_VALIDATOR_NAME_TOKENS);

/// \def USD_VALIDATOR_KEYWORD_TOKENS
/// Tokens representing keywords associated with any validator in the usd
/// plugin. Clients can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(UsdValidatorKeywordTokens, USDVALIDATION_API,
                         USD_VALIDATOR_KEYWORD_TOKENS);

/// \def USD_VALIDATION_ERROR_NAME_TOKENS
/// Tokens representing validation error identifier.
TF_DECLARE_PUBLIC_TOKENS(UsdValidationErrorNameTokens, USDVALIDATION_API,
                         USD_VALIDATION_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
