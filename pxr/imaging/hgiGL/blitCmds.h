//
// Copyright 2019 Pixar
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
    void MemoryBarrier(HgiMemoryBarrier barrier) override;

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
