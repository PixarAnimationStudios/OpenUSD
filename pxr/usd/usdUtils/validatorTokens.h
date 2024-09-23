//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef USDGEOM_VALIDATOR_TOKENS_H
#define USDGEOM_VALIDATOR_TOKENS_H

/// \file

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_UTILS_VALIDATOR_NAME_TOKENS                   \
    ((packageEncapsulationValidator, "usdUtils:PackageEncapsulationValidator"))

#define USD_UTILS_VALIDATOR_KEYWORD_TOKENS                \
    (UsdUtilsValidators)                                  \
    (UsdzValidators)

#define USD_UTILS_VALIDATION_ERROR_NAME_TOKENS                          \
    ((layerNotInPackage, "LayerNotInPackage"))                          \
    ((assetNotInPackage, "AssetNotInPackage"))                          

///\def
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdGeom:, which is the name of
/// the usdGeom plugin.
    TF_DECLARE_PUBLIC_TOKENS(UsdUtilsValidatorNameTokens, USDUTILS_API,
                             USD_UTILS_VALIDATOR_NAME_TOKENS);

///\def
/// Tokens representing keywords associated with any validator in the usdGeom
/// plugin. Clients can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
    TF_DECLARE_PUBLIC_TOKENS(UsdUtilsValidatorKeywordTokens, USDUTILS_API,
                             USD_UTILS_VALIDATOR_KEYWORD_TOKENS);

/// \def USD_UTILS_VALIDATION_ERROR_NAME_TOKENS
/// Tokens representing validation error identifier.
TF_DECLARE_PUBLIC_TOKENS(UsdUtilsValidationErrorNameTokens, USDUTILS_API, 
                         USD_UTILS_VALIDATION_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
