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
#ifndef PXR_IMAGING_HGI_GRAPHICS_ENCODER_H
#define PXR_IMAGING_HGI_GRAPHICS_ENCODER_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/pipeline.h"
#include "pxr/imaging/hgi/resourceBindings.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HgiGraphicsEncoder
///
/// A graphics API independent abstraction of graphics commands.
/// HgiGraphicsEncoder is a lightweight object that cannot be re-used after
/// EndEncoding. A new encoder should be acquired from CommandBuffer each frame.
///
/// The API provided by this encoder should be agnostic to whether the
/// encoder operates via immediate or deferred command buffers.
///
class HgiGraphicsEncoder
{
public:
    HGI_API
    virtual ~HgiGraphicsEncoder();

    /// Finish recording of commands. No further commands can be recorded.
    HGI_API
    virtual void EndEncoding() = 0;

    /// Set viewport [left, BOTTOM, width, height] - OpenGL coords
    HGI_API
    virtual void SetViewport(GfVec4i const& vp) = 0;

    /// Only pixels that lie within the scissor box are modified by
    /// drawing commands.
    HGI_API
    virtual void SetScissor(GfVec4i const& sc) = 0;

    /// Bind a pipeline state object. Usually you call this right after calling
    /// CreateGraphicsEncoder to set the graphics pipeline state.
    /// The resource bindings used when creating the pipeline must be compatible
    /// with the resources bound via BindResources().
    HGI_API
    virtual void BindPipeline(HgiPipelineHandle pipeline) = 0;

    /// Bind resources such as textures and uniform buffers.
    /// Usually you call this right after BindPipeline() and the resources bound
    /// must be compatible with the bound pipeline.
    HGI_API
    virtual void BindResources(HgiResourceBindingsHandle resources) = 0;

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
    /// using an indexBuffer starting from the base vertex of the base instance.
    /// `indexCount` is the number of vertices.
    /// `indexBufferByteOffset`: Byte offset within indexBuffer to start reading
    ///                          indices from.
    /// `vertexOffset`: the value added to the vertex index before indexing into
    ///                 the vertex buffer (baseVertex).
    /// `instanceCount`: number of instances (min 1) of the primitves to render.
    HGI_API
    virtual void DrawIndexed(
        HgiBufferHandle const& indexBuffer,
        uint32_t indexCount,
        uint32_t indexBufferByteOffset,
        uint32_t firstIndex,
        uint32_t vertexOffset,
        uint32_t instanceCount) = 0;

    /// Push a debug marker onto the encoder.
    HGI_API
    virtual void PushDebugGroup(const char* label) = 0;

    /// Pop the lastest debug marker off encoder.
    HGI_API
    virtual void PopDebugGroup() = 0;

protected:
    HGI_API
    HgiGraphicsEncoder();

private:
    HgiGraphicsEncoder & operator=(const HgiGraphicsEncoder&) = delete;
    HgiGraphicsEncoder(const HgiGraphicsEncoder&) = delete;
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif
