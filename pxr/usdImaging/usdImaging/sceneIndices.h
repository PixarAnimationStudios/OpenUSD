//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_SCENE_INDICES_H
#define PXR_USD_IMAGING_USD_IMAGING_SCENE_INDICES_H

#include "pxr/imaging/hd/noticeBatchingSceneIndex.h"
#include "pxr/pxr.h"

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdNoticeBatchingSceneIndex);
TF_DECLARE_REF_PTRS(UsdStage);
TF_DECLARE_REF_PTRS(UsdImagingStageSceneIndex);
TF_DECLARE_REF_PTRS(UsdImagingSelectionSceneIndex);

using UsdImagingSceneIndexAppendCallback =
    std::function<
        HdSceneIndexBaseRefPtr(HdSceneIndexBaseRefPtr const &)>;

/// Info needed to create a chain of filtering scene indices (resolving
/// e.g. USD native instancing) for clients to consume a UsdStage.
struct UsdImagingCreateSceneIndicesInfo
{
    using SceneIndexAppendCallback = UsdImagingSceneIndexAppendCallback;

    /// Stage. Note that it can be set after the scene indices have been
    /// created later by UsdImagingStageSceneIndex::SetStage.
    UsdStageRefPtr stage;
    /// Inputs to UsdImagingStageSceneIndex (note that
    /// includeUnloadedPrims is set automatically when
    /// displayUnloadedPrimsWithBounds is enabled).
    HdContainerDataSourceHandle stageSceneIndexInputArgs;
    /// Add scene index resolving usd draw mode.
    bool addDrawModeSceneIndex = true;
    /// Should we switch the draw mode for unloaded prims to
    /// bounds.
    bool displayUnloadedPrimsWithBounds = false;
    /// A client can insert scene indices after stage scene index.
    SceneIndexAppendCallback overridesSceneIndexCallback;
};

/// Some scene indices in the chain of filtering scene indices created
/// by UsdImagingCreateSceneIndices.
struct UsdImagingSceneIndices
{
    UsdImagingStageSceneIndexRefPtr stageSceneIndex;
    HdNoticeBatchingSceneIndexRefPtr postInstancingNoticeBatchingSceneIndex;
    UsdImagingSelectionSceneIndexRefPtr selectionSceneIndex;
    HdSceneIndexBaseRefPtr finalSceneIndex;    
};

/// Creates a chain of filtering scene indices for clients to consume
/// a UsdStage.
USDIMAGING_API
UsdImagingSceneIndices
UsdImagingCreateSceneIndices(
    const UsdImagingCreateSceneIndicesInfo &createInfo);

/// New API that gets us closer to the direction that all scene indices are
/// created with inputArgs which come by overlaying the container data
/// sources from the application and from
/// HdRendererPlugin::GetSceneIndexInputArgs().
///
/// We have not moved UsdImagingSceneIndexAppendCallback into inputArgs because
/// a std::function is not a value type (no operator==) and, thus, cannot be
/// returned by a HdTypedSampledDataSource. We might also revisit the return
/// type.
///
USDIMAGING_API
UsdImagingSceneIndices
UsdImagingCreateSceneIndices(
    HdContainerDataSourceHandle const &inputArgs,
    const UsdImagingSceneIndexAppendCallback &overridesSceneIndexCallback);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
