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

    // Process attachments to get clear values.
    for (HgiAttachmentDesc const& attachmentDesc :
        _descriptor.colorAttachmentDescs) {
        VkClearValue vkClearValue;
        vkClearValue.color.float32[0] = attachmentDesc.clearValue[0];
        vkClearValue.color.float32[1] = attachmentDesc.clearValue[1];
        vkClearValue.color.float32[2] = attachmentDesc.clearValue[2];
        vkClearValue.color.float32[3] = attachmentDesc.clearValue[3];
        _vkClearValues.push_back(vkClearValue);
    }

    bool const hasDepth =
        _descriptor.depthAttachmentDesc.format != HgiFormatInvalid;
    if (hasDepth) {
        HgiAttachmentDesc const& attachmentDesc =
            _descriptor.depthAttachmentDesc;
        VkClearValue vkClearValue;
        vkClearValue.depthStencil.depth = attachmentDesc.clearValue[0];
        vkClearValue.depthStencil.stencil =
            static_cast<uint32_t>(attachmentDesc.clearValue[1]);
        _vkClearValues.push_back(vkClearValue);
    }
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
            const float offsetX = (float) vp[0];
            const float offsetY = (float) vp[1];
            const float width = (float) vp[2];
            const float height = (float) vp[3];

            // Though we continue to use an OpenGL-style projection matrix in 
            // Storm, we choose not to flip the viewport here.
            // We instead WANT to render an upside down image, as this makes 
            // the handling of clip-space and downstream coordinate system 
            // differences betwen Vulkan and OpenGL easier.
            //
            // For example, since framebuffers in Vulkan are y-down (versus y-up
            // for OpenGL by default), sampling (0,0) from an AOV texture in 
            // the shader will grab from the top left of the texture in Vulkan
            // (versus bottom left in GL). But since we rendered the Vulkan 
            // image upside down, this ends up being the same texel value as it 
            // would've been for GL.
            // Vulkan-GL differences between the value of gl_FragCoord.y and 
            // the sign of screenspace derivatives w.r.t. to y are resolved 
            // similarly.
            // Rendering Vulkan upside down also means we can also flip AOVs
            // when writing them to file as we currently do for OpenGL and get 
            // the correct result for Vulkan, too.
            //
            // We do however flip the winding order for Vulkan, as otherwise
            // the rendered geometry would be both upside down AND facing 
            // the wrong way, as Vulkan clip-space is right-handed while
            // OpenGL's is left-handed. This happens in 
            // hgiVulkan/conversions.cpp and hgiVulkan/shaderGenerator.cpp.
            //
            VkViewport viewport;
            viewport.x = offsetX;
            viewport.y = offsetY;
            viewport.width = width;
            viewport.height = height;
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
    HgiVertexBufferBindingVector const &bindings)
{
    // Delay until the pipeline is set and the render pass has begun.
    _pendingUpdates.push_back(
        [this, bindings] {
        std::vector<VkBuffer> buffers;
        std::vector<VkDeviceSize> bufferOffsets;

        for (HgiVertexBufferBinding const &binding : bindings) {
            HgiVulkanBuffer* buf =
                static_cast<HgiVulkanBuffer*>(binding.buffer.Get());
            VkBuffer vkBuf = buf->GetVulkanBuffer();
            if (vkBuf) {
                buffers.push_back(vkBuf);
                bufferOffsets.push_back(binding.byteOffset);
            }
        }

        vkCmdBindVertexBuffers(
            _commandBuffer->GetVulkanCommandBuffer(),
            bindings[0].index, // first binding
            buffers.size(),
            buffers.data(),
            bufferOffsets.data());
    });
}

void
HgiVulkanGraphicsCmds::Draw(
    uint32_t vertexCount,
    uint32_t baseVertex,
    uint32_t instanceCount,
    uint32_t baseInstance)
{
    // Make sure the render pass has begun and resource are bound
    _ApplyPendingUpdates();

    vkCmdDraw(
        _commandBuffer->GetVulkanCommandBuffer(),
        vertexCount,
        instanceCount,
        baseVertex,
        baseInstance);
}

void
HgiVulkanGraphicsCmds::DrawIndirect(
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferByteOffset,
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
        drawBufferByteOffset,
        drawCount,
        stride);
}

void
HgiVulkanGraphicsCmds::DrawIndexed(
    HgiBufferHandle const& indexBuffer,
    uint32_t indexCount,
    uint32_t indexBufferByteOffset,
    uint32_t baseVertex,
    uint32_t instanceCount,
    uint32_t baseInstance)
{
    // Make sure the render pass has begun and resource are bound
    _ApplyPendingUpdates();

    HgiVulkanBuffer* ibo = static_cast<HgiVulkanBuffer*>(indexBuffer.Get());

    vkCmdBindIndexBuffer(
        _commandBuffer->GetVulkanCommandBuffer(),
        ibo->GetVulkanBuffer(),
        0,
        VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(
        _commandBuffer->GetVulkanCommandBuffer(),
        indexCount,
        instanceCount,
        static_cast<uint32_t>(indexBufferByteOffset / sizeof(uint32_t)),
        baseVertex,
        baseInstance);
}

void
HgiVulkanGraphicsCmds::DrawIndexedIndirect(
    HgiBufferHandle const& indexBuffer,
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferByteOffset,
    uint32_t drawCount,
    uint32_t stride,
    std::vector<uint32_t> const& /*drawParameterBufferUInt32*/,
    uint32_t /*patchBaseVertexByteOffset*/)
{
    // Make sure the render pass has begun and resource are bound
    _ApplyPendingUpdates();

    HgiVulkanBuffer* ibo = static_cast<HgiVulkanBuffer*>(indexBuffer.Get());

    vkCmdBindIndexBuffer(
        _commandBuffer->GetVulkanCommandBuffer(),
        ibo->GetVulkanBuffer(),
        0,
        VK_INDEX_TYPE_UINT32);

    HgiVulkanBuffer* drawBuf =
        static_cast<HgiVulkanBuffer*>(drawParameterBuffer.Get());

    vkCmdDrawIndexedIndirect(
        _commandBuffer->GetVulkanCommandBuffer(),
        drawBuf->GetVulkanBuffer(),
        drawBufferByteOffset,
        drawCount,
        stride);

}

void
HgiVulkanGraphicsCmds::InsertMemoryBarrier(HgiMemoryBarrier barrier)
{
    _CreateCommandBuffer();
    _commandBuffer->InsertMemoryBarrier(barrier);
}

HgiVulkanCommandBuffer*
HgiVulkanGraphicsCmds::GetCommandBuffer()
{
    return _commandBuffer;
}

void
HgiVulkanGraphicsCmds::_ClearAttachmentsIfNeeded()
{
    _CreateCommandBuffer();

    for (size_t i = 0; i < _descriptor.colorAttachmentDescs.size(); i++) {
        HgiAttachmentDesc const colorAttachmentDesc =
            _descriptor.colorAttachmentDescs[i];
        if (colorAttachmentDesc.loadOp == HgiAttachmentLoadOpClear) {
            VkClearColorValue vkClearColor; 
            vkClearColor.float32[0] = colorAttachmentDesc.clearValue[0];
            vkClearColor.float32[1] = colorAttachmentDesc.clearValue[1];
            vkClearColor.float32[2] = colorAttachmentDesc.clearValue[2];
            vkClearColor.float32[3] = colorAttachmentDesc.clearValue[3];
                
            if (_descriptor.colorTextures[i]) {
                HgiVulkanTexture* texture = static_cast<HgiVulkanTexture*>(
                    _descriptor.colorTextures[i].Get());
                VkImage vkImage = texture->GetImage();
                VkImageLayout oldVkLayout = texture->GetImageLayout();
                
                VkImageSubresourceRange vkImageSubRange;
                vkImageSubRange.aspectMask =
                    HgiVulkanConversions::GetImageAspectFlag(
                        texture->GetDescriptor().usage);
                vkImageSubRange.baseMipLevel = 0;
                vkImageSubRange.levelCount =
                    texture->GetDescriptor().mipLevels;
                vkImageSubRange.baseArrayLayer = 0;
                vkImageSubRange.layerCount =
                    texture->GetDescriptor().layerCount;
                
                HgiVulkanTexture::TransitionImageBarrier(
                    _commandBuffer,
                    texture,
                    /*oldLayout*/oldVkLayout,
                    /*newLayout*/VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    /*producerAccess*/0,
                    /*consumerAccess*/VK_ACCESS_TRANSFER_WRITE_BIT,
                    /*producerStage*/VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    /*consumerStage*/VK_PIPELINE_STAGE_TRANSFER_BIT);
                
                vkCmdClearColorImage(
                    _commandBuffer->GetVulkanCommandBuffer(),
                    vkImage,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    &vkClearColor,
                    1,
                    &vkImageSubRange);

                HgiVulkanTexture::TransitionImageBarrier(
                    _commandBuffer,
                    texture,
                    /*oldLayout*/VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    /*newLayout*/oldVkLayout,
                    /*producerAccess*/VK_ACCESS_TRANSFER_WRITE_BIT,
                    /*consumerAccess*/VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                    /*producerStage*/VK_PIPELINE_STAGE_TRANSFER_BIT,
                    /*consumerStage*/VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
            }

            if (_descriptor.colorResolveTextures.size() > i &&
                _descriptor.colorResolveTextures[i]) {
                HgiVulkanTexture* texture = static_cast<HgiVulkanTexture*>(
                    _descriptor.colorResolveTextures[i].Get());
                VkImage vkImage = texture->GetImage();
                VkImageLayout oldVkLayout = texture->GetImageLayout();
                    
                VkImageSubresourceRange vkImageSubRange;
                vkImageSubRange.aspectMask =
                    HgiVulkanConversions::GetImageAspectFlag(
                        texture->GetDescriptor().usage);
                vkImageSubRange.baseMipLevel = 0;
                vkImageSubRange.levelCount =
                    texture->GetDescriptor().mipLevels;
                vkImageSubRange.baseArrayLayer = 0;
                vkImageSubRange.layerCount =
                    texture->GetDescriptor().layerCount;
                    
                HgiVulkanTexture::TransitionImageBarrier(
                    _commandBuffer,
                    texture,
                    /*oldLayout*/oldVkLayout,
                    /*newLayout*/VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    /*producerAccess*/0,
                    /*consumerAccess*/VK_ACCESS_TRANSFER_WRITE_BIT,
                    /*producerStage*/VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    /*consumerStage*/VK_PIPELINE_STAGE_TRANSFER_BIT);
                
                vkCmdClearColorImage(
                    _commandBuffer->GetVulkanCommandBuffer(),
                    vkImage,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    &vkClearColor,
                    1,
                    &vkImageSubRange);
                
                HgiVulkanTexture::TransitionImageBarrier(
                    _commandBuffer,
                    texture,
                    /*oldLayout*/VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    /*newLayout*/oldVkLayout,
                    /*producerAccess*/VK_ACCESS_TRANSFER_WRITE_BIT,
                    /*consumerAccess*/VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                    /*producerStage*/VK_PIPELINE_STAGE_TRANSFER_BIT,
                    /*consumerStage*/VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
            }
        }
    }
        
    if (_descriptor.depthAttachmentDesc.loadOp == HgiAttachmentLoadOpClear) {
        VkClearDepthStencilValue vkClearDepthStencil; 
        vkClearDepthStencil.depth =
            _descriptor.depthAttachmentDesc.clearValue[0];
        vkClearDepthStencil.stencil = static_cast<uint32_t>(
            _descriptor.depthAttachmentDesc.clearValue[1]);
        
        if (_descriptor.depthTexture) {
            HgiVulkanTexture* texture = static_cast<HgiVulkanTexture*>(
                _descriptor.depthTexture.Get());
            VkImage vkImage = texture->GetImage();
            VkImageLayout oldVkLayout = texture->GetImageLayout();
            
            VkImageSubresourceRange vkImageSubRange;
            vkImageSubRange.aspectMask =
                HgiVulkanConversions::GetImageAspectFlag(
                    texture->GetDescriptor().usage);
            vkImageSubRange.baseMipLevel = 0;
            vkImageSubRange.levelCount =
                texture->GetDescriptor().mipLevels;
            vkImageSubRange.baseArrayLayer = 0;
            vkImageSubRange.layerCount =
                texture->GetDescriptor().layerCount;
                
            HgiVulkanTexture::TransitionImageBarrier(
                _commandBuffer,
                texture,
                /*oldLayout*/oldVkLayout,
                /*newLayout*/VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                /*producerAccess*/0,
                /*consumerAccess*/VK_ACCESS_TRANSFER_WRITE_BIT,
                /*producerStage*/VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                /*consumerStage*/VK_PIPELINE_STAGE_TRANSFER_BIT);
            
            vkCmdClearDepthStencilImage(
                _commandBuffer->GetVulkanCommandBuffer(),
                vkImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                &vkClearDepthStencil,
                1,
                &vkImageSubRange);
                
            HgiVulkanTexture::TransitionImageBarrier(
                _commandBuffer,
                texture,
                /*oldLayout*/VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                /*newLayout*/oldVkLayout,
                /*producerAccess*/VK_ACCESS_TRANSFER_WRITE_BIT,
                /*consumerAccess*/VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                /*producerStage*/VK_PIPELINE_STAGE_TRANSFER_BIT,
                /*consumerStage*/VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        }

        if (_descriptor.depthResolveTexture) {
            HgiVulkanTexture* texture = static_cast<HgiVulkanTexture*>(
                _descriptor.depthResolveTexture.Get());
            VkImage vkImage = texture->GetImage();
            VkImageLayout oldVkLayout = texture->GetImageLayout();
                
            VkImageSubresourceRange vkImageSubRange;
            vkImageSubRange.aspectMask =
                HgiVulkanConversions::GetImageAspectFlag(
                    texture->GetDescriptor().usage);
            vkImageSubRange.baseMipLevel = 0;
            vkImageSubRange.levelCount = texture->GetDescriptor().mipLevels;
            vkImageSubRange.baseArrayLayer = 0;
            vkImageSubRange.layerCount =
                texture->GetDescriptor().layerCount;
            
            HgiVulkanTexture::TransitionImageBarrier(
                _commandBuffer,
                texture,
                /*oldLayout*/oldVkLayout,
                /*newLayout*/VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                /*producerAccess*/0,
                /*consumerAccess*/VK_ACCESS_TRANSFER_WRITE_BIT,
                /*producerStage*/VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                /*consumerStage*/VK_PIPELINE_STAGE_TRANSFER_BIT);
            
            vkCmdClearDepthStencilImage(
                _commandBuffer->GetVulkanCommandBuffer(),
                vkImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                &vkClearDepthStencil,
                1,
                &vkImageSubRange);
            
            HgiVulkanTexture::TransitionImageBarrier(
                _commandBuffer,
                texture,
                /*oldLayout*/VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                /*newLayout*/oldVkLayout,
                /*producerAccess*/VK_ACCESS_TRANSFER_WRITE_BIT,
                /*consumerAccess*/VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                /*producerStage*/VK_PIPELINE_STAGE_TRANSFER_BIT,
                /*consumerStage*/VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        }
    }
}

bool
HgiVulkanGraphicsCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    // Any drawing should go inside a Vulkan render pass. However, there are
    // situations in which we create and submit graphics cmds but do not 
    // actually draw anything or bind a pipeline, meaning we don't begin a 
    // render pass. We may still want to clear the attachments in such a
    // situation, so we do that here.
    // We assume that if we are submiting the graphics cmds without having
    // started a render pass, we'll want to clear the attachments manually.
    if (!_renderPassStarted) {
        _ClearAttachmentsIfNeeded();
    }

    // End render pass
    _EndRenderPass();

    _viewportSet = false;
    _scissorSet = false;

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

        VkRenderPassBeginInfo beginInfo =
            {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        beginInfo.renderPass = pso->GetVulkanRenderPass();
        beginInfo.framebuffer= pso->AcquireVulkanFramebuffer(
            _descriptor, &size);
        beginInfo.renderArea.extent.width = size[0];
        beginInfo.renderArea.extent.height = size[1];

        // Only pass clear values to VkRenderPassBeginInfo if the pipeline has
        // attachments that specify a clear op.
        if (pso->GetClearNeeded()) {
            beginInfo.clearValueCount =
                static_cast<uint32_t>(_vkClearValues.size());
            beginInfo.pClearValues = _vkClearValues.data();
        }

        VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE;

        vkCmdBeginRenderPass(
            _commandBuffer->GetVulkanCommandBuffer(),
            &beginInfo,
            contents);

        // Make sure viewport and scissor are set since our
        // HgiVulkanGraphicsPipeline hardcodes one dynamic viewport and scissor.
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
