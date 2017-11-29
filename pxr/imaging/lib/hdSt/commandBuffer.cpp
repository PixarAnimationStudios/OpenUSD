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

#include "pxr/imaging/hdSt/commandBuffer.h"
#include "pxr/imaging/hdSt/immediateDrawBatch.h"
#include "pxr/imaging/hdSt/indirectDrawBatch.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/geometricShader.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stl.h"

#include "pxr/base/work/loops.h"

#include <boost/functional/hash.hpp>

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE


HdStCommandBuffer::HdStCommandBuffer()
    : _visibleSize(0)
    , _visChangeCount(0)
    , _shaderBindingsVersion(0)
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
    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();

    if (caps.multiDrawIndirectEnabled) {
        return HdSt_DrawBatchSharedPtr(
            new HdSt_IndirectDrawBatch(drawItemInstance));
    } else {
        return HdSt_DrawBatchSharedPtr(
            new HdSt_ImmediateDrawBatch(drawItemInstance));
    }
}

void
HdStCommandBuffer::PrepareDraw(HdRenderPassStateSharedPtr const &renderPassState,
                             HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    HD_TRACE_FUNCTION();

    TF_FOR_ALL(batchIt, _drawBatches) {
        (*batchIt)->PrepareDraw(renderPassState, resourceRegistry);
    }
}

void
HdStCommandBuffer::ExecuteDraw(HdRenderPassStateSharedPtr const &renderPassState,
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
    TF_FOR_ALL(batchIt, _drawBatches) {
        (*batchIt)->ExecuteDraw(renderPassState, resourceRegistry);
    }
    HD_PERF_COUNTER_SET(HdPerfTokens->drawBatches, _drawBatches.size());

    if (!glBindBuffer) {
        // useful when testing with GL drawing disabled
        HD_PERF_COUNTER_SET(HdTokens->itemsDrawn, _visibleSize);
    }
}

void
HdStCommandBuffer::SwapDrawItems(std::vector<HdDrawItem const*>* items,
                               unsigned currentShaderBindingsVersion)
{
    _drawItems.swap(*items);
    _RebuildDrawBatches();
    _shaderBindingsVersion = currentShaderBindingsVersion;
}

void
HdStCommandBuffer::RebuildDrawBatchesIfNeeded(unsigned currentShaderBindingsVersion)
{
    HD_TRACE_FUNCTION();

    bool deepValidation
        = (currentShaderBindingsVersion != _shaderBindingsVersion);

    for (auto const& batch : _drawBatches) {
        if (!batch->Validate(deepValidation) && !batch->Rebuild()) {
            TRACE_SCOPE("Invalid Batches");
            _RebuildDrawBatches();
            _shaderBindingsVersion = currentShaderBindingsVersion;
            return;
        }
    }
    _shaderBindingsVersion = currentShaderBindingsVersion;
}

void
HdStCommandBuffer::_RebuildDrawBatches()
{
    HD_TRACE_FUNCTION();

    _visibleSize = 0;

    _drawBatches.clear();
    _drawItemInstances.clear();
    _drawItemInstances.reserve(_drawItems.size());

    HD_PERF_COUNTER_INCR(HdPerfTokens->rebuildBatches);

    bool bindlessTexture = HdRenderContextCaps::GetInstance()
                                               .bindlessTextureEnabled;

    // XXX: Temporary sorting by shader.
    std::map<size_t, HdSt_DrawBatchSharedPtr> batchMap;

    for (size_t i = 0; i < _drawItems.size(); i++) {
        HdDrawItem const * drawItem = _drawItems[i];

        HdShaderCodeSharedPtr const &geometricShader
            = drawItem->GetGeometricShader();
        if (!TF_VERIFY(geometricShader, "%s",
                       drawItem->GetRprimID().GetText())) {
            continue;
        }

        _drawItemInstances.push_back(HdStDrawItemInstance(drawItem));
        HdStDrawItemInstance* drawItemInstance = &_drawItemInstances.back();

        size_t key = geometricShader->ComputeHash();
        boost::hash_combine(key, drawItem->GetBufferArraysHash());

        if (!bindlessTexture) {
            // Geometric, RenderPass and Lighting shaders should never break
            // batches, however materials can. We consider the material 
            // parameters to be part of the batch key here for that reason.
            boost::hash_combine(key, HdShaderParam::ComputeHash(
                                    drawItem->GetMaterial()->GetParams()));
        }

        TF_DEBUG(HD_DRAW_BATCH).Msg("%lu (%lu)\n", 
                key, 
                drawItem->GetBufferArraysHash());
                //, drawItem->GetRprimID().GetText());

        HdSt_DrawBatchSharedPtr batch;
        TfMapLookup(batchMap, key, &batch);
        if (!batch || !batch->Append(drawItemInstance)) {
            batch = _NewDrawBatch(drawItemInstance);
            _drawBatches.push_back(batch);
            batchMap[key] = batch;
        }
    }
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
            HdDrawItem const* item = _drawItemInstances[i].GetDrawItem();

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

    const bool 
        mtCullingDisabled = TfDebug::IsEnabled(HD_DISABLE_MULTITHREADED_CULLING)
        || _drawItems.size() < 10000;

    struct _Worker {
        static
        void cull(std::vector<HdStDrawItemInstance> * drawItemInstances,
                GfMatrix4d const &viewProjMatrix,
                size_t begin, size_t end) 
        {
            for(size_t i = begin; i < end; i++) {
                HdStDrawItemInstance& itemInstance = (*drawItemInstances)[i];
                HdDrawItem const* item = itemInstance.GetDrawItem();
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
    TF_FOR_ALL(dIt, _drawItemInstances) {
        if (dIt->IsVisible()) {
            ++_visibleSize;
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

