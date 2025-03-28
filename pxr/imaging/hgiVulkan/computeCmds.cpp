//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

HgiVulkanComputeCmds::HgiVulkanComputeCmds(
    HgiVulkan* hgi,
    HgiComputeCmdsDesc const&)
    : HgiComputeCmds()
    , _hgi(hgi)
    , _commandBuffer(nullptr)
    , _pipelineLayout(nullptr)
    , _pushConstantsDirty(false)
    , _pushConstants(nullptr)
    , _pushConstantsByteSize(0)
    , _localWorkGroupSize(GfVec3i(1, 1, 1))
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

    // Get and store local work group size from shader function desc
    const HgiShaderFunctionHandleVector shaderFunctionsHandles = 
        pipeline.Get()->GetDescriptor().shaderProgram.Get()->GetDescriptor().
            shaderFunctions;

    for (const auto &handle : shaderFunctionsHandles) {
        const HgiShaderFunctionDesc &shaderDesc = handle.Get()->GetDescriptor();
        if (shaderDesc.shaderStage == HgiShaderStageCompute) {
            if (shaderDesc.computeDescriptor.localSize[0] > 0 && 
                shaderDesc.computeDescriptor.localSize[1] > 0 &&
                shaderDesc.computeDescriptor.localSize[2] > 0) {
                _localWorkGroupSize = shaderDesc.computeDescriptor.localSize;
            }
        }
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
    if (!_pushConstants || _pushConstantsByteSize != byteSize) {
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

    const int threadsPerGroupX = _localWorkGroupSize[0];
    const int threadsPerGroupY = _localWorkGroupSize[1];
    int numWorkGroupsX = (dimX + (threadsPerGroupX - 1)) / threadsPerGroupX;
    int numWorkGroupsY = (dimY + (threadsPerGroupY - 1)) / threadsPerGroupY;

    // Determine device's num compute work group limits
    const VkPhysicalDeviceLimits limits = 
        _hgi->GetCapabilities()->vkDeviceProperties.limits;
    const GfVec3i maxNumWorkGroups = GfVec3i(
        limits.maxComputeWorkGroupCount[0],
        limits.maxComputeWorkGroupCount[1],
        limits.maxComputeWorkGroupCount[2]);

    if (numWorkGroupsX > maxNumWorkGroups[0]) {
        TF_WARN("Max number of work group available from device is %i, larger "
                "than %i", maxNumWorkGroups[0], numWorkGroupsX);
        numWorkGroupsX = maxNumWorkGroups[0];
    }
    if (numWorkGroupsY > maxNumWorkGroups[1]) {
        TF_WARN("Max number of work group available from device is %i, larger "
                "than %i", maxNumWorkGroups[1], numWorkGroupsY);
        numWorkGroupsY = maxNumWorkGroups[1];
    }

    vkCmdDispatch(
        _commandBuffer->GetVulkanCommandBuffer(),
        (uint32_t) numWorkGroupsX,
        (uint32_t) numWorkGroupsY,
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
HgiVulkanComputeCmds::InsertMemoryBarrier(HgiMemoryBarrier barrier)
{
    _CreateCommandBuffer();
    _commandBuffer->InsertMemoryBarrier(barrier);
}

HgiComputeDispatch
HgiVulkanComputeCmds::GetDispatchMethod() const
{
    return HgiComputeDispatchSerial;
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
