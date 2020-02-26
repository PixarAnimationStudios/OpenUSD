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
#include <GL/glew.h>
#include "pxr/imaging/hgi/graphicsEncoderDesc.h"
#include "pxr/imaging/hgiGL/buffer.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/graphicsEncoder.h"
#include "pxr/imaging/hgiGL/pipeline.h"
#include "pxr/imaging/hgiGL/resourceBindings.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLGraphicsEncoder::HgiGLGraphicsEncoder(
    HgiGraphicsEncoderDesc const& desc)
    : HgiGraphicsEncoder()
{
    TF_VERIFY(desc.width>0 && desc.height>0);
}

HgiGLGraphicsEncoder::~HgiGLGraphicsEncoder()
{
}

void
HgiGLGraphicsEncoder::EndEncoding()
{
}

void
HgiGLGraphicsEncoder::SetViewport(GfVec4i const& vp)
{
    glViewport(vp[0], vp[1], vp[2], vp[3]);
}

void
HgiGLGraphicsEncoder::SetScissor(GfVec4i const& sc)
{
    glScissor(sc[0], sc[1], sc[2], sc[3]);
}

void
HgiGLGraphicsEncoder::BindPipeline(HgiPipelineHandle pipeline)
{
    if (HgiGLPipeline* p = static_cast<HgiGLPipeline*>(pipeline.Get())) {
        p->BindPipeline();
    }
}

void
HgiGLGraphicsEncoder::BindResources(HgiResourceBindingsHandle r)
{
    if (HgiGLResourceBindings* rb= static_cast<HgiGLResourceBindings*>(r.Get())) 
    {
        rb->BindResources();
    }
}

void
HgiGLGraphicsEncoder::BindVertexBuffers(
    uint32_t firstBinding,
    HgiBufferHandleVector const& vertexBuffers,
    std::vector<uint32_t> const& byteOffsets)
{
    TF_VERIFY(byteOffsets.size() == vertexBuffers.size());
    TF_VERIFY(byteOffsets.size() == vertexBuffers.size());

    for (size_t i=0; i<vertexBuffers.size(); i++) {
        HgiBufferHandle bufHandle = vertexBuffers[i];
        HgiGLBuffer* buf = static_cast<HgiGLBuffer*>(bufHandle.Get());
        HgiBufferDesc const& desc = buf->GetDescriptor();

        TF_VERIFY(desc.usage & HgiBufferUsageVertex);

        glBindVertexBuffer(
            firstBinding + i,
            buf->GetBufferId(),
            byteOffsets[i], 
            desc.vertexStride);
    }

    HGIGL_POST_PENDING_GL_ERRORS();
}

void
HgiGLGraphicsEncoder::DrawIndexed(
    HgiBufferHandle const& indexBuffer,
    uint32_t indexCount,
    uint32_t indexBufferByteOffset,
    uint32_t vertexOffset,
    uint32_t instanceCount,
    uint32_t firstInstance)
{
    TF_VERIFY(instanceCount>0);

    HgiGLBuffer* indexBuf = static_cast<HgiGLBuffer*>(indexBuffer.Get());
    HgiBufferDesc const& indexDesc = indexBuf->GetDescriptor();

    // We assume 32bit indices: GL_UNSIGNED_INT
    TF_VERIFY(indexDesc.usage & HgiBufferUsageIndex32);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf->GetBufferId());

    glDrawElementsInstancedBaseVertex(
        GL_TRIANGLES, // XXX GL_PATCHES for tessellation
        indexCount,
        GL_UNSIGNED_INT,
        (void*)(uintptr_t(indexBufferByteOffset)),
        instanceCount,
        vertexOffset);

    HGIGL_POST_PENDING_GL_ERRORS();
}

void
HgiGLGraphicsEncoder::PushDebugGroup(const char* label)
{
    #if defined(GL_KHR_debug)
        if (GLEW_KHR_debug) {
            glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 0, -1, label);
        }

    #endif
}

void
HgiGLGraphicsEncoder::PopDebugGroup()
{
    #if defined(GL_KHR_debug)
        if (GLEW_KHR_debug) {
            glPopDebugGroup();
        }
    #endif
}


PXR_NAMESPACE_CLOSE_SCOPE
