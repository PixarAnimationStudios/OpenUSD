//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_COMPUTE_CMDS_H
#define PXR_IMAGING_HGI_WEBGPU_COMPUTE_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/computeCmds.h"
#include "pxr/imaging/hgi/computePipeline.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/resourceBindings.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HgiComputeCmdsDesc;
class HgiWebGPU;
class HgiWebGPUComputePipeline;

/// \class HgiWebGPUComputeCmds
///
/// OpenGL implementation of HgiComputeCmds.
///
class HgiWebGPUComputeCmds final : public HgiComputeCmds
{
public:
    HGIWEBGPU_API
    ~HgiWebGPUComputeCmds() override;

    HGIWEBGPU_API
    void PushDebugGroup(const char* label,
        const GfVec4f& color = s_computeDebugColor) override;

    HGIWEBGPU_API
    void PopDebugGroup() override;

    HGIWEBGPU_API
    void InsertDebugMarker(
        const char* label,
        const GfVec4f& color = s_markerDebugColor) override;

    HGIWEBGPU_API
    void BindPipeline(HgiComputePipelineHandle pipeline) override;

    HGIWEBGPU_API
    void BindResources(HgiResourceBindingsHandle resources) override;

    HGIWEBGPU_API
    void SetConstantValues(
        HgiComputePipelineHandle pipeline,
        uint32_t bindIndex,
        uint32_t byteSize,
        const void* data) override;
    
    HGIWEBGPU_API
    void Dispatch(int dimX, int dimY) override;

    HGIWEBGPU_API
    void InsertMemoryBarrier(HgiMemoryBarrier barrier) override;

    HGIWEBGPU_API
    HgiComputeDispatch GetDispatchMethod() const override;

protected:
    friend class HgiWebGPU;

    HGIWEBGPU_API
    HgiWebGPUComputeCmds(HgiWebGPU* hgi, HgiComputeCmdsDesc const& desc);

    HGIWEBGPU_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiWebGPUComputeCmds() = delete;
    HgiWebGPUComputeCmds & operator=(const HgiWebGPUComputeCmds&) = delete;
    HgiWebGPUComputeCmds(const HgiWebGPUComputeCmds&) = delete;

    void _BindResources();
    void _CreateCommandEncoder();
    void _EndComputePass();

    HgiWebGPU* _hgi;
    wgpu::BindGroupEntry _constantBindGroupEntry;
    wgpu::ComputePassEncoder _computePassEncoder;
    wgpu::CommandEncoder _commandEncoder;
    wgpu::CommandBuffer _commandBuffer;
    HgiWebGPUComputePipeline *_pipeline;
    std::string _debugLabel;
    bool _computePassStarted;
    bool _pushConstantsDirty;
    HgiComputeDispatch _dispatchMethod;
    GfVec3i _localWorkGroupSize;
    HgiResourceBindingsHandle _resourceBindings;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
