//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GL_BLIT_CMDS_H
#define PXR_IMAGING_HGI_GL_BLIT_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgiGL/hgi.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class HgiGLBlitCmds
///
/// OpenGL implementation of HgiBlitCmds.
///
class HgiGLBlitCmds final : public HgiBlitCmds
{
public:
    HGIGL_API
    ~HgiGLBlitCmds() override;

    HGIGL_API
    void PushDebugGroup(const char* label) override;

    HGIGL_API
    void PopDebugGroup() override;

    HGIGL_API
    void CopyTextureGpuToCpu(HgiTextureGpuToCpuOp const& copyOp) override;

    HGIGL_API
    void CopyTextureCpuToGpu(HgiTextureCpuToGpuOp const& copyOp) override;

    HGIGL_API
    void CopyBufferGpuToGpu(HgiBufferGpuToGpuOp const& copyOp) override;

    HGIGL_API
    void CopyBufferCpuToGpu(HgiBufferCpuToGpuOp const& copyOp) override;

    HGIGL_API
    void CopyBufferGpuToCpu(HgiBufferGpuToCpuOp const& copyOp) override;

    HGIGL_API
    void CopyTextureToBuffer(HgiTextureToBufferOp const& copyOp) override;
    
    HGIGL_API
    void CopyBufferToTexture(HgiBufferToTextureOp const& copyOp) override;

    HGIGL_API
    void GenerateMipMaps(HgiTextureHandle const& texture) override;
    
    HGIGL_API
    void FillBuffer(HgiBufferHandle const& buffer, uint8_t value) override;

    HGIGL_API
    void InsertMemoryBarrier(HgiMemoryBarrier barrier) override;

protected:
    friend class HgiGL;

    HGIGL_API
    HgiGLBlitCmds();

    HGIGL_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiGLBlitCmds & operator=(const HgiGLBlitCmds&) = delete;
    HgiGLBlitCmds(const HgiGLBlitCmds&) = delete;

    HgiGLOpsVector _ops;
    int _pushStack;

    // BlitCmds is used only one frame so storing multi-frame state here will
    // not survive.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
