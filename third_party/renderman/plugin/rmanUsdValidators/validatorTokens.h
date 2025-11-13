//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_RMAN_USD_VALIDATORS_TOKENS_H
#define PXR_RMAN_USD_VALIDATORS_TOKENS_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define RMAN_USD_VALIDATORS_NAME_TOKENS                                        \
    ((PxrRenderTerminalsAPIRelationships,                                      \
      "rmanUsdValidators:PxrRenderTerminalsAPIRelationships"))

#define RMAN_USD_VALIDATORS_KEYWORD_TOKENS (UsdValidators)                     \
    (RmanUsdValidators)

#define RMAN_USD_VALIDATORS_ERROR_NAME_TOKENS                                  \
    (invalidRenderTerminalsAttr)

/// \def RMAN_USD_VALIDATORS_NAME_TOKENS
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdValidators:, which is the
/// name of the usdValidators plugin.
TF_DECLARE_PUBLIC_TOKENS(RmanUsdValidatorsNameTokens,
                         RMAN_USD_VALIDATORS_NAME_TOKENS);

/// \def RMAN_USD_VALIDATORS_KEYWORD_TOKENS
/// Tokens representing keywords associated with any validator in this
/// plugin. Clients can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(RmanUsdValidatorsKeywordTokens,
                         RMAN_USD_VALIDATORS_KEYWORD_TOKENS);

/// \def RMAN_USD_VALIDATORS_ERROR_NAME_TOKENS
/// Tokens representing validation error identifier.
TF_DECLARE_PUBLIC_TOKENS(RmanUsdValidatorsErrorNameTokens,
                         RMAN_USD_VALIDATORS_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
