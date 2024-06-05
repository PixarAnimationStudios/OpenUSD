//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_COMPUTE_CMDS_H
#define PXR_IMAGING_HGIVULKAN_COMPUTE_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/computeCmds.h"
#include "pxr/imaging/hgi/computePipeline.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HgiComputeCmdsDesc;
class HgiVulkan;
class HgiVulkanCommandBuffer;


/// \class HgiVulkanComputeCmds
///
/// OpenGL implementation of HgiComputeCmds.
///
class HgiVulkanComputeCmds final : public HgiComputeCmds
{
public:
    HGIVULKAN_API
    ~HgiVulkanComputeCmds() override;

    HGIVULKAN_API
    void PushDebugGroup(const char* label) override;

    HGIVULKAN_API
    void PopDebugGroup() override;

    HGIVULKAN_API
    void BindPipeline(HgiComputePipelineHandle pipeline) override;

    HGIVULKAN_API
    void BindResources(HgiResourceBindingsHandle resources) override;

    HGIVULKAN_API
    void SetConstantValues(
        HgiComputePipelineHandle pipeline,
        uint32_t bindIndex,
        uint32_t byteSize,
        const void* data) override;
    
    HGIVULKAN_API
    void Dispatch(int dimX, int dimY) override;

    HGIVULKAN_API
    void InsertMemoryBarrier(HgiMemoryBarrier barrier) override;

    HGIVULKAN_API
    HgiComputeDispatch GetDispatchMethod() const override;

protected:
    friend class HgiVulkan;

    HGIVULKAN_API
    HgiVulkanComputeCmds(HgiVulkan* hgi, HgiComputeCmdsDesc const& desc);

    HGIVULKAN_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiVulkanComputeCmds() = delete;
    HgiVulkanComputeCmds & operator=(const HgiVulkanComputeCmds&) = delete;
    HgiVulkanComputeCmds(const HgiVulkanComputeCmds&) = delete;

    void _BindResources();
    void _CreateCommandBuffer();

    HgiVulkan* _hgi;
    HgiVulkanCommandBuffer* _commandBuffer;
    VkPipelineLayout _pipelineLayout;
    HgiResourceBindingsHandle _resourceBindings;
    bool _pushConstantsDirty;
    uint8_t* _pushConstants;
    uint32_t _pushConstantsByteSize;
    GfVec3i _localWorkGroupSize;

    // Cmds is used only one frame so storing multi-frame state on will not
    // survive.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
