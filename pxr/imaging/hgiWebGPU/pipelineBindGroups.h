//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HGI_WEBGPU_PIPELINE_BIND_GROUPS_H
#define PXR_IMAGING_HGI_WEBGPU_PIPELINE_BIND_GROUPS_H

#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/shaderFunction.h"

#include <string>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiWebGPUPipelineBindGroups
{
    HGIWEBGPU_API
    wgpu::PipelineLayout CreatePipelineLayout(wgpu::Device const& device,
        BindGroupsLayoutMap const& bindGroupsLayoutMap,
        std::string const& debugName = {});

    HGIWEBGPU_API
    std::vector<wgpu::BindGroupEntry> FilterLiveBufferBindings(
        std::vector<wgpu::BindGroupEntry> const& bindings) const;

    HGIWEBGPU_API
    const std::vector<wgpu::BindGroupLayout>& GetLayouts() const
    {
        return _bindGroupLayouts;
    }

    HGIWEBGPU_API
    const std::unordered_set<uint32_t>& GetLiveBufferBindings() const
    {
        return _liveBufferBindings;
    }

private:
    wgpu::PipelineLayout _pipelineLayout;
    std::vector<wgpu::BindGroupLayout> _bindGroupLayouts;
    std::unordered_set<uint32_t> _liveBufferBindings;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
