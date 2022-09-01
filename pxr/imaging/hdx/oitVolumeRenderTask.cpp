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
#include "pxr/imaging/hdx/oitVolumeRenderTask.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/oitBufferAccessor.h"

#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/volume.h"

#include "pxr/imaging/glf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

HdxOitVolumeRenderTask::HdxOitVolumeRenderTask(
                HdSceneDelegate* delegate, SdfPath const& id)
    : HdxRenderTask(delegate, id)
    , _oitVolumeRenderPassShader(
        std::make_shared<HdStRenderPassShader>(
            HdxPackageRenderPassOitVolumeShader()))
    , _isOitEnabled(HdxOitBufferAccessor::IsOitEnabled())
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

void
HdxOitVolumeRenderTask::Prepare(HdTaskContext* ctx,
                                HdRenderIndex* renderIndex)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // OIT buffers take up significant GPU resources. Skip if there are no
    // oit draw items (i.e. no volumetric draw items)
    if (!_isOitEnabled || !HdxRenderTask::_HasDrawItems()) {
        return;
    }
    
    HdxRenderTask::Prepare(ctx, renderIndex);
    HdxOitBufferAccessor(ctx).RequestOitBuffers();

    if (HdRenderPassStateSharedPtr const state = _GetRenderPassState(ctx)) {
        _oitVolumeRenderPassShader->UpdateAovInputTextures(
            state->GetAovInputBindings(),
            renderIndex);
    }
}

void
HdxOitVolumeRenderTask::Execute(HdTaskContext* ctx)
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

    HdxOitBufferAccessor oitBufferAccessor(ctx);

    oitBufferAccessor.RequestOitBuffers();
    oitBufferAccessor.InitializeOitBuffersIfNecessary(_GetHgi());

    HdRenderPassStateSharedPtr renderPassState = _GetRenderPassState(ctx);
    if (!TF_VERIFY(renderPassState)) return;

    HdStRenderPassState* extendedState =
        dynamic_cast<HdStRenderPassState*>(renderPassState.get());
    if (!TF_VERIFY(extendedState, "OIT only works with HdSt")) {
        return;
    }

    extendedState->SetUseSceneMaterials(true);
    renderPassState->SetDepthFunc(HdCmpFuncAlways);
    // Setting cull style for consistency even though it is hard-coded in
    // shaders/volume.glslfx.
    renderPassState->SetCullStyle(HdCullStyleBack);

    if(!oitBufferAccessor.AddOitBufferBindings(_oitVolumeRenderPassShader)) {
        TF_CODING_ERROR(
            "No OIT buffers allocated but needed by OIT volume render task");
        return;
    }

    // We render into an SSBO -- not MSSA compatible
    renderPassState->SetMultiSampleEnabled(false);

    // XXX
    //
    // To show volumes that intersect the far clipping plane, we might consider
    // calling glEnable(GL_DEPTH_CLAMP) here.

    //
    // Translucent pixels pass
    //
    extendedState->SetRenderPassShader(_oitVolumeRenderPassShader);
    renderPassState->SetEnableDepthMask(false);
    renderPassState->SetColorMasks({HdRenderPassState::ColorMaskNone});
    HdxRenderTask::Execute(ctx);
}


PXR_NAMESPACE_CLOSE_SCOPE
