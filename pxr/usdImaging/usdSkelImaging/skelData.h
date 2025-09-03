//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_SKEL_DATA_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_SKEL_DATA_H

#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usdSkel/animMapper.h"
#include "pxr/usd/usdSkel/topology.h"

#include "pxr/base/vt/array.h"

#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdSceneIndexBase);

/// Some of the data necessary to compute the skinning transforms of a skeleton.
/// These data come from the skeleton and the skelAnimation's joints.
struct UsdSkelImagingSkelData
{
    /// Path of deformable prim. Used only for warnings/error messages.
    SdfPath primPath;

    /// Path of animation prim.
    SdfPath animationSource;

    /// From skeleton's joints.
    UsdSkelTopology topology;

    /// Remapping of skelAnimation's data to skeleton's hierarchy.
    UsdSkelAnimMapper animMapper;

    /// From skeleton.
    VtArray<GfMatrix4f> bindTransforms;
    VtArray<GfMatrix4f> inverseBindTransforms;
};

/// Compute data for prim in scene index.
USDSKELIMAGING_API
UsdSkelImagingSkelData
UsdSkelImagingComputeSkelData(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath);

/// Compute skinning transforms for extComputation.
USDSKELIMAGING_API
VtArray<GfMatrix4f>
UsdSkelImagingComputeSkinningTransforms(
    const UsdSkelImagingSkelData &data,
    /// From skeleton (might not be needed).
    HdMatrix4fArrayDataSourceHandle const &restTransforms,
    /// From skelAnimation.
    const VtArray<GfVec3f> &translations,
    const VtArray<GfQuatf> &rotations,
    const VtArray<GfVec3h> &scales);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
