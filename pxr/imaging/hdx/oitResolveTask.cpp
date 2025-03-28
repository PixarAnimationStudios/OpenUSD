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
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/imageShaderRenderPass.h"

#include "pxr/imaging/hdx/oitBufferAccessor.h"

#include "pxr/imaging/glf/diagnostic.h"

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
    static const int numSamples = 8; // Should match glslfx files

    if (!(screenSize[0] >= 0 && screenSize[1] >= 0)) {
        TF_CODING_ERROR("Invalid screen size for OIT resolve task %s",
                        GetId().GetText());
        return;
    }

    HdStResourceRegistrySharedPtr const& hdStResourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            renderIndex->GetResourceRegistry());
    
    const bool createOitBuffers = !_counterBar;
    if (createOitBuffers) {
        //
        // Counter Buffer
        //
        HdBufferSpecVector counterSpecs{
            { HdxTokens->hdxOitCounterBuffer, HdTupleType{HdTypeInt32, 1} }
        };
        _counterBar = hdStResourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitCounter,
                                            counterSpecs,
                                            HdBufferArrayUsageHintBitsStorage);
        //
        // Index Buffer
        //
        HdBufferSpecVector indexSpecs{
            { HdxTokens->hdxOitIndexBuffer, HdTupleType{HdTypeInt32, 1} }
        };
        _indexBar = hdStResourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitIndices,
                                            indexSpecs,
                                            HdBufferArrayUsageHintBitsStorage);

        //
        // Data Buffer
        //        
        HdBufferSpecVector dataSpecs{
            { HdxTokens->hdxOitDataBuffer, HdTupleType{HdTypeFloatVec4, 1} }
        };
        _dataBar = hdStResourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitData,
                                            dataSpecs,
                                            HdBufferArrayUsageHintBitsStorage);

        //
        // Depth Buffer
        //
        HdBufferSpecVector depthSpecs{
            { HdxTokens->hdxOitDepthBuffer, HdTupleType{HdTypeFloat, 1} }
        };
        _depthBar = hdStResourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitDepth,
                                            depthSpecs,
                                            HdBufferArrayUsageHintBitsStorage);

        //
        // Uniforms
        //
        HdBufferSpecVector uniformSpecs{
            { HdxTokens->oitScreenSize, HdTupleType{HdTypeInt32Vec2, 1} }
        };
        _uniformBar = hdStResourceRegistry->AllocateUniformBufferArrayRange(
                                            /*role*/HdxTokens->oitUniforms,
                                            uniformSpecs,
                                            HdBufferArrayUsageHintBitsUniform);
    }

    // Make sure task context has our buffer each frame (in case its cleared)
    (*ctx)[HdxTokens->oitCounterBufferBar] = _counterBar;
    (*ctx)[HdxTokens->oitIndexBufferBar] = _indexBar;
    (*ctx)[HdxTokens->oitDataBufferBar] = _dataBar;
    (*ctx)[HdxTokens->oitDepthBufferBar] = _depthBar;
    (*ctx)[HdxTokens->oitUniformBar] = _uniformBar;

    // The OIT buffer are sized based on the size of the screen and use 
    // fragCoord to index into the buffers.
    // We must update uniform screenSize when either X or Y increases in size.
    const bool resizeOitBuffers = (screenSize[0] > _screenSize[0] ||
                                   screenSize[1] > _screenSize[1]);

    if (resizeOitBuffers) {
        _screenSize = screenSize;
        const int newBufferSize = screenSize[0] * screenSize[1];

        // +1 because element 0 of the counter buffer is used as an atomic
        // counter in the shader to give each fragment a unique index.
        _counterBar->Resize(newBufferSize + 1);
        _indexBar->Resize(newBufferSize * numSamples);
        _dataBar->Resize(newBufferSize * numSamples);
        _depthBar->Resize(newBufferSize * numSamples);

        // Update the values in the uniform buffer
        hdStResourceRegistry->AddSource(
            _uniformBar,
            std::make_shared<HdVtBufferSource>(
                HdxTokens->oitScreenSize,
                VtValue(screenSize)));
    }
}

void
HdxOitResolveTask::Prepare(HdTaskContext* ctx,
                           HdRenderIndex* renderIndex)
{
    // Only allocate/resize buffer if a render task requested it.
    if (ctx->find(HdxTokens->oitRequestFlag) == ctx->end()) {
        // Deallocate buffers here?
        return;
    }

    // The HdTaskContext might not be cleared between two engine execute
    // iterations, so we explicitly delete the cleared flag here so that the
    // execute of the first oit render task will clear the buffer in this
    // iteration.
    ctx->erase(HdxTokens->oitClearedFlag);

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

    _PrepareOitBuffers(
        ctx, renderIndex, _ComputeScreenSize(ctx, renderIndex)); 
}

void
HdxOitResolveTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    GLF_GROUP_FUNCTION();

    // Check whether the request flag was set and delete it so that for the
    // next iteration the request flag is not set unless an OIT render task
    // explicitly sets it.
    if (ctx->erase(HdxTokens->oitRequestFlag) == 0) {
        return;
    }

    // Explicitly erase clear flag so that it can be re-used by subsequent
    // OIT render and resolve tasks.
    ctx->erase(HdxTokens->oitClearedFlag);

    if (!TF_VERIFY(_renderPassState)) return;
    if (!TF_VERIFY(_renderPassShader)) return;

    _renderPassState->SetAovBindings(_GetAovBindings(ctx));
    _UpdateCameraFraming(ctx);

    HdxOitBufferAccessor oitBufferAccessor(ctx);
    if (!oitBufferAccessor.AddOitBufferBindings(_renderPassShader)) {
        TF_CODING_ERROR(
            "No OIT buffers allocated but needed by OIT resolve task");
        return;
    }

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
