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
#ifndef PXR_IMAGING_HGI_METAL_GRAPHICS_CMDS_H
#define PXR_IMAGING_HGI_METAL_GRAPHICS_CMDS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgiMetal/api.h"
#include "pxr/imaging/hgi/graphicsCmds.h"
#include <cstdint>

#include <Metal/Metal.h>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiGraphicsCmdsDesc;


/// \class HgiMetalGraphicsCmds
///
/// Metal implementation of HgiGraphicsEncoder.
///
class HgiMetalGraphicsCmds final : public HgiGraphicsCmds
{
public:
    HGIMETAL_API
    ~HgiMetalGraphicsCmds() override;

    HGIMETAL_API
    void SetViewport(GfVec4i const& vp) override;

    HGIMETAL_API
    void SetScissor(GfVec4i const& sc) override;

    HGIMETAL_API
    void BindPipeline(HgiGraphicsPipelineHandle pipeline) override;

    HGIMETAL_API
    void BindResources(HgiResourceBindingsHandle resources) override;

    HGIMETAL_API
    void SetConstantValues(
        HgiGraphicsPipelineHandle pipeline,
        HgiShaderStage stages,
        uint32_t bindIndex,
        uint32_t byteSize,
        const void* data) override;

    HGIMETAL_API
    void BindVertexBuffers(
        uint32_t firstBinding,
        HgiBufferHandleVector const& buffers,
        std::vector<uint32_t> const& byteOffsets) override;

    HGIMETAL_API
    void Draw(
        uint32_t vertexCount,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance) override;

    HGIMETAL_API
    void DrawIndirect(
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t bufferOffset,
        uint32_t drawCount,
        uint32_t stride) override;

    HGIMETAL_API
    void DrawIndexed(
        HgiBufferHandle const& indexBuffer,
        uint32_t indexCount,
        uint32_t indexBufferByteOffset,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance) override;

    HGIMETAL_API
    void DrawIndexedIndirect(
        HgiBufferHandle const& indexBuffer,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride,
        std::vector<uint32_t> const& drawParameterBufferUInt32,
        uint32_t patchBaseVertexByteOffset) override;

    HGIMETAL_API
    void PushDebugGroup(const char* label) override;

    HGIMETAL_API
    void PopDebugGroup() override;

    HGIMETAL_API
    void MemoryBarrier(HgiMemoryBarrier barrier) override;

protected:
    friend class HgiMetal;

    HGIMETAL_API
    HgiMetalGraphicsCmds(
        HgiMetal* hgi,
        HgiGraphicsCmdsDesc const& desc);

    HGIMETAL_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiMetalGraphicsCmds() = delete;
    HgiMetalGraphicsCmds & operator=(const HgiMetalGraphicsCmds&) = delete;
    HgiMetalGraphicsCmds(const HgiMetalGraphicsCmds&) = delete;

    void _CreateEncoder();

    void _CreateArgumentBuffer();
    void _SyncArgumentBuffer();

    // We implement multi-draw indirect commands on Metal by encoding
    // separate draw commands for each draw.
    //
    // Some aspects of drawing command primitive input assembly work
    // differently on Metal than other graphics APIs. There are two
    // concerns that we need to account for while processing a buffer
    // with multiple indirect draw commands.
    //
    // 1) Metal does not support a vertex attrib divisor, so in order to
    // have vertex attributes which advance once per draw command we use
    // a constant vertex buffer step function and advance the vertex buffer
    // binding offset explicitly by executing setVertexBufferOffset for
    // the vertex buffers associated with "perDrawCommand" vertex attributes.
    //
    // 2) Metal does not support a base vertex offset for control point
    // vertex attributes when drawing patches. It is inconvenient and
    // expensive to encode a distinct controlPointIndex buffer for each
    // draw that shares a patch topology. Instead, we use a per patch
    // control point vertex buffer step function, and explicitly advance
    // the vertex buffer binding offset by executing setVertexBufferOffset
    // for the vertex buffers associated with "perPatchControlPoint"
    // vertex attributes.

    struct _VertexBufferStepFunctionDesc
    {
        _VertexBufferStepFunctionDesc(
                uint32_t bindingIndex,
                uint32_t byteOffset,
                uint32_t vertexStride)
            : bindingIndex(bindingIndex)
            , byteOffset(byteOffset)
            , vertexStride(vertexStride)
            { }
        uint32_t bindingIndex;
        uint32_t byteOffset;
        uint32_t vertexStride;
    };
    
    using _StepFunctionDescVector = std::vector<_VertexBufferStepFunctionDesc>;
    
    _StepFunctionDescVector _vertexBufferStepFunctionDescs;
    _StepFunctionDescVector _patchBaseVertexBufferStepFunctionDescs;

    void _InitVertexBufferStepFunction(HgiGraphicsPipeline const * pipeline);
    void _BindVertexBufferStepFunction(uint32_t byteOffset,
                                       uint32_t bindingIndex);

    void _SetVertexBufferStepFunctionOffsets(
        id<MTLRenderCommandEncoder> encoder,
        uint32_t baseInstance);

    void _SetPatchBaseVertexBufferStepFunctionOffsets(
        id<MTLRenderCommandEncoder> encoder,
        uint32_t baseVertex);

    HgiMetal* _hgi;
    MTLRenderPassDescriptor* _renderPassDescriptor;
    id<MTLRenderCommandEncoder> _encoder;
    id<MTLBuffer> _argumentBuffer;
    HgiGraphicsCmdsDesc _descriptor;
    HgiPrimitiveType _primitiveType;
    uint32_t _primitiveIndexSize;
    bool _hasWork;
    MTLViewport _viewport;
    NSString* _debugLabel;
    bool _viewportSet;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
