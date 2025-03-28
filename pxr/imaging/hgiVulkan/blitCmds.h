//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_BLIT_CMDS_H
#define PXR_IMAGING_HGIVULKAN_BLIT_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgi/blitCmds.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkan;
class HgiVulkanCommandBuffer;


/// \class HgiVulkanBlitCmds
///
/// Vulkan implementation of HgiBlitCmds.
///
class HgiVulkanBlitCmds final : public HgiBlitCmds
{
public:
    HGIVULKAN_API
    ~HgiVulkanBlitCmds() override;

    HGIVULKAN_API
    void PushDebugGroup(const char* label) override;

    HGIVULKAN_API
    void PopDebugGroup() override;

    HGIVULKAN_API
    void CopyTextureGpuToCpu(HgiTextureGpuToCpuOp const& copyOp) override;

    HGIVULKAN_API
    void CopyTextureCpuToGpu(HgiTextureCpuToGpuOp const& copyOp) override;

    HGIVULKAN_API
    void CopyBufferGpuToGpu(HgiBufferGpuToGpuOp const& copyOp) override;

    HGIVULKAN_API
    void CopyBufferCpuToGpu(HgiBufferCpuToGpuOp const& copyOp) override;

    HGIVULKAN_API
    void CopyBufferGpuToCpu(HgiBufferGpuToCpuOp const& copyOp) override;

    HGIVULKAN_API
    void CopyTextureToBuffer(HgiTextureToBufferOp const& copyOp) override;
    
    HGIVULKAN_API
    void CopyBufferToTexture(HgiBufferToTextureOp const& copyOp) override;

    HGIVULKAN_API
    void GenerateMipMaps(HgiTextureHandle const& texture) override;

    HGIVULKAN_API
    void FillBuffer(HgiBufferHandle const& buffer, uint8_t value) override;

    HGIVULKAN_API
    void InsertMemoryBarrier(HgiMemoryBarrier barrier) override;
        
    /// Returns the command buffer used inside this cmds.
    HGIVULKAN_API
    HgiVulkanCommandBuffer* GetCommandBuffer();

protected:
    friend class HgiVulkan;

    HGIVULKAN_API
    HgiVulkanBlitCmds(HgiVulkan* hgi);

    HGIVULKAN_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiVulkanBlitCmds & operator=(const HgiVulkanBlitCmds&) = delete;
    HgiVulkanBlitCmds(const HgiVulkanBlitCmds&) = delete;

    void _CreateCommandBuffer();

    HgiVulkan* _hgi;
    HgiVulkanCommandBuffer* _commandBuffer;

    // BlitCmds is used only one frame so storing multi-frame state on BlitCmds
    // will not survive.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
