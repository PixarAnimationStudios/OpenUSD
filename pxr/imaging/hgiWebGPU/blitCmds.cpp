//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
#include <cstddef>

#if defined(ARCH_OS_WASM_VM)
#include <emscripten.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPUBlitCmds::HgiWebGPUBlitCmds(HgiWebGPU *hgi)
    : _hgi(hgi)
    , _blitEncoder(nullptr)
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
HgiWebGPUBlitCmds::PushDebugGroup(const char* label, const GfVec4f& color)
{
}

void
HgiWebGPUBlitCmds::PopDebugGroup()
{
}

void
HgiWebGPUBlitCmds::InsertDebugMarker(const char* label, const GfVec4f& color)
{
}

void
HgiWebGPUBlitCmds::_MapAsyncAndWait(const wgpu::Buffer &buffer,
                                    wgpu::MapMode mode,
                                    size_t offset,
                                    size_t size) {
    bool done = false;
    buffer.MapAsync(mode, offset, size,
        wgpu::CallbackMode::AllowProcessEvents,
        [&done](wgpu::MapAsyncStatus status, wgpu::StringView) {
            if (status != wgpu::MapAsyncStatus::Success) {
                TF_WARN("Failed to call MapAsync");
            }
            // Make sure we dont need to wait for it anymore.
            done = true;
    });

    while (!done) {
#if defined(ARCH_OS_WASM_VM)
        emscripten_sleep(1);
#else
        _hgi->EndFrame();

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

	wgpu::TexelCopyTextureInfo textureCopyView ;
	textureCopyView.texture = srcTexture->GetTextureHandle();
	textureCopyView.origin = {
        static_cast<uint32_t>(copyOp.sourceTexelOffset[0]),
        static_cast<uint32_t>(copyOp.sourceTexelOffset[1]),
        static_cast<uint32_t>(depthOffset)};
 
	wgpu::TexelCopyBufferLayout textureDataLayout;
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

	wgpu::TexelCopyBufferInfo bufferCopyView;
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

    auto stagingData = std::make_unique<StagingData>();
    stagingData->src = stagingBuffer;
    stagingData->dst = dst;
    stagingData->alignedSize = bytesPerRowAligned * texDesc.dimensions[1];
    stagingData->bytesPerRowAligned = bytesPerRowAligned;
    stagingData->bytesPerRow = bytesPerRow;
    stagingData->isTmp = true;
    _stagingDataItems.push_back(std::move(stagingData));
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

    wgpu::TexelCopyTextureInfo destination;
    destination.texture = dstTexture->GetTextureHandle();
    destination.mipLevel = copyOp.mipLevel;
    destination.origin = { 
        static_cast<uint32_t>(offsets[0]),
        static_cast<uint32_t>(offsets[1]),
        static_cast<uint32_t>(offsets[2]) };

    wgpu::TexelCopyBufferLayout dataLayout;
    dataLayout.bytesPerRow = copyOp.bufferByteSize / height;
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

    // WebGPU does not allow copying from a buffer to itself, even for non-overlapping ranges
    // Use a temporary buffer for same-buffer copies
    if (srcBufHandle.Get() == dstBufHandle.Get()) {
        // Create a temporary buffer for the intermediate copy
        wgpu::Device device = _hgi->GetPrimaryDevice();
        wgpu::BufferDescriptor desc;
        desc.label = "TempBuffer";
        desc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
        desc.size = copyOp.byteSize;
        wgpu::Buffer tempBuffer = device.CreateBuffer(&desc);

        // Copy from source to temp buffer
        _blitEncoder.CopyBufferToBuffer(
            srcBuffer->GetBufferHandle(), copyOp.sourceByteOffset,
            tempBuffer, 0,
            copyOp.byteSize);

        // Copy from temp buffer to destination
        _blitEncoder.CopyBufferToBuffer(
            tempBuffer, 0,
            dstBuffer->GetBufferHandle(), copyOp.destinationByteOffset,
            copyOp.byteSize);
    } else {
        _blitEncoder.CopyBufferToBuffer(
            srcBuffer->GetBufferHandle(), copyOp.sourceByteOffset,
            dstBuffer->GetBufferHandle(), copyOp.destinationByteOffset,
            copyOp.byteSize);
    }
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

    auto buffer = dynamic_cast<HgiWebGPUBuffer*>(copyOp.gpuSourceBuffer.Get());

    if (buffer) {
        auto device = _hgi->GetPrimaryDevice();
        // Create a new buffer with MapRead usage
        wgpu::BufferDescriptor desc;
        desc.label = "MapReadBuffer";
        desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
        desc.size = copyOp.byteSize;
        wgpu::Buffer dstBuffer = device.CreateBuffer(&desc);

        // Create a command encoder
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        // Copy the source buffer to the destination buffer
        encoder.CopyBufferToBuffer(buffer->GetBufferHandle(), copyOp.sourceByteOffset, dstBuffer, 0, copyOp.byteSize);

        // Finish encoding and submit the commands
        wgpu::CommandBuffer commandBuffer = encoder.Finish();
        wgpu::Queue queue = device.GetQueue();
        queue.Submit(1, &commandBuffer);

        _MapAsyncAndWait(dstBuffer, wgpu::MapMode::Read, 0, copyOp.byteSize);
        const void *memoryPtr = dstBuffer.GetConstMappedRange(0, copyOp.byteSize);
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
    auto wgpuBuffer = dynamic_cast<HgiWebGPUBuffer*>(buffer.Get());
    if (wgpuBuffer) {
        if (value == 0) {
            // We can use ClearBuffer to clear the buffer to 0
            // This is faster than writing the buffer with a value in the case of WebGPU
            _CreateEncoder();
            _blitEncoder.ClearBuffer(wgpuBuffer->GetBufferHandle(), 0, wgpuBuffer->GetDescriptor().byteSize);
        } else {
            HgiBufferDesc const& desc = wgpuBuffer->GetDescriptor();
            std::vector<uint8_t> fillVector(desc.byteSize, value);
            _hgi->GetQueue().WriteBuffer(wgpuBuffer->GetBufferHandle(), 0, fillVector.data(), desc.byteSize);
        }
    }
}

void
HgiWebGPUBlitCmds::GenerateMipMaps(HgiTextureHandle const& texture)
{
    auto wgpuTex = static_cast<HgiWebGPUTexture*>(texture.Get());
    HgiTextureDesc const& desc = texture->GetDescriptor();
    _hgi->GenerateMipmap(wgpuTex->GetTextureHandle(), desc);
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
                TF_CODING_ERROR("Waiting %u type not supported\n", wait);
                break;
        }

        _commandBuffer = nullptr;

        // once we have submitted the copy commands we can do the mapping and copy
        // this could in theory be done in a completion handler but it seems emscripten
        // doesn't like an emscripten_sleep within another block containing and emscripten_sleep
        for(std::unique_ptr<StagingData>& stagingData : _stagingDataItems)
        {
            stagingData->src.MapAsync(wgpu::MapMode::Read, 0, stagingData->alignedSize, wgpu::CallbackMode::AllowSpontaneous,
                [](wgpu::MapAsyncStatus status, wgpu::StringView message, StagingData* rawStagingData)
            {
                std::unique_ptr<StagingData> stagingData{rawStagingData};

                if (status != wgpu::MapAsyncStatus::Success)
                {
                    TF_WARN("Failed to call MapAsync: %s",
                        ((std::string)message).c_str());
                    return;
                }

                // copy to staging data
                const void *memoryPtr = stagingData->src.GetConstMappedRange(0, stagingData->alignedSize);

                if (stagingData->bytesPerRow != stagingData->bytesPerRowAligned) 
                {
                    uint32_t height = stagingData->alignedSize / stagingData->bytesPerRowAligned;
                    uint32_t offset = 0;
                    const char* srcPtr = static_cast<const char*>(memoryPtr);
                    char* dstPtr = static_cast<char*>(stagingData->dst);
                    for (uint32_t y = 0; y < height; ++y) 
                    {
                        uint32_t offset2 = y * stagingData->bytesPerRowAligned;
                        for (uint32_t x = 0; x < stagingData->bytesPerRow; ++x) 
                        {
                            dstPtr[offset++] = srcPtr[offset2++];
                        }
                    }
                } 
                else
                {
                    memcpy(stagingData->dst, memoryPtr, stagingData->alignedSize);
                }

                if (stagingData->isTmp)
                {
                    stagingData->src.Destroy();
                }

                stagingData->asyncDone = true;
                (void)stagingData.release();
            }, stagingData.get());

            // wait for the async call to finish
            while (!stagingData->asyncDone)
            {
                #if defined(ARCH_OS_WASM_VM)
                    emscripten_sleep(1);
                #else
                    _hgi->EndFrame();
                #endif
            }
        }

        _stagingDataItems.clear();
    }

    return submittedWork;
}

PXR_NAMESPACE_CLOSE_SCOPE
