//
// Copyright 2019 Pixar
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

HdxOitRenderTask::HdxOitRenderTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdxRenderTask(delegate, id)
    , _oitTranslucentRenderPassShader(
        std::make_shared<HdStRenderPassShader>(
            HdxPackageRenderPassOitShader()))
    , _oitOpaqueRenderPassShader(
        std::make_shared<HdStRenderPassShader>(
            HdxPackageRenderPassOitOpaqueShader()))
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
