//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef USD_VALIDATOR_TOKENS_H
#define USD_VALIDATOR_TOKENS_H

/// \file usd/validatorTokens.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_VALIDATOR_NAME_TOKENS                   \
    ((compositionErrorTest, "usd:CompositionErrorTest")) \
    ((stageMetadataChecker, "usd:StageMetadataChecker"))

#define USD_VALIDATOR_KEYWORD_TOKENS                \
    (UsdCoreValidators)

/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usd:, which is the name of
/// the usd plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdValidatorNameTokens, USD_API, 
                         USD_VALIDATOR_NAME_TOKENS);

/// Tokens representing keywords associated with any validator in the usd
/// plugin. Cliends can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(UsdValidatorKeywordTokens, USD_API, 
                         USD_VALIDATOR_KEYWORD_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
