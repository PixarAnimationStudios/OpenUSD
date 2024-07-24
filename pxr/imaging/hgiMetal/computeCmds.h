//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_METAL_COMPUTE_CMDS_H
#define PXR_IMAGING_HGI_METAL_COMPUTE_CMDS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgiMetal/api.h"
#include "pxr/imaging/hgi/computeCmds.h"
#include <cstdint>

#include <Metal/Metal.h>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiComputeCmdsDesc;
class HgiMetalComputePipeline;

/// \class HgiMetalGraphicsCmds
///
/// Metal implementation of HgiGraphicsEncoder.
///
class HgiMetalComputeCmds final : public HgiComputeCmds
{
public:
    HGIMETAL_API
    ~HgiMetalComputeCmds() override;

    HGIMETAL_API
    void BindPipeline(HgiComputePipelineHandle pipeline) override;

    HGIMETAL_API
    void BindResources(HgiResourceBindingsHandle resources) override;

    HGIMETAL_API
    void SetConstantValues(
        HgiComputePipelineHandle pipeline,
        uint32_t bindIndex,
        uint32_t byteSize,
        const void* data) override;

    HGIMETAL_API
    void Dispatch(int dimX, int dimY) override;

    HGIMETAL_API
    void PushDebugGroup(const char* label) override;

    HGIMETAL_API
    void PopDebugGroup() override;

    HGIMETAL_API
    void InsertMemoryBarrier(HgiMemoryBarrier barrier) override;

    HGIMETAL_API
    HgiComputeDispatch GetDispatchMethod() const override;

    HGIMETAL_API
    id<MTLComputeCommandEncoder> GetEncoder();

protected:
    friend class HgiMetal;

    HGIMETAL_API
    HgiMetalComputeCmds(HgiMetal* hgi, HgiComputeCmdsDesc const& desc);

    HGIMETAL_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiMetalComputeCmds() = delete;
    HgiMetalComputeCmds & operator=(const HgiMetalComputeCmds&) = delete;
    HgiMetalComputeCmds(const HgiMetalComputeCmds&) = delete;

    void _CreateEncoder();
    void _CreateArgumentBuffer();
    
    HgiMetal* _hgi;
    HgiMetalComputePipeline* _pipelineState;
    id<MTLCommandBuffer> _commandBuffer;
    id<MTLBuffer> _argumentBuffer;
    id<MTLComputeCommandEncoder> _encoder;
    bool _secondaryCommandBuffer;
    HgiComputeDispatch _dispatchMethod;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
