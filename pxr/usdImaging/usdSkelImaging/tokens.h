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

#define USD_SKEL_IMAGING_EXT_COMPUTATION_TYPE_TOKENS \
    (points) \
    (normals)

TF_DECLARE_PUBLIC_TOKENS(
    UsdSkelImagingExtComputationTypeTokens, USDSKELIMAGING_API,
    USD_SKEL_IMAGING_EXT_COMPUTATION_TYPE_TOKENS);

#define USD_SKEL_IMAGING_PRIM_TYPE_TOKENS \
    (skeleton)                            \
    (skelAnimation)                       \
    (skelBlendShape)

TF_DECLARE_PUBLIC_TOKENS(
    UsdSkelImagingPrimTypeTokens,
    USDSKELIMAGING_API, USD_SKEL_IMAGING_PRIM_TYPE_TOKENS);

#define USD_SKEL_IMAGING_EXT_COMPUTATION_NAME_TOKENS                \
    ((pointsAggregatorComputation, "skinningPointsInputAggregatorComputation")) \
    ((pointsComputation,           "skinningPointsComputation")) \
    ((normalsAggregatorComputation, "skinningNormalsInputAggregatorComputation")) \
    ((normalsComputation,           "skinningNormalsComputation"))

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
    (numBlendShapeOffsetRanges)                                       \
    (restNormals)                                                     \
    (faceVertexIndices)                                               \
    (hasFaceVaryingNormals)

TF_DECLARE_PUBLIC_TOKENS(
    UsdSkelImagingExtAggregatorComputationInputNameTokens, USDSKELIMAGING_API,
    USD_SKEL_IMAGING_EXT_AGGREGATOR_COMPUTATION_INPUT_NAME_TOKENS);

#define USD_SKEL_IMAGING_EXT_COMPUTATION_INPUT_NAME_TOKENS \
    (blendShapeWeights)                                    \
    (skinningXforms)                                       \
    (skinningScaleXforms)                                  \
    (skinningDualQuats)                                    \
    ((skelLocalToCommonSpace, "skelLocalToWorld"))         \
    ((commonSpaceToPrimLocal, "primWorldToLocal"))

// Legacy tokens used in the ext computation that make stronger assumptions
// about the transforms than necessary.
//
// That is, the ext computation consumes both
//                      primWorldToLocal and skelLocalToWorld
// but only ever uses their product
//    skelToPrimLocal = primWorldToLocal * skelLocalToWorld.
//
// In other words, the two matrices are only ever used to go from skel to prim
// space and the intermediate space used to achieve this is irrelevant to the
// ext computation.
//
// Let us rename
//     primWorldToLocal to commonSpaceToPrimLocal and
//     skelLocalToWorld to skelLocalToCommonSpace.
// to reflect this.
//
// For the Hydra 1.0 implementation we use indeed world space as space common
// to all prims under a skel root.
// For the Hydra 2.0 implementation we use the space that is also common to all
// prims under a skel root but that is defined by the
// UsdSkelImagingXformResolver.
//
TF_DECLARE_PUBLIC_TOKENS(
    UsdSkelImagingExtComputationInputNameTokens, USDSKELIMAGING_API,
    USD_SKEL_IMAGING_EXT_COMPUTATION_INPUT_NAME_TOKENS);

#define USD_SKEL_IMAGING_EXT_COMPUTATION_LEGACY_INPUT_NAME_TOKENS \
    (skelLocalToWorld)                                            \
    (primWorldToLocal)

TF_DECLARE_PUBLIC_TOKENS(
    UsdSkelImagingExtComputationLegacyInputNameTokens, USDSKELIMAGING_API,
    USD_SKEL_IMAGING_EXT_COMPUTATION_LEGACY_INPUT_NAME_TOKENS);


#define USD_SKEL_IMAGING_EXT_COMPUTATION_OUTPUT_NAME_TOKENS \
    (skinnedPoints)                                         \
    (skinnedNormals)

TF_DECLARE_PUBLIC_TOKENS(
    UsdSkelImagingExtComputationOutputNameTokens, USDSKELIMAGING_API,
    USD_SKEL_IMAGING_EXT_COMPUTATION_OUTPUT_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_SKEL_IMAGING_TOKENS_H
