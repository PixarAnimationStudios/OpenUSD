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
#ifndef PXR_IMAGING_HGIVULKAN_GRAPHICS_CMDS_H
#define PXR_IMAGING_HGIVULKAN_GRAPHICS_CMDS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"
#include "pxr/imaging/hgi/graphicsCmds.h"
#include <cstdint>
#include <functional>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiGraphicsCmdsDesc;
class HgiVulkan;
class HgiVulkanCommandBuffer;

using HgiVulkanGfxFunction = std::function<void(void)>;
using HgiVulkanGfxFunctionVector = std::vector<HgiVulkanGfxFunction>;


/// \class HgiVulkanGraphicsCmds
///
/// Vulkan implementation of HgiGraphicsEncoder.
///
class HgiVulkanGraphicsCmds final : public HgiGraphicsCmds
{
public:
    HGIVULKAN_API
    ~HgiVulkanGraphicsCmds() override;

    HGIVULKAN_API
    void PushDebugGroup(const char* label) override;

    HGIVULKAN_API
    void PopDebugGroup() override;

    HGIVULKAN_API
    void SetViewport(GfVec4i const& vp) override;

    HGIVULKAN_API
    void SetScissor(GfVec4i const& sc) override;

    HGIVULKAN_API
    void BindPipeline(HgiGraphicsPipelineHandle pipeline) override;

    HGIVULKAN_API
    void BindResources(HgiResourceBindingsHandle resources) override;

    HGIVULKAN_API
    void SetConstantValues(
        HgiGraphicsPipelineHandle pipeline,
        HgiShaderStage stages,
        uint32_t bindIndex,
        uint32_t byteSize,
        const void* data) override;

    HGIVULKAN_API
    void BindVertexBuffers(
        uint32_t firstBinding,
        HgiBufferHandleVector const& buffers,
        std::vector<uint32_t> const& byteOffsets) override;

    HGIVULKAN_API
    void Draw(
        uint32_t vertexCount,
        uint32_t firstVertex,
        uint32_t instanceCount) override;

    HGIVULKAN_API
    void DrawIndirect(
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferOffset,
        uint32_t drawCount,
        uint32_t stride) override;

    HGIVULKAN_API
    void DrawIndexed(
        HgiBufferHandle const& indexBuffer,
        uint32_t indexCount,
        uint32_t indexBufferByteOffset,
        uint32_t vertexOffset,
        uint32_t instanceCount) override;

    HGIVULKAN_API
    void DrawIndexedIndirect(
        HgiBufferHandle const& indexBuffer,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferOffset,
        uint32_t drawCount,
        uint32_t stride) override;

    HGIVULKAN_API
    void MemoryBarrier(HgiMemoryBarrier barrier) override;

    /// Returns the command buffer used inside this cmds.
    HGIVULKAN_API
    HgiVulkanCommandBuffer* GetCommandBuffer();

protected:
    friend class HgiVulkan;

    HGIVULKAN_API
    HgiVulkanGraphicsCmds(HgiVulkan* hgi, HgiGraphicsCmdsDesc const& desc);

    HGIVULKAN_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiVulkanGraphicsCmds() = delete;
    HgiVulkanGraphicsCmds & operator=(const HgiVulkanGraphicsCmds&) = delete;
    HgiVulkanGraphicsCmds(const HgiVulkanGraphicsCmds&) = delete;

    void _ApplyPendingUpdates();
    void _EndRenderPass();
    void _CreateCommandBuffer();

    HgiVulkan* _hgi;
    HgiGraphicsCmdsDesc _descriptor;
    HgiVulkanCommandBuffer* _commandBuffer;
    HgiGraphicsPipelineHandle _pipeline;
    bool _renderPassStarted;
    bool _viewportSet;
    bool _scissorSet;
    HgiVulkanGfxFunctionVector _pendingUpdates;

    // GraphicsCmds is used only one frame so storing multi-frame state on
    // GraphicsCmds will not survive.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
