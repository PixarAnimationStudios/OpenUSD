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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/renderPass.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/dirtyList.h"
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/indirectDrawBatch.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/renderPassShader.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/shader.h"
#include "pxr/imaging/hd/surfaceShader.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/bind.hpp>

HdRenderPass::HdRenderPass(HdRenderIndex *index)
    : _renderIndex(index)
    , _collectionVersion(0)
    , _collectionChanged(false)
    , _lastCullingDisabledState(false)
{
}

HdRenderPass::HdRenderPass(HdRenderIndex *index,
                           const HdRprimCollection &collection)
    : _renderIndex(index)
    , _collectionVersion(0)
    , _collectionChanged(false)
    , _lastCullingDisabledState(false)
{
    // initialize
    SetRprimCollection(collection);
}

HdRenderPass::~HdRenderPass()
{
    /*NOTHING*/
}

void 
HdRenderPass::SetRprimCollection(HdRprimCollection const& col)
{
    if (col == _collection){
        return;
    }
         
    _collection = col; 
    // Force any cached data based on collection to be refreshed.
    _collectionChanged = true;
    // reset the cached collection version
    _collectionVersion = 0;

    // update dirty list subscription for the new collection.
    // holding shared_ptr for the lifetime of the dirty list.
    bool isMinorChange = true;
    if (not _dirtyList or not _dirtyList->ApplyEdit(col)) {
        _dirtyList.reset(new HdDirtyList(_collection, *_renderIndex));
        isMinorChange = false;
    }

    if (TfDebug::IsEnabled(HD_DIRTY_LIST)) {
        std::stringstream s;
        s << "  Include: \n";
        for (auto i : col.GetRootPaths()) {
            s << "    - " << i << "\n";
        }
        s << "  Exclude: \n";
        for (auto i : col.GetExcludePaths()) {
            s << "    - " << i << "\n";
        }
        s << "  Repr: " << col.GetReprName() << "\n";

        TF_DEBUG(HD_DIRTY_LIST).Msg("RenderPass(%p)::SetRprimCollection (%s) - "
            "constructing new DirtyList(%p) minorChange(%d) \n%s\n",
            (void*)this,
            col.GetName().GetText(),
            (void*)&*_dirtyList,
            isMinorChange,
            s.str().c_str());
    }
}

void
HdRenderPass::_PrepareCommandBuffer(
    HdRenderPassStateSharedPtr const &renderPassState)
{
    HD_TRACE_FUNCTION();
    // ------------------------------------------------------------------- #
    // SCHEDULE PREPARATION
    // ------------------------------------------------------------------- #
    // We know what must be drawn and that the stream needs to be updated, 
    // so iterate over each prim, cull it and schedule it to be drawn.

    HdChangeTracker& tracker = _renderIndex->GetChangeTracker();
    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();

    const int
       collectionVersion = tracker.GetCollectionVersion(_collection.GetName());

    const int shaderBindingsVersion = tracker.GetShaderBindingsVersion();

    const bool 
       skipCulling = TfDebug::IsEnabled(HD_DISABLE_FRUSTUM_CULLING) or
           (caps.multiDrawIndirectEnabled
               and Hd_IndirectDrawBatch::IsEnabledGPUFrustumCulling());

    const bool 
       cameraChanged = true,
       extentsChanged = true,
       collectionChanged = _collectionChanged 
                           or (_collectionVersion != collectionVersion);

    const bool cullingStateJustChanged = 
                                    skipCulling != _lastCullingDisabledState;
    _lastCullingDisabledState = skipCulling;

    bool freezeCulling = TfDebug::IsEnabled(HD_FREEZE_CULL_FRUSTUM);

    // Bypass freezeCulling if  collection has changed
    // Need to add extents in here as well, once they are
    // hooked up to detect proper change.
    if(collectionChanged || cullingStateJustChanged) {
        freezeCulling = false;
    }

    // Now either the collection is dirty or culling needs to be applied.
    if (collectionChanged) {
        HD_PERF_COUNTER_INCR(HdPerfTokens->collectionsRefreshed);
        TF_DEBUG(HD_COLLECTION_CHANGED).Msg("CollectionChanged: %s "
                                            "version: %d -> %d\n", 
                                             _collection.GetName().GetText(),
                                             _collectionVersion,
                                             collectionVersion);
        HdRenderIndex::HdDrawItemView items = _renderIndex->GetDrawItems(_collection);
        _cmdBuffer.SwapDrawItems(&items, shaderBindingsVersion);
        _collectionVersion = collectionVersion;
        _collectionChanged = false;
        HD_PERF_COUNTER_SET(HdTokens->totalItemCount, 
                            _cmdBuffer.GetTotalSize());
    } else {
        // validate command buffer to not include expired drawItems,
        // which could be produced by migrating BARs at the new repr creation.
        _cmdBuffer.RebuildDrawBatchesIfNeeded(shaderBindingsVersion);
    }

    if(skipCulling) {
        // Since culling state is stored across renders,
        // we need to update all items visible state
        _cmdBuffer.SyncDrawItemVisibility(tracker.GetVisibilityChangeCount());
        TF_DEBUG(HD_DRAWITEMS_CULLED).Msg("CULLED: skipped\n");
    }
    else {
        // XXX: this process should be moved to Hd_DrawBatch::PrepareDraw
        //      to be consistent with GPU culling.
        if((not freezeCulling)
            and (collectionChanged or cameraChanged or extentsChanged)) {
            // Re-cull the command buffer. 
            _cmdBuffer.FrustumCull(renderPassState->GetCullMatrix());
        }
        TF_DEBUG(HD_DRAWITEMS_CULLED).Msg("CULLED: %zu drawItems\n", 
                                             _cmdBuffer.GetCulledSize());
    }
}

void
HdRenderPass::Execute(HdRenderPassStateSharedPtr const &renderPassState)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // CPU frustum culling (if chosen)
    _PrepareCommandBuffer(renderPassState);

    // GPU frustum culling (if chosen)
    _cmdBuffer.PrepareDraw(renderPassState);

    _cmdBuffer.ExecuteDraw(renderPassState);
}

void
HdRenderPass::Sync()
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // Sync the dirty list of prims
    _renderIndex->Sync(_dirtyList);
}
