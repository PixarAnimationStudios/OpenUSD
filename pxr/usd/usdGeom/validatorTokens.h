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
#include "pxr/usd/usdGeom/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_GEOM_VALIDATOR_NAME_TOKENS                              \
    ((stageMetadataChecker, "usdGeom:StageMetadataChecker"))        \
    ((subsetFamilies, "usdGeom:SubsetFamilies"))                    \
    ((subsetParentIsImageable, "usdGeom:SubsetParentIsImageable"))  \
    ((gPrimDescendantValidator, "usdGeom:GPrimDescendantValidator"))

#define USD_GEOM_VALIDATOR_KEYWORD_TOKENS                           \
    (UsdGeomSubset)                                                 \
    (UsdGeomValidators)

#define USD_GEOM_VALIDATION_ERROR_NAME_TOKENS                     \
    ((missingMetersPerUnitMetadata, "MissingMetersPerUnitMetadata"))    \
    ((missingUpAxisMetadata, "MissingUpAxisMetadata"))                  \
    ((invalidSubsetFamily,  "InvalidSubsetFamily"))                     \
    ((notImageableSubsetParent, "NotImageableSubsetParent"))            \
    ((invalidGPrimDescendant, "InvalidGPrimDescendant"))
/// \def USD_GEOM_VALIDATOR_NAME_TOKENS
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdGeom:, which is the name of
/// the usdGeom plugin.
    TF_DECLARE_PUBLIC_TOKENS(UsdGeomValidatorNameTokens, USDGEOM_API,
                             USD_GEOM_VALIDATOR_NAME_TOKENS);

/// \def USD_GEOM_VALIDATOR_KEYWORD_TOKENS
/// Tokens representing keywords associated with any validator in the usdGeom
/// plugin. Clients can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
    TF_DECLARE_PUBLIC_TOKENS(UsdGeomValidatorKeywordTokens, USDGEOM_API,
                             USD_GEOM_VALIDATOR_KEYWORD_TOKENS);

/// \def USD_GEOM_VALIDATION_ERROR_NAME_TOKENS
/// Tokens representing validation error identifier.
TF_DECLARE_PUBLIC_TOKENS(UsdGeomValidationErrorNameTokens, USDGEOM_API, 
                         USD_GEOM_VALIDATION_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
