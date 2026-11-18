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
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/envSetting.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

class HdMergingSceneIndex;
TF_DECLARE_REF_PTRS(HdMergingSceneIndex);

/// The open-source version of the Hydra Scene Debugger cannot descend into
/// HdEncapsulatingSceneIndexBase. Thus, we have an environment variable to
/// disable the use HdEncapsulatingSceneIndexBase.
///
HD_API
extern TfEnvSetting<bool> HD_USE_ENCAPSULATING_SCENE_INDICES;

/// Make a scene index that encapsulates the given \p encapsulatedScene and 
/// (recursively) all its inputs until a scene in \p inputScenes is hit.
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

/// Traverse the scene index graph starting from \p sceneIndex (breadth-first),
/// descending into both HdFilteringSceneIndexBase::GetInputScenes() and
/// HdEncapsulatingSceneIndexBase::GetEncapsulatedScenes(). \p visitor is
/// invoked on each node; return false from the visitor to stop traversal.
///
HD_API
void HdVisitSceneIndexGraph(
    const HdSceneIndexBaseRefPtr &sceneIndex,
    const std::function<bool(const HdSceneIndexBaseRefPtr &)> &visitor);

/// Make a merging scene index tagged with
/// HdSceneIndexTokens->sceneAssemblyMergingTag.
///
/// This tag is used to identify the scene index responsible for merging
/// scene assembly inputs. Use HdFindSceneAssemblyMergingSceneIndex to
/// locate it by traversing the input graph of a scene index.
///
HD_API
HdMergingSceneIndexRefPtr
HdMakeSceneAssemblyMergingSceneIndex();

/// Traverses the scene index graph starting from \p sceneIndex and returns the
/// first merging scene index that has the
/// HdSceneIndexTokens->sceneAssemblyMergingTag tag, or nullptr if none was
/// found.
///
HD_API
HdMergingSceneIndexRefPtr
HdFindSceneAssemblyMergingSceneIndex(
    const HdSceneIndexBaseRefPtr &sceneIndex);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_SCENE_INDEX_UTIL_H
