//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_METAL_BLIT_CMDS_H
#define PXR_IMAGING_HGI_METAL_BLIT_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiMetal/api.h"
#include "pxr/imaging/hgi/blitCmds.h"

#include <Metal/Metal.h>

PXR_NAMESPACE_OPEN_SCOPE

class HgiMetal;


/// \class HgiMetalBlitCmds
///
/// Metal implementation of HgiBlitCmds.
///
class HgiMetalBlitCmds final : public HgiBlitCmds
{
public:
    HGIMETAL_API
    ~HgiMetalBlitCmds() override;

    HGIMETAL_API
    void PushDebugGroup(const char* label) override;

    HGIMETAL_API
    void PopDebugGroup() override;

    HGIMETAL_API
    void CopyTextureGpuToCpu(HgiTextureGpuToCpuOp const& copyOp) override;

    HGIMETAL_API
    void CopyTextureCpuToGpu(HgiTextureCpuToGpuOp const& copyOp) override;

    HGIMETAL_API
    void CopyBufferGpuToGpu(HgiBufferGpuToGpuOp const& copyOp) override;

    HGIMETAL_API
    void CopyBufferCpuToGpu(HgiBufferCpuToGpuOp const& copyOp) override;

    HGIMETAL_API
    void CopyBufferGpuToCpu(HgiBufferGpuToCpuOp const& copyOp) override;

    HGIMETAL_API
    void CopyTextureToBuffer(HgiTextureToBufferOp const& copyOp) override;
    
    HGIMETAL_API
    void CopyBufferToTexture(HgiBufferToTextureOp const& copyOp) override;

    HGIMETAL_API
    void GenerateMipMaps(HgiTextureHandle const& texture) override;

    HGIMETAL_API
    void FillBuffer(HgiBufferHandle const& buffer, uint8_t value) override;

    HGIMETAL_API
    void InsertMemoryBarrier(HgiMemoryBarrier barrier) override;

protected:
    friend class HgiMetal;

    HGIMETAL_API
    HgiMetalBlitCmds(HgiMetal* hgi);

    HGIMETAL_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiMetalBlitCmds() = delete;
    HgiMetalBlitCmds & operator=(const HgiMetalBlitCmds&) = delete;
    HgiMetalBlitCmds(const HgiMetalBlitCmds&) = delete;

    void _CreateEncoder();

    HgiMetal* _hgi;
    id<MTLCommandBuffer> _commandBuffer;
    id<MTLBlitCommandEncoder> _blitEncoder;
    NSString* _label;
    bool _secondaryCommandBuffer;

    // BlitCmds is used only one frame so storing multi-frame state on BlitCmds
    // will not survive.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
