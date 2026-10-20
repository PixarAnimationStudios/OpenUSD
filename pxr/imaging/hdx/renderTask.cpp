//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"

#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/renderPassShader.h"

PXR_NAMESPACE_OPEN_SCOPE


// -------------------------------------------------------------------------- //

HdxRenderTask::HdxRenderTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdxTask(id)
    , _pass()
    , _renderTags()
    , _setupTask()
{
}

HdxRenderTask::~HdxRenderTask() = default;

bool
HdxRenderTask::IsConverged() const
{
    if (_pass) {
        return _pass->IsConverged();
    }

    return true;
}

void
HdxRenderTask::_Sync(HdSceneDelegate* delegate,
                     HdTaskContext* ctx,
                     HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();

    HdDirtyBits bits = *dirtyBits;

    if (bits & HdChangeTracker::DirtyCollection) {

        VtValue val = delegate->Get(GetId(), HdTokens->collection);

        HdRprimCollection collection = val.Get<HdRprimCollection>();

        // Check for cases where the collection is empty (i.e. default
        // constructed).  To do this, the code looks at the root paths,
        // if it is empty, the collection doesn't refer to any prims at
        // all.
        if (collection.GetName().IsEmpty()) {
            _pass.reset();
            _passState.reset();
        } else {
            if (!_pass) {
                HdRenderIndex &index = delegate->GetRenderIndex();
                HdRenderDelegate *renderDelegate = index.GetRenderDelegate();
                _pass = renderDelegate->CreateRenderPass(&index, collection);
                _passState = renderDelegate->CreateRenderPassState();
            } else {
                _pass->SetRprimCollection(collection);
            }
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
                _setupTask = std::make_shared<HdxRenderSetupTask>(
                    delegate, GetId());
            }

            _setupTask->SyncParams(delegate, params);

        } else {
            // If params are not set, expect the renderpass state to be passed
            // in the task context.
        }
    }

    if (bits & HdChangeTracker::DirtyRenderTags) {
        _renderTags = _GetTaskRenderTags(delegate);
    }

    // sync render pass
    if (_pass) {
        _pass->Sync();
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxRenderTask::_PrepareRenderPassState(HdTaskContext* ctx,
                                       HdRenderIndex* renderIndex)
{
    if (_setupTask) {
        _setupTask->Prepare(ctx, renderIndex);
    }

    if (_passState) {
        HdRenderPassStateSharedPtr stateSource;
        if (_setupTask) {
            // If HdxRenderTaskParams is set on this task, we will have created an
            // internal HdxRenderSetupTask in _Sync, to sync and unpack the params,
            // and we should use the resulting resources.
            stateSource = _setupTask->GetRenderPassState();
        } else {
            // Otherwise, we expect an application-created HdxRenderSetupTask to
            // have run and put the renderpass resources in the task context.
            // See HdxRenderSetupTask::_Execute.
            _GetTaskContextData(ctx, HdxTokens->renderPassState, &stateSource);
        }

        _passState->Copy(*stateSource);
    }
}

void
HdxRenderTask::Prepare(HdTaskContext* ctx,
                       HdRenderIndex* renderIndex)
{
    _PrepareRenderPassState(ctx, renderIndex);

    if (_passState) {
        HdStRenderPassStateSharedPtr extendedState =
            std::dynamic_pointer_cast<HdStRenderPassState>(_passState);
        if (extendedState) {
            _AddStormSelectionBindings(ctx,
                extendedState->GetRenderPassShader());
            _AddStormLightingShader(ctx, extendedState);
        }
    }
}

static
HdRenderParam * _GetRenderParam(HdRenderPassSharedPtr const &pass)
{
    if (!pass) {
        return nullptr;
    }
    HdRenderIndex * const renderIndex = pass->GetRenderIndex();
    if (!renderIndex) {
        return nullptr;
    }
    HdRenderDelegate * const renderDelegate = renderIndex->GetRenderDelegate();
    if (!renderDelegate) {
        return nullptr;
    }
    return renderDelegate->GetRenderParam();
}

void
HdxRenderTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Bail out early for Storm tasks that have no rendering work to submit
    // and don't need to clear AOVs.
    if (!_passState ||
        (!_HasDrawItems() && !_NeedToClearAovs(_passState))) {
        return;
    }

    // Render geometry with the rendertags (if any)
    if (_pass) {
        _pass->Execute(_passState, GetRenderTags());

        // Copy scene state id from render param to task context.
        //
        // This allows clients to use the
        // existing HdLegacyRenderControlInterface::GetTaskContextData
        // to retrieve it.
        if (HdRenderParam * const renderParam = _GetRenderParam(_pass)) {
            static const TfToken &key =
                HdSceneGlobalsSchemaTokens->sceneStateId;
            if (renderParam->HasArbitraryValue(key)) {
                (*ctx)[key] = renderParam->GetArbitraryValue(key);
            }
        }
    }
}

const TfTokenVector &
HdxRenderTask::GetRenderTags() const
{
    return _renderTags;
}

HdRenderPassStateSharedPtr
HdxRenderTask::_GetRenderPassState() const
{
    return _passState;
}

HdRenderPassSharedPtr
HdxRenderTask::_GetRenderPass() const
{
    return _pass;
}

bool
HdxRenderTask::_HasDrawItems() const
{
    if (HdSt_RenderPass* hdStRenderPass =
            dynamic_cast<HdSt_RenderPass*>(_pass.get())) {
        return hdStRenderPass->HasDrawItems(GetRenderTags());
    } else {
        // Non-Storm backends don't typically use the draw item subsystem.
        // Return true to signify that there is rendering work to do.
        return true;
    }
}

void
HdxRenderTask::_AddStormLightingShader(
    HdTaskContext* ctx,
    HdStRenderPassStateSharedPtr passState)
{
    // Lighting shader setup
    VtValue lightingShader = (*ctx)[HdxTokens->lightingShader];

    // it's possible to not set lighting shader to HdRenderPassState.
    // Hd_DefaultLightingShader will be used in that case.
    if (lightingShader.IsHolding<HdStLightingShaderSharedPtr>()) {
        passState->SetLightingShader(
            lightingShader.Get<HdStLightingShaderSharedPtr>());
    }
}

void
HdxRenderTask::_AddStormSelectionBindings(
    HdTaskContext* ctx,
    HdStRenderPassShaderSharedPtr passShader)
{
    // Selection buffer setup
    VtValue vo = (*ctx)[HdxTokens->selectionOffsets];
    VtValue vu = (*ctx)[HdxTokens->selectionUniforms];

    if (!vo.IsEmpty() && !vu.IsEmpty()) {
        HdBufferArrayRangeSharedPtr obar
            = vo.Get<HdBufferArrayRangeSharedPtr>();
        HdBufferArrayRangeSharedPtr ubar
            = vu.Get<HdBufferArrayRangeSharedPtr>();

        passShader->AddBufferBinding(
            HdStBindingRequest(HdStBinding::SSBO,
                               HdxTokens->selectionOffsets, obar,
                               /*interleave*/false));
        passShader->AddBufferBinding(
            HdStBindingRequest(HdStBinding::UBO,
                               HdxTokens->selectionUniforms, ubar,
                               /*interleave*/true));
    } else {
        passShader->RemoveBufferBinding(HdxTokens->selectionOffsets);
        passShader->RemoveBufferBinding(HdxTokens->selectionUniforms);
    }
}

bool
HdxRenderTask::_NeedToClearAovs(
    HdRenderPassStateSharedPtr const &renderPassState) const
{
    HdRenderPassAovBindingVector const &aovBindings =
            renderPassState->GetAovBindings();
    for (auto const & binding : aovBindings) {
        if (!binding.clearValue.IsEmpty()) {
            return true;
        }
    }
    return false;
}


PXR_NAMESPACE_CLOSE_SCOPE

