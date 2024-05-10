//
// Copyright 2024 Pixar
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
