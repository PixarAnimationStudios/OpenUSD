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
#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/imaging/hdSt/commandBuffer.h"
#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/immediateDrawBatch.h"
#include "pxr/imaging/hdSt/indirectDrawBatch.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stl.h"

#include "pxr/base/work/loops.h"

#include <boost/functional/hash.hpp>

#include <tbb/enumerable_thread_specific.h>

#include <functional>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE


HdStCommandBuffer::HdStCommandBuffer()
    : _visibleSize(0)
    , _visChangeCount(0)
    , _batchVersion(0)
{
    /*NOTHING*/
}

HdStCommandBuffer::~HdStCommandBuffer()
{
}

static
HdSt_DrawBatchSharedPtr
_NewDrawBatch(HdStDrawItemInstance * drawItemInstance)
{
    GlfContextCaps const &caps = GlfContextCaps::GetInstance();

    if (caps.multiDrawIndirectEnabled) {
        return HdSt_DrawBatchSharedPtr(
            new HdSt_IndirectDrawBatch(drawItemInstance));
    } else {
        return HdSt_DrawBatchSharedPtr(
            new HdSt_ImmediateDrawBatch(drawItemInstance));
    }
}

void
HdStCommandBuffer::PrepareDraw(
    HdStRenderPassStateSharedPtr const &renderPassState,
    HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    HD_TRACE_FUNCTION();

    for (auto const& batch : _drawBatches) {
        batch->PrepareDraw(renderPassState, resourceRegistry);
    }
}

void
HdStCommandBuffer::ExecuteDraw(
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
        batch->ExecuteDraw(renderPassState, resourceRegistry);
    }
    HD_PERF_COUNTER_SET(HdPerfTokens->drawBatches, _drawBatches.size());

    if (!glBindBuffer) {
        // useful when testing with GL drawing disabled
        HD_PERF_COUNTER_SET(HdTokens->itemsDrawn, _visibleSize);
    }
}

void
HdStCommandBuffer::SwapDrawItems(std::vector<HdStDrawItem const*>* items,
                               unsigned currentBatchVersion)
{
    _drawItems.swap(*items);
    _RebuildDrawBatches();
    _batchVersion = currentBatchVersion;
}

void
HdStCommandBuffer::RebuildDrawBatchesIfNeeded(unsigned currentBatchVersion)
{
    HD_TRACE_FUNCTION();

    bool deepValidation = (currentBatchVersion != _batchVersion);
    _batchVersion = currentBatchVersion;
    
    // Force rebuild of all batches for debugging purposes. This helps quickly
    // triage issues wherein the command buffer wasn't updated correctly.
    bool rebuildAllDrawBatches =
        TfDebug::IsEnabled(HDST_FORCE_DRAW_BATCH_REBUILD);

    if (ARCH_LIKELY(!rebuildAllDrawBatches)) {
        for (auto const& batch : _drawBatches) {
            // Validate checks if the batch is referring to up-to-date
            // buffer arrays (via a cheap version number hash check).
            // If deepValidation is set, we loop over the draw items to check
            // if they can be aggregated. If these checks fail, we need to
            // rebuild the batch.
            bool needToRebuildBatch = !batch->Validate(deepValidation);
            if (needToRebuildBatch) {
                // Attempt to rebuild the batch. If that fails, we use a big
                // hammer and rebuilt ALL batches.
                bool rebuildSuccess = batch->Rebuild();
                if (!rebuildSuccess) {
                    rebuildAllDrawBatches = true;
                    break;
                }
            }
        }
    }

    if (rebuildAllDrawBatches) {
        _RebuildDrawBatches();
    }   
}

void
HdStCommandBuffer::_RebuildDrawBatches()
{
    HD_TRACE_FUNCTION();

    TF_DEBUG(HDST_DRAW_BATCH).Msg(
        "Rebuilding all draw batches for command buffer %p ...\n", (void*)this);

    _visibleSize = 0;

    _drawBatches.clear();
    _drawItemInstances.clear();
    _drawItemInstances.reserve(_drawItems.size());

    HD_PERF_COUNTER_INCR(HdPerfTokens->rebuildBatches);

    bool bindlessTexture = GlfContextCaps::GetInstance()
                                               .bindlessTextureEnabled;

    // Use a cheap bucketing strategy to reduce to number of comparison tests
    // required to figure out if a draw item can be batched.
    // We use a hash of the geometric shader, BAR version and (optionally)
    // material params as the key, and test (in the worst case) against each of 
    // the batches for the key.
    // Test against the previous draw item's hash and batch prior to looking up
    // the map.
    struct _PrevBatchHit {
        _PrevBatchHit() : key(0) {}
        void Update(size_t _key, HdSt_DrawBatchSharedPtr &_batch) {
            key = _key;
            batch = _batch;
        }
        size_t key;
        HdSt_DrawBatchSharedPtr batch;
    };
    _PrevBatchHit prevBatch;
    
    using _DrawBatchMap = 
        std::unordered_map<size_t, HdSt_DrawBatchSharedPtrVector>;
    _DrawBatchMap batchMap;

    for (size_t i = 0; i < _drawItems.size(); i++) {
        HdStDrawItem const * drawItem = _drawItems[i];

        if (!TF_VERIFY(drawItem->GetGeometricShader(), "%s",
                       drawItem->GetRprimID().GetText()) ||
            !TF_VERIFY(drawItem->GetMaterialShader(), "%s",
                       drawItem->GetRprimID().GetText())) {
            continue;
        }

        _drawItemInstances.push_back(HdStDrawItemInstance(drawItem));
        HdStDrawItemInstance* drawItemInstance = &_drawItemInstances.back();

        size_t key = drawItem->GetGeometricShader()->ComputeHash();
        boost::hash_combine(key, drawItem->GetBufferArraysHash());
        if (!bindlessTexture) {
            // Geometric, RenderPass and Lighting shaders should never break
            // batches, however materials can. We consider the material 
            // parameters to be part of the batch key here for that reason.
            boost::hash_combine(key, HdMaterialParam::ComputeHash(
                            drawItem->GetMaterialShader()->GetParams()));
        }

        // Do a quick check to see if the draw item can be batched with the
        // previous draw item, before checking the batchMap.
        if (key == prevBatch.key && prevBatch.batch) {
            if (prevBatch.batch->Append(drawItemInstance)) {
                continue;
            }
        }

        _DrawBatchMap::iterator const batchIter = batchMap.find(key);
        bool const foundKey = batchIter != batchMap.end();
        bool batched = false;
        if (foundKey) {
            HdSt_DrawBatchSharedPtrVector &batches = batchIter->second;
            for (HdSt_DrawBatchSharedPtr &batch : batches) {
                if (batch->Append(drawItemInstance)) {
                    batched = true;
                    prevBatch.Update(key, batch);
                    break;
                }
            }
        }

        if (!batched) {
            HdSt_DrawBatchSharedPtr batch = _NewDrawBatch(drawItemInstance);
            _drawBatches.emplace_back(batch);
            prevBatch.Update(key, batch);

            if (foundKey) {
                HdSt_DrawBatchSharedPtrVector &batches = batchIter->second;
                batches.emplace_back(batch);
            } else {
                batchMap[key] = HdSt_DrawBatchSharedPtrVector({batch});
            }
        }
    }

    TF_DEBUG(HDST_DRAW_BATCH).Msg(
        "   %lu draw batches created for %lu draw items\n", _drawBatches.size(),
        _drawItems.size());
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
HdStCommandBuffer::FrustumCull(GfMatrix4d const &viewProjMatrix)
{
    HD_TRACE_FUNCTION();

    const bool mtCullingDisabled = 
        TfDebug::IsEnabled(HDST_DISABLE_MULTITHREADED_CULLING) || 
        _drawItems.size() < 10000;

    struct _Worker {
        static
        void cull(std::vector<HdStDrawItemInstance> * drawItemInstances,
                GfMatrix4d const &viewProjMatrix,
                size_t begin, size_t end) 
        {
            for(size_t i = begin; i < end; i++) {
                HdStDrawItemInstance& itemInstance = (*drawItemInstances)[i];
                HdStDrawItem const* item = itemInstance.GetDrawItem();
                bool visible = item->GetVisible() && 
                    item->IntersectsViewVolume(viewProjMatrix);
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
                                   std::cref(viewProjMatrix),
                                   std::placeholders::_1,
                                   std::placeholders::_2));
    } else {
        _Worker::cull(&_drawItemInstances, 
                      viewProjMatrix, 
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

