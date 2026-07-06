//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_VALIDATION_USD_LOD_VALIDATORS_VALIDATOR_TOKENS_H
#define PXR_USD_VALIDATION_USD_LOD_VALIDATORS_VALIDATOR_TOKENS_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usdValidation/usdLodValidators/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_LOD_VALIDATOR_NAME_TOKENS                                        \
    ((heuristicLodDomain,                                                    \
      "usdLodValidators:HeuristicLodDomain"))                                \
    ((distanceHeuristicThresholds,                                           \
      "usdLodValidators:DistanceHeuristicThresholds"))                       \
    ((screenSizeHeuristicThresholds,                                         \
      "usdLodValidators:ScreenSizeHeuristicThresholds"))                     \
    ((lodOverrideApiMode,                                                     \
      "usdLodValidators:LodOverrideApiMode"))                                \
    ((lodRootApiHeuristics,                                                  \
      "usdLodValidators:LodRootApiHeuristics"))

#define USD_LOD_VALIDATOR_KEYWORD_TOKENS (UsdLodValidators)

#define USD_LOD_VALIDATION_ERROR_NAME_TOKENS                                 \
    ((lodDomainEmpty,                                                        \
      "LodDomainEmpty"))                                                     \
    ((thresholdsNotStrictlyIncreasing,                                       \
      "ThresholdsNotStrictlyIncreasing"))                                    \
    ((thresholdsNotStrictlyDecreasing,                                       \
      "ThresholdsNotStrictlyDecreasing"))                                    \
    ((blendThresholdBelowThreshold,                                          \
      "BlendThresholdBelowThreshold"))                                       \
    ((blendThresholdAboveThreshold,                                          \
      "BlendThresholdAboveThreshold"))                                       \
    ((blendThresholdAboveNextThreshold,                                      \
      "BlendThresholdAboveNextThreshold"))                                   \
    ((blendThresholdBelowNextThreshold,                                      \
      "BlendThresholdBelowNextThreshold"))                                   \
    ((invalidProjectionMethod,                                               \
      "InvalidProjectionMethod"))                                            \
    ((invalidExtentSize,                                                     \
      "InvalidExtentSize"))                                                  \
    ((invalidOverrideMode,                                                   \
      "InvalidOverrideMode"))                                                \
    ((unreadableOverrideIndex,                                               \
      "UnreadableOverrideIndex"))                                            \
    ((invalidHeuristicTarget,                                                \
      "InvalidHeuristicTarget"))

/// \def USD_LOD_VALIDATOR_NAME_TOKENS
/// Tokens representing validator names. Note that for plugin-provided
/// validators, the names must be prefixed by usdLodValidators:, which is
/// the name of the usdLodValidators plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdLodValidatorNameTokens, USDLODVALIDATORS_API,
                         USD_LOD_VALIDATOR_NAME_TOKENS);

/// \def USD_LOD_VALIDATOR_KEYWORD_TOKENS
/// Tokens representing keywords associated with any validator in the
/// usdLodValidators plugin. Clients can use these to select validators by
/// keyword or to tag new validators.
TF_DECLARE_PUBLIC_TOKENS(UsdLodValidatorKeywordTokens, USDLODVALIDATORS_API,
                         USD_LOD_VALIDATOR_KEYWORD_TOKENS);

/// \def USD_LOD_VALIDATION_ERROR_NAME_TOKENS
/// Tokens representing validation error identifiers.
TF_DECLARE_PUBLIC_TOKENS(UsdLodValidationErrorNameTokens,
                         USDLODVALIDATORS_API,
                         USD_LOD_VALIDATION_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
