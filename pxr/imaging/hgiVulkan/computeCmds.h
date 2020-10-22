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
#ifndef PXR_IMAGING_HGIVULKAN_COMPUTE_CMDS_H
#define PXR_IMAGING_HGIVULKAN_COMPUTE_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/computeCmds.h"
#include "pxr/imaging/hgi/computePipeline.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

PXR_NAMESPACE_OPEN_SCOPE

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
    void MemoryBarrier(HgiMemoryBarrier barrier) override;

protected:
    friend class HgiVulkan;

    HGIVULKAN_API
    HgiVulkanComputeCmds(HgiVulkan* hgi);

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

    // Cmds is used only one frame so storing multi-frame state on will not
    // survive.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
