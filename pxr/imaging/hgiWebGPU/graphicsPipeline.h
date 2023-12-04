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
#ifndef PXR_IMAGING_HGI_WEBGPU_PIPELINE_H
#define PXR_IMAGING_HGI_WEBGPU_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE

class HgiWebGPU;

/// \class HgiWebGPUPipeline
///
/// WebGPU implementation of HgiGraphicsPipeline.
///
class HgiWebGPUGraphicsPipeline final : public HgiGraphicsPipeline
{
public:
    HGIWEBGPU_API
    ~HgiWebGPUGraphicsPipeline() override;

    HGIWEBGPU_API
    wgpu::RenderPipeline GetPipeline() const;

    HGIWEBGPU_API
    const std::vector<wgpu::BindGroupLayout>& GetBindGroupLayoutList() const;

protected:
    friend class HgiWebGPU;

    HGIWEBGPU_API
    HgiWebGPUGraphicsPipeline(
        HgiWebGPU *hgi,
        HgiGraphicsPipelineDesc const& desc);

private:
    HgiWebGPUGraphicsPipeline() = delete;
    HgiWebGPUGraphicsPipeline & operator=(const HgiWebGPUGraphicsPipeline&) = delete;
    HgiWebGPUGraphicsPipeline(const HgiWebGPUGraphicsPipeline&) = delete;

    wgpu::RenderPipeline _pipeline;
    std::vector<wgpu::BindGroupLayout> _bindGroupLayoutList;

    struct ShaderStates
    {
        wgpu::VertexState vertexState;
        wgpu::FragmentState fragmentState;
    };
    std::vector<ShaderStates> _shaderStates;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
