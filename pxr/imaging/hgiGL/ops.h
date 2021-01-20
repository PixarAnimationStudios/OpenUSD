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
        uint32_t firstBinding,
        HgiBufferHandleVector const& buffers,
        std::vector<uint32_t> const& byteOffsets);

    HGIGL_API
    static HgiGLOpsFn Draw(
        HgiPrimitiveType primitiveType,
        uint32_t vertexCount,
        uint32_t firstVertex,
        uint32_t instanceCount);

    HGIGL_API
    static HgiGLOpsFn DrawIndirect(
        HgiPrimitiveType primitiveType,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferOffset,
        uint32_t drawCount,
        uint32_t stride);

    HGIGL_API
    static HgiGLOpsFn DrawIndexed(
        HgiPrimitiveType primitiveType,
        HgiBufferHandle const& indexBuffer,
        uint32_t indexCount,
        uint32_t indexBufferByteOffset,
        uint32_t vertexOffset,
        uint32_t instanceCount);

    HGIGL_API
    static HgiGLOpsFn DrawIndexedIndirect(
        HgiPrimitiveType primitiveType,
        HgiBufferHandle const& indexBuffer,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferOffset,
        uint32_t drawCount,
        uint32_t stride);

    HGIGL_API
    static HgiGLOpsFn BindFramebufferOp(
        HgiGLDevice* device,
        HgiGraphicsCmdsDesc const& desc);

    HGIGL_API
    static HgiGLOpsFn Dispatch(int dimX, int dimY);

    HGIGL_API
    static HgiGLOpsFn GenerateMipMaps(HgiTextureHandle const& texture);

    HGIGL_API
    static HgiGLOpsFn MemoryBarrier(HgiMemoryBarrier barrier);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
