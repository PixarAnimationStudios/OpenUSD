//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_VALIDATION_USD_LUX_VALIDATOR_TOKENS_H
#define PXR_USD_VALIDATION_USD_LUX_VALIDATOR_TOKENS_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usdValidation/usdLuxValidators/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_LUX_VALIDATOR_NAME_TOKENS                                       \
    ((encapsulationValidator,                                               \
      "usdLuxValidators:EncapsulationRulesValidator"))

#define USD_LUX_VALIDATOR_KEYWORD_TOKENS (UsdLuxValidators)

#define USD_LUX_VALIDATION_ERROR_NAME_TOKENS                                 \
    ((lightUnderConnectable, "LightUnderConnectable"))                       \
    ((lightFilterUnderLightFilter, "LightFilterUnderLightFilter"))           \
    ((connectableInNonContainer, "ConnectableInNonContainer"))               \
    ((invalidConnectableHierarchy, "InvalidConnectableHierarchy"))

/// \def USD_LUX_VALIDATOR_NAME_TOKENS
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdLuxValidators:, which is the
/// name of the usdLuxValidators plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdLuxValidatorNameTokens, USDLUXVALIDATORS_API,
                         USD_LUX_VALIDATOR_NAME_TOKENS);

/// \def USD_LUX_VALIDATOR_KEYWORD_TOKENS
/// Tokens representing keywords associated with any validator in the usdLux
/// plugin. Cliends can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(UsdLuxValidatorKeywordTokens, USDLUXVALIDATORS_API,
                         USD_LUX_VALIDATOR_KEYWORD_TOKENS);

/// \def USD_LUX_VALIDATION_ERROR_NAME_TOKENS
/// Tokens representing validation error identifier.
TF_DECLARE_PUBLIC_TOKENS(UsdLuxValidationErrorNameTokens,
                         USDLUXVALIDATORS_API,
                         USD_LUX_VALIDATION_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
