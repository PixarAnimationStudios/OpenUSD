//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_VALIDATION_USD_SKEL_VALIDATORS_TOKENS_H
#define PXR_USD_VALIDATION_USD_SKEL_VALIDATORS_TOKENS_H

/// \file
#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usdValidation/usdSkelValidators/api.h"
PXR_NAMESPACE_OPEN_SCOPE

#define USD_SKEL_VALIDATOR_NAME_TOKENS                                         \
    ((skelBindingApiAppliedValidator,                                          \
      "usdSkelValidators:SkelBindingApiAppliedValidator"))                     \
    ((skelBindingApiValidator, "usdSkelValidators:SkelBindingApiValidator"))

#define USD_SKEL_VALIDATOR_KEYWORD_TOKENS                                      \
    (UsdSkelValidators)

#define USD_SKEL_VALIDATION_ERROR_NAME_TOKENS                                  \
    ((missingSkelBindingAPI, "MissingSkelBindingAPI"))                         \
    ((invalidSkelBindingAPIApply, "InvalidSkelBindingAPIApply"))

/// \def USD_SKEL_VALIDATOR_NAME_TOKENS
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdSkelValidators:, which is the
/// name of the usdSkelValidators plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdSkelValidatorNameTokens, USDSKELVALIDATORS_API,
                         USD_SKEL_VALIDATOR_NAME_TOKENS);
/// \def USD_SKEL_VALIDATOR_KEYWORD_TOKENS
/// Tokens representing keywords associated with any validator in the usdSkel
/// plugin. Clients can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(UsdSkelValidatorKeywordTokens, USDSKELVALIDATORS_API,
                         USD_SKEL_VALIDATOR_KEYWORD_TOKENS);
/// \def USD_SKEL_VALIDATION_ERROR_NAME_TOKENS
/// Tokens representing validation error identifier.
TF_DECLARE_PUBLIC_TOKENS(UsdSkelValidationErrorNameTokens,
                         USDSKELVALIDATORS_API,
                         USD_SKEL_VALIDATION_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE
#endif
