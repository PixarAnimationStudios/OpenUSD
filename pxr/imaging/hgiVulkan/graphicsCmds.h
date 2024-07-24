//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
using VkClearValueVector = std::vector<VkClearValue>;


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
        HgiVertexBufferBindingVector const &bindings) override;

    HGIVULKAN_API
    void Draw(
        uint32_t vertexCount,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance) override;

    HGIVULKAN_API
    void DrawIndirect(
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride) override;

    HGIVULKAN_API
    void DrawIndexed(
        HgiBufferHandle const& indexBuffer,
        uint32_t indexCount,
        uint32_t indexBufferByteOffset,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance) override;

    HGIVULKAN_API
    void DrawIndexedIndirect(
        HgiBufferHandle const& indexBuffer,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride,
        std::vector<uint32_t> const& drawParameterBufferUInt32,
        uint32_t patchBaseVertexByteOffset) override;

    HGIVULKAN_API
    void InsertMemoryBarrier(HgiMemoryBarrier barrier) override;

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

    void _ClearAttachmentsIfNeeded();
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
    VkClearValueVector _vkClearValues;

    // GraphicsCmds is used only one frame so storing multi-frame state on
    // GraphicsCmds will not survive.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
