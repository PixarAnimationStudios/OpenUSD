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
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hdSt/renderPassShader.h"

PXR_NAMESPACE_OPEN_SCOPE


// -------------------------------------------------------------------------- //

HdxRenderTask::HdxRenderTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdTask(id)
    , _passes()
    , _setupTask()
{
}

HdxRenderTask::~HdxRenderTask()
{
}

bool
HdxRenderTask::IsConverged() const
{
    TF_FOR_ALL(pass, _passes) {
        if (!(*pass)->IsConverged()) {
            return false;
        }
    }
    return true;
}


void
HdxRenderTask::Sync(HdSceneDelegate* delegate,
                    HdTaskContext*   ctx,
                    HdDirtyBits*     dirtyBits)
{
    HD_TRACE_FUNCTION();

    HdDirtyBits bits = *dirtyBits;

    if (bits & HdChangeTracker::DirtyCollection) {

        HdRprimCollectionVector collections;
        VtValue val = delegate->Get(GetId(), HdTokens->collection);

        if (val.IsHolding<HdRprimCollection>()) {
            collections.push_back(val.UncheckedGet<HdRprimCollection>());
        } else if (val.IsHolding<HdRprimCollectionVector>()) {
            collections = val.UncheckedGet<HdRprimCollectionVector>();
        } else {
            TF_CODING_ERROR("The task collection is the wrong type");
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
            HdRenderIndex &index = delegate->GetRenderIndex();
            TF_FOR_ALL(it, collections) {
                _passes.push_back(HdRenderPassSharedPtr(
                    index.GetRenderDelegate()->CreateRenderPass(&index, *it)));
            }
            bits |= HdChangeTracker::DirtyParams;
        }
    }

    if (bits & HdChangeTracker::DirtyParams) {
        HdxRenderTaskParams params;

        // if HdxRenderTaskParams is set on this task, create an
        // HdxRenderSetupTask to unpack them internally.
        //
        // As params is optional, the base class helpper can't be used.
        VtValue valueVt = delegate->Get(GetId(), HdTokens->params);
        if (valueVt.IsHolding<HdxRenderTaskParams>()) {
            params = valueVt.UncheckedGet<HdxRenderTaskParams>();

            if (!_setupTask) {
                // note that _setupTask should have the same id, since it will
                // use that id to look up params in the scene delegate.
                // this setup task isn't indexed, so there's no concern
                // about name conflicts.
                _setupTask.reset(
                    new HdxRenderSetupTask(delegate, GetId()));
            }

            _setupTask->SyncParams(delegate, params);

        } else {
            // If params are not set, expect the renderpass state to be passed
            // in the task context.
        }
    }

    if (_setupTask) {
        _setupTask->SyncAovBindings(delegate);
        _setupTask->SyncCamera(delegate);
        _setupTask->SyncRenderPassState(delegate);
    }

    // sync render passes
    TF_FOR_ALL (it, _passes){
        HdRenderPassSharedPtr const &pass = (*it);
        pass->Sync();
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxRenderTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdRenderPassStateSharedPtr renderPassState;
    TfTokenVector renderTags;

    if (_setupTask) {
        // If HdxRenderTaskParams is set on this task, we will have created an
        // internal HdxRenderSetupTask in _Sync, to sync and unpack the params,
        // and we should use the resulting resources.
        renderPassState = _setupTask->GetRenderPassState();
        renderTags = _setupTask->GetRenderTags();
    } else {
        // Otherwise, we expect an application-created HdxRenderSetupTask to
        // have run and put the renderpass resources in the task context.
        // See HdxRenderSetupTask::_Execute.
        _GetTaskContextData(ctx, HdxTokens->renderPassState, &renderPassState);
        _GetTaskContextData(ctx, HdxTokens->renderTags, &renderTags);
    }
    if (!TF_VERIFY(renderPassState)) return;

    if (HdStRenderPassState* extendedState =
            dynamic_cast<HdStRenderPassState*>(renderPassState.get())) {
        _SetHdStRenderPassState(ctx, extendedState);
    }

    // Bind the render state and render geometry with the rendertags (if any)
    renderPassState->Bind();
    if(renderTags.size() == 0) {
        // execute all render passes.
        TF_FOR_ALL(it, _passes) {
            (*it)->Execute(renderPassState);
        }
    } else {
        // execute all render passes with only a subset of render tags
        TF_FOR_ALL(it, _passes) {
            (*it)->Execute(renderPassState, renderTags);
        }
    }
    renderPassState->Unbind();
}

void
HdxRenderTask::_SetHdStRenderPassState(HdTaskContext *ctx,
                                       HdStRenderPassState *renderPassState)
{
    // Can't use GetTaskContextData because the lightingShader
    // is optional.
    VtValue lightingShader = (*ctx)[HdxTokens->lightingShader];

    // it's possible to not set lighting shader to HdRenderPassState.
    // Hd_DefaultLightingShader will be used in that case.
    if (lightingShader.IsHolding<HdStLightingShaderSharedPtr>()) {
        renderPassState->SetLightingShader(
            lightingShader.Get<HdStLightingShaderSharedPtr>());
    }

    // Selection Setup
    // Note that selectionTask comes after renderTask, so that
    // it can access rprimIDs populated in RenderTask::_Sync.
    VtValue vo = (*ctx)[HdxTokens->selectionOffsets];
    VtValue vu = (*ctx)[HdxTokens->selectionUniforms];
    VtValue vc = (*ctx)[HdxTokens->selectionPointColors];

    HdStRenderPassShaderSharedPtr renderPassShader
        = renderPassState->GetRenderPassShader();

    if (!vo.IsEmpty() && !vu.IsEmpty() && !vc.IsEmpty()) {
        HdBufferArrayRangeSharedPtr obar
            = vo.Get<HdBufferArrayRangeSharedPtr>();
        HdBufferArrayRangeSharedPtr ubar
            = vu.Get<HdBufferArrayRangeSharedPtr>();
        HdBufferArrayRangeSharedPtr cbar
            = vc.Get<HdBufferArrayRangeSharedPtr>();

        renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->selectionOffsets, obar,
                             /*interleave*/false));
        renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::UBO,
                             HdxTokens->selectionUniforms, ubar,
                             /*interleave*/true));
        renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::SSBO,
                             HdxTokens->selectionPointColors, cbar,
                             /*interleave*/false));
    } else {
        renderPassShader->RemoveBufferBinding(HdxTokens->selectionOffsets);
        renderPassShader->RemoveBufferBinding(HdxTokens->selectionUniforms);
        renderPassShader->RemoveBufferBinding(HdxTokens->selectionPointColors);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

