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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgiGL/ops.h"
#include "pxr/imaging/hgiGL/buffer.h"
#include "pxr/imaging/hgiGL/computePipeline.h"
#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/graphicsCmds.h"
#include "pxr/imaging/hgiGL/graphicsPipeline.h"
#include "pxr/imaging/hgiGL/resourceBindings.h"
#include "pxr/imaging/hgiGL/shaderProgram.h"
#include "pxr/imaging/hgiGL/texture.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE


HgiGLOpsFn
HgiGLOps::PushDebugGroup(const char* label)
{
    // Make copy of string string since the lamda will execute later.
    std::string lbl = label;

    return [lbl] {
        #if defined(GL_KHR_debug)
        if (GARCH_GLAPI_HAS(KHR_debug)) {
            glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 0, -1, lbl.c_str());
        }
        #endif
    };
}

HgiGLOpsFn
HgiGLOps::PopDebugGroup()
{
    return [] {
        #if defined(GL_KHR_debug)
        if (GARCH_GLAPI_HAS(KHR_debug)) {
            glPopDebugGroup();
        }
        #endif
    };
}

HgiGLOpsFn
HgiGLOps::CopyTextureGpuToCpu(HgiTextureGpuToCpuOp const& copyOp)
{
    return [copyOp] {
        TRACE_SCOPE("HgiGLOps::CopyTextureGpuToCpu");

        HgiTextureHandle texHandle = copyOp.gpuSourceTexture;
        HgiGLTexture* srcTexture = static_cast<HgiGLTexture*>(texHandle.Get());

        if (!TF_VERIFY(srcTexture && srcTexture->GetTextureId(),
            "Invalid texture handle")) {
            return;
        }

        if (copyOp.destinationBufferByteSize == 0) {
            TF_WARN("The size of the data to copy was zero (aborted)");
            return;
        }

        HgiTextureDesc const& texDesc = srcTexture->GetDescriptor();

        GLenum glFormat = 0;
        GLenum glPixelType = 0;

        if (texDesc.usage & HgiTextureUsageBitsDepthTarget) {
            TF_VERIFY(texDesc.format == HgiFormatFloat32 ||
                      texDesc.format == HgiFormatFloat32UInt8);
            // XXX: Copy only the depth component. To copy stencil, we'd need
            // to set the format to GL_STENCIL_INDEX separately..
            glFormat = GL_DEPTH_COMPONENT;
            glPixelType = GL_FLOAT;
        } else if (texDesc.usage & HgiTextureUsageBitsStencilTarget) {
            TF_WARN("Copying a stencil-only texture is unsupported currently\n"
                   );
            return;
        } else {
            HgiGLConversions::GetFormat(
                texDesc.format,
                texDesc.usage,
                &glFormat,
                &glPixelType);
        }

        if (HgiIsCompressed(texDesc.format)) {
            TF_CODING_ERROR(
                "Copying from compressed GPU texture not supported.");
            return;
        }

        glGetTextureSubImage(
            srcTexture->GetTextureId(),
            copyOp.mipLevel,
            copyOp.sourceTexelOffset[0], // x offset
            copyOp.sourceTexelOffset[1], // y offset
            copyOp.sourceTexelOffset[2], // z offset (depth or layer)
            texDesc.dimensions[0], // width
            texDesc.dimensions[1], // height
            texDesc.dimensions[2], // layerCnt or depth
            glFormat,
            glPixelType,
            copyOp.destinationBufferByteSize,
            copyOp.cpuDestinationBuffer);

        HGIGL_POST_PENDING_GL_ERRORS();
    };
}

HgiGLOpsFn
HgiGLOps::CopyTextureCpuToGpu(HgiTextureCpuToGpuOp const& copyOp)
{
    return [copyOp] {
        TRACE_SCOPE("HgiGLOps::CopyTextureCpuToGpu");

        HgiTextureDesc const& desc =
            copyOp.gpuDestinationTexture->GetDescriptor();

        GLenum format = 0;
        GLenum type = 0;

        HgiGLConversions::GetFormat(desc.format,desc.usage,&format,&type);

        const bool isCompressed = HgiIsCompressed(desc.format);
        GfVec3i const& offsets = copyOp.destinationTexelOffset;
        GfVec3i const& dimensions = desc.dimensions;

        HgiGLTexture* dstTexture = static_cast<HgiGLTexture*>(
            copyOp.gpuDestinationTexture.Get());

        switch(desc.type) {
        case HgiTextureType2D:
            if (isCompressed) {
                glCompressedTextureSubImage2D(
                    dstTexture->GetTextureId(),
                    copyOp.mipLevel,
                    offsets[0], offsets[1],
                    dimensions[0], dimensions[1],
                    format,
                    copyOp.bufferByteSize,
                    copyOp.cpuSourceBuffer);
            } else {
                glTextureSubImage2D(
                    dstTexture->GetTextureId(),
                    copyOp.mipLevel,
                    offsets[0], offsets[1],
                    dimensions[0], dimensions[1],
                    format,
                    type,
                    copyOp.cpuSourceBuffer);
            }
            break;
        case HgiTextureType3D:
            if (isCompressed) {
                glCompressedTextureSubImage3D(
                    dstTexture->GetTextureId(),
                    copyOp.mipLevel,
                    offsets[0], offsets[1], offsets[2],
                    dimensions[0], dimensions[1], dimensions[2],
                    format,
                    copyOp.bufferByteSize,
                    copyOp.cpuSourceBuffer);
            } else {
                glTextureSubImage3D(
                    dstTexture->GetTextureId(),
                    copyOp.mipLevel,
                    offsets[0], offsets[1], offsets[2],
                    dimensions[0], dimensions[1], dimensions[2],
                    format,
                    type,
                    copyOp.cpuSourceBuffer);
            }
            break;
        default:
            TF_CODING_ERROR("Unsupported HgiTextureType enum value");
            break;
        }

        HGIGL_POST_PENDING_GL_ERRORS();
    };
}

HgiGLOpsFn
HgiGLOps::CopyBufferGpuToGpu(HgiBufferGpuToGpuOp const& copyOp)
{
    return [copyOp] {
        TRACE_SCOPE("HgiGLOps::CopyBufferGpuToGpu");

        HgiBufferHandle const& srcBufHandle = copyOp.gpuSourceBuffer;
        HgiGLBuffer* srcBuffer = static_cast<HgiGLBuffer*>(srcBufHandle.Get());

        if (!TF_VERIFY(srcBuffer && srcBuffer->GetBufferId(),
            "Invalid source buffer handle")) {
            return;
        }

        HgiBufferHandle const& dstBufHandle = copyOp.gpuDestinationBuffer;
        HgiGLBuffer* dstBuffer = static_cast<HgiGLBuffer*>(dstBufHandle.Get());

        if (!TF_VERIFY(dstBuffer && dstBuffer->GetBufferId(),
            "Invalid destination buffer handle")) {
            return;
        }

        if (copyOp.byteSize == 0) {
            TF_WARN("The size of the data to copy was zero (aborted)");
            return;
        }

        glCopyNamedBufferSubData(srcBuffer->GetBufferId(),
                                 dstBuffer->GetBufferId(),
                                 copyOp.sourceByteOffset,
                                 copyOp.destinationByteOffset,
                                 copyOp.byteSize);
        
        HGIGL_POST_PENDING_GL_ERRORS();
    };
}

HgiGLOpsFn 
HgiGLOps::CopyBufferCpuToGpu(HgiBufferCpuToGpuOp const& copyOp)
{
    return [copyOp] {
        TRACE_SCOPE("HgiGLOps::CopyBufferCpuToGpu");

        if (copyOp.byteSize == 0 ||
            !copyOp.cpuSourceBuffer ||
            !copyOp.gpuDestinationBuffer)
        {
            return;
        }

        HgiGLBuffer* glBuffer = static_cast<HgiGLBuffer*>(
            copyOp.gpuDestinationBuffer.Get());

        // Offset into the src buffer
        const char* src = ((const char*) copyOp.cpuSourceBuffer) +
            copyOp.sourceByteOffset;

        // Offset into the dst buffer
        GLintptr dstOffset = copyOp.destinationByteOffset;

        glNamedBufferSubData(
            glBuffer->GetBufferId(),
            dstOffset,
            copyOp.byteSize,
            src);

        HGIGL_POST_PENDING_GL_ERRORS();
    };
}

HgiGLOpsFn 
HgiGLOps::CopyBufferGpuToCpu(HgiBufferGpuToCpuOp const& copyOp)
{
    return [copyOp] {
        TRACE_SCOPE("HgiGLOps::CopyBufferGpuToCpu");

        if (copyOp.byteSize == 0 ||
            !copyOp.cpuDestinationBuffer ||
            !copyOp.gpuSourceBuffer)
        {
            return;
        }

        HgiGLBuffer* glBuffer = static_cast<HgiGLBuffer*>(
            copyOp.gpuSourceBuffer.Get());

        // Offset into the dst buffer
        const char* dst = ((const char*) copyOp.cpuDestinationBuffer) +
            copyOp.destinationByteOffset;

        // Offset into the src buffer
        GLintptr srcOffset = copyOp.sourceByteOffset;

        glGetNamedBufferSubData(
            glBuffer->GetBufferId(),
            srcOffset,
            copyOp.byteSize,
            (void*)dst);

        HGIGL_POST_PENDING_GL_ERRORS();
    };
}

HgiGLOpsFn 
HgiGLOps::CopyTextureToBuffer(HgiTextureToBufferOp const& copyOp)
{
    return [copyOp] {
        TRACE_SCOPE("HgiGLOps::CopyTextureToBuffer");

        HgiTextureHandle texHandle = copyOp.gpuSourceTexture;
        HgiGLTexture* srcTexture = static_cast<HgiGLTexture*>(texHandle.Get());

        if (!TF_VERIFY(srcTexture && srcTexture->GetTextureId(),
            "Invalid texture handle")) {
            return;
        }

        // There is no super efficient way of copying a texture region with an
        // offset to a PBO. Note that glGetTextureSubImage() does not work with
        // a bound PBO, so glGetTextureImage() is used instead, which does not
        // allow to specify an offset. Only the whole texture copy is supported
        // in HgiGL.
        if (copyOp.sourceTexelOffset != GfVec3i(0)) {
            TF_WARN("Texture offset not supported (aborted).");
            return;
        }
        
        HgiBufferHandle const& bufHandle = copyOp.gpuDestinationBuffer;
        HgiGLBuffer* dstBuffer = static_cast<HgiGLBuffer*>(bufHandle.Get());

        if (!TF_VERIFY(dstBuffer && dstBuffer->GetBufferId(),
            "Invalid destination buffer handle")) {
            return;
        }

        if (copyOp.byteSize == 0) {
            TF_WARN("The size of the data to copy was zero (aborted)");
            return;
        }

        HgiTextureDesc const& texDesc = srcTexture->GetDescriptor();

        // In a PBO transfer the pixels argument of glGetTextureImage() is
        // interpreted as the PBO byte offset.
        void* byteOffset = (void*)copyOp.destinationByteOffset;

        // Bind the buffer as a pixel packing PBO and transfer the data
        glBindBuffer(GL_PIXEL_PACK_BUFFER, dstBuffer->GetBufferId());
        if (HgiIsCompressed(texDesc.format)) {
            glGetCompressedTextureImage(srcTexture->GetTextureId(),
                                        copyOp.mipLevel,
                                        copyOp.byteSize,
                                        byteOffset);
        } else {
            GLenum format = 0;
            GLenum type = 0;
            HgiGLConversions::GetFormat(texDesc.format,
                                        texDesc.usage,
                                        &format,
                                        &type);
            glGetTextureImage(srcTexture->GetTextureId(),
                              copyOp.mipLevel,
                              format,
                              type,
                              copyOp.byteSize,
                              byteOffset);
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        HGIGL_POST_PENDING_GL_ERRORS();
    };
}

HgiGLOpsFn 
HgiGLOps::CopyBufferToTexture(HgiBufferToTextureOp const& copyOp)
{
     return [copyOp] {
        TRACE_SCOPE("HgiGLOps::CopyTextureToBuffer");

        HgiBufferHandle const& bufHandle = copyOp.gpuSourceBuffer;
        HgiGLBuffer* srcBuffer = static_cast<HgiGLBuffer*>(bufHandle.Get());

        if (!TF_VERIFY(srcBuffer && srcBuffer->GetBufferId(),
            "Invalid source buffer handle")) {
            return;
        }

        HgiTextureHandle texHandle = copyOp.gpuDestinationTexture;
        HgiGLTexture* dstTexture = static_cast<HgiGLTexture*>(texHandle.Get());

        if (!TF_VERIFY(dstTexture && dstTexture->GetTextureId(),
            "Invalid texture handle")) {
            return;
        }

        if (copyOp.byteSize == 0) {
            TF_WARN("The size of the data to copy was zero (aborted)");
            return;
        }

        HgiTextureDesc const& texDesc = dstTexture->GetDescriptor();

        GLenum format = 0;
        GLenum type = 0;

        HgiGLConversions::GetFormat(texDesc.format,
                                    texDesc.usage,
                                    &format,
                                    &type);

        const bool isCompressed = HgiIsCompressed(texDesc.format);
        GfVec3i const& offsets = copyOp.destinationTexelOffset;
        GfVec3i const& dimensions = texDesc.dimensions;

        // In a PBO transfer the pixels argument of glTextureSubImage*() and
        // glCompressedTextureSubImage*() is interpreted as the PBO byte offset.
        void* byteOffset = (void*)copyOp.sourceByteOffset;

        // Bind the buffer as a pixel unpacking PBO
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, srcBuffer->GetBufferId());

        switch(texDesc.type) {
        case HgiTextureType2D:
            if (isCompressed) {
                glCompressedTextureSubImage2D(
                    dstTexture->GetTextureId(),
                    copyOp.mipLevel,
                    offsets[0], offsets[1],
                    dimensions[0], dimensions[1],
                    format,
                    copyOp.byteSize,
                    byteOffset);
            } else {
                glTextureSubImage2D(
                    dstTexture->GetTextureId(),
                    copyOp.mipLevel,
                    offsets[0], offsets[1],
                    dimensions[0], dimensions[1],
                    format,
                    type,
                    byteOffset);
            }
            break;
        case HgiTextureType3D:
            if (isCompressed) {
                glCompressedTextureSubImage3D(
                    dstTexture->GetTextureId(),
                    copyOp.mipLevel,
                    offsets[0], offsets[1], offsets[2],
                    dimensions[0], dimensions[1], dimensions[2],
                    format,
                    copyOp.byteSize,
                    byteOffset);
            } else {
                glTextureSubImage3D(
                    dstTexture->GetTextureId(),
                    copyOp.mipLevel,
                    offsets[0], offsets[1], offsets[2],
                    dimensions[0], dimensions[1], dimensions[2],
                    format,
                    type,
                    byteOffset);
            }
            break;
        default:
            TF_CODING_ERROR("Unsupported HgiTextureType enum value");
            break;
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        HGIGL_POST_PENDING_GL_ERRORS();
    };
}

HgiGLOpsFn
HgiGLOps::SetViewport(GfVec4i const& vp)
{
    return [vp] {
        glViewport(vp[0], vp[1], vp[2], vp[3]);
    };
}

HgiGLOpsFn
HgiGLOps::SetScissor(GfVec4i const& sc)
{
    return [sc] {
        glScissor(sc[0], sc[1], sc[2], sc[3]);
    };
}

HgiGLOpsFn
HgiGLOps::BindPipeline(HgiGraphicsPipelineHandle pipeline)
{
    return [pipeline] {
        TRACE_SCOPE("HgiGLOps::BindPipeline");
        if (HgiGLGraphicsPipeline* p = static_cast<HgiGLGraphicsPipeline*>(pipeline.Get())) {
            p->BindPipeline();
        }
    };
}

HgiGLOpsFn
HgiGLOps::BindPipeline(HgiComputePipelineHandle pipeline)
{
    return [pipeline] {
        TRACE_SCOPE("HgiGLOps::BindPipeline");
        if (HgiGLComputePipeline* p = static_cast<HgiGLComputePipeline*>(pipeline.Get())) {
            p->BindPipeline();
        }
    };
}

HgiGLOpsFn
HgiGLOps::BindResources(HgiResourceBindingsHandle res)
{
    return [res] {
        TRACE_SCOPE("HgiGLOps::BindResources");
        if (HgiGLResourceBindings* rb =
            static_cast<HgiGLResourceBindings*>(res.Get()))
        {
            rb->BindResources();
        }
    };
}

HgiGLOpsFn
HgiGLOps::SetConstantValues(
    HgiGraphicsPipelineHandle pipeline,
    HgiShaderStage stages,
    uint32_t bindIndex,
    uint32_t byteSize,
    const void* data)
{
    // The data provided could be local stack memory that goes out of scope
    // before we execute this op. Make a copy to prevent that.
    uint8_t* dataCopy = new uint8_t[byteSize];
    memcpy(dataCopy, data, byteSize);

    return [pipeline, bindIndex, byteSize, dataCopy] {
        TRACE_SCOPE("HgiGLOps::SetConstantValues");
        HgiGLShaderProgram* glProgram =
            static_cast<HgiGLShaderProgram*>(
                pipeline->GetDescriptor().shaderProgram.Get());
        uint32_t ubo = glProgram->GetUniformBuffer(byteSize);
        glNamedBufferData(ubo, byteSize, dataCopy, GL_STATIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, bindIndex, ubo);
        delete[] dataCopy;
    };
}

HgiGLOpsFn
HgiGLOps::SetConstantValues(
    HgiComputePipelineHandle pipeline,
    uint32_t bindIndex,
    uint32_t byteSize,
    const void* data)
{
    // The data provided could be local stack memory that goes out of scope
    // before we execute this op. Make a copy to prevent that.
    uint8_t* dataCopy = new uint8_t[byteSize];
    memcpy(dataCopy, data, byteSize);

    return [pipeline, bindIndex, byteSize, dataCopy] {
        TRACE_SCOPE("HgiGLOps::SetConstantValues");
        HgiGLShaderProgram* glProgram =
            static_cast<HgiGLShaderProgram*>(
                pipeline->GetDescriptor().shaderProgram.Get());
        uint32_t ubo = glProgram->GetUniformBuffer(byteSize);
        glNamedBufferData(ubo, byteSize, dataCopy, GL_STATIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, bindIndex, ubo);
        delete[] dataCopy;
    };
}

HgiGLOpsFn
HgiGLOps::BindVertexBuffers(
    HgiVertexBufferBindingVector const &bindings)
{
    return [bindings] {
        TRACE_SCOPE("HgiGLOps::BindVertexBuffers");

        // XXX use glBindVertexBuffers to bind all VBs in one go.
        for (HgiVertexBufferBinding const &binding : bindings) {
            HgiGLBuffer* buf = static_cast<HgiGLBuffer*>(binding.buffer.Get());
            HgiBufferDesc const& desc = buf->GetDescriptor();

            TF_VERIFY(desc.usage & HgiBufferUsageVertex);

            glBindVertexBuffer(
                binding.index,
                buf->GetBufferId(),
                binding.byteOffset,
                desc.vertexStride);
        }

        HGIGL_POST_PENDING_GL_ERRORS();
    };
}

HgiGLOpsFn
HgiGLOps::Draw(
    HgiPrimitiveType primitiveType,
    uint32_t primitiveIndexSize,
    uint32_t vertexCount,
    uint32_t baseVertex,
    uint32_t instanceCount,
    uint32_t baseInstance)
{
    return [primitiveType, primitiveIndexSize,
            vertexCount, baseVertex, instanceCount, baseInstance] {
        TRACE_SCOPE("HgiGLOps::Draw");

        if (primitiveType == HgiPrimitiveTypePatchList) {
            glPatchParameteri(GL_PATCH_VERTICES, primitiveIndexSize);
        }

        glDrawArraysInstancedBaseInstance(
            HgiGLConversions::GetPrimitiveType(primitiveType),
            baseVertex,
            vertexCount,
            instanceCount,
            baseInstance);

        HGIGL_POST_PENDING_GL_ERRORS();
    };
}

HgiGLOpsFn
HgiGLOps::DrawIndirect(
    HgiPrimitiveType primitiveType,
    uint32_t primitiveIndexSize,
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferByteOffset,
    uint32_t drawCount,
    uint32_t stride)
{
    return [primitiveType, primitiveIndexSize,
            drawParameterBuffer, drawBufferByteOffset, drawCount, stride] {
        TRACE_SCOPE("HgiGLOps::DrawIndirect");

        HgiGLBuffer* drawBuf =
            static_cast<HgiGLBuffer*>(drawParameterBuffer.Get());

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawBuf->GetBufferId());

        if (primitiveType == HgiPrimitiveTypePatchList) {
            glPatchParameteri(GL_PATCH_VERTICES, primitiveIndexSize);
        }

        glMultiDrawArraysIndirect(
            HgiGLConversions::GetPrimitiveType(primitiveType),
            reinterpret_cast<const void*>(
                static_cast<uintptr_t>(drawBufferByteOffset)),
            drawCount,
            stride);

        HGIGL_POST_PENDING_GL_ERRORS();
    };
}

HgiGLOpsFn
HgiGLOps::DrawIndexed(
    HgiPrimitiveType primitiveType,
    uint32_t primitiveIndexSize,
    HgiBufferHandle const& indexBuffer,
    uint32_t indexCount,
    uint32_t indexBufferByteOffset,
    uint32_t baseVertex,
    uint32_t instanceCount,
    uint32_t baseInstance)
{
    return [primitiveType, primitiveIndexSize,
            indexBuffer, indexCount, indexBufferByteOffset,
            baseVertex, instanceCount, baseInstance] {
        TRACE_SCOPE("HgiGLOps::DrawIndexed");

        HgiGLBuffer* indexBuf = static_cast<HgiGLBuffer*>(indexBuffer.Get());

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf->GetBufferId());

        if (primitiveType == HgiPrimitiveTypePatchList) {
            glPatchParameteri(GL_PATCH_VERTICES, primitiveIndexSize);
        }

        glDrawElementsInstancedBaseVertexBaseInstance(
            HgiGLConversions::GetPrimitiveType(primitiveType),
            indexCount,
            GL_UNSIGNED_INT,
            reinterpret_cast<const void*>(
                static_cast<uintptr_t>(indexBufferByteOffset)),
            instanceCount,
            baseVertex,
            baseInstance);

        HGIGL_POST_PENDING_GL_ERRORS();
    };
}

HgiGLOpsFn
HgiGLOps::DrawIndexedIndirect(
    HgiPrimitiveType primitiveType,
    uint32_t primitiveIndexSize,
    HgiBufferHandle const& indexBuffer,
    HgiBufferHandle const& drawParameterBuffer,
    uint32_t drawBufferByteOffset,
    uint32_t drawCount,
    uint32_t stride)
{
    return [primitiveType, primitiveIndexSize,
            indexBuffer, drawParameterBuffer, drawBufferByteOffset,
            drawCount, stride] {
        TRACE_SCOPE("HgiGLOps::DrawIndexedIndirect");

        HgiGLBuffer* indexBuf = static_cast<HgiGLBuffer*>(indexBuffer.Get());

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf->GetBufferId());

        HgiGLBuffer* drawBuf =
            static_cast<HgiGLBuffer*>(drawParameterBuffer.Get());

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawBuf->GetBufferId());

        if (primitiveType == HgiPrimitiveTypePatchList) {
            glPatchParameteri(GL_PATCH_VERTICES, primitiveIndexSize);
        }

        glMultiDrawElementsIndirect(
            HgiGLConversions::GetPrimitiveType(primitiveType),
            GL_UNSIGNED_INT,
            reinterpret_cast<const void*>(
                static_cast<uintptr_t>(drawBufferByteOffset)),
            drawCount,
            stride);

        HGIGL_POST_PENDING_GL_ERRORS();
    };
}

HgiGLOpsFn
HgiGLOps::Dispatch(int dimX, int dimY)
{
    return [dimX, dimY] {
        TRACE_SCOPE("HgiGLOps::Dispatch");

        glDispatchCompute(dimX, dimY, 1);

        HGIGL_POST_PENDING_GL_ERRORS();
    };
}

HgiGLOpsFn
HgiGLOps::BindFramebufferOp(
    HgiGLDevice* device,
    HgiGraphicsCmdsDesc const& desc)
{
    return [device, desc] {
        TRACE_SCOPE("HgiGLOps::BindFramebufferOp");

        TF_VERIFY(desc.HasAttachments(), "Missing attachments");

        uint32_t framebuffer = device->AcquireFramebuffer(desc);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glEnable(GL_FRAMEBUFFER_SRGB);

        bool blendEnabled = false;

        // Apply LoadOps and blend mode
        for (size_t i=0; i<desc.colorAttachmentDescs.size(); i++) {
            HgiAttachmentDesc const& colorAttachment =
                desc.colorAttachmentDescs[i];

            if (colorAttachment.format == HgiFormatInvalid) {
                TF_CODING_ERROR(
                    "Binding framebuffer with invalid format "
                    "for color attachment %zu.", i);
            }

            if (colorAttachment.loadOp == HgiAttachmentLoadOpClear) {
                glClearBufferfv(GL_COLOR, i, colorAttachment.clearValue.data());
            }

            blendEnabled |= colorAttachment.blendEnabled;

            GLenum srcColor = HgiGLConversions::GetBlendFactor(
                colorAttachment.srcColorBlendFactor);
            GLenum dstColor = HgiGLConversions::GetBlendFactor(
                colorAttachment.dstColorBlendFactor);

            GLenum srcAlpha = HgiGLConversions::GetBlendFactor(
                colorAttachment.srcAlphaBlendFactor);
            GLenum dstAlpha = HgiGLConversions::GetBlendFactor(
                colorAttachment.dstAlphaBlendFactor);

            GLenum colorOp = HgiGLConversions::GetBlendEquation(
                colorAttachment.colorBlendOp);
            GLenum alphaOp = HgiGLConversions::GetBlendEquation(
                colorAttachment.alphaBlendOp);

            glBlendFuncSeparatei(i, srcColor, dstColor, srcAlpha, dstAlpha);
            glBlendEquationSeparatei(i, colorOp, alphaOp);
            glBlendColor(colorAttachment.blendConstantColor[0],
                         colorAttachment.blendConstantColor[1],
                         colorAttachment.blendConstantColor[2],
                         colorAttachment.blendConstantColor[3]);
        }

        HgiAttachmentDesc const& depthAttachment =
            desc.depthAttachmentDesc;

        if (desc.depthTexture) {
            if (depthAttachment.format == HgiFormatInvalid) {
                TF_CODING_ERROR(
                    "Binding framebuffer with invalid format "
                    "for depth attachment.");
            }
        }

        if (desc.depthTexture &&
            depthAttachment.loadOp == HgiAttachmentLoadOpClear) {
            if (depthAttachment.usage & HgiTextureUsageBitsStencilTarget) {
                glClearBufferfi(
                    GL_DEPTH_STENCIL,
                    0,
                    depthAttachment.clearValue[0],
                    static_cast<uint32_t>(depthAttachment.clearValue[1]));
            } else {
                glClearBufferfv(
                    GL_DEPTH,
                    0,
                    depthAttachment.clearValue.data());
            }
        }

        // Setup blending
        if (blendEnabled) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }

        HGIGL_POST_PENDING_GL_ERRORS();
    };
}

HgiGLOpsFn
HgiGLOps::FillBuffer(HgiBufferHandle const& buffer, uint8_t value)
{
    return [buffer, value] {
        TRACE_SCOPE("HgiGLOps::FillBuffer");

        HgiGLBuffer* glBuffer = static_cast<HgiGLBuffer*>(buffer.Get());
        if (glBuffer && glBuffer->GetBufferId()) {
            glClearNamedBufferData(glBuffer->GetBufferId(),
                                   GL_R8UI,
                                   GL_RED_INTEGER,
                                   GL_UNSIGNED_BYTE,
                                   &value);
            HGIGL_POST_PENDING_GL_ERRORS();
        }
    };
}

HgiGLOpsFn
HgiGLOps::GenerateMipMaps(HgiTextureHandle const& texture)
{
    return [texture] {
        TRACE_SCOPE("HgiGLOps::GenerateMipMaps");

        HgiGLTexture* glTex = static_cast<HgiGLTexture*>(texture.Get());
        if (glTex && glTex->GetTextureId()) {
            glGenerateTextureMipmap(glTex->GetTextureId());
            HGIGL_POST_PENDING_GL_ERRORS();
        }
    };
}

HgiGLOpsFn
HgiGLOps::ResolveFramebuffer(
    HgiGLDevice* device,
    HgiGraphicsCmdsDesc const &graphicsCmds)
{
    return [device, graphicsCmds] {
        TRACE_SCOPE("HgiGLOps::ResolveFramebuffer");

        const uint32_t resolvedFramebuffer = device->AcquireFramebuffer(
            graphicsCmds, /* resolved = */ true);
        if (!resolvedFramebuffer) {
            return;
        }

        const uint32_t framebuffer = device->AcquireFramebuffer(
            graphicsCmds);

        GfVec3i dim(0);
        GLbitfield mask = 0;
        size_t numResolvesRequired = 0;
        if (!graphicsCmds.colorResolveTextures.empty()) {
            mask |= GL_COLOR_BUFFER_BIT;
            HgiTextureHandle const& tex = 
                graphicsCmds.colorResolveTextures.front();
            dim = tex->GetDescriptor().dimensions;
            numResolvesRequired = graphicsCmds.colorTextures.size();
        }
        if (graphicsCmds.depthResolveTexture) {
            mask |= GL_DEPTH_BUFFER_BIT;
            dim = graphicsCmds.depthResolveTexture->GetDescriptor().dimensions;
            numResolvesRequired = std::max<size_t>(1, numResolvesRequired);
        }

        // glBlitFramebuffer transfers the contents of the read buffer in the 
        // read fbo to *all* the draw buffers in the draw fbo.
        // In order to transfer to the contents of each color attachment to
        // the corresponding resolved attachment, we need to manipulate the
        // read and draw buffer accordingly.
        // See https://www.khronos.org/opengl/wiki/Framebuffer#Blitting
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolvedFramebuffer);
        glEnable(GL_FRAMEBUFFER_SRGB);
        GLint restoreReadBuffer;
        glGetIntegerv(GL_READ_BUFFER, &restoreReadBuffer);
        GLint restoreDrawBuffer;
        glGetIntegerv(GL_DRAW_BUFFER, &restoreDrawBuffer);
        
        for (size_t i = 0; i < numResolvesRequired; i++) {
            glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
            glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
            glBlitFramebuffer(0, 0, dim[0], dim[1],
                            0, 0, dim[0], dim[1],
                            /* resolve depth buffer just the once */
                            i == 0 ? mask : mask & ~GL_DEPTH_BUFFER_BIT,
                            GL_NEAREST);
        }
        glReadBuffer(restoreReadBuffer);
        glDrawBuffer(restoreDrawBuffer);
    };
}

HgiGLOpsFn
HgiGLOps::InsertMemoryBarrier(HgiMemoryBarrier barrier)
{
    return [barrier] {
        if (TF_VERIFY(barrier == HgiMemoryBarrierAll)) {
            glMemoryBarrier(GL_ALL_BARRIER_BITS);
        }
    };
}

PXR_NAMESPACE_CLOSE_SCOPE
