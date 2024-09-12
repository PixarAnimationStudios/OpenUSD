//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDSKEL_VALIDATOR_TOKENS_H
#define USDSKEL_VALIDATOR_TOKENS_H
/// \file
#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"
#include "pxr/base/tf/staticTokens.h"
PXR_NAMESPACE_OPEN_SCOPE

#define USD_SKEL_VALIDATOR_NAME_TOKENS                   \
     ((skelBindingApiAppliedValidator, "usdSkel:SkelBindingApiAppliedValidator")) \
     ((skelBindingApiValidator, "usdSkel:SkelBindingApiValidator"))

#define USD_SKEL_VALIDATOR_KEYWORD_TOKENS                \
     (UsdSkelValidators)

#define USD_SKEL_VALIDATION_ERROR_NAME_TOKENS                          \
    ((missingSkelBindingAPI, "MissingSkelBindingAPI"))  \
    ((invalidSkelBindingAPIApply, "InvalidSkelBindingAPIApply"))                          \

/// \def USD_SKEL_VALIDATOR_NAME_TOKENS
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdSkel:, which is the name of
/// the usdSkel plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdSkelValidatorNameTokens, USDSKEL_API,
                         USD_SKEL_VALIDATOR_NAME_TOKENS);
/// \def USD_SKEL_VALIDATOR_KEYWORD_TOKENS
/// Tokens representing keywords associated with any validator in the usdSkel
/// plugin. Clients can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(UsdSkelValidatorKeywordTokens, USDSKEL_API,
                         USD_SKEL_VALIDATOR_KEYWORD_TOKENS);
/// \def USD_SKEL_VALIDATION_ERROR_NAME_TOKENS
/// Tokens representing validation error identifier.
TF_DECLARE_PUBLIC_TOKENS(UsdSkelValidationErrorNameTokens, USDSKEL_API, 
                         USD_SKEL_VALIDATION_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE
#endif
