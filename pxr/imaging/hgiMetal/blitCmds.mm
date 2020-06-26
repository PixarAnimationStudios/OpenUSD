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
    , _blitEncoder(nil)
    , _label(nil)
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
        _blitEncoder = [_hgi->GetCommandBuffer() blitCommandEncoder];
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
        _label = @(label);
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

    uint32_t layerCnt = copyOp.startLayer + copyOp.numLayers;
    if (!TF_VERIFY(texDesc.layerCount >= layerCnt,
        "Texture has less layers than attempted to be copied")) {
        return;
    }

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

    size_t bytesPerPixel = HgiDataSizeOfFormat(texDesc.format);
    id<MTLBuffer> cpuBuffer =
        [device newBufferWithBytesNoCopy:copyOp.cpuDestinationBuffer
                                  length:copyOp.destinationBufferByteSize
                                 options:options
                             deallocator:nil];

    MTLOrigin origin = MTLOriginMake(
        copyOp.sourceTexelOffset[0],
        copyOp.sourceTexelOffset[1],
        copyOp.sourceTexelOffset[2]);
    MTLSize size = MTLSizeMake(
        texDesc.dimensions[0] - copyOp.sourceTexelOffset[0],
        texDesc.dimensions[1] - copyOp.sourceTexelOffset[1],
        texDesc.dimensions[2] - copyOp.sourceTexelOffset[2]);
    
    MTLBlitOption blitOptions = MTLBlitOptionNone;

    _CreateEncoder();

    [_blitEncoder copyFromTexture:srcTexture->GetTextureId()
                      sourceSlice:0
                      sourceLevel:copyOp.startLayer
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
    memcpy(copyOp.cpuDestinationBuffer,
        [cpuBuffer contents], copyOp.destinationBufferByteSize);
    [cpuBuffer release];
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

    // Offset into the src buffer
    const char* src = ((const char*) copyOp.cpuSourceBuffer) +
        copyOp.sourceByteOffset;

    // Offset into the dst buffer
    size_t dstOffset = copyOp.destinationByteOffset;
    uint8_t *dst = static_cast<uint8_t*>([metalBuffer->GetBufferId() contents]);
    memcpy(dst + dstOffset, src, copyOp.byteSize);

    if([metalBuffer->GetBufferId()
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
HgiMetalBlitCmds::GenerateMipMaps(HgiTextureHandle const& texture)
{
    HgiMetalTexture* metalTex = static_cast<HgiMetalTexture*>(texture.Get());
    if (metalTex) {
        _CreateEncoder();
        
        [_blitEncoder generateMipmapsForTexture:metalTex->GetTextureId()];
    }
}

bool
HgiMetalBlitCmds::_Submit(Hgi* hgi)
{
    if (_blitEncoder) {
        [_blitEncoder endEncoding];
        _blitEncoder = nil;
        return true;
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
