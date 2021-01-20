//
// Copyright 2020 Pixar
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
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgiMetal/buffer.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/graphicsCmds.h"
#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/graphicsPipeline.h"
#include "pxr/imaging/hgiMetal/resourceBindings.h"
#include "pxr/imaging/hgiMetal/texture.h"

#include "pxr/base/arch/defines.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalGraphicsCmds::HgiMetalGraphicsCmds(
    HgiMetal* hgi,
    HgiGraphicsCmdsDesc const& desc)
    : HgiGraphicsCmds()
    , _hgi(hgi)
    , _renderPassDescriptor(nil)
    , _encoder(nil)
    , _descriptor(desc)
    , _debugLabel(nil)
    , _viewportSet(false)
{
    TF_VERIFY(desc.colorTextures.size() == desc.colorAttachmentDescs.size());
    
    if (!desc.colorResolveTextures.empty() &&
            desc.colorResolveTextures.size() !=
                desc.colorTextures.size()) {
        TF_CODING_ERROR("color and resolve texture count mismatch.");
        return;
    }

    if (desc.depthResolveTexture && !desc.depthTexture) {
        TF_CODING_ERROR("DepthResolve texture without depth texture.");
        return;
    }

    _renderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];

    // Color attachments
    bool resolvingColor = !desc.colorResolveTextures.empty();
    bool hasClear = false;
    for (size_t i=0; i<desc.colorAttachmentDescs.size(); i++) {
        HgiAttachmentDesc const &hgiColorAttachment =
            desc.colorAttachmentDescs[i];
        MTLRenderPassColorAttachmentDescriptor *metalColorAttachment =
            _renderPassDescriptor.colorAttachments[i];

        if (hgiColorAttachment.loadOp == HgiAttachmentLoadOpClear) {
            hasClear = true;
        }
        
        if (@available(macos 100.100, ios 8.0, *)) {
            metalColorAttachment.loadAction = MTLLoadActionLoad;
        }
        else {
            metalColorAttachment.loadAction =
                HgiMetalConversions::GetAttachmentLoadOp(
                    hgiColorAttachment.loadOp);
        }

        metalColorAttachment.storeAction =
            HgiMetalConversions::GetAttachmentStoreOp(
                hgiColorAttachment.storeOp);
        if (hgiColorAttachment.loadOp == HgiAttachmentLoadOpClear) {
            GfVec4f const& clearColor = hgiColorAttachment.clearValue;
            metalColorAttachment.clearColor =
                MTLClearColorMake(
                    clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
        }
        
        HgiMetalTexture *colorTexture =
            static_cast<HgiMetalTexture*>(desc.colorTextures[i].Get());

        TF_VERIFY(
            colorTexture->GetDescriptor().format == hgiColorAttachment.format);
        metalColorAttachment.texture = colorTexture->GetTextureId();
        
        if (resolvingColor) {
            HgiMetalTexture *resolveTexture =
                static_cast<HgiMetalTexture*>(desc.colorResolveTextures[i].Get());

            metalColorAttachment.resolveTexture =
                resolveTexture->GetTextureId();

            if (hgiColorAttachment.storeOp == HgiAttachmentStoreOpStore) {
                metalColorAttachment.storeAction =
                    MTLStoreActionStoreAndMultisampleResolve;
            }
            else {
                metalColorAttachment.storeAction =
                    MTLStoreActionMultisampleResolve;
            }
        }
    }

    // Depth attachment
    if (desc.depthTexture) {
        HgiAttachmentDesc const &hgiDepthAttachment =
            desc.depthAttachmentDesc;
        MTLRenderPassDepthAttachmentDescriptor *metalDepthAttachment =
            _renderPassDescriptor.depthAttachment;

        if (hgiDepthAttachment.loadOp == HgiAttachmentLoadOpClear) {
            hasClear = true;
        }

        metalDepthAttachment.loadAction =
            HgiMetalConversions::GetAttachmentLoadOp(
                hgiDepthAttachment.loadOp);
        metalDepthAttachment.storeAction =
            HgiMetalConversions::GetAttachmentStoreOp(
                hgiDepthAttachment.storeOp);
        
        metalDepthAttachment.clearDepth = hgiDepthAttachment.clearValue[0];
        
        HgiMetalTexture *depthTexture =
            static_cast<HgiMetalTexture*>(desc.depthTexture.Get());
        
        TF_VERIFY(
            depthTexture->GetDescriptor().format == hgiDepthAttachment.format);
        metalDepthAttachment.texture = depthTexture->GetTextureId();
        
        if (desc.depthResolveTexture) {
            HgiMetalTexture *resolveTexture =
                static_cast<HgiMetalTexture*>(desc.depthResolveTexture.Get());

            metalDepthAttachment.resolveTexture =
                resolveTexture->GetTextureId();
            
            if (hgiDepthAttachment.storeOp == HgiAttachmentStoreOpStore) {
                metalDepthAttachment.storeAction =
                    MTLStoreActionStoreAndMultisampleResolve;
            }
            else {
                metalDepthAttachment.storeAction =
                    MTLStoreActionMultisampleResolve;
            }
        }
    }
    
    if (hasClear) {
        _CreateEncoder();
    }

}

HgiMetalGraphicsCmds::~HgiMetalGraphicsCmds()
{
    TF_VERIFY(_encoder == nil, "Encoder created, but never commited.");
    
    [_renderPassDescriptor release];
    if (_debugLabel) {
        [_debugLabel release];
    }
}

void
HgiMetalGraphicsCmds::_CreateEncoder()
{
    if (!_encoder) {
        _encoder = [
            _hgi->GetPrimaryCommandBuffer(false)
            renderCommandEncoderWithDescriptor:_renderPassDescriptor];
        
        if (_debugLabel) {
            [_encoder setLabel:_debugLabel];
        }
        if (_viewportSet) {
            [_encoder setViewport:_viewport];
        }
    }
}

void
HgiMetalGraphicsCmds::SetViewport(GfVec4i const& vp)
{
    double x = vp[0];
    double y = vp[1];
    double w = vp[2];
    double h = vp[3];
    if (_encoder) {
        [_encoder setViewport:(MTLViewport){x, y, w, h, 0.0, 1.0}];
    }
    else {
        _viewport = (MTLViewport){x, y, w, h, 0.0, 1.0};
    }
    _viewportSet = true;
}

void
HgiMetalGraphicsCmds::SetScissor(GfVec4i const& sc)
{
    uint32_t x = sc[0];
    uint32_t y = sc[1];
    uint32_t w = sc[2];
    uint32_t h = sc[3];
    
    _CreateEncoder();
    
    [_encoder setScissorRect:(MTLScissorRect){x, y, w, h}];
}

void
HgiMetalGraphicsCmds::BindPipeline(HgiGraphicsPipelineHandle pipeline)
{
    _CreateEncoder();

    _primitiveType = pipeline->GetDescriptor().primitiveType;
    if (HgiMetalGraphicsPipeline* p =
        static_cast<HgiMetalGraphicsPipeline*>(pipeline.Get())) {
        p->BindPipeline(_encoder);
    }
}

void
HgiMetalGraphicsCmds::BindResources(HgiResourceBindingsHandle r)
{
    _CreateEncoder();

    if (HgiMetalResourceBindings* rb=
        static_cast<HgiMetalResourceBindings*>(r.Get()))
    {
        rb->BindResources(_encoder);
    }
}

void
HgiMetalGraphicsCmds::SetConstantValues(
    HgiGraphicsPipelineHandle pipeline,
    HgiShaderStage stages,
    uint32_t bindIndex,
    uint32_t byteSize,
    const void* data)
{
    _CreateEncoder();

    if (stages & HgiShaderStageVertex) {
        [_encoder setVertexBytes:data
                          length:byteSize
                         atIndex:bindIndex];
    }
    if (stages & HgiShaderStageFragment) {
        [_encoder setFragmentBytes:data
                            length:byteSize
                           atIndex:bindIndex];
    }
}

void
HgiMetalGraphicsCmds::BindVertexBuffers(
    uint32_t firstBinding,
    HgiBufferHandleVector const& vertexBuffers,
    std::vector<uint32_t> const& byteOffsets)
{
    TF_VERIFY(byteOffsets.size() == vertexBuffers.size());
    TF_VERIFY(byteOffsets.size() == vertexBuffers.size());

    _CreateEncoder();

    for (size_t i=0; i<vertexBuffers.size(); i++) {
        HgiBufferHandle bufHandle = vertexBuffers[i];
        HgiMetalBuffer* buf = static_cast<HgiMetalBuffer*>(bufHandle.Get());
        HgiBufferDesc const& desc = buf->GetDescriptor();

        TF_VERIFY(desc.usage & HgiBufferUsageVertex);
        
        [_encoder setVertexBuffer:buf->GetBufferId()
                           offset:byteOffsets[i]
                          atIndex:firstBinding + i];
    }
}

void
HgiMetalGraphicsCmds::Draw(
    uint32_t vertexCount,
    uint32_t firstVertex,
    uint32_t instanceCount)
{
    TF_VERIFY(instanceCount>0);

    _CreateEncoder();
    
    MTLPrimitiveType type=HgiMetalConversions::GetPrimitiveType(_primitiveType);

    if (instanceCount == 1) {
        [_encoder drawPrimitives:type
                     vertexStart:firstVertex
                     vertexCount:vertexCount];
    } else {
        [_encoder drawPrimitives:type
                     vertexStart:firstVertex
                     vertexCount:vertexCount
                   instanceCount:instanceCount];
    }

    _hasWork = true;
}

void
HgiMetalGraphicsCmds::DrawIndirect(
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t bufferOffset,
    uint32_t drawCount,
    uint32_t stride)
{
    _CreateEncoder();
    
    HgiMetalBuffer* drawBuf =
        static_cast<HgiMetalBuffer*>(drawParameterBuffer.Get());

    MTLPrimitiveType type=HgiMetalConversions::GetPrimitiveType(_primitiveType);

    for (uint32_t i = 0; i < drawCount; i++) {
        [_encoder drawPrimitives:type
                  indirectBuffer:drawBuf->GetBufferId()
            indirectBufferOffset:bufferOffset + (i * stride)];
    }
}

void
HgiMetalGraphicsCmds::DrawIndexed(
    HgiBufferHandle const& indexBuffer,
    uint32_t indexCount,
    uint32_t indexBufferByteOffset,
    uint32_t vertexOffset,
    uint32_t instanceCount)
{
    TF_VERIFY(instanceCount>0);

    _CreateEncoder();

    HgiMetalBuffer* indexBuf = static_cast<HgiMetalBuffer*>(indexBuffer.Get());
    HgiBufferDesc const& indexDesc = indexBuf->GetDescriptor();

    // We assume 32bit indices: GL_UNSIGNED_INT
    TF_VERIFY(indexDesc.usage & HgiBufferUsageIndex32);

    MTLPrimitiveType type=HgiMetalConversions::GetPrimitiveType(_primitiveType);

    [_encoder drawIndexedPrimitives:type
                         indexCount:indexCount
                          indexType:MTLIndexTypeUInt32
                        indexBuffer:indexBuf->GetBufferId()
                  indexBufferOffset:indexBufferByteOffset
                      instanceCount:instanceCount
                         baseVertex:vertexOffset
                       baseInstance:0];

    _hasWork = true;
}

void
HgiMetalGraphicsCmds::DrawIndexedIndirect(
    HgiBufferHandle const& indexBuffer,
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferOffset,
    uint32_t drawCount,
    uint32_t stride)
{
    _CreateEncoder();
    
    HgiMetalBuffer* indexBuf = static_cast<HgiMetalBuffer*>(indexBuffer.Get());
    HgiBufferDesc const& indexDesc = indexBuf->GetDescriptor();

    // We assume 32bit indices: GL_UNSIGNED_INT
    TF_VERIFY(indexDesc.usage & HgiBufferUsageIndex32);

    HgiMetalBuffer* drawBuf =
        static_cast<HgiMetalBuffer*>(drawParameterBuffer.Get());

    MTLPrimitiveType type=HgiMetalConversions::GetPrimitiveType(_primitiveType);

    for (uint32_t i = 0; i < drawCount; i++) {
        [_encoder drawIndexedPrimitives:type
                              indexType:MTLIndexTypeUInt32
                            indexBuffer:indexBuf->GetBufferId()
                      indexBufferOffset:0
                         indirectBuffer:drawBuf->GetBufferId()
                   indirectBufferOffset:drawBufferOffset + (i * stride)];
    }
}

void
HgiMetalGraphicsCmds::PushDebugGroup(const char* label)
{
    if (_encoder) {
        HGIMETAL_DEBUG_LABEL(_encoder, label)
    }
    else if (HgiMetalDebugEnabled()) {
        _debugLabel = [@(label) copy];
    }
}

void
HgiMetalGraphicsCmds::PopDebugGroup()
{
    if (_debugLabel) {
        [_debugLabel release];
        _debugLabel = nil;
    }
}

void
HgiMetalGraphicsCmds::MemoryBarrier(HgiMemoryBarrier barrier)
{
    TF_VERIFY(barrier==HgiMemoryBarrierAll, "Unknown barrier");

    MTLBarrierScope scope =
        MTLBarrierScopeBuffers |
        MTLBarrierScopeTextures |
        MTLBarrierScopeRenderTargets;

    MTLRenderStages srcStages = MTLRenderStageVertex | MTLRenderStageFragment;
    MTLRenderStages dstStages = MTLRenderStageVertex | MTLRenderStageFragment;

    [_encoder memoryBarrierWithScope:scope
                         afterStages:srcStages
                         beforeStages:dstStages];
}

bool
HgiMetalGraphicsCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    if (_encoder) {
        [_encoder endEncoding];
        _encoder = nil;

        HgiMetal::CommitCommandBufferWaitType waitType;
        switch(wait) {
            case HgiSubmitWaitTypeNoWait:
                waitType = HgiMetal::CommitCommandBuffer_NoWait;
                break;
            case HgiSubmitWaitTypeWaitUntilCompleted:
                waitType = HgiMetal::CommitCommandBuffer_WaitUntilCompleted;
                break;
        }

        _hgi->CommitPrimaryCommandBuffer(waitType);
    }

    return _hasWork;
}

PXR_NAMESPACE_CLOSE_SCOPE
