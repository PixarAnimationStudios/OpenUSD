//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_SCENE_INDEX_UTIL_H
#define PXR_IMAGING_HD_SCENE_INDEX_UTIL_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

/// The open-source version of the Hydra Scene Browser cannot descend into
/// HdEncapsulatingSceneIndexBase. Thus, we have an environment variable to
/// disable the use HdEncapsulatingSceneIndexBase.
///
HD_API
extern TfEnvSetting<bool> HD_USE_ENCAPSULATING_SCENE_INDICES;

/// Make a scene index that encapsulates the given scene and (recursively)
/// all its inputs until a given input scene is hit.
///
/// The resulting scene index is simply forwarding any calls to the
/// given encapsulated scene index.
///
/// See HdEncapsulatingSceneIndexBase and HdFilteringSceneIndexBase for
/// details.
///
/// The resulting scene index should be thought of one node in the
/// nested scene index graph. The inputs of this node are the given
/// input scenes. The terminal node in the graph internal to the resulting
/// scene index is the given encapsulated scene index.
///
HD_API
HdSceneIndexBaseRefPtr
HdMakeEncapsulatingSceneIndex(
    const std::vector<HdSceneIndexBaseRefPtr> &inputScenes,
    HdSceneIndexBaseRefPtr const &encapsulatedScene);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_SCENE_INDEX_UTIL_H
