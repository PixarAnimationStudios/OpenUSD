//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/oitRenderTask.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/oitBufferAccessor.h"

#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hdSt/renderPassShader.h"

#include "pxr/imaging/glf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

static const HioGlslfxSharedPtr &
_GetRenderPassOitGlslfx()
{
    static const HioGlslfxSharedPtr glslfx =
        std::make_shared<HioGlslfx>(HdxPackageRenderPassOitShader());
    return glslfx;
}

static const HioGlslfxSharedPtr &
_GetRenderPassOitOpaqueGlslfx()
{
    static const HioGlslfxSharedPtr glslfx =
        std::make_shared<HioGlslfx>(HdxPackageRenderPassOitOpaqueShader());
    return glslfx;
}

HdxOitRenderTask::HdxOitRenderTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdxRenderTask(delegate, id)
    , _oitTranslucentRenderPassShader(
        std::make_shared<HdStRenderPassShader>(
            _GetRenderPassOitGlslfx()))
    , _oitOpaqueRenderPassShader(
        std::make_shared<HdStRenderPassShader>(
            _GetRenderPassOitOpaqueGlslfx()))
    , _isOitEnabled(HdxOitBufferAccessor::IsOitEnabled())
{
}

HdxOitRenderTask::~HdxOitRenderTask() = default;

void
HdxOitRenderTask::_Sync(
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

void
HdxOitRenderTask::Prepare(HdTaskContext* ctx,
                          HdRenderIndex* renderIndex)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
 
    // OIT buffers take up significant GPU resources. Skip if there are no
    // oit draw items (i.e. no translucent draw items)
    if (_isOitEnabled && HdxRenderTask::_HasDrawItems()) {
        HdxRenderTask::Prepare(ctx, renderIndex);
        HdxOitBufferAccessor(ctx).RequestOitBuffers();
    }
}

void
HdxOitRenderTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    GLF_GROUP_FUNCTION();

    if (!_isOitEnabled || !HdxRenderTask::_HasDrawItems()) {
        return;
    }
    
    //
    // Pre Execute Setup
    //
    {
        HdxOitBufferAccessor oitBufferAccessor(ctx);

        oitBufferAccessor.RequestOitBuffers();
        oitBufferAccessor.InitializeOitBuffersIfNecessary(_GetHgi());
        if (!oitBufferAccessor.AddOitBufferBindings(
                _oitTranslucentRenderPassShader)) {
            TF_CODING_ERROR(
                "No OIT buffers allocated but needed by OIT render task");
            return;
        }
    }

    HdRenderPassStateSharedPtr renderPassState = _GetRenderPassState(ctx);
    if (!TF_VERIFY(renderPassState)) return;

    HdStRenderPassState* extendedState =
        dynamic_cast<HdStRenderPassState*>(renderPassState.get());
    if (!TF_VERIFY(extendedState, "OIT only works with HdSt")) {
        return;
    }

    // Render pass state overrides
    {
        extendedState->SetUseSceneMaterials(true);
        // blending is relevant only for the oitResolve task.
        extendedState->SetBlendEnabled(false);
        extendedState->SetAlphaToCoverageEnabled(false);
        extendedState->SetAlphaThreshold(0.f);
    }

    // We render into an SSBO -- not MSAA compatible
    renderPassState->SetMultiSampleEnabled(false);

    //
    // 1. Opaque pixels pass
    // 
    // Fragments that are opaque (alpha > 1.0) are rendered to the active
    // framebuffer. Translucent fragments are discarded.
    // This can reduce the data written to the OIT SSBO buffers because of
    // improved depth testing.
    //
    {
        extendedState->SetRenderPassShader(_oitOpaqueRenderPassShader);
        renderPassState->SetEnableDepthMask(true);
        renderPassState->SetColorMaskUseDefault(false);
        renderPassState->SetColorMasks({HdRenderPassState::ColorMaskRGBA});

        HdxRenderTask::Execute(ctx);
    }

    //
    // 2. Translucent pixels pass
    //
    // Fill OIT SSBO buffers for the translucent fragments.  
    //
    {
        extendedState->SetRenderPassShader(_oitTranslucentRenderPassShader);
        renderPassState->SetEnableDepthMask(false);
        renderPassState->SetColorMasks({HdRenderPassState::ColorMaskNone});
        HdxRenderTask::Execute(ctx);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
