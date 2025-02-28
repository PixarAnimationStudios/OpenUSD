//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_VALIDATION_USD_UTILS_VALIDATORS_TOKENS_H
#define PXR_USD_VALIDATION_USD_UTILS_VALIDATORS_TOKENS_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usdValidation/usdUtilsValidators/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_UTILS_VALIDATOR_NAME_TOKENS                                        \
    ((packageEncapsulationValidator,                                           \
      "usdUtilsValidators:PackageEncapsulationValidator"))                     \
    ((fileExtensionValidator,                                                  \
      "usdUtilsValidators:FileExtensionValidator"))                            \
    ((missingReferenceValidator,                                               \
      "usdUtilsValidators:MissingReferenceValidator"))                         \
    ((rootPackageValidator, "usdUtilsValidators:RootPackageValidator"))        \
    ((usdzPackageValidator, "usdUtilsValidators:UsdzPackageValidator"))

#define USD_UTILS_VALIDATOR_KEYWORD_TOKENS                                     \
    (UsdUtilsValidators)                                                       \
    (UsdzValidators)

#define USD_UTILS_VALIDATION_ERROR_NAME_TOKENS                                 \
    ((layerNotInPackage, "LayerNotInPackage"))                                 \
    ((assetNotInPackage, "AssetNotInPackage"))                                 \
    ((invalidLayerInPackage, "InvalidLayerInPackage"))                         \
    ((unsupportedFileExtensionInPackage,                                       \
        "UnsupportedFileExtensionInPackage"))                                  \
    ((unresolvableDependency, "UnresolvableDependency"))		               \
    ((compressionDetected, "CompressionDetected"))                             \
    ((byteMisalignment, "ByteMisalignment"))

///\def
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdGeom:, which is the name of
/// the usdGeom plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdUtilsValidatorNameTokens, USDUTILSVALIDATORS_API,
                         USD_UTILS_VALIDATOR_NAME_TOKENS);

///\def
/// Tokens representing keywords associated with any validator in the usdGeom
/// plugin. Clients can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(UsdUtilsValidatorKeywordTokens, USDUTILSVALIDATORS_API,
                         USD_UTILS_VALIDATOR_KEYWORD_TOKENS);

/// \def USD_UTILS_VALIDATION_ERROR_NAME_TOKENS
/// Tokens representing validation error identifier.
TF_DECLARE_PUBLIC_TOKENS(UsdUtilsValidationErrorNameTokens,
                         USDUTILSVALIDATORS_API,
                         USD_UTILS_VALIDATION_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
