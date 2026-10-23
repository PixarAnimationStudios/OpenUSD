//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/oitRenderTask.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/oitBufferAccessor.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hgi/capabilities.h"

#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/renderPassShader.h"

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
    , _translucentPassState(
        std::make_shared<HdStRenderPassState>(_translucentPassShader))
    , _isOitEnabled(HdxOitBufferAccessor::IsOitEnabled())
    , _oitShouldDraw(false)
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
        // Translucent pass needs to listen to DirtyCollection, otherwise gets
        // state from opaque pass.
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

        // Sync opaque pass - note this may update dirtyBits.
        HdxRenderTask::_Sync(delegate, ctx, dirtyBits);

        // Sync translucent pass
        if (_translucentPass) {
            _translucentPass->Sync();
        }
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
HdxOitRenderTask::Prepare(HdTaskContext* ctx,
                          HdRenderIndex* renderIndex)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Default to skipping OIT this frame, unless the checks below pass.
    _oitShouldDraw = false;

    // OIT buffers take up significant GPU resources. Skip if there are no
    // oit draw items (i.e. no translucent draw items)
    if (!_isOitEnabled || !HdxRenderTask::_HasDrawItems()) {
        return;
    }

    // Prepare opaque pass state... Need to do this here before we call
    // _GetRenderPassState.
    _PrepareRenderPassState(ctx, renderIndex);

    // If we're not Storm, we can skip OIT.
    const HdRenderPassStateSharedPtr opaquePassState =
        _GetRenderPassState();
    const HdStRenderPassStateSharedPtr stOpaquePassState =
        std::dynamic_pointer_cast<HdStRenderPassState>(opaquePassState);
    if (!stOpaquePassState) {
        return;
    }

    // Likewise, if we're not rendering color, we can skip OIT.
    HdRenderPassAovBindingVector aovBindings =
        opaquePassState->GetAovBindings();
    if (!aovBindings.empty() && !_HasColorAov(aovBindings)) {
        return;
    }

    // Mark that we need to draw this frame.
    _oitShouldDraw = true;

    // Prepare opaque pass
    // Note: the opaque pass state has already run Prepare(), so the code below
    // is a little sketch. The state overrides we're setting happen to be
    // rasterization state that don't end up in the uniform buffer, which is
    // what makes this API violation work. Ideally, in the future, if we break
    // the renderpass state struct into more useful pieces, we can replace this
    // with something more robust.

    stOpaquePassState->SetRenderPassShader(_opaquePassShader);
    _AddStormLightingShader(ctx, stOpaquePassState);
    _AddStormSelectionBindings(ctx, _opaquePassShader);
    // blending is relevant only for the oitResolve task.
    opaquePassState->SetBlendEnabled(false);
    opaquePassState->SetAlphaToCoverageEnabled(false);
    // We render into an SSBO -- not MSAA compatible
    opaquePassState->SetMultiSampleEnabled(false);
    opaquePassState->SetEnableDepthMask(true);
    opaquePassState->SetColorMaskUseDefault(false);
    opaquePassState->SetColorMasks({HdRenderPassState::ColorMaskRGBA});

    // Prepare translucent pass
    // We start by duplicating the opaque state.
    const HdStRenderPassStateSharedPtr stTranslucentPassState =
        std::dynamic_pointer_cast<HdStRenderPassState>(_translucentPassState);
    (*stTranslucentPassState) = (*stOpaquePassState);
    // Ensure we point at the right renderpass shader.
    stTranslucentPassState->SetRenderPassShader(_translucentPassShader);

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

    // Ensure OIT bars exist before binding so that the shader hash is stable
    // from the first frame regardless of when oitResolveTask::Prepare runs.
    HdxOitBufferAccessor oitBufferAccessor(ctx);
    oitBufferAccessor.AllocateOitBuffersIfNecessary(renderIndex);
    oitBufferAccessor.AddOitBufferBindings(_translucentPassShader);
    // Ensure selection highlight bindings are registered with the shader
    _AddStormSelectionBindings(ctx, _translucentPassShader);

    // Translucent pass state overrides
    _translucentPassState->SetEnableDepthMask(false);
    _translucentPassState->SetColorMasks({HdRenderPassState::ColorMaskNone});

    // Prepare the translucent pass state
    _translucentPassState->Prepare(renderIndex->GetResourceRegistry());
}

void
HdxOitRenderTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!_oitShouldDraw) {
        return;
    }

    // Signal the OitResolveTask that it should run this frame, to compose the
    // pixels from the translucent pass.
    HdxOitBufferAccessor(ctx).RequestOitBuffers();

    // Clear OIT buffers if needed (note that we set a flag in the context to
    // make sure we don't clear twice...)
    HdxOitBufferAccessor(ctx).InitializeOitBuffersIfNecessary(_GetHgi());

    //
    // 1. Opaque pixels pass
    //
    // Fragments that are opaque (alpha >= 1.0) are rendered to the active
    // framebuffer. Translucent fragments are discarded.
    // This can reduce the data written to the OIT SSBO buffers because of
    // improved depth testing.
    //

    HdxRenderTask::Execute(ctx);

    if (!_translucentPass || ! _translucentPassState) {
        return;
    }

    //
    // 2. Translucent pixels pass
    //
    // Fill OIT SSBO buffers for the translucent fragments.
    //

    _translucentPass->Execute(_translucentPassState, GetRenderTags());
}

PXR_NAMESPACE_CLOSE_SCOPE
