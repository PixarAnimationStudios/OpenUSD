//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_TOKENS_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdSkelImaging/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_SKEL_IMAGING_PRIM_TYPE_TOKENS \
    (skeleton)                            \
    (skelAnimation)                       \
    (skelBlendShape)

TF_DECLARE_PUBLIC_TOKENS(
    UsdSkelImagingPrimTypeTokens,
    USDSKELIMAGING_API, USD_SKEL_IMAGING_PRIM_TYPE_TOKENS);

#define USD_SKEL_IMAGING_EXT_COMPUTATION_NAME_TOKENS                \
    ((aggregatorComputation, "skinningInputAggregatorComputation")) \
    ((computation,           "skinningComputation"))

TF_DECLARE_PUBLIC_TOKENS(
    UsdSkelImagingExtComputationNameTokens, USDSKELIMAGING_API,
    USD_SKEL_IMAGING_EXT_COMPUTATION_NAME_TOKENS);

#define USD_SKEL_IMAGING_EXT_AGGREGATOR_COMPUTATION_INPUT_NAME_TOKENS \
    (restPoints)                                                      \
    (geomBindXform)                                                   \
    (influences)                                                      \
    (numInfluencesPerComponent)                                       \
    (hasConstantInfluences)                                           \
    (blendShapeOffsets)                                               \
    (blendShapeOffsetRanges)                                          \
    (numBlendShapeOffsetRanges)

TF_DECLARE_PUBLIC_TOKENS(
    UsdSkelImagingExtAggregatorComputationInputNameTokens, USDSKELIMAGING_API,
    USD_SKEL_IMAGING_EXT_AGGREGATOR_COMPUTATION_INPUT_NAME_TOKENS);

#define USD_SKEL_IMAGING_EXT_COMPUTATION_INPUT_NAME_TOKENS \
    (primWorldToLocal)                                     \
    (blendShapeWeights)                                    \
    (skinningXforms)                                       \
    (skinningScaleXforms)                                  \
    (skinningDualQuats)                                    \
    (skelLocalToWorld)

TF_DECLARE_PUBLIC_TOKENS(
    UsdSkelImagingExtComputationInputNameTokens, USDSKELIMAGING_API,
    USD_SKEL_IMAGING_EXT_COMPUTATION_INPUT_NAME_TOKENS);

#define USD_SKEL_IMAGING_EXT_COMPUTATION_OUTPUT_NAME_TOKENS \
    (skinnedPoints)                                    

TF_DECLARE_PUBLIC_TOKENS(
    UsdSkelImagingExtComputationOutputNameTokens, USDSKELIMAGING_API,
    USD_SKEL_IMAGING_EXT_COMPUTATION_OUTPUT_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_SKEL_IMAGING_TOKENS_H
