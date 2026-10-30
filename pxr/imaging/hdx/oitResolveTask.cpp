//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hdx/oitResolveTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/package.h"

#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/imageShaderRenderPass.h"

#include "pxr/imaging/hdx/oitBufferAccessor.h"

PXR_NAMESPACE_OPEN_SCOPE

using HdBufferSourceSharedPtrVector = std::vector<HdBufferSourceSharedPtr>;

static GfVec2i
_GetScreenSize()
{
    // XXX Ideally we want screenSize to be passed in via the app.
    // (see Presto Stagecontext/TaskGraph), but for now we query this from GL.
    //
    // Using GL_VIEWPORT here (or viewport from RenderParams) is in-correct!
    //
    // The gl_FragCoord we use in the OIT shaders is relative to the FRAMEBUFFER
    // size (screen size), not the gl_viewport size.
    // We do various tricks with glViewport for Presto slate mode so we cannot
    // rely on it to determine the 'screenWidth' we need in the gl shaders.
    //
    // The CounterBuffer is especially fragile to this because in the glsl shdr
    // we calculate a 'screenIndex' based on gl_fragCoord that indexes into
    // the CounterBuffer. If we did not make enough room in the CounterBuffer
    // we may be reading/writing an invalid index into the CounterBuffer.
    //

    GLint attachType = 0;
    glGetFramebufferAttachmentParameteriv(
        GL_DRAW_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
        &attachType);

    GLint attachId = 0;
    glGetFramebufferAttachmentParameteriv(
        GL_DRAW_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
        &attachId);

    // XXX Fallback to gl viewport in case we do not find a non-default FBO for
    // bakends that do not attach a custom FB. This is in-correct, but gl does
    // not let us query size properties of default framebuffer. For this we
    // need the screenSize to be passed in via app (see note above)
    if (attachId <= 0) {
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        return GfVec2i(viewport[2], viewport[3]);
    }

    if (attachType == GL_TEXTURE) {
        GLint w, h;
        glGetTextureLevelParameteriv(attachId, 0, GL_TEXTURE_WIDTH, &w);
        glGetTextureLevelParameteriv(attachId, 0, GL_TEXTURE_HEIGHT, &h);
        return GfVec2i(w,h);
    }

    if (attachType == GL_RENDERBUFFER) {
        GLint w, h;
        glGetNamedRenderbufferParameteriv(attachId, GL_RENDERBUFFER_WIDTH, &w);
        glGetNamedRenderbufferParameteriv(attachId, GL_RENDERBUFFER_HEIGHT, &h);
        return GfVec2i(w,h);
    }

    constexpr int oitScreenSizeFallback = 2048;

    return GfVec2i(oitScreenSizeFallback, oitScreenSizeFallback);
}

HdxOitResolveTask::HdxOitResolveTask(
    HdSceneDelegate* delegate,
    SdfPath const& id)
    : HdTask(id)
    , _screenSize(0)
    , _oitShouldDraw(false)
{
}

HdxOitResolveTask::~HdxOitResolveTask() = default;

void
HdxOitResolveTask::Sync(
    HdSceneDelegate* delegate,
    HdTaskContext*   ctx,
    HdDirtyBits*     dirtyBits)
{
    HD_TRACE_FUNCTION();

    if (!_renderPassState) {
        // We do not use renderDelegate->CreateRenderPassState because
        // ImageShaders always use HdSt
        _renderPassState = std::make_shared<HdStRenderPassState>();
        _renderPassState->SetEnableDepthTest(false);
        _renderPassState->SetEnableDepthMask(false);
        _renderPassState->SetAlphaThreshold(0.0f);
        _renderPassState->SetAlphaToCoverageEnabled(false);
        _renderPassState->SetColorMasks({HdRenderPassState::ColorMaskRGBA});
        _renderPassState->SetBlendEnabled(true);

        // We expect pre-multiplied color as input into the OIT resolve shader
        // e.g. vec4(rgb * a, a). Hence the src factor for rgb is "One" since
        // src alpha is already accounted for.
        // Alpha's are blended with the same blending equation as the rgb's.
        // Thinking about it conceptually, if you're looking through two glass
        // windows both occluding 50% of light, some light would still be
        // passing through. 50% of light passes through the first window, then
        // 50% of the remaining light through the second window. Hence the
        // equation: 0.5 + 0.5 * (1 - 0.5) = 0.75, as 75% of light is occluded.
        _renderPassState->SetBlend(
            HdBlendOp::HdBlendOpAdd,
            HdBlendFactor::HdBlendFactorOne,
            HdBlendFactor::HdBlendFactorOneMinusSrcAlpha,
            HdBlendOp::HdBlendOpAdd,
            HdBlendFactor::HdBlendFactorOne,
            HdBlendFactor::HdBlendFactorOneMinusSrcAlpha);

        _renderPassShader = std::make_shared<HdStRenderPassShader>(
            HdxPackageOitResolveImageShader());
        _renderPassState->SetRenderPassShader(_renderPassShader);
    }

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxOitResolveTaskParams params;

        if (!_GetTaskParams(delegate, &params)) {
            return;
        }

        _renderPassState->SetUseAovMultiSample(
            params.useAovMultiSample);
        _renderPassState->SetResolveAovMultiSample(
            params.resolveAovMultiSample);
    }

    *dirtyBits = HdChangeTracker::Clean;

    // Note: We defer creation of the render pass to the Prepare phase since
    // the notion of a "collection" is irrelevant to this task.
    // So, the Sync step for the image shader render pass is skipped as well.
}

HdRenderPassStateSharedPtr
HdxOitResolveTask::_GetContextRenderPassState(
    HdTaskContext* const ctx) const
{
    if (!_HasTaskContextData(ctx, HdxTokens->renderPassState)) {
        return nullptr;
    }

    HdRenderPassStateSharedPtr renderPassState;
    _GetTaskContextData(ctx, HdxTokens->renderPassState, &renderPassState);
    return renderPassState;
}

void
HdxOitResolveTask::_UpdateCameraFraming(
    HdTaskContext* const ctx)
{
    const HdRenderPassStateSharedPtr ctxRenderPassState =
        _GetContextRenderPassState(ctx);
    if (!ctxRenderPassState) {
        TF_CODING_ERROR("Unable to set camera framing data due to missing "
                        "render pass state on task context");
        return;
    }

    _renderPassState->SetCamera(
        ctxRenderPassState->GetCamera());
    _renderPassState->SetOverrideWindowPolicy(
        ctxRenderPassState->GetOverrideWindowPolicy());

    const CameraUtilFraming& framing = ctxRenderPassState->GetFraming();
    if (framing.IsValid()) {
        _renderPassState->SetFraming(framing);
    } else {
        _renderPassState->SetViewport(
            ctxRenderPassState->GetViewport());
    }
}

const HdRenderPassAovBindingVector&
HdxOitResolveTask::_GetAovBindings(
    HdTaskContext* const ctx) const
{
    static HdRenderPassAovBindingVector empty;

    const HdRenderPassStateSharedPtr ctxRenderPassState =
        _GetContextRenderPassState(ctx);
    if (!ctxRenderPassState) {
        return empty;
    }

    return ctxRenderPassState->GetAovBindings();
}

GfVec2i
HdxOitResolveTask::_ComputeScreenSize(
    HdTaskContext* ctx,
    HdRenderIndex* renderIndex) const
{
    const HdRenderPassAovBindingVector& aovBindings = _GetAovBindings(ctx);
    if (aovBindings.empty()) {
        return _GetScreenSize();
    }

    const SdfPath &bufferId = aovBindings.front().renderBufferId;
    HdRenderBuffer * const buffer = static_cast<HdRenderBuffer*>(
        renderIndex->GetBprim(HdPrimTypeTokens->renderBuffer, bufferId));
    if (!buffer) {
        TF_CODING_ERROR("No render buffer at path %s specified in AOV bindings",
                        bufferId.GetText());
        return _GetScreenSize();
    }

    return GfVec2i(buffer->GetWidth(), buffer->GetHeight());
}

void
HdxOitResolveTask::_PrepareOitBuffers(
    HdTaskContext* ctx,
    HdRenderIndex* renderIndex,
    GfVec2i const& screenSize)
{
    // Pick up bars pre-allocated by AllocateOitBuffersIfNecessary in the OIT
    // render tasks' Prepare().
    if (!_counterBar) {
        const auto it = ctx->find(HdxTokens->oitCounterBufferBar);
        if (it != ctx->end()) {
            _counterBar = it->second.Get<HdBufferArrayRangeSharedPtr>();
            _jointBar   = (*ctx)[HdxTokens->oitJointBufferBar]
                              .Get<HdBufferArrayRangeSharedPtr>();
            _dataBar    = (*ctx)[HdxTokens->oitDataBufferBar]
                              .Get<HdBufferArrayRangeSharedPtr>();
            _uniformBar = (*ctx)[HdxTokens->oitUniformBar]
                              .Get<HdBufferArrayRangeSharedPtr>();
        }
    }

    if (!_counterBar) {
        return;
    }

    // Restore bars to ctx each frame in case the context was cleared.
    (*ctx)[HdxTokens->oitJointBufferBar] = _jointBar;
    (*ctx)[HdxTokens->oitCounterBufferBar] = _counterBar;
    (*ctx)[HdxTokens->oitDataBufferBar] = _dataBar;
    (*ctx)[HdxTokens->oitUniformBar] = _uniformBar;

    HdxOitBufferAccessor(ctx).ResizeOitBuffersIfNecessary(
        renderIndex, screenSize, &_screenSize);
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
HdxOitResolveTask::Prepare(HdTaskContext* ctx,
                           HdRenderIndex* renderIndex)
{
    HD_TRACE_FUNCTION();

    _oitShouldDraw = false;

    // Pull OIT buffers off the context...
    _PrepareOitBuffers(
        ctx, renderIndex, _ComputeScreenSize(ctx, renderIndex));

    // If there aren't any OIT buffers, we can early out. If OIT is disabled,
    // or if we're not running Storm, we'll always early out here.
    if (!_counterBar) {
        return;
    }

    const HdRenderPassAovBindingVector aovBindings = _GetAovBindings(ctx);

    // If we're not rendering color, we can early out of prepare.
    if (!aovBindings.empty() && !_HasColorAov(aovBindings)) {
        return;
    }

    _oitShouldDraw = true;

    // OitRenderTask also earlies out if there are no relevant draw items;
    // we can't check that here, so we need to prepare the resolve pass,
    // but we can potentially skip execute due to the oitRequest flag.

    // Update renderpass state.
    _renderPassState->SetAovBindings(aovBindings);
    _UpdateCameraFraming(ctx);
    _renderPassState->Prepare(renderIndex->GetResourceRegistry());

    // Create the renderpass if needed.
    if (!_renderPass) {
        HdRprimCollection collection;
        HdRenderDelegate* renderDelegate = renderIndex->GetRenderDelegate();

        if (!TF_VERIFY(dynamic_cast<HdStRenderDelegate*>(renderDelegate),
             "OIT Task only works with HdSt")) {
            return;
        }

        _renderPass = std::make_shared<HdSt_ImageShaderRenderPass>(
            renderIndex, collection);
        _renderPass->SetupFullscreenTriangleDrawItem();
    }

    // Add OIT buffer bindings to the renderpass shader.
    HdxOitBufferAccessor oitBufferAccessor(ctx);
    if (!oitBufferAccessor.AddOitBufferBindings(_renderPassShader)) {
        TF_CODING_ERROR(
            "No OIT buffers allocated but needed by OIT resolve task");
        return;
    }
}

void
HdxOitResolveTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Render tasks that write to the OIT buffers will set oitRequestFlag if
    // they contain any draw items.  If we don't see oitRequestFlag, skip this
    // task. If we *do* see oitRequestFlag, we continue, but we also delete it
    // to reset the state for the next frame, since this is the last OIT task.
    if (ctx->erase(HdxTokens->oitRequestFlag) == 0) {
        return;
    }

    // Explicitly erase clear flag (if set) so that the next frame knows to
    // re-clear the OIT buffers.
    ctx->erase(HdxTokens->oitClearedFlag);

    if (!_oitShouldDraw) {
        return;
    }

    if (!TF_VERIFY(_renderPassState)) return;
    if (!TF_VERIFY(_renderPassShader)) return;

    _renderPass->Execute(_renderPassState, GetRenderTags());
}

bool
operator==(HdxOitResolveTaskParams const& lhs,
           HdxOitResolveTaskParams const& rhs)
{
    return lhs.useAovMultiSample == rhs.useAovMultiSample
        && lhs.resolveAovMultiSample == rhs.resolveAovMultiSample;
}

bool
operator!=(HdxOitResolveTaskParams const& lhs,
           HdxOitResolveTaskParams const& rhs)
{
    return !(lhs==rhs);
}

std::ostream&
operator<<(std::ostream& out, HdxOitResolveTaskParams const& p)
{
    out << "OitResolveTask Params: (...) "
        << p.useAovMultiSample << " "
        << p.resolveAovMultiSample;
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE
