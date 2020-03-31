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
#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/device.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/graphicsEncoder.h"
#include "pxr/imaging/hgiGL/ops.h"
#include "pxr/imaging/hgiGL/pipeline.h"
#include "pxr/imaging/hgiGL/resourceBindings.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLGraphicsEncoder::HgiGLGraphicsEncoder(
    HgiGLDevice* device,
    HgiGraphicsEncoderDesc const& desc)
    : HgiGraphicsEncoder()
    , _committed(false)
{
    if (desc.HasAttachments()) {
        _ops.push_back( HgiGLOps::BindFramebufferOp(device, desc) );
    }
}

HgiGLGraphicsEncoder::~HgiGLGraphicsEncoder()
{
    TF_VERIFY(_committed, "Encoder created, but never commited.");
}

void
HgiGLGraphicsEncoder::InsertFunctionOp(std::function<void(void)> const& fn)
{
    _ops.push_back( fn );
}

void
HgiGLGraphicsEncoder::Commit()
{
    if (!_committed) {
        _committed = true;
        HgiGLDevice::Commit(_ops);
    }
}

void
HgiGLGraphicsEncoder::SetViewport(GfVec4i const& vp)
{
    _ops.push_back( HgiGLOps::SetViewport(vp) );
}

void
HgiGLGraphicsEncoder::SetScissor(GfVec4i const& sc)
{
    _ops.push_back( HgiGLOps::SetScissor(sc) );
}

void
HgiGLGraphicsEncoder::BindPipeline(HgiPipelineHandle pipeline)
{
    _ops.push_back( HgiGLOps::BindPipeline(pipeline) );
}

void
HgiGLGraphicsEncoder::BindResources(HgiResourceBindingsHandle res)
{
    _ops.push_back( HgiGLOps::BindResources(res) );
}

void
HgiGLGraphicsEncoder::BindVertexBuffers(
    uint32_t firstBinding,
    HgiBufferHandleVector const& vertexBuffers,
    std::vector<uint32_t> const& byteOffsets)
{
    _ops.push_back( 
        HgiGLOps::BindVertexBuffers(firstBinding, vertexBuffers, byteOffsets) );
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
    _ops.push_back(
        HgiGLOps::DrawIndexed(
            indexBuffer,
            indexCount,
            indexBufferByteOffset,
            vertexOffset,
            instanceCount,
            firstInstance)
        );
}

void
HgiGLGraphicsEncoder::PushDebugGroup(const char* label)
{
    _ops.push_back( HgiGLOps::PushDebugGroup(label) );
}

void
HgiGLGraphicsEncoder::PopDebugGroup()
{
    _ops.push_back( HgiGLOps::PopDebugGroup() );
}


PXR_NAMESPACE_CLOSE_SCOPE
