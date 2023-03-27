//
// Copyright 2020 Pixar
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
    bool _hasWork;
    HgiComputeDispatch _dispatchMethod;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
