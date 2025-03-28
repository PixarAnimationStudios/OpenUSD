//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_BLEND_SHAPE_DATA_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_BLEND_SHAPE_DATA_H

#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/vt/array.h"

#include "pxr/base/tf/declarePtrs.h"

#include <map>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdSceneIndexBase);

///
/// Data to determine sub shape contributions.
///
/// Sub shape:
///
/// A sub shape consists of offsets for a subset of points or all points of a
/// deformable prim with SkelBindingAPI.
///
/// The offsets ultimately applied to the prim are computed as linear
/// combination of the sub shapes.
///
/// Sub shapes come from the BlendShape prim's targeted by the deformable prim.
/// Each BlendShape can provide several subshapes: one from BlendShape.offsets
/// and several from BlendShape.inbetweens:BETWEEN_NAME:offsets.
///
struct UsdSkelImagingWeightAndSubShapeIndex
{
    /// Weight authored for BlendShape.inbetweens:BETWEEN_NAME
    ///
    /// weight = 1.0 for the sub shape corresponding to a BlendShape.offsets.
    ///
    /// If weight = 0.0, this pair does not correspond to any sub shape and
    /// subShapeIndex = -1.
    float weight;

    /// Index to sub shape. -1 if this pair does not correspond to a sub shape.
    int subShapeIndex;
};

using UsdSkelImagingWeightsAndSubShapeIndices =
    std::vector<UsdSkelImagingWeightAndSubShapeIndex>;

/// Data for skinned prim to compute the skel ext computation inputs related to
/// blend shapes. These data come from the skeleton and the skelBinding, but
/// not from the skelAnimation.
struct UsdSkelImagingBlendShapeData
{
    /// Path of deformable prim. Used only for warnings/error messages.
    SdfPath primPath;

    /// List of (offset, subShapeIndex)
    VtArray<GfVec4f> blendShapeOffsets;
    /// For each point, pair of indices into blendShapeOffsets.
    VtArray<GfVec2i> blendShapeOffsetRanges;

    size_t numSubShapes;

    /// For each blend shape name in SkelBindingAPI.skel:blendShapes, a
    /// list of (weight, subShapeIndex).
    ///
    /// Includes (0.0, -1) to indicate that weight zero in
    /// SkelAnimation.blendShapeWeights corresponds to applying any
    /// sub shape from that BlendShape.
    ///
    /// Includes (1.0, sub shape index) for the BlendShape.offsets and
    /// (weight, sub shape index) for the BlendShape.inbetweens:BETWEEN_NAME.
    ///
    std::map<TfToken, UsdSkelImagingWeightsAndSubShapeIndices>
        blendShapeNameToWeightsAndSubShapeIndices;
};

/// Computed blend shape for deformable prim with skelBindingAPI.
USDSKELIMAGING_API
UsdSkelImagingBlendShapeData
UsdSkelImagingComputeBlendShapeData(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath);

/// blendShapeWeights for skel ext computation inputs.
///
/// One weight for each sub shape.
///
USDSKELIMAGING_API
VtArray<float>
UsdSkelImagingComputeBlendShapeWeights(
    const UsdSkelImagingBlendShapeData &data,
    // from skel animation
    const VtArray<TfToken> &blendShapeNames,
    // from skel animation
    const VtArray<float> &blendShapeWeights);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
