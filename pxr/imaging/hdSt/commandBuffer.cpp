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
#include "pxr/imaging/hdSt/commandBuffer.h"
#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/indirectDrawBatch.h"
#include "pxr/imaging/hdSt/pipelineDrawBatch.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/materialNetworkShader.h"

#include "pxr/imaging/hgi/computeCmds.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"

#include "pxr/base/work/loops.h"

#include "pxr/base/tf/hash.h"

#include <tbb/enumerable_thread_specific.h>

#include <algorithm>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE


HdStCommandBuffer::HdStCommandBuffer()
    : _visibleSize(0)
    , _visChangeCount(0)
    , _drawBatchesVersion(0)
{
    /*NOTHING*/
}

HdStCommandBuffer::~HdStCommandBuffer() = default;

namespace {

HdSt_DrawBatchSharedPtr
_NewDrawBatch(HdStDrawItemInstance * drawItemInstance, 
              bool const usePipelineDrawBatch,
              bool const allowTextureResourceRebinding)
{
    if (usePipelineDrawBatch) {
        return std::make_shared<HdSt_PipelineDrawBatch>(drawItemInstance);
    } else {
        return std::make_shared<HdSt_IndirectDrawBatch>(
                drawItemInstance, true, allowTextureResourceRebinding);
    }
}

TF_DEFINE_ENV_SETTING(HDST_DRAW_BATCH_TEXTURE_AGGREGATION_THRESHOLD, 10,
    "Specifies the draw item instance count threshold "
    "for aggregating draw items which use distinct textures.");

size_t
_GetTextureAggregationThreshold()
{
    static const size_t threshold =
        TfGetEnvSetting(HDST_DRAW_BATCH_TEXTURE_AGGREGATION_THRESHOLD);
    return threshold;
}

// Sort draw item instances by texture so that the number of texture
// changes is minimized within a sequence of aggregated draw item instances.
struct _SortByTexture {
    bool operator() (HdStDrawItemInstance const *aInstance,
                     HdStDrawItemInstance const *bInstance) {
        HdStDrawItem const *a = aInstance->GetDrawItem();
        HdStDrawItem const *b = bInstance->GetDrawItem();

        size_t const textureA =
            a->GetMaterialNetworkShader()->ComputeTextureSourceHash();
        size_t const textureB =
            b->GetMaterialNetworkShader()->ComputeTextureSourceHash();
        return textureA < textureB;
    }
};

// Use a cheap bucketing strategy to reduce to number of comparison tests
// required to figure out if a draw item can be batched.
// Test against the previous draw item's hash and batch prior to looking up
// the map.
struct _BatchMap {
    using Key = size_t;

    _BatchMap() : _prevKey(0), _prevBatchForKey() {}

    // Returns the new draw batch if one is created for the draw item
    // instance being inserted, otherwise returns nullptr when the
    // draw item instance has been appended to an existing draw batch.
    HdSt_DrawBatchSharedPtr InsertOrAppend(
        Key key,
        HdStDrawItemInstance *drawItemInstance,
        bool usePipelineDrawBatch,
        bool allowTextureResourceRebinding) {

        // Do a quick check to see if the draw item can be batched with the
        // previous draw item, before looking up in the map.
        if (key == _prevKey && _prevBatchForKey) {
            if (_prevBatchForKey->Append(drawItemInstance)) {
                return {};
            }
        }

        HdSt_DrawBatchSharedPtrVector &batchesForKey = _batchesByKey[key];
        for (HdSt_DrawBatchSharedPtr &batch : batchesForKey) {
            if (batch->Append(drawItemInstance)) {
                _SetPreviousBatchForKey(key, batch);
                return {};
            }
        }

        HdSt_DrawBatchSharedPtr newBatch =
                _NewDrawBatch(drawItemInstance,
                        usePipelineDrawBatch, allowTextureResourceRebinding);

        batchesForKey.push_back(newBatch);
        _SetPreviousBatchForKey(key, newBatch);

        return newBatch;
    }

private:
    void _SetPreviousBatchForKey(
        Key key, HdSt_DrawBatchSharedPtr const &batch) {
        _prevKey = key;
        _prevBatchForKey = batch;
    }

    std::unordered_map<Key, HdSt_DrawBatchSharedPtrVector> _batchesByKey;

    Key _prevKey;
    HdSt_DrawBatchSharedPtr _prevBatchForKey;
};

void
_InsertDrawItemInstance(
    HdSt_DrawBatchSharedPtrVector *drawBatches,
    HdStDrawItemInstance *drawItemInstance,
    _BatchMap *batchMap,
    bool const usePipelineDrawBatch,
    bool const allowTextureResourceRebinding)
{
    HdStDrawItem const *drawItem = drawItemInstance->GetDrawItem();

    // The draw item instances in a batch need to have compatible
    // pipeline configurations and resource allocations.
    // Currently, draw items with distinct geometric shader hashes
    // or buffer array hashes can never be part of the same batch.
    // We combine these two hashes into a key that can be used to to
    // reduce the number of batches which need to be considered
    // as candidate batches.
    size_t key = TfHash::Combine(
        drawItem->GetGeometricShader()->ComputeHash(),
        drawItem->GetBufferArraysHash()
    );

    // When we're not allowing texture resource rebinding within a
    // batch, we'll also combine the texture source hash into the key.
    // (Note the texture source hash will be 0 for bindless textures).
    if (!allowTextureResourceRebinding) {
        size_t const textureHash =
            drawItem->GetMaterialNetworkShader()->ComputeTextureSourceHash();
        key = TfHash::Combine(key, textureHash);
    }

    // Keep track of newly created draw batches.
    if (HdSt_DrawBatchSharedPtr newBatch =
        batchMap->InsertOrAppend(
                key, drawItemInstance,
                    usePipelineDrawBatch, allowTextureResourceRebinding)) {
        drawBatches->push_back(newBatch);
    }
}

void
_BatchDrawItemInstances(
    HdSt_DrawBatchSharedPtrVector *drawBatches,
    std::vector<HdStDrawItemInstance> &instances,
    bool const usePipelineDrawBatch)
{
    _BatchMap batchMap;
    for (HdStDrawItemInstance &drawItemInstance : instances) {
        _InsertDrawItemInstance(
            drawBatches,
            &drawItemInstance,
            &batchMap,
            usePipelineDrawBatch,
            /*allowTextureResourceRebinding=*/false);
    }
}

bool
_HasTextureResourceBinding(HdStDrawItemInstance const *instance)
{
    if (HdSt_MaterialNetworkShaderSharedPtr const &materialNetworkShader =
            instance->GetDrawItem()->GetMaterialNetworkShader()) {
        return (materialNetworkShader->ComputeTextureSourceHash() != 0);
    }
    return false;
}

bool
_ShouldAttemptToAggregate(
    std::vector<HdStDrawItemInstance const *> const &instances)
{
    size_t const threshold = _GetTextureAggregationThreshold();
    return (instances.size() <= threshold) &&
           _HasTextureResourceBinding(instances.front());
}

void
_AggregateDrawBatches(
    HdSt_DrawBatchSharedPtrVector *drawBatches,
    bool const usePipelineDrawBatch)
{
    HD_TRACE_FUNCTION();

    HdSt_DrawBatchSharedPtrVector result;
    result.reserve(drawBatches->size());

    // Collect draw item instances to aggregate
    std::vector<HdStDrawItemInstance *> toAggregate;

    for (auto const &batch : *drawBatches) {
        std::vector<HdStDrawItemInstance const *> const &instances =
                batch->GetDrawItemInstances();
        if (_ShouldAttemptToAggregate(instances)) {
            for (auto const *instance : instances) {
                toAggregate.push_back(
                    const_cast<HdStDrawItemInstance *>(instance));
            }
        } else {
            result.push_back(batch);
        }
    }

    if (toAggregate.empty()) {
        return;
    }

    // Sort the draw item instances to improve sequential coherence
    // within the resulting aggregated draw batches.
    std::sort(toAggregate.begin(), toAggregate.end(), _SortByTexture());

    _BatchMap batchMap;
    for (HdStDrawItemInstance *drawItemInstance : toAggregate) {
        _InsertDrawItemInstance(
            &result,
            drawItemInstance,
            &batchMap,
            usePipelineDrawBatch,
            /*allowTextureResourceRebinding=*/true);
    }

    drawBatches->swap(result);
}

bool
_IsEnabledFrustumCullCPU(Hgi const *hgi)
{
    if (TfDebug::IsEnabled(HDST_DISABLE_FRUSTUM_CULLING)) {
        return false;
    }
    HgiCapabilities const * const capabilities = hgi->GetCapabilities();

    const bool multiDrawIndirectEnabled =
        capabilities->IsSet(HgiDeviceCapabilitiesBitsMultiDrawIndirect);

    const bool gpuFrustumCullingEnabled =
        HdSt_PipelineDrawBatch::IsEnabled(hgi) ?
            HdSt_PipelineDrawBatch::IsEnabledGPUFrustumCulling() :
            HdSt_IndirectDrawBatch::IsEnabledGPUFrustumCulling();

    // Enable CPU Frustum culling only when GPU frustum culling is not enabled.
    return !(multiDrawIndirectEnabled && gpuFrustumCullingEnabled);
}

}

void
HdStCommandBuffer::PrepareDraw(
    HgiGraphicsCmds *gfxCmds,
    HdStRenderPassStateSharedPtr const &renderPassState,
    HdRenderIndex *renderIndex)
{
    HD_TRACE_FUNCTION();

    // Downcast the resource registry
    HdStResourceRegistrySharedPtr const& resourceRegistry =
        std::dynamic_pointer_cast<HdStResourceRegistry>(
        renderIndex->GetResourceRegistry());
    if (!TF_VERIFY(resourceRegistry)) {
        return;
    }

    Hgi const * const hgi = resourceRegistry->GetHgi();

    if (_IsEnabledFrustumCullCPU(hgi)) {    
        const bool freezeCulling = TfDebug::IsEnabled(HD_FREEZE_CULL_FRUSTUM);

        if (!freezeCulling) {
            _FrustumCullCPU(renderPassState->GetCullMatrix());
        }

        TF_DEBUG(HD_DRAWITEMS_CULLED).Msg("CPU CULLED: %zu drawItems\n",
                                          GetCulledSize());
    } else {
        // Since culling state is stored across renders,
        // we need to update all items visible state
        HdChangeTracker const &tracker = renderIndex->GetChangeTracker();
        SyncDrawItemVisibility(tracker.GetVisibilityChangeCount());
    }

    for (auto const& batch : _drawBatches) {
        batch->PrepareDraw(gfxCmds, renderPassState, resourceRegistry);
    }

    // Once all the prepare work is done, add a memory barrier before the next
    // stage.
    HgiComputeCmds *computeCmds =
        resourceRegistry->GetGlobalComputeCmds(HgiComputeDispatchConcurrent);

    computeCmds->InsertMemoryBarrier(HgiMemoryBarrierAll);

    for (auto const& batch : _drawBatches) {
        batch->EncodeDraw(renderPassState, resourceRegistry,
            /*firstDrawBatch*/batch == *_drawBatches.begin());
    }

    computeCmds->InsertMemoryBarrier(HgiMemoryBarrierAll);

    //
    // Compute work that was set up for indirect command buffers and frustum
    // culling in the batch preparation is submitted to device.
    //
    resourceRegistry->SubmitComputeWork();
}

void
HdStCommandBuffer::ExecuteDraw(
    HgiGraphicsCmds *gfxCmds,
    HdStRenderPassStateSharedPtr const &renderPassState,
    HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    HD_TRACE_FUNCTION();

    //
    // TBD: sort draw items
    //

    // Reset per-commandBuffer performance counters, updated by batch execution
    HD_PERF_COUNTER_SET(HdPerfTokens->drawCalls, 0);
    HD_PERF_COUNTER_SET(HdTokens->itemsDrawn, 0);

    //
    // draw batches
    //
    for (auto const& batch : _drawBatches) {
        batch->ExecuteDraw(gfxCmds, renderPassState, resourceRegistry,
            /*firstDrawBatch*/batch == *_drawBatches.begin());
    }

    HD_PERF_COUNTER_SET(HdPerfTokens->drawBatches, _drawBatches.size());
}

void
HdStCommandBuffer::SetDrawItems(
    HdDrawItemConstPtrVectorSharedPtr const &drawItems,
    unsigned currentDrawBatchesVersion,
    Hgi const *hgi)
{
    if (drawItems == _drawItems &&
        currentDrawBatchesVersion == _drawBatchesVersion) {
        return;
    }
    _drawItems = drawItems;
    _RebuildDrawBatches(hgi);
    _drawBatchesVersion = currentDrawBatchesVersion;
}

void
HdStCommandBuffer::RebuildDrawBatchesIfNeeded(unsigned currentBatchesVersion,
    Hgi const *hgi)
{
    HD_TRACE_FUNCTION();

    bool deepValidation = (currentBatchesVersion != _drawBatchesVersion);
    _drawBatchesVersion = currentBatchesVersion;

    if (TfDebug::IsEnabled(HDST_DRAW_BATCH) && !_drawBatches.empty()) {
        TfDebug::Helper().Msg(
            "Command buffer %p : RebuildDrawBatchesIfNeeded "
            "(deepValidation=%d)\n", (void*)(this), deepValidation);
    }
    
    // Force rebuild of all batches for debugging purposes. This helps quickly
    // triage issues wherein the command buffer wasn't updated correctly.
    bool rebuildAllDrawBatches =
        TfDebug::IsEnabled(HDST_FORCE_DRAW_BATCH_REBUILD);

    if (ARCH_LIKELY(!rebuildAllDrawBatches)) {
        // Gather results of validation ...
        std::vector<HdSt_DrawBatch::ValidationResult> results;
        results.reserve(_drawBatches.size());

        for (auto const& batch : _drawBatches) {
            const HdSt_DrawBatch::ValidationResult result =
                batch->Validate(deepValidation);
            
            if (result == HdSt_DrawBatch::ValidationResult::RebuildAllBatches) {
                // Skip validation of remaining batches since we need to rebuild
                // all batches. We don't expect to use this hammer on a frequent
                // basis.
                rebuildAllDrawBatches = true;
                break;
            }
            
            results.push_back(result);
        }

        // ... and attempt to rebuild necessary batches
        if (!rebuildAllDrawBatches) {
            TF_VERIFY(results.size() == _drawBatches.size());
            size_t const numBatches = results.size();
            for (size_t i = 0; i < numBatches; i++) {
                if (results[i] ==
                    HdSt_DrawBatch::ValidationResult::RebuildBatch) {
                    
                    if (!_drawBatches[i]->Rebuild()) {
                        // If a batch rebuild fails, we fallback to rebuilding
                        // all draw batches. This can be improved in the future.
                        rebuildAllDrawBatches = true;
                        break;
                    }
                }
            }
        }
    }

    if (rebuildAllDrawBatches) {
        _RebuildDrawBatches(hgi);
    }   
}

void
HdStCommandBuffer::_RebuildDrawBatches(Hgi const *hgi)
{
    HD_TRACE_FUNCTION();

    TF_DEBUG(HDST_DRAW_BATCH).Msg(
        "Rebuilding all draw batches for command buffer %p ...\n", (void*)this);

    _visibleSize = 0;

    _drawBatches.clear();
    _drawItemInstances.clear();
    _drawItemInstances.reserve(_drawItems->size());

    HD_PERF_COUNTER_INCR(HdPerfTokens->rebuildBatches);

    // Downcast the HdDrawItem entries to HdStDrawItems:
    std::vector<HdStDrawItem const*>* stDrawItemsPtr =
        reinterpret_cast< std::vector<HdStDrawItem const*>* >(_drawItems.get());
    auto const &drawItems = *stDrawItemsPtr;

    for (size_t i = 0; i < drawItems.size(); i++) {
        HdStDrawItem const * drawItem = drawItems[i];

        if (!TF_VERIFY(drawItem->GetGeometricShader(), "%s",
                       drawItem->GetRprimID().GetText()) ||
            !TF_VERIFY(drawItem->GetMaterialNetworkShader(), "%s",
                       drawItem->GetRprimID().GetText())) {
            continue;
        }

        _drawItemInstances.emplace_back(drawItem);
    }

    bool const usePipelineDrawBatch = HdSt_PipelineDrawBatch::IsEnabled(hgi);

    _drawBatches.clear();

    _BatchDrawItemInstances(
        &_drawBatches,
        _drawItemInstances,
        usePipelineDrawBatch);

    if (!usePipelineDrawBatch && (_GetTextureAggregationThreshold() > 0)) {
        _AggregateDrawBatches(
            &_drawBatches,
            usePipelineDrawBatch);
    }

    TF_DEBUG(HDST_DRAW_BATCH).Msg(
        "   %lu draw batches created for %lu draw items\n", _drawBatches.size(),
        drawItems.size());
}

void
HdStCommandBuffer::SyncDrawItemVisibility(unsigned visChangeCount)
{
    HD_TRACE_FUNCTION();

    if (_visChangeCount == visChangeCount) {
        // There were no changes to visibility since the last time sync was
        // called, no need to re-sync now. Note that visChangeCount starts at
        // 0 in the class and starts at 1 in the change tracker, which ensures a
        // sync after contruction.
        return;
    }

    _visibleSize = 0;
    int const N = 10000;
    tbb::enumerable_thread_specific<size_t> visCounts;

    WorkParallelForN(_drawItemInstances.size()/N+1,
      [&visCounts, this, N](size_t start, size_t end) {
        TRACE_SCOPE("SetVis");
        start *= N;
        end = std::min(end*N, _drawItemInstances.size());
        size_t& count = visCounts.local();
        for (size_t i = start; i < end; ++i) {
            HdStDrawItem const* item = _drawItemInstances[i].GetDrawItem();

            bool visible = item->GetVisible();
            // DrawItemInstance->SetVisible is not only an inline function but
            // also internally calling virtual HdDrawBatch
            // DrawItemInstanceChanged.  shortcut by looking IsVisible(), which
            // is inline, if it's not actually changing.

            // however, if this is an instancing prim and visible, it always has
            // to be called since instanceCount may changes over time.
            if ((_drawItemInstances[i].IsVisible() != visible) || 
                (visible && item->HasInstancer())) {
                _drawItemInstances[i].SetVisible(visible);
            }
            if (visible) {
                ++count;
            }
        }
    });

    for (size_t i : visCounts) {
        _visibleSize += i;
    }

    // Mark visible state as clean;
    _visChangeCount = visChangeCount;
}

void
HdStCommandBuffer::_FrustumCullCPU(GfMatrix4d const &cullMatrix)
{
    HD_TRACE_FUNCTION();

    const bool mtCullingDisabled = 
        TfDebug::IsEnabled(HDST_DISABLE_MULTITHREADED_CULLING) || 
        _drawItems->size() < 10000;

    struct _Worker {
        static
        void cull(std::vector<HdStDrawItemInstance> * drawItemInstances,
                  GfMatrix4d const &cullMatrix,
                  size_t begin, size_t end) 
        {
            for(size_t i = begin; i < end; i++) {
                HdStDrawItemInstance& itemInstance = (*drawItemInstances)[i];
                HdStDrawItem const* item = itemInstance.GetDrawItem();
                bool visible = item->GetVisible() && 
                    item->IntersectsViewVolume(cullMatrix);
                if ((itemInstance.IsVisible() != visible) || 
                    (visible && item->HasInstancer())) {
                    itemInstance.SetVisible(visible);
                }
            }
        }
    };

    if (!mtCullingDisabled) {
        WorkParallelForN(_drawItemInstances.size(), 
                         std::bind(&_Worker::cull, &_drawItemInstances, 
                                   std::cref(cullMatrix),
                                   std::placeholders::_1,
                                   std::placeholders::_2));
    } else {
        _Worker::cull(&_drawItemInstances, 
                      cullMatrix, 
                      0, 
                      _drawItemInstances.size());
    }

    _visibleSize = 0;
    for (auto const& instance : _drawItemInstances) {
        if (instance.IsVisible()) {
            ++_visibleSize;
        }
    }
}

void
HdStCommandBuffer::SetEnableTinyPrimCulling(bool tinyPrimCulling)
{
    for(auto const& batch : _drawBatches) {
        batch->SetEnableTinyPrimCulling(tinyPrimCulling);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

