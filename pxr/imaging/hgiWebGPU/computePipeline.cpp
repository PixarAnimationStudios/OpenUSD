//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/computePipeline.h"
#include "pxr/imaging/hgiWebGPU/pipelineBindGroups.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/shaderFunction.h"
#include "pxr/imaging/hgiWebGPU/shaderProgram.h"
PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPUComputePipeline::HgiWebGPUComputePipeline(
    HgiWebGPU* hgi,
    HgiComputePipelineDesc const& desc)
    : HgiComputePipeline(desc)
    , _pipeline(nullptr)
{
    TF_VERIFY(desc.shaderProgram->GetShaderFunctions().size() == 1 );

    HgiWebGPUShaderFunction const* computeProgram =
            static_cast<HgiWebGPUShaderFunction const*>(desc.shaderProgram->GetShaderFunctions().begin()->Get());
    const HgiShaderStage &shaderStage = computeProgram->GetDescriptor().shaderStage;
    TF_VERIFY(shaderStage == HgiShaderStageCompute );
    const BindGroupsLayoutMap &bindGroupsLayoutEntries = computeProgram->GetBindGroups();

    const wgpu::Device device = hgi->GetPrimaryDevice();

    wgpu::PipelineLayout pipelineLayout =
        _pipelineBindGroups.CreatePipelineLayout(device, bindGroupsLayoutEntries);

    // TODO: desc.shaderConstantsDesc doesnt correspond with the webgpu spec
    std::vector<wgpu::ConstantEntry> constants;

    wgpu::ComputeState computeState;
    computeState.module = computeProgram->GetShaderModule();
    computeState.constantCount = constants.size();
    computeState.constants = constants.data();

    wgpu::ComputePipelineDescriptor pipelineDesc;
    pipelineDesc.label = desc.debugName.c_str();
    pipelineDesc.layout = pipelineLayout;
    pipelineDesc.compute = computeState;

    _pipeline = device.CreateComputePipeline(&pipelineDesc);
}

HgiWebGPUComputePipeline::~HgiWebGPUComputePipeline()
{
}

wgpu::ComputePipeline
HgiWebGPUComputePipeline::GetPipeline() const
{
    return _pipeline;
}

const HgiWebGPUPipelineBindGroups&
HgiWebGPUComputePipeline::GetPipelineBindGroups() const
{
    return _pipelineBindGroups;
}

PXR_NAMESPACE_CLOSE_SCOPE
