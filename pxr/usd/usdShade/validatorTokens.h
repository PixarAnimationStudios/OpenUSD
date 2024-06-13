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

#define USD_SHADE_VALIDATOR_NAME_TOKENS                   \
    ((shaderSdrCompliance, "usdShade:ShaderSdrCompliance"))

#define USD_SHADE_VALIDATOR_KEYWORD_TOKENS                \
    (UsdShadeValidators)

///\def 
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdShade:, which is the name of
/// the usdShade plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdShadeValidatorNameTokens, USDSHADE_API, 
                         USD_SHADE_VALIDATOR_NAME_TOKENS);

///\def 
/// Tokens representing keywords associated with any validator in the usdShade
/// plugin. Cliends can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(UsdShadeValidatorKeywordTokens, USDSHADE_API, 
                         USD_SHADE_VALIDATOR_KEYWORD_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
