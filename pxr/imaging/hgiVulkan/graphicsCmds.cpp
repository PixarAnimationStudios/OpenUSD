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
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/commandBuffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/graphicsCmds.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/graphicsPipeline.h"
#include "pxr/imaging/hgiVulkan/resourceBindings.h"
#include "pxr/imaging/hgiVulkan/texture.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiVulkanGraphicsCmds::HgiVulkanGraphicsCmds(
    HgiVulkan* hgi,
    HgiGraphicsCmdsDesc const& desc)
    : _hgi(hgi)
    , _descriptor(desc)
    , _commandBuffer(nullptr)
    , _renderPassStarted(false)
    , _viewportSet(false)
    , _scissorSet(false)
{
    // We do not acquire the command buffer here, because the Cmds object may
    // have been created on the main thread, but used on a secondary thread.
    // We need to acquire a command buffer for the thread that is doing the
    // recording so we postpone acquiring cmd buffer until first use of Cmds.
}

HgiVulkanGraphicsCmds::~HgiVulkanGraphicsCmds()
{
}

void
HgiVulkanGraphicsCmds::PushDebugGroup(const char* label)
{
    _CreateCommandBuffer();
    HgiVulkanBeginLabel(_hgi->GetPrimaryDevice(), _commandBuffer, label);
}

void
HgiVulkanGraphicsCmds::PopDebugGroup()
{
    _CreateCommandBuffer();
    HgiVulkanEndLabel(_hgi->GetPrimaryDevice(), _commandBuffer);
}

void
HgiVulkanGraphicsCmds::SetViewport(GfVec4i const& vp)
{
    _viewportSet = true;

    // Delay until the pipeline is set and the render pass has begun.
    _pendingUpdates.push_back(
        [this, vp] {
            float offsetX = (float) vp[0];
            float offsetY = (float) vp[1];
            float width = (float) vp[2];
            float height = (float) vp[3];

            // Flip viewport in Y-axis, because the vertex.y position is flipped
            // between opengl and vulkan. This also moves origin to bottom-left.
            // Requires VK_KHR_maintenance1 extension.

            // Alternatives are:
            // 1. Multiply projection by 'inverted Y and half Z' matrix:
            //    const GfMatrix4d clip(
            //        1.0,  0.0, 0.0, 0.0,
            //        0.0, -1.0, 0.0, 0.0,
            //        0.0,  0.0, 0.5, 0.0,
            //        0.0,  0.0, 0.5, 1.0);
            //    projection = clip * projection;
            //
            // 2. Adjust vertex position:
            //    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;

            VkViewport viewport;
            viewport.x = offsetX;
            viewport.y = height - offsetY;
            viewport.width = width;
            viewport.height = -height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            vkCmdSetViewport(
                _commandBuffer->GetVulkanCommandBuffer(),
                0,
                1,
                &viewport);
    });
}

void
HgiVulkanGraphicsCmds::SetScissor(GfVec4i const& sc)
{
    _scissorSet = true;

    // Delay until the pipeline is set and the render pass has begun.
    _pendingUpdates.push_back(
        [this, sc] {
            uint32_t w(sc[2]);
            uint32_t h(sc[3]);
            VkRect2D scissor = {{sc[0], sc[1]}, {w, h}};
            vkCmdSetScissor(
                _commandBuffer->GetVulkanCommandBuffer(),
                0,
                1,
                &scissor);
    });
}

void
HgiVulkanGraphicsCmds::BindPipeline(HgiGraphicsPipelineHandle pipeline)
{
    _CreateCommandBuffer();

    // End the previous render pass in case we are using the same
    // GfxCmds with multiple pipelines.
    _EndRenderPass();

    _pipeline = pipeline;
    HgiVulkanGraphicsPipeline* pso = 
        static_cast<HgiVulkanGraphicsPipeline*>(_pipeline.Get());

    if (TF_VERIFY(pso)) {
        pso->BindPipeline(_commandBuffer->GetVulkanCommandBuffer());
    }
}

void
HgiVulkanGraphicsCmds::BindResources(HgiResourceBindingsHandle res)
{
    // Delay until the pipeline is set and the render pass has begun.
    _pendingUpdates.push_back(
        [this, res] {
            HgiVulkanGraphicsPipeline* pso = 
                static_cast<HgiVulkanGraphicsPipeline*>(_pipeline.Get());

            HgiVulkanResourceBindings * rb =
                static_cast<HgiVulkanResourceBindings*>(res.Get());

            if (pso && rb){
                rb->BindResources(
                    _commandBuffer->GetVulkanCommandBuffer(),
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pso->GetVulkanPipelineLayout());
            }
        }
    );
}

void
HgiVulkanGraphicsCmds::SetConstantValues(
    HgiGraphicsPipelineHandle pipeline,
    HgiShaderStage stages,
    uint32_t bindIndex,
    uint32_t byteSize,
    const void* data)
{
    // The data provided could be local stack memory that goes out of scope
    // before we execute this pending fn. Make a copy to prevent that.
    uint8_t* dataCopy = new uint8_t[byteSize];
    memcpy(dataCopy, data, byteSize);

    // Delay until the pipeline is set and the render pass has begun.
    _pendingUpdates.push_back(
        [this, byteSize, dataCopy, stages] {
            HgiVulkanGraphicsPipeline* pso = 
                static_cast<HgiVulkanGraphicsPipeline*>(_pipeline.Get());

            if (pso) {
                vkCmdPushConstants(
                    _commandBuffer->GetVulkanCommandBuffer(),
                    pso->GetVulkanPipelineLayout(),
                    HgiVulkanConversions::GetShaderStages(stages),
                    0, // offset
                    byteSize,
                    dataCopy);
            }

        delete[] dataCopy;
    });
}

void
HgiVulkanGraphicsCmds::BindVertexBuffers(
    uint32_t firstBinding,
    HgiBufferHandleVector const& vertexBuffers,
    std::vector<uint32_t> const& byteOffsets)
{
    // Delay until the pipeline is set and the render pass has begun.
    _pendingUpdates.push_back(
        [this, firstBinding, vertexBuffers, byteOffsets] {
        std::vector<VkBuffer> buffers;
        std::vector<VkDeviceSize> bufferOffsets;

        for (HgiBufferHandle bufHandle : vertexBuffers) {
            HgiVulkanBuffer* buf=static_cast<HgiVulkanBuffer*>(bufHandle.Get());
            VkBuffer vkBuf = buf->GetVulkanBuffer();
            if (vkBuf) {
                buffers.push_back(vkBuf);
                bufferOffsets.push_back(0);
            }
        }

        vkCmdBindVertexBuffers(
            _commandBuffer->GetVulkanCommandBuffer(),
            0, // first bindings
            buffers.size(),
            buffers.data(),
            bufferOffsets.data());
    });
}

void
HgiVulkanGraphicsCmds::Draw(
    uint32_t vertexCount,
    uint32_t firstVertex,
    uint32_t instanceCount)
{
    // Make sure the render pass has begun and resource are bound
    _ApplyPendingUpdates();

    vkCmdDraw(
        _commandBuffer->GetVulkanCommandBuffer(),
        vertexCount,
        instanceCount,
        firstVertex,
        0); // firstInstance
}

void
HgiVulkanGraphicsCmds::DrawIndirect(
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferOffset,
    uint32_t drawCount,
    uint32_t stride)
{
    // Make sure the render pass has begun and resource are bound
    _ApplyPendingUpdates();

    HgiVulkanBuffer* drawBuf =
        static_cast<HgiVulkanBuffer*>(drawParameterBuffer.Get());

    vkCmdDrawIndirect(
        _commandBuffer->GetVulkanCommandBuffer(),
        drawBuf->GetVulkanBuffer(),
        drawBufferOffset,
        drawCount,
        stride);
}

void
HgiVulkanGraphicsCmds::DrawIndexed(
    HgiBufferHandle const& indexBuffer,
    uint32_t indexCount,
    uint32_t indexBufferByteOffset,
    uint32_t vertexOffset,
    uint32_t instanceCount)
{
    // Make sure the render pass has begun and resource are bound
    _ApplyPendingUpdates();

    TF_VERIFY(instanceCount>0);

    HgiVulkanBuffer* ibo = static_cast<HgiVulkanBuffer*>(indexBuffer.Get());
    HgiBufferDesc const& indexDesc = ibo->GetDescriptor();

    // We assume 32bit indices
    TF_VERIFY(indexDesc.usage & HgiBufferUsageIndex32);

    vkCmdBindIndexBuffer(
        _commandBuffer->GetVulkanCommandBuffer(),
        ibo->GetVulkanBuffer(),
        indexBufferByteOffset,
        VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(
        _commandBuffer->GetVulkanCommandBuffer(),
        indexCount,
        instanceCount,
        0,  // firstIndex,
        vertexOffset,
        0); // firstInstance
}

void
HgiVulkanGraphicsCmds::DrawIndexedIndirect(
    HgiBufferHandle const& indexBuffer,
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferOffset,
    uint32_t drawCount,
    uint32_t stride)
{
    // Make sure the render pass has begun and resource are bound
    _ApplyPendingUpdates();

    HgiVulkanBuffer* ibo = static_cast<HgiVulkanBuffer*>(indexBuffer.Get());
    HgiBufferDesc const& indexDesc = ibo->GetDescriptor();

    // We assume 32bit indices
    TF_VERIFY(indexDesc.usage & HgiBufferUsageIndex32);

    vkCmdBindIndexBuffer(
        _commandBuffer->GetVulkanCommandBuffer(),
        ibo->GetVulkanBuffer(),
        0, // indexBufferByteOffset
        VK_INDEX_TYPE_UINT32);

    HgiVulkanBuffer* drawBuf =
        static_cast<HgiVulkanBuffer*>(drawParameterBuffer.Get());

    vkCmdDrawIndexedIndirect(
        _commandBuffer->GetVulkanCommandBuffer(),
        drawBuf->GetVulkanBuffer(),
        drawBufferOffset,
        drawCount,
        stride);

}

void
HgiVulkanGraphicsCmds::MemoryBarrier(HgiMemoryBarrier barrier)
{
    _CreateCommandBuffer();
    _commandBuffer->MemoryBarrier(barrier);
}

HgiVulkanCommandBuffer*
HgiVulkanGraphicsCmds::GetCommandBuffer()
{
    return _commandBuffer;
}

bool
HgiVulkanGraphicsCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    if (!_commandBuffer) {
        return false;
    }

    // End render pass
    _EndRenderPass();

    HgiVulkanDevice* device = _commandBuffer->GetDevice();
    HgiVulkanCommandQueue* queue = device->GetCommandQueue();

    // Submit the GPU work and optionally do CPU - GPU synchronization.
    queue->SubmitToQueue(_commandBuffer, wait);

    return true;
}

void
HgiVulkanGraphicsCmds::_ApplyPendingUpdates()
{
    if (!_pipeline) {
        TF_CODING_ERROR("No pipeline bound");
        return;
    }

    // Ensure the cmd buf is created on the thread that does the recording.
    _CreateCommandBuffer();

    // Begin render pass
    if (!_renderPassStarted && !_pendingUpdates.empty()) {
        _renderPassStarted = true;

        HgiVulkanGraphicsPipeline* pso = 
            static_cast<HgiVulkanGraphicsPipeline*>(_pipeline.Get());

        GfVec2i size(0);
        VkClearValueVector const& clearValues = pso->GetClearValues();

        VkRenderPassBeginInfo beginInfo =
            {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        beginInfo.renderPass = pso->GetVulkanRenderPass();
        beginInfo.framebuffer= pso->AcquireVulkanFramebuffer(_descriptor,&size);
        beginInfo.renderArea.extent.width = size[0];
        beginInfo.renderArea.extent.height = size[1];
        beginInfo.clearValueCount = (uint32_t) clearValues.size();
        beginInfo.pClearValues = clearValues.data();

        VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE;

        vkCmdBeginRenderPass(
            _commandBuffer->GetVulkanCommandBuffer(),
            &beginInfo,
            contents);

        // Make sure viewport and scissor are set since our HgiVulkanPipeline
        // hardcodes one dynamic viewport and scissor.
        if (!_viewportSet) {
            SetViewport(GfVec4i(0, 0, size[0], size[1]));
        }
        if (!_scissorSet) {
            SetScissor(GfVec4i(0, 0, size[0], size[1]));
        }
    }

    // Now that the render pass has begun we can execute any cmds that
    // require a render pass to be active.
    for (HgiVulkanGfxFunction const& fn : _pendingUpdates) {
        fn();
    }
    _pendingUpdates.clear();
}

void
HgiVulkanGraphicsCmds::_EndRenderPass()
{
    if (_renderPassStarted) {
        vkCmdEndRenderPass(_commandBuffer->GetVulkanCommandBuffer());
        _renderPassStarted = false;
        _viewportSet = false;
        _scissorSet = false;
    }
}

void
HgiVulkanGraphicsCmds::_CreateCommandBuffer()
{
    if (!_commandBuffer) {
        HgiVulkanDevice* device = _hgi->GetPrimaryDevice();
        HgiVulkanCommandQueue* queue = device->GetCommandQueue();
        _commandBuffer = queue->AcquireCommandBuffer();
        TF_VERIFY(_commandBuffer);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
