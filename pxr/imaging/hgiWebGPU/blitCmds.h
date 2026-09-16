//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_BLIT_CMDS_H
#define PXR_IMAGING_HGI_WEBGPU_BLIT_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgi/blitCmds.h"

#include <functional>

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
    void PushDebugGroup(const char* label,
        const GfVec4f& color = s_blitDebugColor) override;

    HGIWEBGPU_API
    void PopDebugGroup() override;

    HGIWEBGPU_API
    void InsertDebugMarker(
        const char* label,
        const GfVec4f& color = s_markerDebugColor) override;

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
    wgpu::CommandBuffer _commandBuffer;
    std::function<void()> _completedHandler;

    struct StagingData
    {
        wgpu::Buffer src;
        void *dst;
        uint32_t alignedSize;
        uint32_t bytesPerRow;
        uint32_t bytesPerRowAligned;
        bool isTmp = false; // to be removed when webgpu bug is fixed
        bool asyncDone = false; // to be removed when async is the only option
    };
    std::vector<std::unique_ptr<StagingData>> _stagingDataItems;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
