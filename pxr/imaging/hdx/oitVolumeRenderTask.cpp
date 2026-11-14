//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/oitVolumeRenderTask.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/oitBufferAccessor.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/volume.h"

PXR_NAMESPACE_OPEN_SCOPE

static const HioGlslfxSharedPtr &
_GetRenderPassOitVolumeGlslfx()
{
    static const HioGlslfxSharedPtr glslfx =
        std::make_shared<HioGlslfx>(HdxPackageRenderPassOitVolumeShader());
    return glslfx;
}

HdxOitVolumeRenderTask::HdxOitVolumeRenderTask(
                HdSceneDelegate* delegate, SdfPath const& id)
    : HdxRenderTask(delegate, id)
    , _oitVolumeRenderPassShader(
        std::make_shared<HdStRenderPassShader>(
            _GetRenderPassOitVolumeGlslfx()))
    , _isOitEnabled(HdxOitBufferAccessor::IsOitEnabled())
    , _oitShouldDraw(false)
{
}

HdxOitVolumeRenderTask::~HdxOitVolumeRenderTask() = default;

void
HdxOitVolumeRenderTask::_Sync(
    HdSceneDelegate* delegate,
    HdTaskContext* ctx,
    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (_isOitEnabled) {
        HdxRenderTask::_Sync(delegate, ctx, dirtyBits);
    }
}

static
bool
_HasColorAov(HdRenderPassAovBindingVector const& aovBindings)
{
    return std::find_if(aovBindings.begin(), aovBindings.end(),
        [](HdRenderPassAovBinding const& binding){
            return binding.aovName == HdAovTokens->color; })
                != aovBindings.end();
}

void
HdxOitVolumeRenderTask::Prepare(HdTaskContext* ctx,
                                HdRenderIndex* renderIndex)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Default to skipping OIT this frame, unless the checks below pass.
    _oitShouldDraw = false;

    // OIT buffers take up significant GPU resources. Skip if there are no
    // oit draw items (i.e. no volumetric draw items)
    if (!_isOitEnabled || !HdxRenderTask::_HasDrawItems()) {
        return;
    }

    // Prepare the renderpass state... Need to do this before we call
    // _GetRenderPassState
    _PrepareRenderPassState(ctx, renderIndex);

    // If we're not Storm, we can skip OIT
    HdRenderPassStateSharedPtr renderPassState = _GetRenderPassState();
    const HdStRenderPassStateSharedPtr stRenderPassState =
        std::dynamic_pointer_cast<HdStRenderPassState>(renderPassState);
    if (!stRenderPassState) {
        return;
    }

    // If there are aovs, but none of them are color, skip rendering for this 
    // task.
    HdRenderPassAovBindingVector const& aovBindings =
        renderPassState->GetAovBindings();
    if (!aovBindings.empty() && !_HasColorAov(aovBindings)) {
        return;
    }

    // Mark that we need to draw this frame.
    _oitShouldDraw = true;
    
    // Prepare volume OIT task
    // Note: the renderpass state has already run Prepare(), so the code below
    // is a little sketch. We're updating bindings and rasterization state,
    // which don't affect the uniform buffer contents, so this happens to work.
    // Ideally, in the future, we can refactor the RenderPassState object into
    // more useful pieces and replace this with something more robust.
    stRenderPassState->SetRenderPassShader(_oitVolumeRenderPassShader);
    _AddStormLightingShader(ctx, stRenderPassState);
    _AddStormSelectionBindings(ctx, _oitVolumeRenderPassShader);

    _oitVolumeRenderPassShader->UpdateAovInputTextures(
            renderPassState->GetAovInputBindings(),
            renderIndex);

    HdxOitBufferAccessor oitBufferAccessor(ctx);
    oitBufferAccessor.AllocateOitBuffersIfNecessary(renderIndex);
    oitBufferAccessor.AddOitBufferBindings(_oitVolumeRenderPassShader);

    renderPassState->SetEnableDepthTest(false);
    // Setting cull style for consistency even though it is hard-coded in
    // shaders/volume.glslfx.
    renderPassState->SetCullStyle(HdCullStyleBack);
    // We render into an SSBO -- not MSSA compatible
    renderPassState->SetMultiSampleEnabled(false);

    // XXX: To show volumes that intersect the far clipping plane, we might
    // consider calling glEnable(GL_DEPTH_CLAMP) here.
    renderPassState->SetEnableDepthMask(false);
    renderPassState->SetColorMasks({HdRenderPassState::ColorMaskNone});
}

void
HdxOitVolumeRenderTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!_oitShouldDraw) {
        return;
    }

    // Signal the OitResolveTask that it should run this frame, to compose the
    // pixels we write to the OIT buffers.
    HdxOitBufferAccessor(ctx).RequestOitBuffers();

    // Clear OIT buffers if needed (note that we set a flag in the context to
    // make sure we don't clear twice...)
    HdxOitBufferAccessor(ctx).InitializeOitBuffersIfNecessary(_GetHgi());

    // Transition depth texture (if it exists) to shader read layout.
    HgiTextureUsage oldLayout {};
    HgiTextureHandle depthTexture {};
    if (_HasTaskContextData(ctx, HdAovTokens->depth)) {
        _GetTaskContextData(ctx, HdAovTokens->depth, &depthTexture);
        oldLayout =
            depthTexture->SubmitLayoutChange(HgiTextureUsageBitsShaderRead);
    }
    
    HdxRenderTask::Execute(ctx);

    // Restore old layout.
    if (oldLayout) {
        depthTexture->SubmitLayoutChange(oldLayout);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
