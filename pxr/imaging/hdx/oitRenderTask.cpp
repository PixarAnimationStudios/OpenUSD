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

#include "pxr/imaging/hgi/capabilities.h"

#include "pxr/imaging/hdSt/renderPass.h"
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
    , _translucentPassShader(
        std::make_shared<HdStRenderPassShader>(_GetRenderPassOitGlslfx()))
    , _opaquePassShader(
        std::make_shared<HdStRenderPassShader>(_GetRenderPassOitOpaqueGlslfx()))
    , _isOitEnabled(HdxOitBufferAccessor::IsOitEnabled())
    , _translucentPassState(
        std::make_shared<HdStRenderPassState>(_translucentPassShader))
{
}

HdxOitRenderTask::~HdxOitRenderTask() = default;

bool
HdxOitRenderTask::IsConverged() const
{
    if (HdxRenderTask::IsConverged()) {
        if (_translucentPass) {
            return _translucentPass->IsConverged();
        }
        return true;
    }
    return false;
}

void
HdxOitRenderTask::_Sync(
    HdSceneDelegate* delegate,
    HdTaskContext* ctx,
    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    if (_isOitEnabled) {
        if (*dirtyBits & HdChangeTracker::DirtyCollection) {
            const auto& coll = delegate->Get(GetId(), HdTokens->collection)
                .GetWithDefault<HdRprimCollection>();
            if (coll.GetName().IsEmpty() && _translucentPass) {
                // destroy the translucent pass
                _translucentPass.reset();
            } else if (_translucentPass) {
                // update the translucent pass
                _translucentPass->SetRprimCollection(coll);
            } else if (!coll.GetName().IsEmpty()) {
                // create the translucent pass
                HdRenderIndex& renderIndex = delegate->GetRenderIndex();
                _translucentPass = renderIndex.GetRenderDelegate()
                    ->CreateRenderPass(&renderIndex, coll);
            }
        }

        // Sync opaque pass
        HdxRenderTask::_Sync(delegate, ctx, dirtyBits);

        // Sync translucent pass

        // XXX: We cannot sync task params from the opaque to the translucent
        // pass here, as they may be managed by a separate, application-
        // controlled setup task whose path we cannot determine. Without the
        // path, we cannot retrieve them from the delegate. So we must wait
        // until the setup task's Prepare() phase has processed the task params
        // into the opaque pass's state and published that into the task
        // context.

        if (_translucentPass) {
            _translucentPass->Sync();
        }
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
        // prepare opaque pass
        HdxRenderTask::Prepare(ctx, renderIndex);
        HdxOitBufferAccessor(ctx).RequestOitBuffers();

        // XXX: We cannot sync or prepare the translucent pass here either
        // because the separate, application-controlled setup task's Prepare()
        // phase may not have completed yet. We have to wait until our own
        // Execute() phase to set up the translucent pass's state and do any
        // tasks required for syncing and preparing the translucent pass.
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
HdxOitRenderTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    GLF_GROUP_FUNCTION();

    if (!_isOitEnabled || !HdxRenderTask::_HasDrawItems()) {
        return;
    }

    HdRenderPassStateSharedPtr opaquePassState = _GetRenderPassState(ctx);
    if (!TF_VERIFY(opaquePassState)) {
        return;
    }
    auto* stOpaquePassState =
        dynamic_cast<HdStRenderPassState*>(opaquePassState.get());
    if (!TF_VERIFY(stOpaquePassState, "OIT only works with Storm")) {
        return;
    }

    // if there are aovs, but none of them is color, skip rendering for this
    // task. NB: Not const& because we'll use it again below.
    HdRenderPassAovBindingVector aovBindings =
        opaquePassState->GetAovBindings();
    if (!aovBindings.empty() && !_HasColorAov(aovBindings)) {
        return;
    }

    HdxOitBufferAccessor oitBufferAccessor(ctx);
    oitBufferAccessor.RequestOitBuffers();
    oitBufferAccessor.InitializeOitBuffersIfNecessary(_GetHgi());
    if (!oitBufferAccessor.AddOitBufferBindings(_translucentPassShader)) {
        TF_CODING_ERROR(
            "No OIT buffers allocated but needed by OIT render task");
        return;
    }

    //
    // 1. Opaque pixels pass
    //
    // Fragments that are opaque (alpha >= 1.0) are rendered to the active
    // framebuffer. Translucent fragments are discarded.
    // This can reduce the data written to the OIT SSBO buffers because of
    // improved depth testing.
    //

    // Opaque pass state overrides
    stOpaquePassState->SetRenderPassShader(_opaquePassShader);
    // blending is relevant only for the oitResolve task.
    opaquePassState->SetBlendEnabled(false);
    opaquePassState->SetAlphaToCoverageEnabled(false);
    opaquePassState->SetAlphaThreshold(0.f);
    // We render into an SSBO -- not MSAA compatible
    opaquePassState->SetMultiSampleEnabled(false);
    opaquePassState->SetEnableDepthMask(true);
    opaquePassState->SetColorMaskUseDefault(false);
    opaquePassState->SetColorMasks({HdRenderPassState::ColorMaskRGBA});

    // opaque pass execution
    HdxRenderTask::Execute(ctx);

    if (!_translucentPass || ! _translucentPassState) {
        return;
    }

    //
    // 2. Translucent pixels pass
    //
    // Fill OIT SSBO buffers for the translucent fragments.
    //

    // Copy the now fully resolved opaque pass state onto the translucent
    // pass state, preserving only the shader.
    auto* stTranslucentPassState =
        dynamic_cast<HdStRenderPassState*>(_translucentPassState.get());
    stTranslucentPassState->CopyAllExceptShaderFrom(*stOpaquePassState);

    HdRenderIndex* renderIndex = _translucentPass->GetRenderIndex();

    const HgiCapabilities* capabilities = _GetHgi()->GetCapabilities();
    if (!capabilities->IsSet(HgiDeviceCapabilitiesForceEarlyFragmentTest)) {
        // In case we don't have support for early fragment test, we need
        // to skip the depth texture for the translucent pass to avoid reading
        // and writing to the same depth texture.
        aovBindings.erase(std::remove_if(
            aovBindings.begin(), aovBindings.end(),
            [](const HdRenderPassAovBinding &aov) {
                return HdAovHasDepthSemantic(aov.aovName) ||
                    HdAovHasDepthStencilSemantic(aov.aovName); }),
            aovBindings.end());
        _translucentPassState->SetAovBindings(aovBindings);
        // We need to bind the depth texture if the platform doesn't allow
        // early fragment test to be forced. This works in tandem with the
        // TaskController setting the depth as input textures for the
        // OIT task.
        _translucentPassShader->UpdateAovInputTextures(
                _translucentPassState->GetAovInputBindings(),
                renderIndex);
    }

    // Ensure OIT buffer bindings are registered with the shader
    oitBufferAccessor.AddOitBufferBindings(_translucentPassShader);
    // Ensure RenderPassState buffer binding is registered with the shader
    stTranslucentPassState->SetRenderPassShader(_translucentPassShader);

    // Translucent pass state overrides
    _translucentPassState->SetEnableDepthMask(false);
    _translucentPassState->SetColorMasks({HdRenderPassState::ColorMaskNone});

    // execute the translucent pass
    _translucentPass->Execute(_translucentPassState, GetRenderTags());
}

PXR_NAMESPACE_CLOSE_SCOPE
