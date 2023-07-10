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
#ifndef PXR_IMAGING_HGI_WEBGPU_BLIT_CMDS_H
#define PXR_IMAGING_HGI_WEBGPU_BLIT_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgi/blitCmds.h"
#include "mipmapGenerator.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiWebGPU;


/// \class HgiWebGPUBlitCmds
///
/// WebGPU implementation of HgiBlitCmds.
///
class HgiWebGPUBlitCmds final : public HgiBlitCmds
{
public:
    HGIWEBGPU_API
    ~HgiWebGPUBlitCmds() override;

    HGIWEBGPU_API
    void PushDebugGroup(const char* label) override;

    HGIWEBGPU_API
    void PopDebugGroup() override;

    HGIWEBGPU_API
    void CopyTextureGpuToCpu(HgiTextureGpuToCpuOp const& copyOp) override;

    HGIWEBGPU_API
    void CopyTextureCpuToGpu(HgiTextureCpuToGpuOp const& copyOp) override;

    HGIWEBGPU_API
    void CopyBufferGpuToGpu(HgiBufferGpuToGpuOp const& copyOp) override;

    HGIWEBGPU_API
    void CopyBufferCpuToGpu(HgiBufferCpuToGpuOp const& copyOp) override;

    HGIWEBGPU_API
    void CopyBufferGpuToCpu(HgiBufferGpuToCpuOp const& copyOp) override;

    HGIWEBGPU_API
    void CopyTextureToBuffer(HgiTextureToBufferOp const& copyOp) override;
    
    HGIWEBGPU_API
    void CopyBufferToTexture(HgiBufferToTextureOp const& copyOp) override;

    HGIWEBGPU_API
    void GenerateMipMaps(HgiTextureHandle const& texture) override;

    HGIWEBGPU_API
    void FillBuffer(HgiBufferHandle const& buffer, uint8_t value) override;

    HGIWEBGPU_API
    void InsertMemoryBarrier(HgiMemoryBarrier barrier) override;

protected:
    friend class HgiWebGPU;

    HGIWEBGPU_API
    HgiWebGPUBlitCmds(HgiWebGPU* hgi);

    HGIWEBGPU_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiWebGPUBlitCmds() = delete;
    HgiWebGPUBlitCmds & operator=(const HgiWebGPUBlitCmds&) = delete;
    HgiWebGPUBlitCmds(const HgiWebGPUBlitCmds&) = delete;

    void _CreateEncoder();
    void _MapAsyncAndWait(const wgpu::Buffer& buffer,
                                        wgpu::MapMode mode,
                                        size_t offset,
                                        size_t size);

    HgiWebGPU* _hgi;
    wgpu::CommandEncoder _blitEncoder;
    WebGPUMipmapGenerator _mipmapGenerator;
    wgpu::CommandBuffer _commandBuffer;

    struct StagingData
    {
        wgpu::Buffer src;
        void *dst;
        uint32_t size;
        uint32_t bytesPerRow;
        uint32_t bytesPerRowAligned;
        bool isTmp = false;
    };
    std::vector<StagingData> _stagingDatas;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
