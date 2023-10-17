//
// Copyright 2022 Pixar
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
#include "pxr/imaging/hgiWebGPU/computeCmds.h"
#include "pxr/imaging/hgiWebGPU/computePipeline.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/diagnostic.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/resourceBindings.h"

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

    // begin compute pass
    _computePassEncoder = _commandEncoder.BeginComputePass();

}

HgiWebGPUComputeCmds::~HgiWebGPUComputeCmds()
{
}

void
HgiWebGPUComputeCmds::PushDebugGroup(const char* label)
{
    _CreateCommandEncoder();
    HgiWebGPUBeginLabel(_commandEncoder, label);
}

void
HgiWebGPUComputeCmds::PopDebugGroup()
{
    _CreateCommandEncoder();
    HgiWebGPUEndLabel(_commandEncoder);
}

void
HgiWebGPUComputeCmds::BindPipeline(HgiComputePipelineHandle pipeline)
{
    _CreateCommandEncoder();

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
    _pendingUpdates.push_back(
        [this, res] {
            HgiWebGPUResourceBindings * resourceBinding =
                static_cast<HgiWebGPUResourceBindings*>(res.Get());
            wgpu::ComputePipeline pipelineHandle = _pipeline->GetPipeline();
            resourceBinding->BindResources(_hgi->GetPrimaryDevice(), _computePassEncoder, _pipeline->GetBindGroupLayoutList(), _constantBindGroupEntry, _pushConstantsDirty);
            _pushConstantsDirty = false;
        }
    );
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
    _ApplyPendingUpdates();

    const int workgroupSizeX = _localWorkGroupSize[0];
    const int workgroupSizeY = _localWorkGroupSize[1];

    _computePassEncoder.DispatchWorkgroups((dimX + workgroupSizeX - 1) / workgroupSizeX, (dimY + workgroupSizeY - 1) / workgroupSizeY, 1);
}

bool
HgiWebGPUComputeCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    // End compute pass
    _EndComputePass();

    HgiWebGPU *wgpuHgi = static_cast<HgiWebGPU *>(hgi);

    wgpuHgi->EnqueueCommandBuffer(_commandBuffer);
    wgpuHgi->QueueSubmit();

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
HgiWebGPUComputeCmds::_ApplyPendingUpdates()
{
    if (!_pipeline) {
        TF_CODING_ERROR("No pipeline bound");
        return;
    }

    _computePassStarted = true;
    // now that the pipeline has been set we can execute any commands that require the pipeline information
    for (auto const& fn : _pendingUpdates)
        fn();

    _pendingUpdates.clear();
}

void
HgiWebGPUComputeCmds::_CreateCommandEncoder()
{
    if (!_commandEncoder) {
        wgpu::Device device = _hgi->GetPrimaryDevice();
	    _commandEncoder = device.CreateCommandEncoder();
        TF_VERIFY(_commandEncoder);
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