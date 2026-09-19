//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_VALIDATION_USD_MEDIA_VALIDATORS_VALIDATOR_TOKENS_H
#define PXR_USD_VALIDATION_USD_MEDIA_VALIDATORS_VALIDATOR_TOKENS_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usdValidation/usdMediaValidators/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_MEDIA_VALIDATOR_NAME_TOKENS                                      \
    ((shadowedOrClobberedAuthorship,                                         \
      "usdMediaValidators:ShadowedOrClobberedAuthorship"))

#define USD_MEDIA_VALIDATOR_KEYWORD_TOKENS (UsdMediaValidators)

#define USD_MEDIA_VALIDATION_ERROR_NAME_TOKENS                               \
    ((shadowedAuthorshipApplication, "ShadowedAuthorshipApplication"))       \
    ((clobberedAuthorshipInstanceName, "ClobberedAuthorshipInstanceName"))

/// \def USD_MEDIA_VALIDATOR_NAME_TOKENS
/// Tokens for validator names. Note that for plugin-provided validators,
/// names must be prefixed by usdMediaValidators:, the name of this plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdMediaValidatorNameTokens, USDMEDIAVALIDATORS_API,
                         USD_MEDIA_VALIDATOR_NAME_TOKENS);

/// \def USD_MEDIA_VALIDATOR_KEYWORD_TOKENS
/// Keyword tokens associated with validators in the usdMediaValidators
/// plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdMediaValidatorKeywordTokens,
                         USDMEDIAVALIDATORS_API,
                         USD_MEDIA_VALIDATOR_KEYWORD_TOKENS);

/// \def USD_MEDIA_VALIDATION_ERROR_NAME_TOKENS
/// Tokens for validation error identifiers.
TF_DECLARE_PUBLIC_TOKENS(UsdMediaValidationErrorNameTokens,
                         USDMEDIAVALIDATORS_API,
                         USD_MEDIA_VALIDATION_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
