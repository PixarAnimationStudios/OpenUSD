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
#ifndef PXR_IMAGING_HGI_GRAPHICS_CMDS_H
#define PXR_IMAGING_HGI_GRAPHICS_CMDS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/cmds.h"
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HgiGraphicsCmdsUniquePtr = std::unique_ptr<class HgiGraphicsCmds>;


/// \class HgiGraphicsCmds
///
/// A graphics API independent abstraction of graphics commands.
/// HgiGraphicsCmds is a lightweight object that cannot be re-used after it has
/// been submitted. A new cmds object should be acquired for each frame.
///
class HgiGraphicsCmds : public HgiCmds
{
public:
    HGI_API
    ~HgiGraphicsCmds() override;

    /// Push a debug marker.
    HGI_API
    virtual void PushDebugGroup(const char* label) = 0;

    /// Pop the last debug marker.
    HGI_API
    virtual void PopDebugGroup() = 0;

    /// Set viewport [left, BOTTOM, width, height] - OpenGL coords
    HGI_API
    virtual void SetViewport(GfVec4i const& vp) = 0;

    /// Only pixels that lie within the scissor box are modified by
    /// drawing commands.
    HGI_API
    virtual void SetScissor(GfVec4i const& sc) = 0;

    /// Bind a pipeline state object. Usually you call this right after calling
    /// CreateGraphicsCmds to set the graphics pipeline state.
    /// The resource bindings used when creating the pipeline must be compatible
    /// with the resources bound via BindResources().
    HGI_API
    virtual void BindPipeline(HgiGraphicsPipelineHandle pipeline) = 0;

    /// Bind resources such as textures and uniform buffers.
    /// Usually you call this right after BindPipeline() and the resources bound
    /// must be compatible with the bound pipeline.
    HGI_API
    virtual void BindResources(HgiResourceBindingsHandle resources) = 0;

    /// Set Push / Function constants.
    /// `pipeline` is the pipeline that you are binding before the draw call. It
    /// contains the program used for the uniform buffer
    /// `stages` describes for what shader stage you are setting the push
    /// constant values for. Each stage can have its own (or none) binding
    /// and they must match what is described in the shader functions.
    /// `bindIndex` is the binding point index in the pipeline's shader
    /// to bind the data to.
    /// `byteSize` is the size of the data you are updating.
    /// `data` is the data you are copying into the push constants block.
    HGI_API
    virtual void SetConstantValues(
        HgiGraphicsPipelineHandle pipeline,
        HgiShaderStage stages,
        uint32_t bindIndex,
        uint32_t byteSize,
        const void* data) = 0;

    /// Binds the vertex buffer(s) that describe the vertex attributes.
    /// `firstBinding` the first index to which buffers are bound (usually 0).
    /// `byteOffsets` offset to where the data of each buffer starts, in bytes.
    /// `strides` the size of a vertex in each of the buffers.
    HGI_API
    virtual void BindVertexBuffers(
        uint32_t firstBinding,
        HgiBufferHandleVector const& buffers,
        std::vector<uint32_t> const& byteOffsets) = 0;

    /// Records a draw command that renders one or more instances of primitives
    /// using the number of vertices provided.
    /// The 'primitive type' (eg. Lines, Triangles, etc) can be acquired from
    /// the bound HgiPipeline.
    /// `vertexCount`: The number of vertices to draw.
    /// `firstVertex`: The index of the first vertex to draw.
    /// `instanceCount`: Number of instances to draw.
    HGI_API
    virtual void Draw(
        uint32_t vertexCount,
        uint32_t firstVertex,
        uint32_t instanceCount) = 0;

    /// Records a multi-draw command that reads the draw parameters
    /// from a provided drawParameterBuffer.
    /// The 'primitive type' (eg. Lines, Triangles, etc) can be acquired from
    /// the bound HgiPipeline.
    /// `drawParameterBuffer`: an array of structures:
    //     struct IndirectCommand {
    //         uint32_t vertexCount;
    //         uint32_t instanceCount;
    //         uint32_t firstVertex;
    //         uint32_t firstInstance;
    //     }
    /// `drawBufferOffset`: Byte offset where the draw parameters begin.
    /// `drawCount`: The number of draws to execute.
    /// `stride`: byte stride between successive sets of draw parameters.
    HGI_API
    virtual void DrawIndirect(
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferOffset,
        uint32_t drawCount,
        uint32_t stride) = 0;

    /// Records a draw command that renders one or more instances of primitives
    /// using an indexBuffer starting from the base vertex.
    /// The 'primitive type' (eg. Lines, Triangles, etc) can be acquired from
    /// the bound HgiPipeline.
    /// `indexCount`: The number of indices in the index buffer (num vertices).
    /// `indexBufferByteOffset`: Byte offset within index buffer to start
    ///                          reading the indices from.
    /// `vertexOffset`: The value added to the vertex index before indexing
    ///                 into the vertex buffer (baseVertex).
    /// `instanceCount`: Number of instances to draw.
    HGI_API
    virtual void DrawIndexed(
        HgiBufferHandle const& indexBuffer,
        uint32_t indexCount,
        uint32_t indexBufferByteOffset,
        uint32_t vertexOffset,
        uint32_t instanceCount) = 0;

    /// Records a indexed multi-draw command that reads the draw parameters
    /// from a provided drawParameterBuffer, and indices from indexBuffer.
    /// The 'primitive type' (eg. Lines, Triangles, etc) can be acquired from
    /// the bound HgiPipeline.
    /// `drawParameterBuffer`: an array of structures:
    //     struct IndirectCommand {
    //         uint32_t indexCount;
    //         uint32_t instanceCount;
    //         uint32_t firstIndex;
    //         uint32_t vertexOffset;
    //         uint32_t firstInstance;
    //     }
    /// `drawBufferOffset`: Byte offset where the draw parameters begin.
    /// `drawCount`: The number of draws to execute.
    /// `stride`: byte stride between successive sets of draw parameters.
    HGI_API
    virtual void DrawIndexedIndirect(
        HgiBufferHandle const& indexBuffer,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferOffset,
        uint32_t drawCount,
        uint32_t stride) = 0;

    /// Inserts a barrier so that data written to memory by commands before
    /// the barrier is available to commands after the barrier.
    HGI_API
    virtual void MemoryBarrier(HgiMemoryBarrier barrier) = 0;

protected:
    HGI_API
    HgiGraphicsCmds();

private:
    HgiGraphicsCmds & operator=(const HgiGraphicsCmds&) = delete;
    HgiGraphicsCmds(const HgiGraphicsCmds&) = delete;
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif
