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
#include "pxr/imaging/hgiVulkan/computeCmds.h"
#include "pxr/imaging/hgiVulkan/commandBuffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/computePipeline.h"
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/resourceBindings.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiVulkanComputeCmds::HgiVulkanComputeCmds(HgiVulkan* hgi)
    : HgiComputeCmds()
    , _hgi(hgi)
    , _commandBuffer(nullptr)
    , _pipelineLayout(nullptr)
    , _pushConstantsDirty(false)
    , _pushConstants(nullptr)
    , _pushConstantsByteSize(0)
{
}

HgiVulkanComputeCmds::~HgiVulkanComputeCmds()
{
    delete[] _pushConstants;
}

void
HgiVulkanComputeCmds::PushDebugGroup(const char* label)
{
    _CreateCommandBuffer();
    HgiVulkanBeginLabel(_hgi->GetPrimaryDevice(), _commandBuffer, label);
}

void
HgiVulkanComputeCmds::PopDebugGroup()
{
    _CreateCommandBuffer();
    HgiVulkanEndLabel(_hgi->GetPrimaryDevice(), _commandBuffer);
}

void
HgiVulkanComputeCmds::BindPipeline(HgiComputePipelineHandle pipeline)
{
    _CreateCommandBuffer();

    HgiVulkanComputePipeline* pso = 
        static_cast<HgiVulkanComputePipeline*>(pipeline.Get());

    if (TF_VERIFY(pso)) {
        _pipelineLayout = pso->GetVulkanPipelineLayout();
        pso->BindPipeline(_commandBuffer->GetVulkanCommandBuffer());
    }
}

void
HgiVulkanComputeCmds::BindResources(HgiResourceBindingsHandle res)
{
    _CreateCommandBuffer();
    // Delay bindings until we know for sure what the pipeline will be.
    _resourceBindings = res;
}

void
HgiVulkanComputeCmds::SetConstantValues(
    HgiComputePipelineHandle pipeline,
    uint32_t bindIndex,
    uint32_t byteSize,
    const void* data)
{
    _CreateCommandBuffer();
    // Delay pushing until we know for sure what the pipeline will be.
    if (!_pushConstants || _pushConstantsByteSize < byteSize) {
        delete[] _pushConstants;
        _pushConstants = new uint8_t[byteSize];
        _pushConstantsByteSize = byteSize;
    }
    memcpy(_pushConstants, data, byteSize);
    _pushConstantsDirty = true;
}

void
HgiVulkanComputeCmds::Dispatch(int dimX, int dimY)
{
    _CreateCommandBuffer();
    _BindResources();

    vkCmdDispatch(
        _commandBuffer->GetVulkanCommandBuffer(),
        (uint32_t) dimX,
        (uint32_t) dimY,
        1);
}

bool
HgiVulkanComputeCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    if (!_commandBuffer) {
        return false;
    }

    HgiVulkanDevice* device = _commandBuffer->GetDevice();
    HgiVulkanCommandQueue* queue = device->GetCommandQueue();

    // Submit the GPU work and optionally do CPU - GPU synchronization.
    queue->SubmitToQueue(_commandBuffer, wait);

    return true;
}

void
HgiVulkanComputeCmds::_BindResources()
{
    if (!_pipelineLayout) {
        return;
    }

    if (_resourceBindings) {
        HgiVulkanResourceBindings * rb =
            static_cast<HgiVulkanResourceBindings*>(_resourceBindings.Get());

        if (rb){
            rb->BindResources(
                _commandBuffer->GetVulkanCommandBuffer(),
                VK_PIPELINE_BIND_POINT_COMPUTE,
                _pipelineLayout);
        }

        // Make sure we bind only once
        _resourceBindings = HgiResourceBindingsHandle();
    }

    if (_pushConstantsDirty && _pushConstants && _pushConstantsByteSize > 0) {
        vkCmdPushConstants(
            _commandBuffer->GetVulkanCommandBuffer(),
            _pipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0, // offset
            _pushConstantsByteSize,
            _pushConstants);

        // Make sure we copy only once
        _pushConstantsDirty = false;
    }
}

void
HgiVulkanComputeCmds::MemoryBarrier(HgiMemoryBarrier barrier)
{
    _CreateCommandBuffer();
    _commandBuffer->MemoryBarrier(barrier);
}

void
HgiVulkanComputeCmds::_CreateCommandBuffer()
{
    if (!_commandBuffer) {
        HgiVulkanDevice* device = _hgi->GetPrimaryDevice();
        HgiVulkanCommandQueue* queue = device->GetCommandQueue();
        _commandBuffer = queue->AcquireCommandBuffer();
        TF_VERIFY(_commandBuffer);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
