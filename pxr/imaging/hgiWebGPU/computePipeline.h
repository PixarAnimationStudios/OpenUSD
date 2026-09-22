//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_COMPUTE_PIPELINE_H
#define PXR_IMAGING_HGI_WEBGPU_COMPUTE_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/computePipeline.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/pipelineBindGroups.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiWebGPU;

/// \class HgiWebGPUComputePipeline
///
/// WebGPU implementation of HgiComputePipeline.
///
class HgiWebGPUComputePipeline final : public HgiComputePipeline
{
public:
    HGIWEBGPU_API
    ~HgiWebGPUComputePipeline() override;

    HGIWEBGPU_API
    wgpu::ComputePipeline GetPipeline() const;

    HGIWEBGPU_API
    const HgiWebGPUPipelineBindGroups& GetPipelineBindGroups() const;

protected:
    friend class HgiWebGPU;

    HGIWEBGPU_API
    HgiWebGPUComputePipeline(
        HgiWebGPU *hgi,
        HgiComputePipelineDesc const& desc);

private:
    HgiWebGPUComputePipeline() = delete;
    HgiWebGPUComputePipeline & operator=(const HgiWebGPUComputePipeline&) = delete;
    HgiWebGPUComputePipeline(const HgiWebGPUComputePipeline&) = delete;

    wgpu::ComputePipeline _pipeline;
    HgiWebGPUPipelineBindGroups _pipelineBindGroups;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
