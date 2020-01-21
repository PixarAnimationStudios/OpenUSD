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
#ifndef PXR_IMAGING_HGI_GL_BLIT_ENCODER_H
#define PXR_IMAGING_HGI_GL_BLIT_ENCODER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgi/blitEncoder.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiGLImmediateCommandBuffer;


/// \class HgiGLBlitEncoder
///
/// OpenGL implementation of HgiBlitEncoder.
///
class HgiGLBlitEncoder final : public HgiBlitEncoder
{
public:
    HGIGL_API
    virtual ~HgiGLBlitEncoder();

    HGIGL_API
    void EndEncoding() override;

    HGIGL_API
    void PushDebugGroup(const char* label) override;

    HGIGL_API
    void PopDebugGroup() override;

    HGIGL_API
    void CopyTextureGpuToCpu(HgiCopyResourceOp const& copyOp) override;

    HGIGL_API
    void ResolveImage(HgiResolveImageOp const& resolveOp) override;

protected:
    friend class HgiGL;
    friend class HgiGLImmediateCommandBuffer;

    HGIGL_API
    HgiGLBlitEncoder(HgiGLImmediateCommandBuffer* cmdBuf);

private:
    HgiGLBlitEncoder() = delete;
    HgiGLBlitEncoder & operator=(const HgiGLBlitEncoder&) = delete;
    HgiGLBlitEncoder(const HgiGLBlitEncoder&) = delete;

    HgiGLImmediateCommandBuffer* _commandBuffer;

    // Encoder is used only one frame so storing multi-frame state on encoder
    // will not survive. Store onto HgiGLImmediateCommandBuffer instead.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
