//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIGL_OPS_H
#define PXR_IMAGING_HGIGL_OPS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"

#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgi/resourceBindings.h"

#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgiGL/device.h"

#include <cstdint>
#include <functional>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using HgiGLOpsFn = std::function<void(void)>;

/// \class HgiGLOps
///
/// A collection of functions used by cmds objects to do deferred cmd recording.
/// Modern API's support command buffer recording of gfx commands ('deferred').
/// Meaning: No commands are executed on the GPU until we Submit the cmd buffer.
///
/// OpenGL uses 'immediate' mode instead where gfx commands are immediately
/// processed and given to the GPU at a time of the drivers choosing.
/// We use 'Ops' functions to record our OpenGL function in a list and only
/// execute them in OpenGL during the SubmitCmds phase.
///
/// This has two benefits:
///
/// 1. OpenGL behaves more like Metal and Vulkan. So when clients write Hgi code
///    they get similar behavior in gpu command execution across all backends.
///    For example, if you are running with HgiGL and recording commands into a
///    Hgi***Cmds object and forget to call 'SubmitCmds' you will notice that
///    your commands are not executed on the GPU, just like what would happen if
///    you were running with HgiMetal.
///
/// 2. It lets us satisfy the Hgi requirement that Hgi***Cmds objects must be
///    able to do their recording on secondary threads.
///
class HgiGLOps
{
public:
    HGIGL_API
    static HgiGLOpsFn PushDebugGroup(const char* label);

    HGIGL_API
    static HgiGLOpsFn PopDebugGroup();

    HGIGL_API
    static HgiGLOpsFn CopyTextureGpuToCpu(HgiTextureGpuToCpuOp const& copyOp);

    HGIGL_API
    static HgiGLOpsFn CopyTextureCpuToGpu(HgiTextureCpuToGpuOp const& copyOp);

    HGIGL_API
    static HgiGLOpsFn CopyBufferGpuToGpu(HgiBufferGpuToGpuOp const& copyOp);

    HGIGL_API
    static HgiGLOpsFn CopyBufferCpuToGpu(HgiBufferCpuToGpuOp const& copyOp);

    HGIGL_API
    static HgiGLOpsFn CopyBufferGpuToCpu(HgiBufferGpuToCpuOp const& copyOp);

    HGIGL_API
    static HgiGLOpsFn CopyTextureToBuffer(HgiTextureToBufferOp const& copyOp);

    HGIGL_API
    static HgiGLOpsFn CopyBufferToTexture(HgiBufferToTextureOp const& copyOp);

    HGIGL_API
    static HgiGLOpsFn ResolveFramebuffer(
        HgiGLDevice* device,
        HgiGraphicsCmdsDesc const &graphicsCmds);
    
    HGIGL_API
    static HgiGLOpsFn SetViewport(GfVec4i const& vp);

    HGIGL_API
    static HgiGLOpsFn SetScissor(GfVec4i const& sc);

    HGIGL_API
    static HgiGLOpsFn BindPipeline(HgiGraphicsPipelineHandle pipeline);

    HGIGL_API
    static HgiGLOpsFn BindPipeline(HgiComputePipelineHandle pipeline);

    HGIGL_API
    static HgiGLOpsFn BindResources(HgiResourceBindingsHandle resources);

    HGIGL_API
    static HgiGLOpsFn SetConstantValues(
        HgiGraphicsPipelineHandle pipeline,
        HgiShaderStage stages,
        uint32_t bindIndex,
        uint32_t byteSize,
        const void* data);

    HGIGL_API
    static HgiGLOpsFn SetConstantValues(
        HgiComputePipelineHandle pipeline,
        uint32_t bindIndex,
        uint32_t byteSize,
        const void* data);

    HGIGL_API
    static HgiGLOpsFn BindVertexBuffers(
        HgiVertexBufferBindingVector const &bindings);

    HGIGL_API
    static HgiGLOpsFn Draw(
        HgiPrimitiveType primitiveType,
        uint32_t primitiveIndexSize,
        uint32_t vertexCount,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance);

    HGIGL_API
    static HgiGLOpsFn DrawIndirect(
        HgiPrimitiveType primitiveType,
        uint32_t primitiveIndexSize,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride);

    HGIGL_API
    static HgiGLOpsFn DrawIndexed(
        HgiPrimitiveType primitiveType,
        uint32_t primitiveIndexSize,
        HgiBufferHandle const& indexBuffer,
        uint32_t indexCount,
        uint32_t indexBufferByteOffset,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance);

    HGIGL_API
    static HgiGLOpsFn DrawIndexedIndirect(
        HgiPrimitiveType primitiveType,
        uint32_t primitiveIndexSize,
        HgiBufferHandle const& indexBuffer,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride);

    HGIGL_API
    static HgiGLOpsFn BindFramebufferOp(
        HgiGLDevice* device,
        HgiGraphicsCmdsDesc const& desc);

    HGIGL_API
    static HgiGLOpsFn Dispatch(int dimX, int dimY);

    HGIGL_API
    static HgiGLOpsFn FillBuffer(HgiBufferHandle const& buffer, uint8_t value);

    HGIGL_API
    static HgiGLOpsFn GenerateMipMaps(HgiTextureHandle const& texture);

    HGIGL_API
    static HgiGLOpsFn InsertMemoryBarrier(HgiMemoryBarrier barrier);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
