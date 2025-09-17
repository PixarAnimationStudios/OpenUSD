//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_VALIDATION_USD_GEOM_VALIDATORS_TOKENS_H
#define PXR_USD_VALIDATION_USD_GEOM_VALIDATORS_TOKENS_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usdValidation/usdGeomValidators/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_GEOM_VALIDATOR_NAME_TOKENS                                         \
    ((stageMetadataChecker, "usdGeomValidators:StageMetadataChecker"))         \
    ((subsetFamilies, "usdGeomValidators:SubsetFamilies"))                     \
    ((subsetParentIsImageable, "usdGeomValidators:SubsetParentIsImageable"))

#define USD_GEOM_VALIDATOR_KEYWORD_TOKENS                                      \
    (UsdGeomSubset)                                                            \
    (UsdGeomValidators)

#define USD_GEOM_VALIDATION_ERROR_NAME_TOKENS                                  \
    ((missingMetersPerUnitMetadata, "MissingMetersPerUnitMetadata"))           \
    ((missingUpAxisMetadata, "MissingUpAxisMetadata"))                         \
    ((invalidSubsetFamily, "InvalidSubsetFamily"))                             \
    ((notImageableSubsetParent, "NotImageableSubsetParent"))

/// \def USD_GEOM_VALIDATOR_NAME_TOKENS
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdGeomValidators:, which is the
/// name of the usdGeomValidators plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdGeomValidatorNameTokens, USDGEOMVALIDATORS_API,
                         USD_GEOM_VALIDATOR_NAME_TOKENS);

/// \def USD_GEOM_VALIDATOR_KEYWORD_TOKENS
/// Tokens representing keywords associated with any validator in the usdGeom
/// plugin. Clients can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(UsdGeomValidatorKeywordTokens, USDGEOMVALIDATORS_API,
                         USD_GEOM_VALIDATOR_KEYWORD_TOKENS);

/// \def USD_GEOM_VALIDATION_ERROR_NAME_TOKENS
/// Tokens representing validation error identifier.
TF_DECLARE_PUBLIC_TOKENS(UsdGeomValidationErrorNameTokens,
                         USDGEOMVALIDATORS_API,
                         USD_GEOM_VALIDATION_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
