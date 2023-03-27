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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgiGL/buffer.h"
#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/device.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/graphicsCmds.h"
#include "pxr/imaging/hgiGL/hgi.h"
#include "pxr/imaging/hgiGL/ops.h"
#include "pxr/imaging/hgiGL/graphicsPipeline.h"
#include "pxr/imaging/hgiGL/resourceBindings.h"
#include "pxr/imaging/hgiGL/scopedStateHolder.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLGraphicsCmds::HgiGLGraphicsCmds(
    HgiGLDevice* device,
    HgiGraphicsCmdsDesc const& desc)
    : HgiGraphicsCmds()
    , _recording(true)
    , _descriptor(desc)
    , _primitiveType(HgiPrimitiveTypeTriangleList)
    , _pushStack(0)
    , _restoreReadFramebuffer(0)
    , _restoreDrawFramebuffer(0)
{
    if (desc.HasAttachments()) {
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &_restoreReadFramebuffer);    
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &_restoreDrawFramebuffer);
        _ops.push_back( HgiGLOps::BindFramebufferOp(device, desc) );
    }
}

static bool
_IsValidFbo(int32_t id)
{
    return id == 0 || glIsFramebuffer(id) == GL_TRUE;
}

HgiGLGraphicsCmds::~HgiGLGraphicsCmds()
{
    if (_descriptor.HasAttachments()) {
        // Restore framebuffer state.
        if (_IsValidFbo(_restoreReadFramebuffer)) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, _restoreReadFramebuffer);
        }
        if (_IsValidFbo(_restoreDrawFramebuffer)) {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _restoreDrawFramebuffer);
        }
    }
}

void
HgiGLGraphicsCmds::InsertFunctionOp(std::function<void(void)> const& fn)
{
    _ops.push_back( fn );
}

void
HgiGLGraphicsCmds::SetViewport(GfVec4i const& vp)
{
    _ops.push_back( HgiGLOps::SetViewport(vp) );
}

void
HgiGLGraphicsCmds::SetScissor(GfVec4i const& sc)
{
    _ops.push_back( HgiGLOps::SetScissor(sc) );
}

void
HgiGLGraphicsCmds::BindPipeline(HgiGraphicsPipelineHandle pipeline)
{
    _primitiveType = pipeline->GetDescriptor().primitiveType;
    _primitiveIndexSize =
        pipeline->GetDescriptor().tessellationState.primitiveIndexSize;
    _ops.push_back( HgiGLOps::BindPipeline(pipeline) );
}

void
HgiGLGraphicsCmds::BindResources(HgiResourceBindingsHandle res)
{
    _ops.push_back( HgiGLOps::BindResources(res) );
}

void
HgiGLGraphicsCmds::SetConstantValues(
    HgiGraphicsPipelineHandle pipeline,
    HgiShaderStage stages,
    uint32_t bindIndex,
    uint32_t byteSize,
    const void* data)
{
    _ops.push_back(
        HgiGLOps::SetConstantValues(
            pipeline,
            stages,
            bindIndex,
            byteSize,
            data)
        );
}

void
HgiGLGraphicsCmds::BindVertexBuffers(
    HgiVertexBufferBindingVector const &bindings)
{
    _ops.push_back( 
        HgiGLOps::BindVertexBuffers(bindings) );
}

void
HgiGLGraphicsCmds::Draw(
    uint32_t vertexCount,
    uint32_t baseVertex,
    uint32_t instanceCount,
    uint32_t baseInstance)
{
    _ops.push_back(
        HgiGLOps::Draw(
            _primitiveType,
            _primitiveIndexSize,
            vertexCount,
            baseVertex,
            instanceCount,
            baseInstance)
        );
}

void
HgiGLGraphicsCmds::DrawIndirect(
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferByteOffset,
    uint32_t drawCount,
    uint32_t stride)
{
    _ops.push_back(
        HgiGLOps::DrawIndirect(
            _primitiveType,
            _primitiveIndexSize,
            drawParameterBuffer,
            drawBufferByteOffset,
            drawCount,
            stride)
        );
}

void
HgiGLGraphicsCmds::DrawIndexed(
    HgiBufferHandle const& indexBuffer,
    uint32_t indexCount,
    uint32_t indexBufferByteOffset,
    uint32_t baseVertex,
    uint32_t instanceCount,
    uint32_t baseInstance)
{
    _ops.push_back(
        HgiGLOps::DrawIndexed(
            _primitiveType,
            _primitiveIndexSize,
            indexBuffer,
            indexCount,
            indexBufferByteOffset,
            baseVertex,
            instanceCount,
            baseInstance)
        );
}

void
HgiGLGraphicsCmds::DrawIndexedIndirect(
    HgiBufferHandle const& indexBuffer,
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferByteOffset,
    uint32_t drawCount,
    uint32_t stride,
    std::vector<uint32_t> const& /*drawParameterBufferUInt32*/,
    uint32_t /*patchBaseVertexByteOffset*/)
{
    _ops.push_back(
        HgiGLOps::DrawIndexedIndirect(
            _primitiveType,
            _primitiveIndexSize,
            indexBuffer,
            drawParameterBuffer,
            drawBufferByteOffset,
            drawCount,
            stride)
        );
}

void
HgiGLGraphicsCmds::PushDebugGroup(const char* label)
{
    if (HgiGLDebugEnabled()) {
        _pushStack++;
        _ops.push_back( HgiGLOps::PushDebugGroup(label) );
    }
}

void
HgiGLGraphicsCmds::PopDebugGroup()
{
    if (HgiGLDebugEnabled()) {
        _pushStack--;
        _ops.push_back( HgiGLOps::PopDebugGroup() );
    }
}

void
HgiGLGraphicsCmds::InsertMemoryBarrier(HgiMemoryBarrier barrier)
{
    _ops.push_back( HgiGLOps::InsertMemoryBarrier(barrier) );
}

bool
HgiGLGraphicsCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    if (_ops.empty()) {
        return false;
    }

    TF_VERIFY(_pushStack==0, "Push and PopDebugGroup do not even out");

    // Capture OpenGL state before executing the 'ops' and restore it when this
    // function ends. We do this defensively because parts of our pipeline may
    // not set and restore all relevant gl state.
    HgiGL_ScopedStateHolder openglStateGuard;

    // Resolve multisample textures
    HgiGL* hgiGL = static_cast<HgiGL*>(hgi);
    HgiGLDevice* device = hgiGL->GetPrimaryDevice();
    _AddResolveToOps(device);

    device->SubmitOps(_ops);
    return true;
}

void
HgiGLGraphicsCmds::_AddResolveToOps(HgiGLDevice *device)
{
    if (!_recording) {
        return;
    }

    if (!_descriptor.colorResolveTextures.empty() && 
            _descriptor.colorResolveTextures.size() != 
                _descriptor.colorTextures.size()) {
        TF_CODING_ERROR("color and resolve texture count mismatch.");
        return;
    }

    if (_descriptor.depthResolveTexture && !_descriptor.depthTexture) {
        TF_CODING_ERROR("DepthResolve texture without depth texture.");
        return;
    }

    if ((!_descriptor.colorResolveTextures.empty()) ||
        _descriptor.depthResolveTexture) {
        // At the end of the GraphicsCmd we resolve the multisample
        // textures.  This emulates what happens in Metal or Vulkan
        // when the multisample resolve happens at the end of a render
        // pass.
        _ops.push_back(HgiGLOps::ResolveFramebuffer(device, _descriptor));
    }

    _recording = false;
}

PXR_NAMESPACE_CLOSE_SCOPE
