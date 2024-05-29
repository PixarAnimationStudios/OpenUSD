//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GL_GRAPHICS_CMDS_H
#define PXR_IMAGING_HGI_GL_GRAPHICS_CMDS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgi/graphicsCmds.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgiGL/hgi.h"
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiGraphicsCmdsDesc;
class HgiGLDevice;

/// \class HgiGLGraphicsCmds
///
/// OpenGL implementation of HgiGraphicsCmds.
///
class HgiGLGraphicsCmds final : public HgiGraphicsCmds
{
public:
    HGIGL_API
    ~HgiGLGraphicsCmds() override;

    /// XXX This function is exposed temporarily for Hgi transition.
    /// It allows code that is not yet converted to Hgi (e.g. HdSt) to insert
    /// its opengl calls into the ops-stack of HgiGL to ensure that all commands
    /// execute in the correct order. Once HdSt has transition fully to Hgi we
    /// should remove this function.
    HGIGL_API
    void InsertFunctionOp(std::function<void(void)> const& fn);

    HGIGL_API
    void PushDebugGroup(const char* label) override;

    HGIGL_API
    void PopDebugGroup() override;

    HGIGL_API
    void SetViewport(GfVec4i const& vp) override;

    HGIGL_API
    void SetScissor(GfVec4i const& sc) override;

    HGIGL_API
    void BindPipeline(HgiGraphicsPipelineHandle pipeline) override;

    HGIGL_API
    void BindResources(HgiResourceBindingsHandle resources) override;

    HGIGL_API
    void SetConstantValues(
        HgiGraphicsPipelineHandle pipeline,
        HgiShaderStage stages,
        uint32_t bindIndex,
        uint32_t byteSize,
        const void* data) override;
    
    HGIGL_API
    void BindVertexBuffers(
        HgiVertexBufferBindingVector const &bindings) override;

    HGIGL_API
    void Draw(
        uint32_t vertexCount,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance) override;

    HGIGL_API
    void DrawIndirect(
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride) override;

    HGIGL_API
    void DrawIndexed(
        HgiBufferHandle const& indexBuffer,
        uint32_t indexCount,
        uint32_t indexBufferByteOffset,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance) override;

    HGIGL_API
    void DrawIndexedIndirect(
        HgiBufferHandle const& indexBuffer,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride,
        std::vector<uint32_t> const& drawParameterBufferUInt32,
        uint32_t patchBaseVertexByteOffset) override;

    HGIGL_API
    void InsertMemoryBarrier(HgiMemoryBarrier barrier) override;

protected:
    friend class HgiGL;

    HGIGL_API
    HgiGLGraphicsCmds(
        HgiGLDevice* device,
        HgiGraphicsCmdsDesc const& desc);

    HGIGL_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiGLGraphicsCmds() = delete;
    HgiGLGraphicsCmds & operator=(const HgiGLGraphicsCmds&) = delete;
    HgiGLGraphicsCmds(const HgiGLGraphicsCmds&) = delete;

    /// This performs multisample resolve when needed at the end of recording.
    void _AddResolveToOps(HgiGLDevice* device);

    bool _recording;
    HgiGraphicsCmdsDesc _descriptor;
    HgiPrimitiveType _primitiveType;
    int _primitiveIndexSize;
    HgiGLOpsVector _ops;
    int _pushStack;
    int32_t _restoreReadFramebuffer;
    int32_t _restoreDrawFramebuffer;

    // Cmds is used only one frame so storing multi-frame state on will not
    // survive.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
