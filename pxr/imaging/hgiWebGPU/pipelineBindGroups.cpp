//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgiWebGPU/pipelineBindGroups.h"
#include "pxr/imaging/hgiWebGPU/shaderSection.h"

PXR_NAMESPACE_OPEN_SCOPE

wgpu::PipelineLayout
HgiWebGPUPipelineBindGroups::CreatePipelineLayout(wgpu::Device const& device,
    BindGroupsLayoutMap const& bindGroupsLayoutMap,
    std::string const& debugName)
{
    if (_pipelineLayout) {
        return _pipelineLayout;
    }

    for (const auto& [bindGroupSet, bindGroupEntries] : bindGroupsLayoutMap) {
        std::vector<wgpu::BindGroupLayoutEntry> entries;
        entries.reserve(bindGroupEntries.size());
        for (const auto& [_, entry] : bindGroupEntries) {
            entries.push_back(entry);
        }
        wgpu::BindGroupLayoutDescriptor bglDesc;
        bglDesc.label = debugName.empty() ? nullptr : debugName.c_str();
        bglDesc.entryCount = entries.size();
        bglDesc.entries = entries.data();
        _bindGroupLayouts.push_back(device.CreateBindGroupLayout(&bglDesc));
        if (bindGroupSet == HgiWebGPUBufferShaderSection::bindingSet) {
            for (const auto& [index, _] : bindGroupEntries) {
                _liveBufferBindings.insert(index);
            }
        }
    }

    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindGroupLayoutCount = _bindGroupLayouts.size();
    pipelineLayoutDesc.bindGroupLayouts = _bindGroupLayouts.data();
    _pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);
    return _pipelineLayout;
}

std::vector<wgpu::BindGroupEntry>
HgiWebGPUPipelineBindGroups::FilterLiveBufferBindings(
    std::vector<wgpu::BindGroupEntry> const& bindings) const
{
    std::vector<wgpu::BindGroupEntry> result;
    for (const auto& entry : bindings) {
        if (_liveBufferBindings.count(entry.binding)) {
            result.push_back(entry);
        }
    }
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
