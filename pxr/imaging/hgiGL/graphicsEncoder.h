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
#ifndef PXR_IMAGING_HGI_GL_GRAPHICS_ENCODER_H
#define PXR_IMAGING_HGI_GL_GRAPHICS_ENCODER_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgi/graphicsEncoder.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgiGL/device.h"
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiGraphicsEncoderDesc;


/// \class HgiGLGraphicsEncoder
///
/// OpenGL implementation of HgiGraphicsEncoder.
///
class HgiGLGraphicsEncoder final : public HgiGraphicsEncoder
{
public:
    HGIGL_API
    ~HgiGLGraphicsEncoder() override;

    /// XXX This function is exposed temporarily for Hgi transition.
    /// It allows code that is not yet converted to Hgi (e.g. HdSt) to insert
    /// its opengl calls into the ops-stack of HgiGL to esnure that all commands
    /// execute in the correct order. Once HdSt has transition fully to Hgi we
    /// should remove this function.
    HGIGL_API
    void InsertFunctionOp(std::function<void(void)> const& fn);

    HGIGL_API
    void Commit() override;

    HGIGL_API
    void SetViewport(GfVec4i const& vp) override;

    HGIGL_API
    void SetScissor(GfVec4i const& sc) override;

    HGIGL_API
    void BindPipeline(HgiPipelineHandle pipeline) override;

    HGIGL_API
    void BindResources(HgiResourceBindingsHandle resources) override;

    HGIGL_API
    void BindVertexBuffers(
        uint32_t firstBinding,
        HgiBufferHandleVector const& buffers,
        std::vector<uint32_t> const& byteOffsets) override;

    HGIGL_API
    void DrawIndexed(
        HgiBufferHandle const& indexBuffer,
        uint32_t indexCount,
        uint32_t indexBufferByteOffset,
        uint32_t firstIndex,
        uint32_t vertexOffset,
        uint32_t instanceCount) override;

    HGIGL_API
    void PushDebugGroup(const char* label) override;

    HGIGL_API
    void PopDebugGroup() override;

protected:
    friend class HgiGL;

    HGIGL_API
    HgiGLGraphicsEncoder(
        HgiGLDevice* device,
        HgiGraphicsEncoderDesc const& desc);

private:
    HgiGLGraphicsEncoder() = delete;
    HgiGLGraphicsEncoder & operator=(const HgiGLGraphicsEncoder&) = delete;
    HgiGLGraphicsEncoder(const HgiGLGraphicsEncoder&) = delete;

    bool _committed;
    HgiGLOpsVector _ops;

    // Encoder is used only one frame so storing multi-frame state on encoder
    // will not survive.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
