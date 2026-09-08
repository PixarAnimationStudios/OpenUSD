//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiWebGPU/computeCmds.h"
#include "pxr/imaging/hgiWebGPU/computePipeline.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/diagnostic.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/resourceBindings.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPUComputeCmds::HgiWebGPUComputeCmds(HgiWebGPU* hgi, HgiComputeCmdsDesc const& desc)
    : _hgi(hgi)
    , _computePassEncoder(nullptr)
    , _commandEncoder(nullptr)
    , _commandBuffer(nullptr)
    , _pipeline(nullptr)
    , _computePassStarted(false)
    , _pushConstantsDirty(false)
    , _dispatchMethod(desc.dispatchMethod)
    , _localWorkGroupSize(GfVec3i(1, 1, 1))
{
    _constantBindGroupEntry = {};
    _constantBindGroupEntry.size = 0;
    _CreateCommandEncoder();

}

HgiWebGPUComputeCmds::~HgiWebGPUComputeCmds()
{
    _commandBuffer = nullptr;
}

void
HgiWebGPUComputeCmds::PushDebugGroup(const char* label, const GfVec4f& color)
{
    if (_computePassEncoder) {
        HgiWebGPUBeginLabel(_computePassEncoder, label);
    } else {
        HgiWebGPUBeginLabel(_commandEncoder, label);
    }
}

void
HgiWebGPUComputeCmds::PopDebugGroup()
{
    if (_computePassEncoder) {
        HgiWebGPUEndLabel(_computePassEncoder);
    } else {
        HgiWebGPUEndLabel(_commandEncoder);
    }
}

void
HgiWebGPUComputeCmds::InsertDebugMarker(const char* label, const GfVec4f& color)
{
}

void
HgiWebGPUComputeCmds::BindPipeline(HgiComputePipelineHandle pipeline)
{
    _pipeline = static_cast<HgiWebGPUComputePipeline *>(pipeline.Get());

    _computePassEncoder.SetPipeline(_pipeline->GetPipeline());

    const HgiComputePipelineDesc pipelineDesc = pipeline.Get()->GetDescriptor();
    const HgiShaderFunctionHandleVector shaderFunctionsHandles = pipelineDesc.shaderProgram.Get()->GetDescriptor().
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
HgiWebGPUComputeCmds::BindResources(HgiResourceBindingsHandle res)
{
    // delay until the pipeline is set, the compute pass has begun and constant buffer has been created
    _resourceBindings = res;
}

void
HgiWebGPUComputeCmds::SetConstantValues(
    HgiComputePipelineHandle pipeline,
    uint32_t bindIndex,
    uint32_t byteSize,
    const void* data)
{
    // XXX: There is still no dedicated functionality to handle this but, it is currently being discussed
    // https://github.com/gpuweb/gpuweb/wiki/gpu-web-f2f-2023-02-16-17#push-constants-75
    wgpu::Device device = _hgi->GetPrimaryDevice();
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.label = static_cast<std::string>("uniform").c_str();
    bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    bufferDesc.size = byteSize;
    wgpu::Buffer constantBuffer = device.CreateBuffer(&bufferDesc);
    wgpu::Queue queue = device.GetQueue();
    queue.WriteBuffer(constantBuffer, 0, data, byteSize);
    _constantBindGroupEntry = wgpu::BindGroupEntry {};
    _constantBindGroupEntry.binding = bindIndex;
    _constantBindGroupEntry.buffer = constantBuffer;
    _constantBindGroupEntry.size = byteSize;
    _pushConstantsDirty = true;
}

void
HgiWebGPUComputeCmds::Dispatch(int dimX, int dimY)
{
    if (dimX == 0 || dimY == 0) {
        return;
    }

    _BindResources();

    const int workgroupSizeX = _localWorkGroupSize[0];
    const int workgroupSizeY = _localWorkGroupSize[1];

    const uint32_t maxWorkgroups =
        _hgi->GetCapabilities()->GetLimits().maxComputeWorkgroupsPerDimension;
    uint32_t groupsX = (dimX + workgroupSizeX - 1) / workgroupSizeX;
    uint32_t groupsY = (dimY + workgroupSizeY - 1) / workgroupSizeY;
    if (groupsX > maxWorkgroups || groupsY > maxWorkgroups) {
        TF_WARN("WebGPU compute dispatch (%u, %u) exceeds per-dimension limit "
                "%u, clamping", groupsX, groupsY, maxWorkgroups);
        groupsX = std::min(groupsX, maxWorkgroups);
        groupsY = std::min(groupsY, maxWorkgroups);
    }

    _computePassEncoder.DispatchWorkgroups(groupsX, groupsY, 1);
}

bool
HgiWebGPUComputeCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    // End compute pass
    _EndComputePass();

    HgiWebGPU *wgpuHgi = static_cast<HgiWebGPU *>(hgi);

    wgpuHgi->EnqueueCommandBuffer(_commandBuffer);
    wgpuHgi->QueueSubmit();

    _commandBuffer = nullptr;

    return true;
}

void
HgiWebGPUComputeCmds::InsertMemoryBarrier(HgiMemoryBarrier barrier)
{
    //TF_WARN("HgiWebGPUComputeCmds::InsertMemoryBarrier not implemented");
}

HgiComputeDispatch
HgiWebGPUComputeCmds::GetDispatchMethod() const
{
    return _dispatchMethod;
}

void
HgiWebGPUComputeCmds::_BindResources()
{
    if (!_pipeline) {
        TF_CODING_ERROR("No pipeline bound");
        return;
    }

    _computePassStarted = true;
    if (_resourceBindings) {
        // now that the pipeline has been set we can bind resources
        HgiWebGPUResourceBindings * resourceBinding =
                static_cast<HgiWebGPUResourceBindings*>(_resourceBindings.Get());
        wgpu::ComputePipeline pipelineHandle = _pipeline->GetPipeline();
        resourceBinding->BindResources(_hgi->GetPrimaryDevice(), _computePassEncoder,
            _pipeline->GetPipelineBindGroups(), _constantBindGroupEntry, _pushConstantsDirty);
        _pushConstantsDirty = false;
        _resourceBindings = HgiResourceBindingsHandle();
    }
}

void
HgiWebGPUComputeCmds::_CreateCommandEncoder()
{
    if (!_commandEncoder) {
        wgpu::Device device = _hgi->GetPrimaryDevice();
	    _commandEncoder = device.CreateCommandEncoder();
        TF_VERIFY(_commandEncoder);
        // begin compute pass
        _computePassEncoder = _commandEncoder.BeginComputePass();
    }
}

void
HgiWebGPUComputeCmds::_EndComputePass()
{
    // release any resources
    if (_computePassStarted) {
        _computePassEncoder.End();
        _computePassEncoder = nullptr;

        _commandBuffer = _commandEncoder.Finish();
        _commandEncoder = nullptr;

        _computePassStarted = false;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
