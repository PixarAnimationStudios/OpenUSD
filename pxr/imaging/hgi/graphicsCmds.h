//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    HGI_API
    virtual void BindVertexBuffers(
        HgiVertexBufferBindingVector const &bindings) = 0;

    /// Records a draw command that renders one or more instances of primitives
    /// using the number of vertices provided.
    /// The 'primitive type' (eg. Lines, Triangles, etc) can be acquired from
    /// the bound HgiPipeline.
    /// `vertexCount`: The number of vertices to draw.
    /// `baseVertex`: The index of the first vertex to draw.
    /// `instanceCount`: Number of instances to draw.
    /// `baseInstance`: The first instance to draw.
    HGI_API
    virtual void Draw(
        uint32_t vertexCount,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance) = 0;

    /// Records a multi-draw command that reads the draw parameters
    /// from a provided drawParameterBuffer.
    /// The 'primitive type' (eg. Lines, Triangles, etc) can be acquired from
    /// the bound HgiPipeline.
    /// `drawParameterBuffer`: an array of structures:
    //     struct IndirectCommand {
    //         uint32_t vertexCount;
    //         uint32_t instanceCount;
    //         uint32_t baseVertex;
    //         uint32_t baseInstance;
    //     }
    /// `drawBufferByteOffset`: Byte offset where the draw parameters begin.
    /// `drawCount`: The number of draws to execute.
    /// `stride`: byte stride between successive sets of draw parameters.
    HGI_API
    virtual void DrawIndirect(
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride) = 0;

    /// Records a draw command that renders one or more instances of primitives
    /// using an indexBuffer starting from the base vertex.
    /// The 'primitive type' (eg. Lines, Triangles, etc) can be acquired from
    /// the bound HgiPipeline.
    /// `indexCount`: The number of indices in the index buffer (num vertices).
    /// `indexBufferByteOffset`: Byte offset within index buffer to start
    ///                          reading the indices from.
    /// `baseVertex`: The value added to the vertex index before indexing
    ///                 into the vertex buffer (baseVertex).
    /// `instanceCount`: Number of instances to draw.
    /// `baseInstance`: The first instance to draw.
    HGI_API
    virtual void DrawIndexed(
        HgiBufferHandle const& indexBuffer,
        uint32_t indexCount,
        uint32_t indexBufferByteOffset,
        uint32_t baseVertex,
        uint32_t instanceCount,
        uint32_t baseInstance) = 0;

    /// Records a indexed multi-draw command that reads the draw parameters
    /// from a provided drawParameterBuffer, and indices from indexBuffer.
    /// The 'primitive type' (eg. Lines, Triangles, etc) can be acquired from
    /// the bound HgiPipeline.
    /// `drawParameterBuffer`: an array of structures (Metal has a different
    /// encoding of indirect commands for tessellated patches):
    //     struct IndirectCommand {
    //         uint32_t indexCount;
    //         uint32_t instanceCount;
    //         uint32_t baseIndex;
    //         uint32_t baseVertex;
    //         uint32_t baseInstance;
    //     }
    //     struct MetalPatchIndirectCommand {
    //         uint32_t patchCount;
    //         uint32_t instanceCount;
    //         uint32_t patchStart;
    //         uint32_t baseInstance;
    //     }
    /// `drawBufferByteOffset`: Byte offset where the draw parameters begin.
    /// `drawCount`: The number of draws to execute.
    /// `stride`: byte stride between successive sets of draw parameters.
    /// `drawParameterBufferUInt32`: CPU addressable `drawParameterBuffer`
    /// which contains the `baseVertex` offset needed for each patch draw
    /// for Metal.
    /// `patchBaseVertexByteOffset`: Byte offset to the uint32_t value
    /// in `drawParameterBufferUint32` which is the `baseVertex` value
    /// which must be applied to each HgiVertexBufferPerPatchControlPoint
    /// vertex buffer for each patch draw for Metal.
    HGI_API
    virtual void DrawIndexedIndirect(
        HgiBufferHandle const& indexBuffer,
        HgiBufferHandle const& drawParameterBuffer,
        uint32_t drawBufferByteOffset,
        uint32_t drawCount,
        uint32_t stride,
        std::vector<uint32_t> const& drawParameterBufferUInt32,
        uint32_t patchBaseVertexByteOffset) = 0;

    /// Inserts a barrier so that data written to memory by commands before
    /// the barrier is available to commands after the barrier.
    HGI_API
    virtual void InsertMemoryBarrier(HgiMemoryBarrier barrier) = 0;

protected:
    HGI_API
    HgiGraphicsCmds();

private:
    HgiGraphicsCmds & operator=(const HgiGraphicsCmds&) = delete;
    HgiGraphicsCmds(const HgiGraphicsCmds&) = delete;
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif
