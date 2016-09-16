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
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/debugCodes.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassShader.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"

// --------------------------------------------------------------------------- //

HdxRenderTask::HdxRenderTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdSceneTask(delegate, id)
    , _passes()
{
}

void
HdxRenderTask::_Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    HdRenderPassStateSharedPtr renderPassState;

    if (_setupTask) {
        // if _setupTask exists (for backward compatibility), use it
        renderPassState = _setupTask->GetRenderPassState();
    } else {
        // otherwise, extract from TaskContext
        _GetTaskContextData(ctx, HdxTokens->renderPassState, &renderPassState);
    }
    if (not TF_VERIFY(renderPassState)) return;

    // Can't use GetTaskContextData because the lightingShader
    // is optional.
    VtValue lightingShader = (*ctx)[HdxTokens->lightingShader];

    // it's possible to not set lighting shader to HdRenderPassState.
    // Hd_DefaultLightingShader will be used in that case.
    if (lightingShader.IsHolding<HdLightingShaderSharedPtr>()) {
        renderPassState->SetLightingShader(
            lightingShader.Get<HdLightingShaderSharedPtr>());
    }

    // Selection Setup
    // Note that selectionTask comes after renderTask, so that
    // it can access rprimIDs populated in RenderTask::_Sync.
    VtValue vo = (*ctx)[HdxTokens->selectionOffsets];
    VtValue vu = (*ctx)[HdxTokens->selectionUniforms];

    HdRenderPassShaderSharedPtr renderPassShader
        = renderPassState->GetRenderPassShader();

    if (not vo.IsEmpty() and not vu.IsEmpty()) {
        HdBufferArrayRangeSharedPtr obar
            = vo.Get<HdBufferArrayRangeSharedPtr>();
        HdBufferArrayRangeSharedPtr ubar
            = vu.Get<HdBufferArrayRangeSharedPtr>();

        renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->selectionOffsets, obar,
                             /*interleave*/false));
        renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::UBO, 
                             HdxTokens->selectionUniforms, ubar,
                             /*interleave*/true));
    } else {
        renderPassShader->RemoveBufferBinding(HdxTokens->selectionOffsets);
        renderPassShader->RemoveBufferBinding(HdxTokens->selectionUniforms);
    }

    renderPassState->Bind();

    // execute all render passes.
    TF_FOR_ALL(it, _passes) {
        (*it)->Execute(renderPassState);
    }

    renderPassState->Unbind();
}

void
HdxRenderTask::_Sync(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();

    HdChangeTracker::DirtyBits bits = _GetTaskDirtyBits();

    if (bits & HdChangeTracker::DirtyCollection) {

        HdRprimCollectionVector collections;
        VtValue val = GetDelegate()->Get(GetId(), HdTokens->collection);

        if (val.IsHolding<HdRprimCollection>()) {
            collections.push_back(val.UncheckedGet<HdRprimCollection>());
        } else if (val.IsHolding<HdRprimCollectionVector>()) {
            collections = val.UncheckedGet<HdRprimCollectionVector>();
        } else {
            TF_CODING_ERROR("The collection from scene delegate is of mismatched type");
            return;
        }

        if (_passes.size() == collections.size()) {
            // reuse same render passes.
            for (size_t i = 0; i < _passes.size(); ++i) {
                _passes[i]->SetRprimCollection(collections[i]);
            }
        } else {
            // reconstruct render passes.
            _passes.clear();
            TF_FOR_ALL(it, collections) {
                _passes.push_back(
                    HdRenderPassSharedPtr(
                        new HdRenderPass(&GetDelegate()->GetRenderIndex(), *it)));
            }
            bits |= HdChangeTracker::DirtyParams;
        }
    }

    // XXX: for compatibility.
    if (bits & HdChangeTracker::DirtyParams) {
        HdxRenderTaskParams params;

        // if HdxRenderTaskParams is set, it's using old API
        VtValue valueVt = GetDelegate()->Get(GetId(), HdTokens->params);
        if (valueVt.IsHolding<HdxRenderTaskParams>()) {
            params = valueVt.UncheckedGet<HdxRenderTaskParams>();

            // this is in compatibility path. delegate to _setupTask
            if (not _setupTask) {
                // note that _setupTask should have same Id, since sceneDelegate
                // thinks this HdxRenderTask is asking the parameters.
                _setupTask.reset(new HdxRenderSetupTask(GetDelegate(), GetId()));
            }

            _setupTask->Sync(params);

        } else {
            // RenderPassState is managed externally (new API).
        }
    }

    if (_setupTask) {
        _setupTask->SyncCamera();
    }

    // sync render passes
    TF_FOR_ALL (it, _passes){
        HdRenderPassSharedPtr const &pass = (*it);
        pass->Sync();
    }
}
