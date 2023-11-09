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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/computePipeline.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/shaderFunction.h"
#include "pxr/imaging/hgiWebGPU/shaderProgram.h"
#include <algorithm>

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

    // create the bind group layout
    for (const auto & [bindGroup, bindGroupEntries]: bindGroupsLayoutEntries) {
        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc;
        std::vector<wgpu::BindGroupLayoutEntry> entries;
        std::transform(
                bindGroupEntries.begin(),
                bindGroupEntries.end(),
                std::back_inserter(entries),
                [](auto &mapEntry){
                    return mapEntry.second;
                }
        );
        bindGroupLayoutDesc.entryCount = bindGroupEntries.size();
        bindGroupLayoutDesc.entries = entries.data();
        wgpu::BindGroupLayout bgl = device.CreateBindGroupLayout(&bindGroupLayoutDesc);
        _bindGroupLayoutList.push_back(bgl);
    }
    
    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindGroupLayoutCount = _bindGroupLayoutList.size();
    pipelineLayoutDesc.bindGroupLayouts = _bindGroupLayoutList.data();
    wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);

    // TODO: desc.shaderConstantsDesc doesnt correspond with the webgpu spec
    std::vector<wgpu::ConstantEntry> constants;

    wgpu::ProgrammableStageDescriptor computeStageDesc;
    computeStageDesc.module = computeProgram->GetShaderModule();
    computeStageDesc.entryPoint = computeProgram->GetShaderEntryPoint();
    computeStageDesc.constantCount = constants.size();
    computeStageDesc.constants = constants.data();

    wgpu::ComputePipelineDescriptor pipelineDesc;
    pipelineDesc.label = desc.debugName.c_str();
    pipelineDesc.layout = pipelineLayout;
    pipelineDesc.compute = computeStageDesc;

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

const std::vector<wgpu::BindGroupLayout>&
HgiWebGPUComputePipeline::GetBindGroupLayoutList() const
{
    return _bindGroupLayoutList;
}

PXR_NAMESPACE_CLOSE_SCOPE
