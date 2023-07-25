//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_SCENE_INDICES_H
#define PXR_USD_IMAGING_USD_IMAGING_SCENE_INDICES_H

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdStage);
TF_DECLARE_REF_PTRS(UsdImagingStageSceneIndex);
TF_DECLARE_REF_PTRS(UsdImagingSelectionSceneIndex);

/// Info needed to create a chain of filtering scene indices (resolving
/// e.g. USD native instancing) for clients to consume a UsdStage.
struct UsdImagingSceneIndicesCreateInfo
{
    using SceneIndexAppendCallback =
        std::function<
            HdSceneIndexBaseRefPtr(HdSceneIndexBaseRefPtr const &)>;

    /// Stage. Note that it can be set after the scene indices have been
    /// created later by UsdImagingStageSceneIndex::SetStage.
    UsdStageRefPtr stage;
    /// Inputs to UsdImagingStageSceneIndex (note that
    /// includeUnloadedPrims is set automatically when
    ///displayUnloadedPrimsWithBounds is enabled).
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
/// by UsdImagingInstantiateSceneIndices.
struct UsdImagingSceneIndices
{
    UsdImagingStageSceneIndexRefPtr stageSceneIndex;
    UsdImagingSelectionSceneIndexRefPtr selectionSceneIndex;
    HdSceneIndexBaseRefPtr finalSceneIndex;    
};

/// Creates a chain of filtering scene indices for clients to consume
/// a UsdStage.
USDIMAGING_API
UsdImagingSceneIndices
UsdImagingInstantiateSceneIndices(
    const UsdImagingSceneIndicesCreateInfo &createInfo);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
