//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
        _commandBuffer = _hgi->GetPrimaryCommandBuffer(this);
        if (_commandBuffer == nil) {
            _commandBuffer = _hgi->GetSecondaryCommandBuffer();
            _secondaryCommandBuffer = true;
        }
        _blitEncoder = [_commandBuffer blitCommandEncoder];

        if (_label) {
            [_blitEncoder pushDebugGroup:_label];
            [_label release];
            _label = nil;
        }
    }
}

void
HgiMetalBlitCmds::PushDebugGroup(const char* label)
{
    if (!HgiMetalDebugEnabled()) {
        return;
    }

    if (_blitEncoder) {
        HGIMETAL_DEBUG_PUSH_GROUP(_blitEncoder, label)
    }
    else  {
        _label = [@(label) copy];
    }
}

void
HgiMetalBlitCmds::PopDebugGroup()
{
    if (_blitEncoder) {
        HGIMETAL_DEBUG_POP_GROUP(_blitEncoder)
    }
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

    MTLPixelFormat metalFormat = HgiMetalConversions::GetPixelFormat(
        texDesc.format, texDesc.usage);
    
    if (metalFormat == MTLPixelFormatDepth32Float_Stencil8) {
        TF_WARN("Cannot read back depth stencil on Metal.");
        return;
    }

    id<MTLDevice> device = _hgi->GetPrimaryDevice();

    MTLResourceOptions options = _hgi->GetCapabilities()->defaultStorageMode;

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

#if defined(ARCH_OS_OSX)
    if ([cpuBuffer storageMode] == MTLStorageModeManaged) {
        [_blitEncoder performSelector:@selector(synchronizeResource:)
                           withObject:cpuBuffer];
    }
#endif
    
    // Offset into the dst buffer
    char* dst = ((char*) copyOp.cpuDestinationBuffer) +
        copyOp.destinationByteOffset;
    
    // bytes to copy
    size_t byteSize = copyOp.destinationBufferByteSize;

    [_commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
        {
            const char* src = (const char*) [cpuBuffer contents];
            memcpy(dst, src, byteSize);
            [cpuBuffer release];
        }];
}

void
HgiMetalBlitCmds::CopyTextureCpuToGpu(
    HgiTextureCpuToGpuOp const& copyOp)
{
    HgiMetalTexture* dstTexture = static_cast<HgiMetalTexture*>(
        copyOp.gpuDestinationTexture.Get());
    id<MTLTexture> dstTextureId = dstTexture->GetTextureId();
    HgiTextureDesc const& dstTexDesc = dstTexture->GetDescriptor();

    const size_t width = dstTexDesc.dimensions[0];
    const size_t height = dstTexDesc.dimensions[1];
    const size_t depth = dstTexDesc.dimensions[2];

    // Depth, stencil, depth-stencil, and multisample textures must be
    // allocated with MTLStorageModePrivate, which should not be used with
    // replaceRegion. We create a temporary, non-private texture to fill with
    // the cpu texture data, then blit the data from that texture to our
    // private destination texture.

    // Create texture descriptor to describe the temp texture.
    MTLTextureDescriptor* mtlDesc;
    MTLPixelFormat mtlFormat = HgiMetalConversions::GetPixelFormat(
        dstTexDesc.format, HgiTextureUsageBitsShaderRead);
    mtlDesc = [MTLTextureDescriptor
         texture2DDescriptorWithPixelFormat:mtlFormat
                                      width:width
                                     height:height
                                  mipmapped:NO];
    
    mtlDesc.mipmapLevelCount = dstTexDesc.mipLevels;
    mtlDesc.arrayLength = dstTexDesc.layerCount;
    mtlDesc.resourceOptions = MTLResourceStorageModeManaged;
    mtlDesc.sampleCount = 1;
    if (dstTexDesc.type == HgiTextureType3D) {
        mtlDesc.depth = depth;
        mtlDesc.textureType = MTLTextureType3D;
    } else if (dstTexDesc.type == HgiTextureType2DArray) {
        mtlDesc.textureType = MTLTextureType2DArray;
    } else if (dstTexDesc.type == HgiTextureType1D) {
        mtlDesc.textureType = MTLTextureType1D;
    } else if (dstTexDesc.type == HgiTextureType1DArray) {
        mtlDesc.textureType = MTLTextureType1DArray;
    }
    mtlDesc.usage = MTLTextureUsageShaderRead;

    // Create temp texture and fill with initial data.
    id<MTLTexture> tempTextureId =
        [_hgi->GetPrimaryDevice() newTextureWithDescriptor:mtlDesc];

    const bool isTexArray = dstTexDesc.layerCount>1;

    GfVec3i const& offsets = copyOp.destinationTexelOffset;
    int depthOffset = isTexArray ? 0 : offsets[2];
    if (dstTexDesc.type == HgiTextureType1D) {
        [tempTextureId
            replaceRegion:MTLRegionMake1D(offsets[0], width)
              mipmapLevel:copyOp.mipLevel
                withBytes:copyOp.cpuSourceBuffer
              bytesPerRow:copyOp.bufferByteSize];
    } else if (dstTexDesc.type == HgiTextureType2D) {
        [tempTextureId
            replaceRegion:MTLRegionMake2D(
                offsets[0], offsets[1], width, height)
              mipmapLevel:copyOp.mipLevel
                withBytes:copyOp.cpuSourceBuffer
              bytesPerRow:copyOp.bufferByteSize / height];
    } else {
        [tempTextureId
            replaceRegion:MTLRegionMake3D(
                offsets[0], offsets[1], depthOffset, width, height, depth)
              mipmapLevel:copyOp.mipLevel 
                    slice:isTexArray ? offsets[2] : 0
                withBytes:copyOp.cpuSourceBuffer
              bytesPerRow:copyOp.bufferByteSize / height / width
            bytesPerImage:copyOp.bufferByteSize / depth];
    }

    // Blit data from temp texture to destination texture.
    id<MTLCommandBuffer> commandBuffer = [_hgi->GetQueue() commandBuffer];
    id<MTLBlitCommandEncoder> blitCommandEncoder =
        [commandBuffer blitCommandEncoder];
    int sliceCount = 1;
    if (dstTexDesc.type == HgiTextureType1DArray ||
        dstTexDesc.type == HgiTextureType2DArray) {
        sliceCount = dstTexDesc.layerCount;
    }
    [blitCommandEncoder copyFromTexture:tempTextureId
                            sourceSlice:0
                            sourceLevel:copyOp.mipLevel
                              toTexture:dstTextureId
                       destinationSlice:0
                       destinationLevel:copyOp.mipLevel
                             sliceCount:sliceCount
                             levelCount:1];
    [blitCommandEncoder endEncoding];
    [commandBuffer commit];
    [tempTextureId release];
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

#if defined(ARCH_OS_OSX)
    // APPLE METAL: We need to do this copy host side, otherwise later
    // cpu copies into OTHER parts of this destination buffer see some of
    // our GPU copied range trampled by alignment of the blit. The Metal
    // spec says bytes outside of the range may be copied when calling
    // didModifyRange

    // We still need this for the staging buffer on AMD GPUs, so I'd like
    // to investigate further.
    if ([srcBuffer->GetBufferId() storageMode] == MTLStorageModeManaged) {
        memcpy((char*)dstBuffer->GetCPUStagingAddress() + copyOp.destinationByteOffset,
               (char*)srcBuffer->GetCPUStagingAddress() + copyOp.sourceByteOffset,
               copyOp.byteSize);
    }
#endif
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

    _CreateEncoder();

    if ([metalBuffer->GetBufferId() storageMode] == MTLStorageModeManaged) {
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
HgiMetalBlitCmds::FillBuffer(HgiBufferHandle const& buffer, uint8_t value)
{
    HgiMetalBuffer* metalBuf = static_cast<HgiMetalBuffer*>(buffer.Get());
    if (metalBuf) {
        _CreateEncoder();
        
        size_t length = metalBuf->GetDescriptor().byteSize;
        [_blitEncoder fillBuffer:metalBuf->GetBufferId()
                           range:NSMakeRange(0, length)
                           value:value];
    }
}

static bool
_HgiTextureCanBeFiltered(HgiTextureDesc const &descriptor)
{
    HgiFormat const componentFormat =
        HgiGetComponentBaseFormat(descriptor.format);

    if (componentFormat == HgiFormatInt16 ||
        componentFormat == HgiFormatUInt16 ||
        componentFormat == HgiFormatInt32) {
        return false;
    }

    GfVec3i const dims = descriptor.dimensions;
    switch (descriptor.type) {
        case HgiTextureType1D:
        case HgiTextureType1DArray:
            return (dims[0] > 1);

        case HgiTextureType2D:
        case HgiTextureType2DArray:
            return (dims[0] > 1 || dims[1] > 1);

        case HgiTextureType3D:
            return (dims[0] > 1 || dims[1] > 1 || dims[2] > 1);

        default:
            TF_CODING_ERROR("Unsupported HgiTextureType enum value");
            return false;
    }

    return false;
}

void
HgiMetalBlitCmds::GenerateMipMaps(HgiTextureHandle const& texture)
{
    HgiMetalTexture* metalTex = static_cast<HgiMetalTexture*>(texture.Get());

    if (metalTex) {
        if (_HgiTextureCanBeFiltered(metalTex->GetDescriptor())) {
            _CreateEncoder();
            [_blitEncoder generateMipmapsForTexture:metalTex->GetTextureId()];
        }
    }
}

void
HgiMetalBlitCmds::InsertMemoryBarrier(HgiMemoryBarrier barrier)
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
            if (waitType != HgiMetal::CommitCommandBuffer_WaitUntilCompleted) {
                // Ordering problems between CPU updates and GPU updates to a buffer
                // result in incorrect buffer contents. This is avoided by waiting
                // until work is scheduled, before future calls to didModifyRange
                // are made
                waitType = HgiMetal::CommitCommandBuffer_WaitUntilScheduled;
            }
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
