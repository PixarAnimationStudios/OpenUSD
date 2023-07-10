//
// Copyright 2022 Pixar
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
#include "pxr/imaging/hgiWebGPU/buffer.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/graphicsCmds.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/graphicsPipeline.h"
#include "pxr/imaging/hgiWebGPU/resourceBindings.h"
#include "pxr/imaging/hgiWebGPU/texture.h"
#include "pxr/imaging/hgiWebGPU/buffer.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPUGraphicsCmds::HgiWebGPUGraphicsCmds(
        HgiWebGPU* hgi,
        HgiGraphicsCmdsDesc const& desc)
        : _hgi(hgi)
        , _descriptor(desc)
        , _renderPassEncoder(nullptr)
        , _commandEncoder(nullptr)
        , _commandBuffer(nullptr)
        , _pipeline(nullptr)
        , _renderPassStarted(false)
        , _pushConstantsDirty(false)
        , _viewportSet(false)
        , _scissorSet(false)
        , _hasWork(false)
{
    _constantBindGroupEntry = {};
    _constantBindGroupEntry.size = 0;

    wgpu::RenderPassDescriptor renderPass;

    std::vector<wgpu::RenderPassColorAttachment> colorAttachments;
    for( size_t i=0; i<_descriptor.colorTextures.size(); ++i )
    {
        auto colorTarget = static_cast<HgiWebGPUTexture *>(_descriptor.colorTextures[i].Get());
        HgiAttachmentDesc colorAttachmentDesc = _descriptor.colorAttachmentDescs[i];

        wgpu::RenderPassColorAttachment colorDesc;
        colorDesc.view = colorTarget->GetTextureView();
        colorDesc.loadOp  = HgiWebGPUConversions::GetAttachmentLoadOp(colorAttachmentDesc.loadOp);
        colorDesc.storeOp = HgiWebGPUConversions::GetAttachmentStoreOp(colorAttachmentDesc.storeOp);
        colorDesc.clearValue.r = colorAttachmentDesc.clearValue[0];
        colorDesc.clearValue.g = colorAttachmentDesc.clearValue[1];
        colorDesc.clearValue.b = colorAttachmentDesc.clearValue[2];
        colorDesc.clearValue.a = colorAttachmentDesc.clearValue[3];
        if (_descriptor.colorResolveTextures.size() > i) {
            auto colorResolveTarget = static_cast<HgiWebGPUTexture *>(_descriptor.colorResolveTextures[i].Get());
            colorDesc.resolveTarget = colorResolveTarget->GetTextureView();
        }
        colorAttachments.push_back(colorDesc);
    }
    renderPass.colorAttachmentCount = colorAttachments.size();
    renderPass.colorAttachments = colorAttachments.data();

    auto depthTarget = static_cast<HgiWebGPUTexture *>(_descriptor.depthTexture.Get());
    _renderPassStarted = depthTarget || _descriptor.colorTextures.size() > 0;
    wgpu::RenderPassDepthStencilAttachment depthStencilDesc;
    if( depthTarget )
    {
        depthStencilDesc.depthLoadOp = HgiWebGPUConversions::GetAttachmentLoadOp(_descriptor.depthAttachmentDesc.loadOp);
        depthStencilDesc.depthStoreOp = HgiWebGPUConversions::GetAttachmentStoreOp(_descriptor.depthAttachmentDesc.storeOp);

        if (depthTarget->GetDescriptor().format == HgiFormatFloat32UInt8 ) {
            depthStencilDesc.stencilLoadOp = depthStencilDesc.depthLoadOp;
            depthStencilDesc.stencilStoreOp = depthStencilDesc.depthStoreOp;
        }

        if (_descriptor.depthResolveTexture) {
            // TODO: Feature not implemented in WebGPU
        }
        // depth is a single channel, using first component
        depthStencilDesc.depthClearValue = _descriptor.depthAttachmentDesc.clearValue[0];
        depthStencilDesc.view = depthTarget->GetTextureView();
        renderPass.depthStencilAttachment = &depthStencilDesc;
    }

    wgpu::Device device = _hgi->GetPrimaryDevice();
    _commandEncoder = device.CreateCommandEncoder();
    TF_VERIFY(_commandEncoder);

    if (_renderPassStarted) {
        _renderPassEncoder = _commandEncoder.BeginRenderPass(&renderPass);
    }

    if (_descriptor.colorTextures.size()>0) {
        auto size = _descriptor.colorTextures[0]->GetDescriptor().dimensions;
        if (!_viewportSet) {
            SetViewport(GfVec4i(0, 0, size[0], size[1]));
        }
        if (!_scissorSet) {
            SetScissor(GfVec4i(0, 0, size[0], size[1]));
        }
    }
}

HgiWebGPUGraphicsCmds::~HgiWebGPUGraphicsCmds()

{
    _commandBuffer = nullptr;
}

void
HgiWebGPUGraphicsCmds::PushDebugGroup(const char*)
{
}

void
HgiWebGPUGraphicsCmds::PopDebugGroup()
{
}

void
HgiWebGPUGraphicsCmds::SetViewport(GfVec4i const& vp)
{
    _viewportSet = true;

    float offsetX = (float) vp[0];
    float offsetY = (float) vp[1];
    float width = (float) vp[2];
    float height = (float) vp[3];

    _renderPassEncoder.SetViewport(offsetX, offsetY, width, height, 0.f, 1.f);
}

void
HgiWebGPUGraphicsCmds::SetScissor(GfVec4i const& sc)
{
    _scissorSet = true;

    uint32_t w(sc[2]);
    uint32_t h(sc[3]);

    _renderPassEncoder.SetScissorRect(sc[0], sc[1], w, h);
}

void
HgiWebGPUGraphicsCmds::BindPipeline(HgiGraphicsPipelineHandle pipeline)
{
    _stepFunctions.Init(pipeline->GetDescriptor());

    _pipeline = static_cast<HgiWebGPUGraphicsPipeline *>(pipeline.Get());

    _renderPassEncoder.SetPipeline(_pipeline->GetPipeline());
}

void
HgiWebGPUGraphicsCmds::BindResources(HgiResourceBindingsHandle res)
{
    // delay until the pipeline is set and the render pass has begun
    _pendingUpdates.push_back(
            [this, res] {
                HgiWebGPUResourceBindings * resourceBinding =
                        static_cast<HgiWebGPUResourceBindings*>(res.Get());
                wgpu::RenderPipeline pipelineHandle = _pipeline->GetPipeline();
                resourceBinding->BindResources(_hgi->GetPrimaryDevice(), _renderPassEncoder, _pipeline->GetBindGroupLayoutList(), _constantBindGroupEntry, _pushConstantsDirty);
                _pushConstantsDirty = false;
            }
    );
}

void
HgiWebGPUGraphicsCmds::SetConstantValues(
        HgiGraphicsPipelineHandle ,
        HgiShaderStage ,
        uint32_t bindIndex,
        uint32_t byteSize,
        const void* data)
{
    wgpu::Device device = _hgi->GetPrimaryDevice();
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.label = static_cast<std::string>("uniform").c_str();
    bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    bufferDesc.size = byteSize;
    wgpu::Buffer constantBuffer = device.CreateBuffer(&bufferDesc);
    wgpu::Queue queue = device.GetQueue();
    queue.WriteBuffer(constantBuffer, 0, data, byteSize);

    _constantBindGroupEntry.binding = bindIndex;
    _constantBindGroupEntry.buffer = constantBuffer;
    _constantBindGroupEntry.size = byteSize;;
    _pushConstantsDirty = true;
}

void
HgiWebGPUGraphicsCmds::BindVertexBuffers(
        HgiVertexBufferBindingVector const &bindings)
{
    _stepFunctions.Bind(bindings);
    std::vector<wgpu::Buffer> buffers;
    std::vector<uint32_t> bufferOffsets;
    for (HgiVertexBufferBinding const &binding : bindings) {
        HgiWebGPUBuffer* buf=static_cast<HgiWebGPUBuffer*>(binding.buffer.Get());
        wgpu::Buffer wgpuBuf = buf->GetBufferHandle();
        if (wgpuBuf) {
            buffers.push_back(wgpuBuf);
            bufferOffsets.push_back(binding.byteOffset);
        }
    }
    for( uint32_t i=0; i<buffers.size(); i++ ) {
        _renderPassEncoder.SetVertexBuffer(i, buffers[i], bufferOffsets[i], WGPU_WHOLE_SIZE);
    }
}

void
HgiWebGPUGraphicsCmds::Draw(
        uint32_t vertexCount,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance)
{
    _ApplyPendingUpdates();

    _renderPassEncoder.Draw(vertexCount, instanceCount, baseVertex, 0);
    _hasWork = true;
}

void
HgiWebGPUGraphicsCmds::DrawIndirect(
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferOffset,
        uint32_t drawCount,
        uint32_t stride)
{
    _ApplyPendingUpdates();

    HgiWebGPUBuffer* drawBuf =
            static_cast<HgiWebGPUBuffer*>(drawParameterBuffer.Get());

    _renderPassEncoder.DrawIndirect(drawBuf->GetBufferHandle(), drawBufferOffset);
}

void
HgiWebGPUGraphicsCmds::DrawIndexed(
        HgiBufferHandle const& indexBuffer,
        uint32_t indexCount,
        uint32_t indexBufferByteOffset,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance)
{
    TF_VERIFY(instanceCount>0);

    _ApplyPendingUpdates();
    _stepFunctions.SetVertexBufferOffsets(_renderPassEncoder, baseInstance);
    HgiWebGPUBuffer* ibo = static_cast<HgiWebGPUBuffer*>(indexBuffer.Get());
    uint32_t const baseIndex = indexBufferByteOffset / sizeof(uint32_t);
    _renderPassEncoder.SetIndexBuffer(ibo->GetBufferHandle(), wgpu::IndexFormat::Uint32, 0, ibo->GetByteSizeOfResource());
    _renderPassEncoder.DrawIndexed(indexCount, instanceCount, baseIndex, baseVertex, baseInstance);
    _hasWork = true;
}

void
HgiWebGPUGraphicsCmds::DrawIndexedIndirect(
        HgiBufferHandle const& indexBuffer,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride,
        std::vector<uint32_t> const& drawParameterBufferUInt32,
        uint32_t patchBaseVertexByteOffset)
{
    _ApplyPendingUpdates();

    HgiWebGPUBuffer* ibo = static_cast<HgiWebGPUBuffer*>(indexBuffer.Get());
    HgiWebGPUBuffer* drawBuf =
            static_cast<HgiWebGPUBuffer*>(drawParameterBuffer.Get());

    _renderPassEncoder.SetIndexBuffer(ibo->GetBufferHandle(), wgpu::IndexFormat::Uint32, 0, ibo->GetByteSizeOfResource() - drawBufferByteOffset);
    _renderPassEncoder.DrawIndexedIndirect(drawBuf->GetBufferHandle(), drawBufferByteOffset);
}

bool
HgiWebGPUGraphicsCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    // End render pass
    _EndRenderPass();

    HgiWebGPU *wgpuHgi = static_cast<HgiWebGPU *>(hgi);

    wgpuHgi->EnqueueCommandBuffer(_commandBuffer);
    wgpuHgi->QueueSubmit();

    _commandBuffer = nullptr;

    return _hasWork;
}

void
HgiWebGPUGraphicsCmds::_ApplyPendingUpdates()
{
    if (!_pipeline) {
        TF_CODING_ERROR("No pipeline bound");
        return;
    }

    // now that the render pass has begun we can execute any commands that require a render pass to be active
    for (auto const& fn : _pendingUpdates)
        fn();

    _pendingUpdates.clear();
}

void
HgiWebGPUGraphicsCmds::_EndRenderPass()
{
    if (_renderPassStarted) {
        // release any resources
        _renderPassEncoder.End();
        _renderPassEncoder = nullptr;

        _commandBuffer = _commandEncoder.Finish();
        _commandEncoder = nullptr;

        _viewportSet = false;
        _scissorSet = false;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
