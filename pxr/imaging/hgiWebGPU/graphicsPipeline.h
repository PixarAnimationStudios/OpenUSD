//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_PIPELINE_H
#define PXR_IMAGING_HGI_WEBGPU_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/pipelineBindGroups.h"


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
    const HgiWebGPUPipelineBindGroups& GetPipelineBindGroups() const;

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
    HgiWebGPUPipelineBindGroups _pipelineBindGroups;

    struct ShaderStates
    {
        wgpu::VertexState vertexState;
        wgpu::FragmentState fragmentState;
    };
    std::vector<ShaderStates> _shaderStates;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
