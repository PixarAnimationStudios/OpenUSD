//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgiVulkan/blitCmds.h"
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/commandBuffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/texture.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"


PXR_NAMESPACE_OPEN_SCOPE

namespace {

HgiVulkanResourceUse
_TransferRead()
{
    HgiVulkanResourceUse read;
    read.stages = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
    read.access = VK_ACCESS_2_TRANSFER_READ_BIT;
    return read;
}

HgiVulkanResourceUse
_TransferWrite()
{
    HgiVulkanResourceUse write;
    write.stages = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
    write.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    write.write = true;
    return write;
}

HgiVulkanResourceUse
_HostRead()
{
    HgiVulkanResourceUse read;
    read.stages = VK_PIPELINE_STAGE_2_HOST_BIT;
    read.access = VK_ACCESS_2_HOST_READ_BIT;
    return read;
}

} // anonymous namespace

HgiVulkanBlitCmds::HgiVulkanBlitCmds(HgiVulkan* hgi)
    : _hgi(hgi)
    , _commandBuffer(nullptr)
    , _tracker(hgi->GetCapabilities())
{
    // We do not acquire the command buffer here, because the Cmds object may
    // have been created on the main thread, but used on a secondary thread.
    // We need to acquire a command buffer for the thread that is doing the
    // recording so we postpone acquiring cmd buffer until first use of Cmds.
}

HgiVulkanBlitCmds::~HgiVulkanBlitCmds() = default;

void
HgiVulkanBlitCmds::PushDebugGroup(
        const char* label,
        const GfVec4f& color)
{
    _CreateCommandBuffer();
    HgiVulkanBeginLabel(_hgi->GetPrimaryDevice(), _commandBuffer, label, color);
}

void
HgiVulkanBlitCmds::PopDebugGroup()
{
    _CreateCommandBuffer();
    HgiVulkanEndLabel(_hgi->GetPrimaryDevice(), _commandBuffer);
}

void
HgiVulkanBlitCmds::InsertDebugMarker(
        const char* label,
        const GfVec4f& color)
{
    _CreateCommandBuffer();
    HgiVulkanInsertDebugMarker(_hgi->GetPrimaryDevice(), _commandBuffer, label,
        color);
}

static
VkImageAspectFlags
_GetImageAspectMaskForCopy(HgiTextureUsage textureUsage)
{
    // XXX: Vulkan validation demands that only one flag at a time be used 
    // during the copy operation. Both depth and stencil flags cannot be 
    // simultaneously passed as aspects to copy.
    // So, we assume that the user wants to copy depth when the texture is a 
    // depthStencil texture. If need arises, this part of the implementation 
    // needs to be re-written such that an aspect flag is passed to this copy 
    // operation to resolve the discrepancy.
    VkImageAspectFlags aspectFlags =
        HgiVulkanConversions::GetImageAspectFlag(textureUsage);
    if (aspectFlags & VK_IMAGE_ASPECT_DEPTH_BIT) {
        aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    return aspectFlags;
}

void
HgiVulkanBlitCmds::CopyTextureGpuToCpu(
    HgiTextureGpuToCpuOp const& copyOp)
{
    _CreateCommandBuffer();

    HgiVulkanTexture* srcTexture =
        static_cast<HgiVulkanTexture*>(copyOp.gpuSourceTexture.Get());

    if (!TF_VERIFY(srcTexture && srcTexture->GetImage(),
        "Invalid texture handle")) {
        return;
    }

    if (copyOp.destinationBufferByteSize == 0) {
        TF_WARN("The size of the data to copy was zero (aborted)");
        return;
    }

    HgiTextureDesc const& texDesc = srcTexture->GetDescriptor();

    bool isTexArray = texDesc.layerCount>1;
    int depthOffset = isTexArray ? 0 : copyOp.sourceTexelOffset[2];

    VkOffset3D origin;
    origin.x = copyOp.sourceTexelOffset[0];
    origin.y = copyOp.sourceTexelOffset[1];
    origin.z = depthOffset;

    VkExtent3D size;
    size.width = texDesc.dimensions[0] - copyOp.sourceTexelOffset[0];
    size.height = texDesc.dimensions[1] - copyOp.sourceTexelOffset[1];
    size.depth = texDesc.dimensions[2] - depthOffset;

    VkImageSubresourceLayers imageSub;
    imageSub.baseArrayLayer = isTexArray ? copyOp.sourceTexelOffset[2] : 0;
    imageSub.layerCount = 1;
    imageSub.mipLevel = copyOp.mipLevel;
    imageSub.aspectMask = _GetImageAspectMaskForCopy(texDesc.usage);

    // See vulkan docs: Copying Data Between Buffers and Images
    VkBufferImageCopy region;
    region.bufferImageHeight = 0; // Buffer is tightly packed, like image
    region.bufferRowLength = 0;   // Buffer is tightly packed, like image
    region.bufferOffset = 0;      // We offset cpuDestinationBuffer. Not here.
    region.imageExtent = size;
    region.imageOffset = origin;
    region.imageSubresource = imageSub;

    // Copy gpu texture to gpu staging buffer.
    // We reuse the texture's staging buffer, assuming that any new texel
    // uploads this frame will have been consumed from the staging buffer
    // before any downloads (read backs) overwrite the staging buffer texels.
    uint8_t* src = static_cast<uint8_t*>(srcTexture->GetCPUStagingAddress());
    HgiVulkanBuffer* stagingBuffer = srcTexture->GetStagingBuffer();
    TF_VERIFY(src && stagingBuffer);

    // Transition image to read.
    _tracker.UseImage(
        srcTexture, _TransferRead(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    _tracker.Use(stagingBuffer->GetState(), _TransferWrite());
    auto restore = _tracker.Flush(_commandBuffer->GetVulkanCommandBuffer());

    vkCmdCopyImageToBuffer(
        _commandBuffer->GetVulkanCommandBuffer(),
        srcTexture->GetImage(),
        srcTexture->GetImageLayout(),
        stagingBuffer->GetVulkanBuffer(),
        1,
        &region);

    restore.Restore(_commandBuffer->GetVulkanCommandBuffer());

    _tracker.Use(stagingBuffer->GetState(), _HostRead());
    _tracker.FlushWithoutRestore(_commandBuffer->GetVulkanCommandBuffer());

    // Offset into the dst buffer
    char* dst = ((char*) copyOp.cpuDestinationBuffer) +
        copyOp.destinationByteOffset;

    // bytes to copy
    size_t byteSize = copyOp.destinationBufferByteSize;

    // Copy to cpu buffer when cmd buffer has been executed
    _commandBuffer->AddCompletedHandler(
        [dst, src, byteSize]{ memcpy(dst, src, byteSize);}
    );
}

void
HgiVulkanBlitCmds::CopyTextureCpuToGpu(
    HgiTextureCpuToGpuOp const& copyOp)
{
    _CreateCommandBuffer();

    HgiVulkanTexture* dstTexture = static_cast<HgiVulkanTexture*>(
        copyOp.gpuDestinationTexture.Get());
    HgiTextureDesc const& texDesc = dstTexture->GetDescriptor();

    // If we used GetCPUStagingAddress as the cpuSourceBuffer when the copyOp
    // was created, we can skip the memcpy since the src and dst buffer are
    // the same and dst staging buffer already contains the desired data.
    // See also: HgiVulkanTexture::GetCPUStagingAddress.
    if (!dstTexture->IsCPUStagingAddress(copyOp.cpuSourceBuffer)) {

        // We need to memcpy at the mip's location in the staging buffer.
        // It is possible we CopyTextureCpuToGpu a bunch of mips in a row
        // before submitting the cmd buf. So we can't just use the start
        // of the staging buffer each time.
        const std::vector<HgiMipInfo> mipInfos =
            HgiGetMipInfos(
                texDesc.format,
                texDesc.dimensions,
                1 /*HgiTextureCpuToGpuOp does one layer at a time*/);

        if (mipInfos.size() > copyOp.mipLevel) {
            HgiMipInfo const& mipInfo = mipInfos[copyOp.mipLevel];

            uint8_t* dst = static_cast<uint8_t*>(
                dstTexture->GetCPUStagingAddress());
            TF_VERIFY(dst && dstTexture->GetStagingBuffer());

            dst += mipInfo.byteOffset;
            const size_t size =
                std::min(copyOp.bufferByteSize, 1 * mipInfo.byteSizePerLayer);
            memcpy(dst, copyOp.cpuSourceBuffer, size);
        }
    }

    // Schedule transfer from staging buffer to device-local texture
    HgiVulkanBuffer* stagingBuffer = dstTexture->GetStagingBuffer();
    if (TF_VERIFY(stagingBuffer, "Invalid staging buffer for texture")) {
        dstTexture->CopyBufferToTexture(
            _commandBuffer, 
            dstTexture->GetStagingBuffer(),
            copyOp.destinationTexelOffset,
            copyOp.mipLevel);
    }
}

void HgiVulkanBlitCmds::CopyBufferGpuToGpu(
    HgiBufferGpuToGpuOp const& copyOp)
{
    _CreateCommandBuffer();

    HgiBufferHandle const& srcBufHandle = copyOp.gpuSourceBuffer;
    HgiVulkanBuffer* srcBuffer =
        static_cast<HgiVulkanBuffer*>(srcBufHandle.Get());

    if (!TF_VERIFY(srcBuffer && srcBuffer->GetVulkanBuffer(),
        "Invalid source buffer handle")) {
        return;
    }

    HgiBufferHandle const& dstBufHandle = copyOp.gpuDestinationBuffer;
    HgiVulkanBuffer* dstBuffer =
        static_cast<HgiVulkanBuffer*>(dstBufHandle.Get());

    if (!TF_VERIFY(dstBuffer && dstBuffer->GetVulkanBuffer(),
        "Invalid destination buffer handle")) {
        return;
    }

    if (copyOp.byteSize == 0) {
        TF_WARN("The size of the data to copy was zero (aborted)");
        return;
    }

    _TrackTransfer(srcBuffer->GetState(), dstBuffer->GetState());

    // Copy data from staging buffer to destination (gpu) buffer
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = copyOp.sourceByteOffset;
    copyRegion.dstOffset = copyOp.destinationByteOffset;
    copyRegion.size = copyOp.byteSize;

    vkCmdCopyBuffer(
        _commandBuffer->GetVulkanCommandBuffer(),
        srcBuffer->GetVulkanBuffer(),
        dstBuffer->GetVulkanBuffer(),
        1, // regionCount
        &copyRegion);
}

void HgiVulkanBlitCmds::BlitTexture(HgiTextureHandle src, HgiTextureHandle dst)
{
    _CreateCommandBuffer();

    HgiVulkanTexture* srcTexture =
        static_cast<HgiVulkanTexture*>(src.Get());

    if (!TF_VERIFY(srcTexture && srcTexture->GetImage(),
        "Invalid texture handle")) {
        return;
    }

    HgiVulkanTexture* dstTexture =
        static_cast<HgiVulkanTexture*>(dst.Get());

    if (!TF_VERIFY(srcTexture && srcTexture->GetImage(),
        "Invalid texture handle")) {
        return;
    }

    HgiTextureDesc const& texDesc = srcTexture->GetDescriptor();

    VkOffset3D origin;
    origin.x = 0;
    origin.y = 0;
    origin.z = 0;

    VkOffset3D size;
    size.x = texDesc.dimensions[0];
    size.y = texDesc.dimensions[1];
    size.z = texDesc.dimensions[2];

    VkImageSubresourceLayers imageSub;
    imageSub.baseArrayLayer = 0;
    imageSub.layerCount = 1;
    imageSub.mipLevel = 0;
    imageSub.aspectMask = _GetImageAspectMaskForCopy(texDesc.usage);

    VkImageBlit region;
    region.srcOffsets[0] = origin;
    region.srcOffsets[1] = size;
    region.srcSubresource = imageSub;
    region.dstOffsets[0] = origin;
    region.dstOffsets[1] = size;
    region.dstSubresource = imageSub;

    _tracker.UseImage(
        srcTexture, _TransferRead(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    _tracker.UseImage(
        dstTexture, _TransferWrite(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    auto restore = _tracker.Flush(_commandBuffer->GetVulkanCommandBuffer());

    vkCmdBlitImage(_commandBuffer->GetVulkanCommandBuffer(),
        srcTexture->GetImage(),
        srcTexture->GetImageLayout(),
        dstTexture->GetImage(),
        dstTexture->GetImageLayout(),
        1,
        &region,
        VK_FILTER_NEAREST);

    restore.Restore(_commandBuffer->GetVulkanCommandBuffer());
}

void HgiVulkanBlitCmds::CopyBufferCpuToGpu(
    HgiBufferCpuToGpuOp const& copyOp)
{
    _CreateCommandBuffer();

    if (copyOp.byteSize == 0 ||
        !copyOp.cpuSourceBuffer ||
        !copyOp.gpuDestinationBuffer)
    {
        return;
    }

    HgiVulkanBuffer* buffer = static_cast<HgiVulkanBuffer*>(
        copyOp.gpuDestinationBuffer.Get());

    // If we used GetCPUStagingAddress as the cpuSourceBuffer when the copyOp
    // was created, we can skip the memcpy since the src and dst buffer are
    // the same and dst staging buffer already contains the desired data.
    // See also: HgiBuffer::GetCPUStagingAddress.
    if (!buffer->IsCPUStagingAddress(copyOp.cpuSourceBuffer) ||
        copyOp.sourceByteOffset != copyOp.destinationByteOffset) {

        // Offset into the src buffer
        const auto src =
            static_cast<const std::byte*>(copyOp.cpuSourceBuffer) +
            copyOp.sourceByteOffset;

        // Offset into the dst buffer.
        const auto dst =
            static_cast<std::byte*>(buffer->GetCPUStagingAddress()) +
            copyOp.destinationByteOffset;

        memcpy(dst, src, copyOp.byteSize);
    }

    // Schedule copy data from staging buffer to device-local buffer if needed.
    // With UMA/ReBAR, the staging address is already the device buffer, so no
    // additional copy is necessary.
    if (!_hgi->GetCapabilities()->IsSet(HgiDeviceCapabilitiesBitsUnifiedMemory)
        && !(buffer->GetDescriptor().usage & HgiBufferUsageUpload)) {
        HgiVulkanBuffer* stagingBuffer = buffer->GetStagingBuffer();
        TF_VERIFY(stagingBuffer);

        _TrackTransfer(stagingBuffer->GetState(), buffer->GetState());

        VkBufferCopy copyRegion = {};
        // Note we use the destinationByteOffset as the srcOffset here. The staging buffer
        // should be prepared with the same data layout of the destination buffer.
        copyRegion.srcOffset = copyOp.destinationByteOffset;
        copyRegion.dstOffset = copyOp.destinationByteOffset;
        copyRegion.size = copyOp.byteSize;

        vkCmdCopyBuffer(
            _commandBuffer->GetVulkanCommandBuffer(),
            stagingBuffer->GetVulkanBuffer(),
            buffer->GetVulkanBuffer(),
            1,
            &copyRegion);
    } else {
        _TrackTransfer(nullptr, buffer->GetState());
    }
}

void
HgiVulkanBlitCmds::CopyBufferGpuToCpu(HgiBufferGpuToCpuOp const& copyOp)
{
    _CreateCommandBuffer();

    if (copyOp.byteSize == 0 ||
        !copyOp.cpuDestinationBuffer ||
        !copyOp.gpuSourceBuffer)
    {
        return;
    }

    HgiVulkanBuffer* buffer = static_cast<HgiVulkanBuffer*>(
        copyOp.gpuSourceBuffer.Get());

    // The buffer the host reads the result from.
    HgiVulkanResourceState* hostReadState = buffer->GetState();

    // Schedule copy data from device-local buffer to staging buffer if needed.
    // With UMA/ReBAR, the staging address is already the device buffer, so no
    // additional copy is necessary.
    size_t srcOffset = copyOp.sourceByteOffset;
    if (!_hgi->GetCapabilities()->IsSet(HgiDeviceCapabilitiesBitsUnifiedMemory)) {
        // Make sure there is a staging buffer in the buffer by asking for cpuAddr.
        buffer->GetCPUStagingAddress();
        HgiVulkanBuffer* stagingBuffer = buffer->GetStagingBuffer();
        TF_VERIFY(stagingBuffer);

        _TrackTransfer(buffer->GetState(), stagingBuffer->GetState());
        hostReadState = stagingBuffer->GetState();

        // Copy from device-local GPU buffer into CPU staging buffer
        VkBufferCopy copyRegion = {};
        copyRegion.srcOffset = srcOffset;
        // No need to use dst offset during intermediate step of copying into 
        // staging buffer.
        copyRegion.dstOffset = 0;
        copyRegion.size = copyOp.byteSize;
        vkCmdCopyBuffer(
            _commandBuffer->GetVulkanCommandBuffer(), 
            buffer->GetVulkanBuffer(),
            stagingBuffer->GetVulkanBuffer(),
            1, 
            &copyRegion);
        // No need to offset into the staging buffer for the next copy.
        srcOffset = 0;
    }

    _tracker.Use(hostReadState, _HostRead());
    _tracker.FlushWithoutRestore(_commandBuffer->GetVulkanCommandBuffer());

    // Next schedule a callback when the above GPU-CPU copy completes.

    // Offset into the dst buffer
    const auto dst = static_cast<std::byte*>(copyOp.cpuDestinationBuffer) +
        copyOp.destinationByteOffset;

    const auto src =
        static_cast<const std::byte*>(buffer->GetCPUStagingAddress()) +
        srcOffset;

    // bytes to copy
    const size_t size = copyOp.byteSize;

    // Copy to cpu buffer when cmd buffer has been executed
    _commandBuffer->AddCompletedHandler(
        [dst, src, size]{ memcpy(dst, src, size); }
    );
}

void
HgiVulkanBlitCmds::CopyTextureToBuffer(HgiTextureToBufferOp const& copyOp)
{
    if (copyOp.byteSize == 0) {
        TF_WARN("The size of the data to copy was zero, aborting copy.");
        return;
    }

    _CreateCommandBuffer();

    HgiVulkanTexture* srcTexture =
        static_cast<HgiVulkanTexture*>(copyOp.gpuSourceTexture.Get());
    if (!TF_VERIFY(srcTexture && srcTexture->GetImage(), "Invalid texture")) {
        return;
    }
    HgiTextureDesc const& texDesc = srcTexture->GetDescriptor();

    HgiVulkanBuffer* dstBuffer =
        static_cast<HgiVulkanBuffer*>(copyOp.gpuDestinationBuffer.Get());
    if (!TF_VERIFY(dstBuffer && dstBuffer->GetVulkanBuffer(),
        "Invalid buffer")) {
        return;
    }

    VkOffset3D origin;
    origin.x = copyOp.sourceTexelOffset[0];
    origin.y = copyOp.sourceTexelOffset[1];
    origin.z = copyOp.sourceTexelOffset[2];

    VkExtent3D size;
    size.width = texDesc.dimensions[0];
    size.height = texDesc.dimensions[1];
    size.depth = texDesc.dimensions[2];

    VkImageSubresourceLayers imageSub;
    imageSub.baseArrayLayer = 0;
    imageSub.layerCount = texDesc.layerCount;
    imageSub.mipLevel = copyOp.mipLevel;
    imageSub.aspectMask = _GetImageAspectMaskForCopy(texDesc.usage);

    VkBufferImageCopy region;
    region.bufferImageHeight = 0; // Buffer is tightly packed, like image
    region.bufferRowLength = 0;   // Buffer is tightly packed, like image
    region.bufferOffset = copyOp.destinationByteOffset;
    region.imageExtent = size;
    region.imageOffset = origin;
    region.imageSubresource = imageSub;

    _tracker.UseImage(
        srcTexture, _TransferRead(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    _tracker.Use(dstBuffer->GetState(), _TransferWrite());
    auto restore = _tracker.Flush(_commandBuffer->GetVulkanCommandBuffer());

    vkCmdCopyImageToBuffer(
        _commandBuffer->GetVulkanCommandBuffer(),
        srcTexture->GetImage(),
        srcTexture->GetImageLayout(),
        dstBuffer->GetVulkanBuffer(),
        /*regionCount*/1,
        &region
    );

    restore.Restore(_commandBuffer->GetVulkanCommandBuffer());
}

void
HgiVulkanBlitCmds::CopyBufferToTexture(HgiBufferToTextureOp const& copyOp)
{
    if (copyOp.byteSize == 0) {
        TF_WARN("The size of the data to copy was zero, aborting copy.");
        return;
    }

    _CreateCommandBuffer();

    HgiBufferHandle const& srcBufHandle = copyOp.gpuSourceBuffer;
    HgiVulkanBuffer* srcBuffer =
        static_cast<HgiVulkanBuffer*>(srcBufHandle.Get());
    if (!TF_VERIFY(srcBuffer && srcBuffer->GetVulkanBuffer(),
        "Invalid buffer")) {
        return;
    }

    HgiVulkanTexture* dstTexture =
        static_cast<HgiVulkanTexture*>(copyOp.gpuDestinationTexture.Get());
    if (!TF_VERIFY(dstTexture && dstTexture->GetImage(),
        "Invalid texture handle")) {
        return;
    }

    HgiTextureDesc const& texDesc = dstTexture->GetDescriptor();

    VkOffset3D origin;
    origin.x = copyOp.destinationTexelOffset[0];
    origin.y = copyOp.destinationTexelOffset[1];
    origin.z = copyOp.destinationTexelOffset[2];

    VkExtent3D size;
    size.width = texDesc.dimensions[0] - copyOp.destinationTexelOffset[0];
    size.height = texDesc.dimensions[1] - copyOp.destinationTexelOffset[1];
    size.depth = texDesc.dimensions[2] - copyOp.destinationTexelOffset[2];

    VkImageSubresourceLayers imageSub;
    imageSub.baseArrayLayer = 0;
    imageSub.layerCount = texDesc.layerCount;
    imageSub.mipLevel = copyOp.mipLevel;
    imageSub.aspectMask = _GetImageAspectMaskForCopy(texDesc.usage);

    VkBufferImageCopy region;
    region.bufferImageHeight = 0; // Buffer is tightly packed, like image
    region.bufferRowLength = 0;   // Buffer is tightly packed, like image
    region.bufferOffset = copyOp.sourceByteOffset;
    region.imageExtent = size;
    region.imageOffset = origin;
    region.imageSubresource = imageSub;

    _tracker.Use(srcBuffer->GetState(), _TransferRead());
    _tracker.UseImage(
        dstTexture, _TransferWrite(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    auto restore = _tracker.Flush(_commandBuffer->GetVulkanCommandBuffer());

    vkCmdCopyBufferToImage(
        _commandBuffer->GetVulkanCommandBuffer(),
        srcBuffer->GetVulkanBuffer(),
        dstTexture->GetImage(),
        dstTexture->GetImageLayout(),
        /*regionCount*/1,
        &region);

    restore.Restore(_commandBuffer->GetVulkanCommandBuffer());
}

void
HgiVulkanBlitCmds::GenerateMipMaps(HgiTextureHandle const& texture)
{
    _CreateCommandBuffer();

    HgiVulkanTexture* vkTex = static_cast<HgiVulkanTexture*>(texture.Get());
    HgiVulkanDevice* device = vkTex->GetDevice();

    HgiTextureDesc const& desc = texture->GetDescriptor();
    if (desc.mipLevels < 2) {
        return;
    }

    bool const isDepthBuffer = desc.usage & HgiTextureUsageBitsDepthTarget;
    VkFormat format = HgiVulkanConversions::GetFormat(
        desc.format, isDepthBuffer);
    
    int32_t width = desc.dimensions[0];
    int32_t height = desc.dimensions[1];

    // Ensure texture format supports blit src&dst required for mips
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(
        device->GetVulkanPhysicalDevice(), format, &formatProps);
    if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
        !(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT))
    {
        TF_CODING_ERROR("Texture format does not support "
                        "blit source and destination");
        return;                    
    }

    // Change every mip to TRANSFER_DST at once, then convert one by one to
    // TRANSFER_SRC as blits are executed. In the end the whole image ends up as
    // TRANSFER_SRC.
    HgiVulkanResourceUse blit;
    blit.stages = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
    blit.access = VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
    blit.write = true;
    _tracker.UseImage(vkTex, blit, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    auto restore = _tracker.Flush(_commandBuffer->GetVulkanCommandBuffer());

    // Copy down the whole mip chain doing a blit from mip-1 to mip
    for (uint32_t i = 1; i < desc.mipLevels; i++) {
        VkImageBlit imageBlit{};

        // Source
        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.layerCount = desc.layerCount;
        imageBlit.srcSubresource.mipLevel = i - 1;
        imageBlit.srcOffsets[1].x = std::max(width >> (i - 1), 1);
        imageBlit.srcOffsets[1].y = std::max(height >> (i - 1), 1);
        imageBlit.srcOffsets[1].z = 1;

        // Destination
        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.layerCount = desc.layerCount;
        imageBlit.dstSubresource.mipLevel = i;
        imageBlit.dstOffsets[1].x = std::max(width >> i, 1);
        imageBlit.dstOffsets[1].y = std::max(height >> i, 1);
        imageBlit.dstOffsets[1].z = 1;

        // Blit from previous level
        _TransitionMipToTransferSrc(vkTex, i - 1);
        vkCmdBlitImage(
            _commandBuffer->GetVulkanCommandBuffer(),
            vkTex->GetImage(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            vkTex->GetImage(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &imageBlit,
            VK_FILTER_LINEAR);
    }

    // Transition the last level to TRANSFER_DST
    _TransitionMipToTransferSrc(vkTex, desc.mipLevels - 1);
    vkTex->GetState()->layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    restore.Restore(_commandBuffer->GetVulkanCommandBuffer());
}

void
HgiVulkanBlitCmds::FillBuffer(HgiBufferHandle const& buffer, uint8_t value)
{
    _CreateCommandBuffer();

    HgiVulkanBuffer* buf = static_cast<HgiVulkanBuffer*>(buffer.Get());

    _TrackTransfer(nullptr, buf->GetState());

    // Convert 8-bit value to 32-bit value e.g. if given 0xff, we want to pass
    // 0xffffffff to vkCmdFillBuffer.
    const uint32_t value32Bit = static_cast<uint32_t>(value) | 
                                static_cast<uint32_t>(value) << 8 | 
                                static_cast<uint32_t>(value) << 16 |
                                static_cast<uint32_t>(value) << 24;

    vkCmdFillBuffer(
        _commandBuffer->GetVulkanCommandBuffer(),
        buf->GetVulkanBuffer(),
        0,
        VK_WHOLE_SIZE,
        value32Bit);
}

void
HgiVulkanBlitCmds::_TrackTransfer(
    HgiVulkanResourceState* src,
    HgiVulkanResourceState* dst)
{
    _tracker.Use(src, _TransferRead());
    _tracker.Use(dst, _TransferWrite());
    _tracker.FlushWithoutRestore(_commandBuffer->GetVulkanCommandBuffer());
}

void
HgiVulkanBlitCmds::_TransitionMipToTransferSrc(
    HgiVulkanTexture* texture,
    uint32_t mipLevel)
{
    VkImageMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture->GetImage();
    barrier.subresourceRange.aspectMask =
        HgiVulkanConversions::GetImageAspectFlag(
            texture->GetDescriptor().usage);
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VkDependencyInfo dependencyInfo = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(
        _commandBuffer->GetVulkanCommandBuffer(), &dependencyInfo);
}

void
HgiVulkanBlitCmds::InsertMemoryBarrier(HgiMemoryBarrier barrier)
{
    _CreateCommandBuffer();
    _commandBuffer->InsertMemoryBarrier(barrier);
}

HgiVulkanCommandBuffer*
HgiVulkanBlitCmds::GetCommandBuffer()
{
    return _commandBuffer;
}

bool
HgiVulkanBlitCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    if (!_commandBuffer) {
        return false;
    }

    HgiVulkanDevice* device = _commandBuffer->GetDevice();
    HgiVulkanCommandQueue* queue = device->GetCommandQueue();

    // Submit the GPU work and optionally do CPU - GPU synchronization.
    queue->SubmitToQueue(_commandBuffer, wait);

    return true;
}

void
HgiVulkanBlitCmds::_CreateCommandBuffer()
{
    if (!_commandBuffer) {
        HgiVulkanDevice* device = _hgi->GetPrimaryDevice();
        HgiVulkanCommandQueue* queue = device->GetCommandQueue();
        _commandBuffer = queue->AcquireCommandBuffer();
        TF_VERIFY(_commandBuffer);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
