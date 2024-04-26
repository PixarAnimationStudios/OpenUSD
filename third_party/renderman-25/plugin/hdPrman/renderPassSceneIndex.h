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
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PASS_SCENE_INDEX_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PASS_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/collectionExpressionEvaluator.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include <mutex>
#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdPrman_RenderPassSceneIndex);

/// HdPrman_RenderPassSceneIndex applies the active render pass
/// specified in the HdSceneGlobalsSchema, modifying the scene
/// contents as needed.
///
class HdPrman_RenderPassSceneIndex : 
    public HdSingleInputFilteringSceneIndexBase
{
public:
    static HdPrman_RenderPassSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    HdPrman_RenderPassSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex);
    ~HdPrman_RenderPassSceneIndex();

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    // State specified by a render pass.
    // Collection evaluators are set sparsely, corresponding to
    // the presence of the collection in the render pass schema.
    struct _RenderPassState {
        SdfPath renderPassPath;
        std::optional<HdCollectionExpressionEvaluator> matteEval;
        std::optional<HdCollectionExpressionEvaluator> renderVisEval;
        std::optional<HdCollectionExpressionEvaluator> cameraVisEval;
    };

    // Pull on the scene globals schema for the active render pass,
    // computing and caching any state as needed.
    _RenderPassState const& _PullActiveRenderPasssState() const;

    // State for the render pass that has been applied.
    mutable std::unique_ptr<_RenderPassState> _activeRenderPass;

    // Mutex protecting _activeRenderPass.
    mutable std::mutex _activeRenderPassMutex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
