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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/imaging/hdx/oitResolveTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/debugCodes.h"
#include "pxr/imaging/hdx/package.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/imageShaderRenderPass.h"

#include "pxr/imaging/hdx/oitBufferAccessor.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef std::vector<HdBufferSourceSharedPtr> HdBufferSourceSharedPtrVector;


HdxOitResolveTask::HdxOitResolveTask(
    HdSceneDelegate* delegate, 
    SdfPath const& id)
    : HdTask(id)
    , _screenSize(0)
{
}

HdxOitResolveTask::~HdxOitResolveTask()
{
}

void
HdxOitResolveTask::Sync(
    HdSceneDelegate* delegate,
    HdTaskContext*   ctx,
    HdDirtyBits*     dirtyBits)
{
    HD_TRACE_FUNCTION();
    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxOitResolveTask::_PrepareOitBuffers(
    HdTaskContext* ctx, 
    HdRenderIndex* renderIndex,
    GfVec2i const& screenSize)
{
    const int numSamples = 8; // Should match glslfx files

     HdStResourceRegistrySharedPtr const& hdStResourceRegistry =
        boost::static_pointer_cast<HdStResourceRegistry>(
            renderIndex->GetResourceRegistry());

    bool createOitBuffers = !_counterBar;
    if (createOitBuffers) { 
        //
        // Counter Buffer
        //
        HdBufferSpecVector counterSpecs;
        counterSpecs.push_back(HdBufferSpec(
            HdxTokens->hdxOitCounterBuffer, 
            HdTupleType {HdTypeInt32, 1}));
        _counterBar = hdStResourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitCounter,
                                            counterSpecs,
                                            HdBufferArrayUsageHint());
        //
        // Index Buffer
        //
        HdBufferSpecVector indexSpecs;
        indexSpecs.push_back(HdBufferSpec(
            HdxTokens->hdxOitIndexBuffer,
            HdTupleType {HdTypeInt32, 1}));
        _indexBar = hdStResourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitIndices,
                                            indexSpecs,
                                            HdBufferArrayUsageHint());

        //
        // Data Buffer
        //        
        HdBufferSpecVector dataSpecs;
        dataSpecs.push_back(HdBufferSpec(
            HdxTokens->hdxOitDataBuffer, 
            HdTupleType {HdTypeFloatVec4, 1}));
        _dataBar = hdStResourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitData,
                                            dataSpecs,
                                            HdBufferArrayUsageHint());

        //
        // Depth Buffer
        //
        HdBufferSpecVector depthSpecs;
        depthSpecs.push_back(HdBufferSpec(
            HdxTokens->hdxOitDepthBuffer, 
            HdTupleType {HdTypeFloat, 1}));
        _depthBar = hdStResourceRegistry->AllocateSingleBufferArrayRange(
                                            /*role*/HdxTokens->oitDepth,
                                            depthSpecs,
                                            HdBufferArrayUsageHint());

        //
        // Uniforms
        //
        HdBufferSpecVector uniformSpecs;
        uniformSpecs.push_back( HdBufferSpec(
            HdxTokens->oitScreenSize,HdTupleType{HdTypeInt32Vec2, 1}));

        _uniformBar = hdStResourceRegistry->AllocateUniformBufferArrayRange(
                                            /*role*/HdxTokens->oitUniforms,
                                            uniformSpecs,
                                            HdBufferArrayUsageHint());
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
    bool resizeOitBuffers = (screenSize[0] > _screenSize[0] ||
                             screenSize[1] > _screenSize[1]);

    if (resizeOitBuffers) {
        _screenSize = screenSize;
        int newBufferSize = screenSize[0] * screenSize[1];

        // +1 because element 0 of the counter buffer is used as an atomic
        // counter in the shader to give each fragment a unique index.
        _counterBar->Resize(newBufferSize + 1);
        _indexBar->Resize(newBufferSize * numSamples);
        _dataBar->Resize(newBufferSize * numSamples);
        _depthBar->Resize(newBufferSize * numSamples);;

        // Update the values in the uniform buffer
        HdBufferSourceSharedPtrVector uniformSources;
        uniformSources.push_back(HdBufferSourceSharedPtr(
                              new HdVtBufferSource(HdxTokens->oitScreenSize,
                                                   VtValue(screenSize))));
        hdStResourceRegistry->AddSources(_uniformBar, uniformSources);
    }
}

void
HdxOitResolveTask::_PrepareAovBindings(HdTaskContext* ctx,
                                       HdRenderIndex* renderIndex)
{
    HdRenderPassAovBindingVector aovBindings;
    auto aovIt = ctx->find(HdxTokens->aovBindings);
    if (aovIt != ctx->end()) {
        const VtValue& vtAov = aovIt->second;
        if (vtAov.IsHolding<HdRenderPassAovBindingVector>()) {
            aovBindings = vtAov.UncheckedGet<HdRenderPassAovBindingVector>();
        }
    }

    // OIT should not clear the AOVs.
    for (size_t i = 0; i < aovBindings.size(); ++i) {
        aovBindings[i].clearValue = VtValue();
    }

    _renderPassState->SetAovBindings(aovBindings);
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

        _renderPass = boost::make_shared<HdSt_ImageShaderRenderPass>(
            renderIndex, collection);

        // We do not use renderDelegate->CreateRenderPassState because
        // ImageShaders always use HdSt
        _renderPassState = boost::make_shared<HdStRenderPassState>();
        _renderPassState->SetEnableDepthMask(false);
        _renderPassState->SetColorMask(HdRenderPassState::ColorMaskRGBA);
        _renderPassState->SetBlendEnabled(true);
        _renderPassState->SetBlend(
            HdBlendOp::HdBlendOpAdd,
            HdBlendFactor::HdBlendFactorOne,
            HdBlendFactor::HdBlendFactorOneMinusSrcAlpha,
            HdBlendOp::HdBlendOpAdd,
            HdBlendFactor::HdBlendFactorOne,
            HdBlendFactor::HdBlendFactorOne);

        _renderPassShader = boost::make_shared<HdStRenderPassShader>(
            HdxPackageOitResolveImageShader());
        _renderPassState->SetRenderPassShader(_renderPassShader);

        // We want OIT to resolve into the resolved aov, not the multi sample
        // aov. See HdxTaskController::GetRenderingTasks().
        _renderPassState->SetUseAovMultiSample(false);

        _renderPass->Prepare(GetRenderTags());
    }

    // XXX Fragile AOVs dependency. We expect RenderSetupTask::Prepare
    // to have resolved aob.renderBuffers and then push the AOV bindings onto
    // the SharedContext before we attempt to use those AOVs.
    _PrepareAovBindings(ctx, renderIndex);

    // If we have Aov buffers, resize Oit based on its dimensions.
    GfVec2i screenSize;
    const HdRenderPassAovBindingVector& aovBindings = 
        _renderPassState->GetAovBindings();

    if (!aovBindings.empty()) {
        unsigned int w = aovBindings.front().renderBuffer->GetWidth();
        unsigned int h = aovBindings.front().renderBuffer->GetHeight();
        screenSize = GfVec2i(w,h);
    } else {
        // Without AOVs we don't know the window / screen size.
        const int oitScreenSizeFallback = 2048;
        if (_screenSize[0] != oitScreenSizeFallback) {
            TF_WARN("Invalid AOVs for Oit Resolve Task");
        }
        screenSize[0] = oitScreenSizeFallback;
        screenSize[1] = oitScreenSizeFallback;
    }

    _PrepareOitBuffers(ctx, renderIndex, screenSize); 
}

void
HdxOitResolveTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Check whether the request flag was set and delete it so that for the
    // next iteration the request flag is not set unless an OIT render task
    // explicitly sets it.
    if (ctx->erase(HdxTokens->oitRequestFlag) == 0) {
        return;
    }

    if (!TF_VERIFY(_renderPassState)) return;
    if (!TF_VERIFY(_renderPassShader)) return;

    HdxOitBufferAccessor oitBufferAccessor(ctx);
    if (!oitBufferAccessor.AddOitBufferBindings(_renderPassShader)) {
        TF_CODING_ERROR(
            "No OIT buffers allocated but needed by OIT resolve task");
        return;
    }

    _renderPassState->Bind(); 

    glDisable(GL_DEPTH_TEST);

    _renderPass->Execute(_renderPassState, GetRenderTags());

    glEnable(GL_DEPTH_TEST);

    _renderPassState->Unbind();
}

PXR_NAMESPACE_CLOSE_SCOPE
