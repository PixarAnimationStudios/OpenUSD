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

#include "pxr/imaging/hdSt/glUtils.h"
#include "pxr/imaging/hdSt/imageShaderRenderPass.h"
#include "pxr/imaging/hdSt/imageShaderShaderKey.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/surfaceShader.h"
#include "pxr/imaging/hdSt/glslfxShader.h"
#include "pxr/imaging/hdSt/immediateDrawBatch.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hgi/graphicsCmds.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"


// XXX We do not want to include specific HgiXX backends, but we need to do
// this temporarily until Storm has transitioned fully to Hgi.
#include "pxr/imaging/hgiGL/graphicsCmds.h"

PXR_NAMESPACE_OPEN_SCOPE

void
_ExecuteDraw(
    HdSt_DrawBatchSharedPtr const& drawBatch,
    HdStRenderPassStateSharedPtr const& stRenderPassState,
    HdStResourceRegistrySharedPtr const& resourceRegistry)
{
    drawBatch->ExecuteDraw(stRenderPassState, resourceRegistry);
}

HdSt_ImageShaderRenderPass::HdSt_ImageShaderRenderPass(
    HdRenderIndex *index,
    HdRprimCollection const &collection)
    : HdRenderPass(index, collection)
    , _sharedData(1)
    , _drawItem(&_sharedData)
    , _drawItemInstance(&_drawItem)
    , _hgi(nullptr)
{
    _sharedData.instancerLevels = 0;
    _sharedData.rprimID = SdfPath("/imageShaderRenderPass");
    _immediateBatch = HdSt_DrawBatchSharedPtr(
        new HdSt_ImmediateDrawBatch(&_drawItemInstance));

    HdStRenderDelegate* renderDelegate =
        static_cast<HdStRenderDelegate*>(index->GetRenderDelegate());
    _hgi = renderDelegate->GetHgi();
}

HdSt_ImageShaderRenderPass::~HdSt_ImageShaderRenderPass()
{
}

void
HdSt_ImageShaderRenderPass::_SetupVertexPrimvarBAR(
    HdStResourceRegistrySharedPtr const& registry)
{
    // The current logic in HdSt_ImmediateDrawBatch::ExecuteDraw will use
    // glDrawArraysInstanced if it finds a VertexPrimvar buffer but no
    // index buffer, We setup the BAR to meet this requirement to draw our
    // full-screen triangle for post-process shaders.

    HdBufferSourceSharedPtrVector sources = {
        std::make_shared<HdVtBufferSource>(
            HdTokens->points, VtValue(VtVec3fArray(3))) };

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

    HdBufferArrayRangeSharedPtr vertexPrimvarRange =
        registry->AllocateNonUniformBufferArrayRange(
            HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHint());

    registry->AddSources(vertexPrimvarRange, std::move(sources));

    HdDrawingCoord* drawingCoord = _drawItem.GetDrawingCoord();
    _sharedData.barContainer.Set(
        drawingCoord->GetVertexPrimvarIndex(),
        vertexPrimvarRange);
}

void
HdSt_ImageShaderRenderPass::_Prepare(TfTokenVector const &renderTags)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::dynamic_pointer_cast<HdStResourceRegistry>(
        GetRenderIndex()->GetResourceRegistry());
    TF_VERIFY(resourceRegistry);

    // First time we must create a VertexPrimvar BAR for the triangle and setup
    // the geometric shader that provides the vertex and fragment shaders.
    if (!_sharedData.barContainer.Get(
            _drawItem.GetDrawingCoord()->GetVertexPrimvarIndex()))
    {
        _SetupVertexPrimvarBAR(resourceRegistry);

        HdSt_ImageShaderShaderKey shaderKey;
        HdSt_GeometricShaderSharedPtr geometricShader =
            HdSt_GeometricShader::Create(shaderKey, resourceRegistry);

        _drawItem.SetGeometricShader(geometricShader);
    }
}

void
HdSt_ImageShaderRenderPass::_Execute(
    HdRenderPassStateSharedPtr const &renderPassState,
    TfTokenVector const& renderTags)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Downcast render pass state
    HdStRenderPassStateSharedPtr stRenderPassState =
        std::dynamic_pointer_cast<HdStRenderPassState>(
        renderPassState);
    if (!TF_VERIFY(stRenderPassState)) return;

    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::dynamic_pointer_cast<HdStResourceRegistry>(
        GetRenderIndex()->GetResourceRegistry());
    TF_VERIFY(resourceRegistry);

    _immediateBatch->PrepareDraw(stRenderPassState, resourceRegistry);

    // Create graphics work to render into aovs.
    const HgiGraphicsCmdsDesc desc =
        stRenderPassState->MakeGraphicsCmdsDesc(GetRenderIndex());
    HgiGraphicsCmdsUniquePtr gfxCmds = _hgi->CreateGraphicsCmds(desc);

    // XXX When there are no aovBindings we get a null work object.
    // This would ideally never happen, but currently happens for some
    // custom prims that spawn an imagingGLengine  with a task controller that
    // has no aovBindings.

    if (gfxCmds) {
        gfxCmds->PushDebugGroup(__ARCH_PRETTY_FUNCTION__);
    }

    // XXX: The Bind/Unbind calls below set/restore GL state.
    // This will be reworked to use Hgi.
    stRenderPassState->Bind();

    // Draw
    HdSt_DrawBatchSharedPtr const& batch = _immediateBatch;
    HgiGLGraphicsCmds* glGfxCmds = 
        dynamic_cast<HgiGLGraphicsCmds*>(gfxCmds.get());

    if (gfxCmds && glGfxCmds) {
        // XXX Tmp code path to allow non-hgi code to insert functions into
        // HgiGL ops-stack. Will be removed once Storms uses Hgi everywhere
        auto executeDrawOp = [batch, stRenderPassState, resourceRegistry] {
            _ExecuteDraw(batch, stRenderPassState, resourceRegistry);
        };
        glGfxCmds->InsertFunctionOp(executeDrawOp);
    } else {
        _ExecuteDraw(batch, stRenderPassState, resourceRegistry);
    }

    if (gfxCmds) {
        gfxCmds->PopDebugGroup();
        _hgi->SubmitCmds(gfxCmds.get());
    }

    stRenderPassState->Unbind();
}

void
HdSt_ImageShaderRenderPass::_MarkCollectionDirty()
{
}


PXR_NAMESPACE_CLOSE_SCOPE
