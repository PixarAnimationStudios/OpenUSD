//
// Copyright 2022 Pixar
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
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/blitCmds.h"
#include "pxr/imaging/hgiWebGPU/buffer.h"
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/texture.h"

#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/types.h"

#include "pxr/base/arch/pragmas.h"

#include "mipmapGenerator.h"

#if defined(EMSCRIPTEN)
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPUBlitCmds::HgiWebGPUBlitCmds(HgiWebGPU *hgi)
    : _hgi(hgi)
    , _blitEncoder(nullptr)
    , _mipmapGenerator(hgi->GetPrimaryDevice())
{
}

HgiWebGPUBlitCmds::~HgiWebGPUBlitCmds()
{
    _commandBuffer = nullptr;
    TF_VERIFY(_blitEncoder == nullptr, "Encoder created, but never commited.");
}

void
HgiWebGPUBlitCmds::_CreateEncoder()
{
    if (!_blitEncoder) {
        wgpu::Device device = _hgi->GetPrimaryDevice();
        _blitEncoder = device.CreateCommandEncoder();
    }
}

void
HgiWebGPUBlitCmds::PushDebugGroup(const char* label)
{
}

void
HgiWebGPUBlitCmds::PopDebugGroup()
{
}

void
HgiWebGPUBlitCmds::_MapAsyncAndWait(const wgpu::Buffer &buffer,
                                    wgpu::MapMode mode,
                                    size_t offset,
                                    size_t size) {
    bool done = false;
    buffer.MapAsync(
            mode, offset, size,
            [](WGPUBufferMapAsyncStatus status, void *userdata) {
                if (status == WGPUBufferMapAsyncStatus_Success) {
                    *static_cast<bool *>(userdata) = true;
                } else {
                    TF_WARN("Failed to call MapAsync");
                }
            },
            &done);

    while (!done) {
#if defined(EMSCRIPTEN)
        emscripten_sleep(1);
#else
        _hgi->GetPrimaryDevice().Tick();
#endif
    }
}

void 
HgiWebGPUBlitCmds::CopyTextureGpuToCpu(HgiTextureGpuToCpuOp const& copyOp)
{
    HgiTextureHandle texHandle = copyOp.gpuSourceTexture;
    HgiWebGPUTexture* srcTexture =static_cast<HgiWebGPUTexture*>(texHandle.Get());

    if (!TF_VERIFY(srcTexture && srcTexture->GetTextureHandle(),
        "Invalid texture handle")) {
        return;
    }

    if (copyOp.destinationBufferByteSize == 0) {
        TF_WARN("The size of the data to copy was zero (aborted)");
        return;
    }

    HgiTextureDesc const& texDesc = srcTexture->GetDescriptor();

    auto device = _hgi->GetPrimaryDevice();

    size_t bytesPerPixel = HgiGetDataSizeOfFormat(texDesc.format);

    int depthOffset = texDesc.layerCount>1 ? 0 : copyOp.sourceTexelOffset[2];

	wgpu::ImageCopyTexture textureCopyView ;
	textureCopyView.texture = srcTexture->GetTextureHandle();
	textureCopyView.origin = {
        static_cast<uint32_t>(copyOp.sourceTexelOffset[0]),
        static_cast<uint32_t>(copyOp.sourceTexelOffset[1]),
        static_cast<uint32_t>(depthOffset)};
 
	wgpu::TextureDataLayout textureDataLayout;
    uint32_t bytesPerRow = texDesc.dimensions[0] * bytesPerPixel;
    uint32_t bytesPerRowAligned = std::ceil(bytesPerRow / 256.0) * 256;
    // bytesPerRow has to be multiple of 256 https://www.w3.org/TR/webgpu/#gpuimagecopybuffer
	textureDataLayout.bytesPerRow = bytesPerRowAligned;
	//textureDataLayout.rowsPerImage = texDesc.dimensions[1];

    // create a staging buffer
    wgpu::BufferDescriptor desc;
    desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
    desc.size = bytesPerRowAligned *  texDesc.dimensions[1];

    auto stagingBuffer = device.CreateBuffer(&desc);

	wgpu::ImageCopyBuffer bufferCopyView;
	bufferCopyView.buffer = stagingBuffer;
	bufferCopyView.layout = textureDataLayout;
	wgpu::Extent3D copySize = {
        texDesc.dimensions[0] - static_cast<uint32_t>(copyOp.sourceTexelOffset[0]),
        texDesc.dimensions[1] - static_cast<uint32_t>(copyOp.sourceTexelOffset[1]),
        texDesc.dimensions[2] - static_cast<uint32_t>(depthOffset) };

    _CreateEncoder();

    _blitEncoder.CopyTextureToBuffer(&textureCopyView, &bufferCopyView, &copySize);

    char* dst = ((char*) copyOp.cpuDestinationBuffer) +
        copyOp.destinationByteOffset;

    auto bufferSize = copyOp.destinationBufferByteSize;

    StagingData stagingData;
    stagingData.src = stagingBuffer;
    stagingData.dst = dst;
    stagingData.size = bufferSize;
    stagingData.bytesPerRowAligned = bytesPerRowAligned;
    stagingData.bytesPerRow = bytesPerRow;
    stagingData.isTmp = true;
    _stagingDatas.push_back(stagingData);
}

void
HgiWebGPUBlitCmds::CopyTextureCpuToGpu(
    HgiTextureCpuToGpuOp const& copyOp)
{
    HgiWebGPUTexture* dstTexture = static_cast<HgiWebGPUTexture*>(
        copyOp.gpuDestinationTexture.Get());
    HgiTextureDesc const& texDesc = dstTexture->GetDescriptor();

    const size_t width = texDesc.dimensions[0];
    const size_t height = texDesc.dimensions[1];
    const size_t depth = texDesc.dimensions[2];

    GfVec3i const& offsets = copyOp.destinationTexelOffset;

    wgpu::ImageCopyTexture destination;
    destination.texture = dstTexture->GetTextureHandle();
    destination.mipLevel = copyOp.mipLevel;
    destination.origin = { 
        static_cast<uint32_t>(offsets[0]),
        static_cast<uint32_t>(offsets[1]),
        static_cast<uint32_t>(offsets[2]) };

    wgpu::TextureDataLayout dataLayout;
    dataLayout.bytesPerRow = copyOp.bufferByteSize / height / width;
    dataLayout.rowsPerImage = height;

	wgpu::Extent3D writeSize = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), static_cast<uint32_t>(depth) };

    _hgi->GetQueue().WriteTexture(&destination, copyOp.cpuSourceBuffer, copyOp.bufferByteSize, &dataLayout, &writeSize);
}

void
HgiWebGPUBlitCmds::CopyBufferGpuToGpu(
    HgiBufferGpuToGpuOp const& copyOp)
{
    HgiBufferHandle const& srcBufHandle = copyOp.gpuSourceBuffer;
    HgiWebGPUBuffer* srcBuffer =static_cast<HgiWebGPUBuffer*>(srcBufHandle.Get());

    if (!TF_VERIFY(srcBuffer && srcBuffer->GetBufferHandle(),
        "Invalid source buffer handle")) {
        return;
    }

    HgiBufferHandle const& dstBufHandle = copyOp.gpuDestinationBuffer;
    HgiWebGPUBuffer* dstBuffer =static_cast<HgiWebGPUBuffer*>(dstBufHandle.Get());

    if (!TF_VERIFY(dstBuffer && dstBuffer->GetBufferHandle(),
        "Invalid destination buffer handle")) {
        return;
    }

    if (copyOp.byteSize == 0) {
        TF_WARN("The size of the data to copy was zero (aborted)");
        return;
    }

    _CreateEncoder();

    _blitEncoder.CopyBufferToBuffer(
        srcBuffer->GetBufferHandle(), copyOp.sourceByteOffset,
        dstBuffer->GetBufferHandle(), copyOp.destinationByteOffset,
        copyOp.byteSize);
}

void HgiWebGPUBlitCmds::CopyBufferCpuToGpu(
    HgiBufferCpuToGpuOp const& copyOp)
{
    if (copyOp.byteSize == 0 ||
        !copyOp.cpuSourceBuffer ||
        !copyOp.gpuDestinationBuffer)
    {
        return;
    }

    HgiWebGPUBuffer* buffer = static_cast<HgiWebGPUBuffer*>(
        copyOp.gpuDestinationBuffer.Get());

    const uint8_t* src = ((const uint8_t*) copyOp.cpuSourceBuffer) + copyOp.sourceByteOffset;
    _hgi->GetQueue().WriteBuffer(buffer->GetBufferHandle(), copyOp.destinationByteOffset, src, copyOp.byteSize);
}

void
HgiWebGPUBlitCmds::CopyBufferGpuToCpu(HgiBufferGpuToCpuOp const& copyOp)
{
    if (copyOp.byteSize == 0 ||
        !copyOp.cpuDestinationBuffer ||
        !copyOp.gpuSourceBuffer)
    {
        return;
    }

    HgiWebGPUBuffer* buffer = static_cast<HgiWebGPUBuffer*>(copyOp.gpuSourceBuffer.Get());

    if (buffer) {
        _MapAsyncAndWait(buffer->GetBufferHandle(), wgpu::MapMode::Read,0, copyOp.byteSize);
        const void *memoryPtr = buffer->GetBufferHandle().GetConstMappedRange(0, copyOp.byteSize);
        char* dst = ((char*) copyOp.cpuDestinationBuffer) + copyOp.destinationByteOffset;
        memcpy(dst, memoryPtr, copyOp.byteSize);
    }
}

void
HgiWebGPUBlitCmds::CopyTextureToBuffer(HgiTextureToBufferOp const& copyOp)
{
    TF_CODING_ERROR("Missing Implementation CopyTextureToBuffer");
}

void
HgiWebGPUBlitCmds::CopyBufferToTexture(HgiBufferToTextureOp const& copyOp)
{
    TF_CODING_ERROR("Missing Implementation CopyBufferToTexture");
}

void
HgiWebGPUBlitCmds::FillBuffer(HgiBufferHandle const& buffer, uint8_t value)
{
    TF_CODING_ERROR("Missing Implementation FillBuffer");
}

void
HgiWebGPUBlitCmds::GenerateMipMaps(HgiTextureHandle const& texture)
{
    HgiWebGPUTexture* wgpuTex = static_cast<HgiWebGPUTexture*>(texture.Get());
    HgiTextureDesc const& desc = texture->GetDescriptor();
    _mipmapGenerator.generateMipmap(wgpuTex->GetTextureHandle(), desc);
}

void
HgiWebGPUBlitCmds::InsertMemoryBarrier(HgiMemoryBarrier barrier)
{
    TF_VERIFY(barrier==HgiMemoryBarrierAll, "Unknown barrier");
}

bool
HgiWebGPUBlitCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    bool submittedWork = false;
    if (_blitEncoder) {
        _commandBuffer = _blitEncoder.Finish();
        _blitEncoder = nullptr;

        _hgi->EnqueueCommandBuffer(_commandBuffer);

        submittedWork = true;

        // these are both busy waits...for now
        switch(wait) {
            case HgiSubmitWaitTypeNoWait:
                _hgi->QueueSubmit();
                break;
            case HgiSubmitWaitTypeWaitUntilCompleted:
                _hgi->QueueSubmit();
                break;
            default:
                TF_CODING_ERROR("Waiting %s type not supported\n", wait);
                break;
        }

        _commandBuffer = nullptr;

        // once we have submitted the copy commands we can do the mapping and copy
        // this could in theory be done in a completion handler but it seems emscripten
        // doesn't like an emscripten_sleep within another block containing and emscripten_sleep
        for( auto &stagingData : _stagingDatas )
        {
            _MapAsyncAndWait(stagingData.src, wgpu::MapMode::Read,0, stagingData.size);
            const void *memoryPtr = stagingData.src.GetConstMappedRange(0, stagingData.size);

            if (stagingData.bytesPerRow != stagingData.bytesPerRowAligned) {
                uint32_t height = stagingData.size / stagingData.bytesPerRow;
                uint32_t offset = 0;
                const char* srcPtr = static_cast<const char*>(memoryPtr);
                char* dstPtr = static_cast<char*>(stagingData.dst);
                for (uint32_t y = 0; y < height; ++y) {
                    uint32_t offset2 = y * stagingData.bytesPerRowAligned;
                    for (uint32_t x = 0; x < stagingData.bytesPerRow; ++x) {
                        dstPtr[offset++] = srcPtr[offset2++];
                    }
                }
            } else{
                memcpy(stagingData.dst, memoryPtr, stagingData.size);
            }
            if (stagingData.isTmp) {
                // Call needed by emscripten due to bug https://github.com/emscripten-core/emscripten/pull/18790
                stagingData.src.Unmap();
                stagingData.src.Destroy();
            }
        }
        _stagingDatas.clear();
    }

    return submittedWork;
}

PXR_NAMESPACE_CLOSE_SCOPE
