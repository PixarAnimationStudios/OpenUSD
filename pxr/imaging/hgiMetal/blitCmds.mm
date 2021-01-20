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
#include <Metal/Metal.h>

#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/blitCmds.h"
#include "pxr/imaging/hgiMetal/buffer.h"
#include "pxr/imaging/hgiMetal/capabilities.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/texture.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/types.h"

#include "pxr/base/arch/pragmas.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalBlitCmds::HgiMetalBlitCmds(HgiMetal *hgi)
    : _hgi(hgi)
    , _commandBuffer(nil)
    , _blitEncoder(nil)
    , _label(nil)
    , _secondaryCommandBuffer(false)
{
}

HgiMetalBlitCmds::~HgiMetalBlitCmds()
{
    TF_VERIFY(_blitEncoder == nil, "Encoder created, but never commited.");
    
    if (_label) {
        [_label release];
        _label = nil;
    }
}

void
HgiMetalBlitCmds::_CreateEncoder()
{
    if (!_blitEncoder) {
        _commandBuffer = _hgi->GetPrimaryCommandBuffer();
        if (_commandBuffer == nil) {
            _commandBuffer = _hgi->GetSecondaryCommandBuffer();
            _secondaryCommandBuffer = true;
        }
        _blitEncoder = [_commandBuffer blitCommandEncoder];

        if (_label) {
            if (HgiMetalDebugEnabled()) {
                _blitEncoder.label = _label;
            }
        }
    }
}

void
HgiMetalBlitCmds::PushDebugGroup(const char* label)
{
    if (_blitEncoder) {
        HGIMETAL_DEBUG_LABEL(_blitEncoder, label)
    }
    else if (HgiMetalDebugEnabled()) {
        _label = [@(label) copy];
    }
}

void
HgiMetalBlitCmds::PopDebugGroup()
{
}

void 
HgiMetalBlitCmds::CopyTextureGpuToCpu(
    HgiTextureGpuToCpuOp const& copyOp)
{
    HgiTextureHandle texHandle = copyOp.gpuSourceTexture;
    HgiMetalTexture* srcTexture =static_cast<HgiMetalTexture*>(texHandle.Get());

    if (!TF_VERIFY(srcTexture && srcTexture->GetTextureId(),
        "Invalid texture handle")) {
        return;
    }

    if (copyOp.destinationBufferByteSize == 0) {
        TF_WARN("The size of the data to copy was zero (aborted)");
        return;
    }

    HgiTextureDesc const& texDesc = srcTexture->GetDescriptor();

    MTLPixelFormat metalFormat = MTLPixelFormatInvalid;

    if (texDesc.usage & HgiTextureUsageBitsColorTarget) {
        metalFormat = HgiMetalConversions::GetPixelFormat(texDesc.format);
    } else if (texDesc.usage & HgiTextureUsageBitsDepthTarget) {
        TF_VERIFY(texDesc.format == HgiFormatFloat32);
        metalFormat = MTLPixelFormatDepth32Float;
    } else {
        TF_CODING_ERROR("Unknown HgTextureUsage bit");
    }

    id<MTLDevice> device = _hgi->GetPrimaryDevice();

    MTLResourceOptions options =
        _hgi->GetCapabilities().defaultStorageMode;

    size_t bytesPerPixel = HgiGetDataSizeOfFormat(texDesc.format);
    id<MTLBuffer> cpuBuffer =
        [device newBufferWithBytesNoCopy:copyOp.cpuDestinationBuffer
                                  length:copyOp.destinationBufferByteSize
                                 options:options
                             deallocator:nil];

    bool isTexArray = texDesc.layerCount>1;
    int depthOffset = isTexArray ? 0 : copyOp.sourceTexelOffset[2];

    MTLOrigin origin = MTLOriginMake(
        copyOp.sourceTexelOffset[0],
        copyOp.sourceTexelOffset[1],
        depthOffset);
    MTLSize size = MTLSizeMake(
        texDesc.dimensions[0] - copyOp.sourceTexelOffset[0],
        texDesc.dimensions[1] - copyOp.sourceTexelOffset[1],
        texDesc.dimensions[2] - depthOffset);
    
    MTLBlitOption blitOptions = MTLBlitOptionNone;

    _CreateEncoder();

    [_blitEncoder copyFromTexture:srcTexture->GetTextureId()
                      sourceSlice:isTexArray ? copyOp.sourceTexelOffset[2] : 0
                      sourceLevel:copyOp.mipLevel
                     sourceOrigin:origin
                       sourceSize:size
                         toBuffer:cpuBuffer
                destinationOffset:0
           destinationBytesPerRow:(bytesPerPixel * texDesc.dimensions[0])
         destinationBytesPerImage:(bytesPerPixel * texDesc.dimensions[0] *
                                   texDesc.dimensions[1])
                          options:blitOptions];

    if (@available(macOS 10.11, ios 100.100, *)) {
        [_blitEncoder performSelector:@selector(synchronizeResource:)
                           withObject:cpuBuffer];
    }
    
    // Offset into the dst buffer
    char* dst = ((char*) copyOp.cpuDestinationBuffer) +
        copyOp.destinationByteOffset;
    
    // Offset into the src buffer
    const char* src = (const char*) [cpuBuffer contents];

    // bytes to copy
    size_t byteSize = copyOp.destinationBufferByteSize;

    [_commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
        {
            memcpy(dst, src, byteSize);
        }];
    [cpuBuffer release];
}

void
HgiMetalBlitCmds::CopyTextureCpuToGpu(
    HgiTextureCpuToGpuOp const& copyOp)
{
    HgiMetalTexture* dstTexture = static_cast<HgiMetalTexture*>(
        copyOp.gpuDestinationTexture.Get());
    HgiTextureDesc const& texDesc = dstTexture->GetDescriptor();

    const size_t width = texDesc.dimensions[0];
    const size_t height = texDesc.dimensions[1];
    const size_t depth = texDesc.dimensions[2];

    bool isTexArray = texDesc.layerCount>1;

    GfVec3i const& offsets = copyOp.destinationTexelOffset;
    int depthOffset = isTexArray ? 0 : offsets[2];
    if (texDesc.type == HgiTextureType1D) {
        [dstTexture->GetTextureId()
            replaceRegion:MTLRegionMake1D(offsets[0], width)
              mipmapLevel:copyOp.mipLevel
                withBytes:copyOp.cpuSourceBuffer
              bytesPerRow:copyOp.bufferByteSize];
    } else if (texDesc.type == HgiTextureType2D) {
        [dstTexture->GetTextureId()
            replaceRegion:MTLRegionMake2D(
                offsets[0], offsets[1], width, height)
              mipmapLevel:copyOp.mipLevel
                withBytes:copyOp.cpuSourceBuffer
              bytesPerRow:copyOp.bufferByteSize / height];
    } else {
        [dstTexture->GetTextureId()
            replaceRegion:MTLRegionMake3D(
                offsets[0], offsets[1], depthOffset, width, height, depth)
              mipmapLevel:copyOp.mipLevel 
                    slice:isTexArray ? offsets[2] : 0
                withBytes:copyOp.cpuSourceBuffer
              bytesPerRow:copyOp.bufferByteSize / height / width
            bytesPerImage:copyOp.bufferByteSize / depth];
    }
}

void
HgiMetalBlitCmds::CopyBufferGpuToGpu(
    HgiBufferGpuToGpuOp const& copyOp)
{
    HgiBufferHandle const& srcBufHandle = copyOp.gpuSourceBuffer;
    HgiMetalBuffer* srcBuffer =static_cast<HgiMetalBuffer*>(srcBufHandle.Get());

    if (!TF_VERIFY(srcBuffer && srcBuffer->GetBufferId(),
        "Invalid source buffer handle")) {
        return;
    }

    HgiBufferHandle const& dstBufHandle = copyOp.gpuDestinationBuffer;
    HgiMetalBuffer* dstBuffer =static_cast<HgiMetalBuffer*>(dstBufHandle.Get());

    if (!TF_VERIFY(dstBuffer && dstBuffer->GetBufferId(),
        "Invalid destination buffer handle")) {
        return;
    }

    if (copyOp.byteSize == 0) {
        TF_WARN("The size of the data to copy was zero (aborted)");
        return;
    }

    _CreateEncoder();

    [_blitEncoder copyFromBuffer:srcBuffer->GetBufferId()
                    sourceOffset:copyOp.sourceByteOffset
                        toBuffer:dstBuffer->GetBufferId()
               destinationOffset:copyOp.destinationByteOffset
                            size:copyOp.byteSize];
}

void HgiMetalBlitCmds::CopyBufferCpuToGpu(
    HgiBufferCpuToGpuOp const& copyOp)
{
    if (copyOp.byteSize == 0 ||
        !copyOp.cpuSourceBuffer ||
        !copyOp.gpuDestinationBuffer)
    {
        return;
    }

    HgiMetalBuffer* metalBuffer = static_cast<HgiMetalBuffer*>(
        copyOp.gpuDestinationBuffer.Get());
    bool sharedBuffer =
        [metalBuffer->GetBufferId() storageMode] == MTLStorageModeShared;

    uint8_t *dst = static_cast<uint8_t*>([metalBuffer->GetBufferId() contents]);
    size_t dstOffset = copyOp.destinationByteOffset;

    // If we used GetCPUStagingAddress as the cpuSourceBuffer when the copyOp
    // was created, we can skip the memcpy since the src and dst buffer are
    // the same and dst already contains the desired data.
    // See also: HgiBuffer::GetCPUStagingAddress.
    if (copyOp.cpuSourceBuffer != dst ||
        copyOp.sourceByteOffset != copyOp.destinationByteOffset) {
        // Offset into the src buffer
        const char* src = ((const char*) copyOp.cpuSourceBuffer) +
            copyOp.sourceByteOffset;

        // Offset into the dst buffer
        memcpy(dst + dstOffset, src, copyOp.byteSize);
    }

    if (!sharedBuffer &&
        [metalBuffer->GetBufferId()
             respondsToSelector:@selector(didModifyRange:)]) {
        NSRange range = NSMakeRange(dstOffset, copyOp.byteSize);
        id<MTLResource> resource = metalBuffer->GetBufferId();
        
        ARCH_PRAGMA_PUSH
        ARCH_PRAGMA_INSTANCE_METHOD_NOT_FOUND
        [resource didModifyRange:range];
        ARCH_PRAGMA_POP
    }
}

void
HgiMetalBlitCmds::CopyBufferGpuToCpu(HgiBufferGpuToCpuOp const& copyOp)
{
    if (copyOp.byteSize == 0 ||
        !copyOp.cpuDestinationBuffer ||
        !copyOp.gpuSourceBuffer)
    {
        return;
    }

    HgiMetalBuffer* metalBuffer = static_cast<HgiMetalBuffer*>(
        copyOp.gpuSourceBuffer.Get());

    if (@available(macOS 10.11, ios 100.100, *)) {
        [_blitEncoder performSelector:@selector(synchronizeResource:)
                           withObject:metalBuffer->GetBufferId()];
    }
    
    // Offset into the dst buffer
    char* dst = ((char*) copyOp.cpuDestinationBuffer) +
        copyOp.destinationByteOffset;
    
    // Offset into the src buffer
    const char* src = ((const char*) metalBuffer->GetCPUStagingAddress()) +
        copyOp.sourceByteOffset;

    // bytes to copy
    size_t size = copyOp.byteSize;

    [_commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
        {
            memcpy(dst, src, size);
        }];
}

void
HgiMetalBlitCmds::CopyTextureToBuffer(HgiTextureToBufferOp const& copyOp)
{
    TF_CODING_ERROR("Missing Implementation");
}

void
HgiMetalBlitCmds::CopyBufferToTexture(HgiBufferToTextureOp const& copyOp)
{
    TF_CODING_ERROR("Missing Implementation");
}

void
HgiMetalBlitCmds::GenerateMipMaps(HgiTextureHandle const& texture)
{
    HgiMetalTexture* metalTex = static_cast<HgiMetalTexture*>(texture.Get());
    if (metalTex) {
        _CreateEncoder();
        
        [_blitEncoder generateMipmapsForTexture:metalTex->GetTextureId()];
    }
}

void
HgiMetalBlitCmds::MemoryBarrier(HgiMemoryBarrier barrier)
{
    TF_VERIFY(barrier==HgiMemoryBarrierAll, "Unknown barrier");
    // Do nothing. All blit encoder work will be visible to next encoder.
}

bool
HgiMetalBlitCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    bool submittedWork = false;
    if (_blitEncoder) {
        [_blitEncoder endEncoding];
        _blitEncoder = nil;
        submittedWork = true;

        HgiMetal::CommitCommandBufferWaitType waitType;
        switch(wait) {
            case HgiSubmitWaitTypeNoWait:
                waitType = HgiMetal::CommitCommandBuffer_NoWait;
                break;
            case HgiSubmitWaitTypeWaitUntilCompleted:
                waitType = HgiMetal::CommitCommandBuffer_WaitUntilCompleted;
                break;
        }

        if (_secondaryCommandBuffer) {
            _hgi->CommitSecondaryCommandBuffer(_commandBuffer, waitType);
        }
        else {
            _hgi->CommitPrimaryCommandBuffer(waitType);
        }
    }
    
    if (_secondaryCommandBuffer) {
        _hgi->ReleaseSecondaryCommandBuffer(_commandBuffer);
    }
    _commandBuffer = nil;

    return submittedWork;
}

PXR_NAMESPACE_CLOSE_SCOPE
