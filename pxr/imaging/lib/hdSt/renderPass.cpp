//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hdSt/renderPass.h"

#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/imaging/hdSt/indirectDrawBatch.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"

#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/gf/frustum.h"

#include "pxr/imaging/glf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

HdSt_RenderPass::HdSt_RenderPass(HdRenderIndex *index,
                                 HdRprimCollection const &collection)
    : HdRenderPass(index, collection)
    , _lastSettingsVersion(0)
    , _useTinyPrimCulling(false)
    , _collectionVersion(0)
    , _collectionChanged(false)
{
}

HdSt_RenderPass::~HdSt_RenderPass()
{
    /* NOTHING */
}

void
HdSt_RenderPass::_Execute(HdRenderPassStateSharedPtr const &renderPassState,
                          TfTokenVector const& renderTags)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    GLF_GROUP_FUNCTION();

    // Downcast render pass state
    HdStRenderPassStateSharedPtr stRenderPassState =
        boost::dynamic_pointer_cast<HdStRenderPassState>(
        renderPassState);
    TF_VERIFY(stRenderPassState);

    _PrepareCommandBuffer();
    
    // CPU frustum culling (if chosen)
    _Cull(stRenderPassState);

    // Downcast the resource registry
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::dynamic_pointer_cast<HdStResourceRegistry>(
        GetRenderIndex()->GetResourceRegistry());
    TF_VERIFY(resourceRegistry);

    // renderTags.empty() means draw everything in the collection.
    if (renderTags.empty()) {
        for (_HdStCommandBufferMap::iterator it  = _cmdBuffers.begin();
                                           it != _cmdBuffers.end(); it++) {
            it->second.PrepareDraw(stRenderPassState, resourceRegistry);
            it->second.ExecuteDraw(stRenderPassState, resourceRegistry);
        }
    } else {
        TF_FOR_ALL(tag, renderTags) {
            // Check if the render tag has an associated command buffer
            if (_cmdBuffers.count(*tag) == 0) {
                continue;
            }

            // GPU frustum culling (if chosen)
            _cmdBuffers[*tag].PrepareDraw(stRenderPassState, resourceRegistry);
            _cmdBuffers[*tag].ExecuteDraw(stRenderPassState, resourceRegistry);
        }
    }
}

void
HdSt_RenderPass::_MarkCollectionDirty()
{
    // Force any cached data based on collection to be refreshed.
    _collectionChanged = true;
    _collectionVersion = 0;
}

void
HdSt_RenderPass::_PrepareCommandBuffer()
{
    HD_TRACE_FUNCTION();
    GLF_GROUP_FUNCTION();

    // -------------------------------------------------------------------
    // SCHEDULE PREPARATION
    // -------------------------------------------------------------------
    // We know what must be drawn and that the stream needs to be updated, 
    // so iterate over each prim, cull it and schedule it to be drawn.

    HdChangeTracker const &tracker = GetRenderIndex()->GetChangeTracker();
    HdRprimCollection const &collection = GetRprimCollection();

    const int collectionVersion =
        tracker.GetCollectionVersion(collection.GetName());

    const int batchVersion = tracker.GetBatchVersion();

    const bool collectionChanged = _collectionChanged ||
        (_collectionVersion != collectionVersion);

    // Now either the collection is dirty or culling needs to be applied.
    if (collectionChanged) {
        HD_PERF_COUNTER_INCR(HdPerfTokens->collectionsRefreshed);
        TF_DEBUG(HD_COLLECTION_CHANGED).Msg("CollectionChanged: %s "
                                            "(repr = %s)"
                                            "version: %d -> %d\n", 
                                             collection.GetName().GetText(),
                                             collection.GetReprSelector().GetText(),
                                             _collectionVersion,
                                             collectionVersion);

        HdRenderIndex::HdDrawItemView items = 
            GetRenderIndex()->GetDrawItems(collection);

        // This loop will extract the tags and bucket the geometry in 
        // the different command buffers.
        size_t itemCount = 0;
        _cmdBuffers.clear();
        for (HdRenderIndex::HdDrawItemView::iterator it = items.begin();
                                                    it != items.end(); it++ ) {
            _cmdBuffers[it->first].SwapDrawItems(
                // Downcast the HdDrawItem entries to HdStDrawItems:
                reinterpret_cast<std::vector<HdStDrawItem const*>*>(&it->second),
                batchVersion);
            itemCount += _cmdBuffers[it->first].GetTotalSize();
        }

        _collectionVersion = collectionVersion;
        _collectionChanged = false;
        HD_PERF_COUNTER_SET(HdTokens->totalItemCount, itemCount);
    } else {
        // validate command buffer to not include expired drawItems,
        // which could be produced by migrating BARs at the new repr creation.
        for (_HdStCommandBufferMap::iterator it  = _cmdBuffers.begin(); 
                                           it != _cmdBuffers.end(); it++) {
            it->second.RebuildDrawBatchesIfNeeded(batchVersion);
        }
    }

    // -------------------------------------------------------------------
    // RENDER SETTINGS
    // -------------------------------------------------------------------
    HdRenderDelegate *renderDelegate = GetRenderIndex()->GetRenderDelegate();
    int currentSettingsVersion = renderDelegate->GetRenderSettingsVersion();
    if (_lastSettingsVersion != currentSettingsVersion) {
        _lastSettingsVersion = currentSettingsVersion;
        _useTinyPrimCulling = renderDelegate->GetRenderSetting<bool>(
            HdStRenderSettingsTokens->enableTinyPrimCulling, false);
    }
    for (_HdStCommandBufferMap::iterator it = _cmdBuffers.begin();
            it != _cmdBuffers.end(); ++it) {
        it->second.SetEnableTinyPrimCulling(_useTinyPrimCulling);
    }
}

void
HdSt_RenderPass::_Cull(
    HdStRenderPassStateSharedPtr const &renderPassState)
{
    // This process should be moved to HdSt_DrawBatch::PrepareDraw
    // to be consistent with GPU culling.

    GlfContextCaps const &caps = GlfContextCaps::GetInstance();
    HdChangeTracker const &tracker = GetRenderIndex()->GetChangeTracker();

    const bool 
       skipCulling = TfDebug::IsEnabled(HD_DISABLE_FRUSTUM_CULLING) ||
           (caps.multiDrawIndirectEnabled
               && HdSt_IndirectDrawBatch::IsEnabledGPUFrustumCulling());
    bool freezeCulling = TfDebug::IsEnabled(HD_FREEZE_CULL_FRUSTUM);

    if(skipCulling) {
        // Since culling state is stored across renders,
        // we need to update all items visible state
        for (_HdStCommandBufferMap::iterator it = _cmdBuffers.begin(); 
                                           it != _cmdBuffers.end(); it++) {
            it->second.SyncDrawItemVisibility(tracker.GetVisibilityChangeCount());
        }

        TF_DEBUG(HD_DRAWITEMS_CULLED).Msg("CULLED: skipped\n");
    }
    else {
        if (!freezeCulling) {
            // Re-cull the command buffer. 
            for (_HdStCommandBufferMap::iterator it  = _cmdBuffers.begin(); 
                 it != _cmdBuffers.end(); it++) {
                it->second.FrustumCull(renderPassState->GetCullMatrix());
            }
        }

        if (TfDebug::IsEnabled(HD_DRAWITEMS_CULLED)) {
            for (_HdStCommandBufferMap::iterator it  = _cmdBuffers.begin(); 
                 it != _cmdBuffers.end(); it++) {
                TF_DEBUG(HD_DRAWITEMS_CULLED).Msg("CULLED: %zu drawItems\n", 
                                                  it->second.GetCulledSize());
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
