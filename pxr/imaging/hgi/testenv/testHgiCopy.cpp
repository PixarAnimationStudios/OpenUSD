//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgi/types.h"
#include "pxr/imaging/hgi/unitTestHelper.h"

#include "pxr/imaging/garch/glApi.h"
#include "pxr/imaging/garch/glDebugWindow.h"

#include "pxr/base/gf/vec3i.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"

#include <cstdint>
#include <iostream>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

static std::vector<uint8_t>
_ReadBuffer(Hgi* hgi, HgiBufferHandle const& buffer, size_t byteSize)
{
    std::vector<uint8_t> result(byteSize, 0);

    HgiBufferGpuToCpuOp copyOp;
    copyOp.gpuSourceBuffer = buffer;
    copyOp.sourceByteOffset = 0;
    copyOp.byteSize = byteSize;
    copyOp.cpuDestinationBuffer = result.data();
    copyOp.destinationByteOffset = 0;

    HgiBlitCmdsUniquePtr blitCmds = hgi->CreateBlitCmds();
    blitCmds->CopyBufferGpuToCpu(copyOp);
    hgi->SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    return result;
}

static std::vector<uint8_t>
_ReadTexture(Hgi* hgi, HgiTextureHandle const& texture, size_t byteSize,
    uint32_t mipLevel = 0)
{
    std::vector<uint8_t> result(byteSize, 0);

    HgiTextureGpuToCpuOp copyOp;
    copyOp.gpuSourceTexture = texture;
    copyOp.sourceTexelOffset = GfVec3i(0);
    copyOp.mipLevel = mipLevel;
    copyOp.cpuDestinationBuffer = result.data();
    copyOp.destinationByteOffset = 0;
    copyOp.destinationBufferByteSize = byteSize;

    HgiBlitCmdsUniquePtr blitCmds = hgi->CreateBlitCmds();
    blitCmds->CopyTextureGpuToCpu(copyOp);
    hgi->SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    return result;
}

static bool
TestBufferCopies(Hgi* hgi)
{
    static constexpr size_t byteSize = 1024;

    std::vector<uint8_t> srcData(byteSize);
    for (size_t i = 0; i < byteSize; ++i) {
        srcData[i] = static_cast<uint8_t>(i);
    }

    HgiBufferDesc bufDesc;
    bufDesc.usage = HgiBufferUsageStorage;
    bufDesc.byteSize = byteSize;
    bufDesc.debugName = "testCopySrc";
    HgiBufferHandle srcBuffer = hgi->CreateBuffer(bufDesc);
    bufDesc.debugName = "testCopyDst";
    HgiBufferHandle dstBuffer = hgi->CreateBuffer(bufDesc);

    bool success = true;

    // CPU -> srcBuffer -> dstBuffer, then read dstBuffer back.
    {
        HgiBufferCpuToGpuOp uploadOp;
        uploadOp.cpuSourceBuffer = srcData.data();
        uploadOp.sourceByteOffset = 0;
        uploadOp.gpuDestinationBuffer = srcBuffer;
        uploadOp.destinationByteOffset = 0;
        uploadOp.byteSize = byteSize;

        HgiBufferGpuToGpuOp copyOp;
        copyOp.gpuSourceBuffer = srcBuffer;
        copyOp.sourceByteOffset = 0;
        copyOp.gpuDestinationBuffer = dstBuffer;
        copyOp.destinationByteOffset = 0;
        copyOp.byteSize = byteSize;

        HgiBlitCmdsUniquePtr blitCmds = hgi->CreateBlitCmds();
        blitCmds->CopyBufferCpuToGpu(uploadOp);
        blitCmds->InsertMemoryBarrier(HgiMemoryBarrierAll);
        blitCmds->CopyBufferGpuToGpu(copyOp);
        hgi->SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

        if (_ReadBuffer(hgi, dstBuffer, byteSize) != srcData) {
            TF_CODING_ERROR("Buffer round-trip failed");
            success = false;
        }
    }

    // FillBuffer with a constant value.
    {
        HgiBlitCmdsUniquePtr blitCmds = hgi->CreateBlitCmds();
        blitCmds->FillBuffer(srcBuffer, 0xab);
        hgi->SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

        const std::vector<uint8_t> expected(byteSize, 0xab);
        if (_ReadBuffer(hgi, srcBuffer, byteSize) != expected) {
            TF_CODING_ERROR("FillBuffer failed");
            success = false;
        }
    }

    hgi->DestroyBuffer(&srcBuffer);
    hgi->DestroyBuffer(&dstBuffer);
    return success;
}

static bool
TestTextureCopies(Hgi* hgi)
{
    static constexpr int width = 16;
    static constexpr int height = 16;
    static constexpr HgiFormat format = HgiFormatUNorm8Vec4;
    const size_t byteSize = HgiGetDataSize(format, GfVec3i(width, height, 1));

    std::vector<uint8_t> srcData(byteSize);
    for (size_t i = 0; i < byteSize; ++i) {
        srcData[i] = static_cast<uint8_t>(i);
    }

    HgiTextureDesc texDesc;
    texDesc.usage =
        HgiTextureUsageBitsShaderRead | HgiTextureUsageBitsColorTarget;
    texDesc.format = format;
    texDesc.type = HgiTextureType2D;
    texDesc.dimensions = GfVec3i(width, height, 1);
    texDesc.debugName = "testCopyTexSrc";
    HgiTextureHandle srcTexture = hgi->CreateTexture(texDesc);
    texDesc.debugName = "testCopyTexDst";
    HgiTextureHandle dstTexture = hgi->CreateTexture(texDesc);

    HgiBufferDesc bufDesc;
    bufDesc.usage = HgiBufferUsageStorage;
    bufDesc.byteSize = byteSize;
    bufDesc.debugName = "testCopyTexBuf";
    HgiBufferHandle buffer = hgi->CreateBuffer(bufDesc);

    bool success = true;

    // CPU -> srcTexture, then read srcTexture back.
    {
        HgiTextureCpuToGpuOp uploadOp;
        uploadOp.cpuSourceBuffer = srcData.data();
        uploadOp.bufferByteSize = byteSize;
        uploadOp.gpuDestinationTexture = srcTexture;
        uploadOp.destinationTexelOffset = GfVec3i(0);
        uploadOp.mipLevel = 0;

        HgiBlitCmdsUniquePtr blitCmds = hgi->CreateBlitCmds();
        blitCmds->CopyTextureCpuToGpu(uploadOp);
        hgi->SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

        if (_ReadTexture(hgi, srcTexture, byteSize) != srcData) {
            TF_CODING_ERROR("Texture round-trip failed");
            success = false;
        }
    }

    // srcTexture -> buffer -> dstTexture, then read dstTexture back.
    {
        HgiTextureToBufferOp toBufferOp;
        toBufferOp.gpuSourceTexture = srcTexture;
        toBufferOp.mipLevel = 0;
        toBufferOp.gpuDestinationBuffer = buffer;
        toBufferOp.destinationByteOffset = 0;
        toBufferOp.byteSize = byteSize;

        HgiBufferToTextureOp toTextureOp;
        toTextureOp.gpuSourceBuffer = buffer;
        toTextureOp.sourceByteOffset = 0;
        toTextureOp.gpuDestinationTexture = dstTexture;
        toTextureOp.destinationTexelOffset = GfVec3i(0);
        toTextureOp.mipLevel = 0;
        toTextureOp.byteSize = byteSize;

        HgiBlitCmdsUniquePtr blitCmds = hgi->CreateBlitCmds();
        blitCmds->CopyTextureToBuffer(toBufferOp);
        blitCmds->InsertMemoryBarrier(HgiMemoryBarrierAll);
        blitCmds->CopyBufferToTexture(toTextureOp);
        hgi->SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

        if (_ReadTexture(hgi, dstTexture, byteSize) != srcData) {
            TF_CODING_ERROR("Texture <-> buffer copy failed");
            success = false;
        }
    }

    hgi->DestroyTexture(&srcTexture);
    hgi->DestroyTexture(&dstTexture);
    hgi->DestroyBuffer(&buffer);
    return success;
}

static bool
TestTextureMipUpload(Hgi* hgi)
{
    static constexpr int width = 16;
    static constexpr int height = 16;
    static constexpr uint32_t totalMipLevels = 5;
    static constexpr uint32_t mipLevel = 2;
    static constexpr HgiFormat format = HgiFormatUNorm8Vec4;

    const GfVec3i mipDim(width >> mipLevel, height >> mipLevel, 1);
    const size_t byteSize = HgiGetDataSize(format, mipDim);

    std::vector<uint8_t> srcData(byteSize);
    for (size_t i = 0; i < byteSize; ++i) {
        srcData[i] = static_cast<uint8_t>(i);
    }

    HgiTextureDesc texDesc;
    texDesc.usage =
        HgiTextureUsageBitsShaderRead | HgiTextureUsageBitsColorTarget;
    texDesc.format = format;
    texDesc.type = HgiTextureType2D;
    texDesc.dimensions = GfVec3i(width, height, 1);
    texDesc.mipLevels = totalMipLevels;
    texDesc.debugName = "testCopyMipTex";
    HgiTextureHandle texture = hgi->CreateTexture(texDesc);

    HgiTextureCpuToGpuOp uploadOp;
    uploadOp.cpuSourceBuffer = srcData.data();
    uploadOp.bufferByteSize = byteSize;
    uploadOp.gpuDestinationTexture = texture;
    uploadOp.destinationTexelOffset = GfVec3i(0);
    uploadOp.mipLevel = mipLevel;

    HgiBlitCmdsUniquePtr blitCmds = hgi->CreateBlitCmds();
    blitCmds->CopyTextureCpuToGpu(uploadOp);
    hgi->SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    bool success = true;
    if (_ReadTexture(hgi, texture, byteSize, mipLevel) != srcData) {
        TF_CODING_ERROR("Texture mip-level upload/readback failed");
        success = false;
    }

    hgi->DestroyTexture(&texture);
    return success;
}

static bool
TestTexture3DCopies(Hgi* hgi)
{
    static constexpr int width = 8;
    static constexpr int height = 4;
    static constexpr int depth = 2;
    static constexpr HgiFormat format = HgiFormatUNorm8Vec4;
    const size_t byteSize =
        HgiGetDataSize(format, GfVec3i(width, height, depth));

    std::vector<uint8_t> srcData(byteSize);
    for (size_t i = 0; i < byteSize; ++i) {
        srcData[i] = static_cast<uint8_t>(i);
    }

    HgiTextureDesc texDesc;
    texDesc.usage = HgiTextureUsageBitsShaderRead;
    texDesc.format = format;
    texDesc.type = HgiTextureType3D;
    texDesc.dimensions = GfVec3i(width, height, depth);
    texDesc.debugName = "testCopyTex3D";
    HgiTextureHandle texture = hgi->CreateTexture(texDesc);

    HgiTextureCpuToGpuOp uploadOp;
    uploadOp.cpuSourceBuffer = srcData.data();
    uploadOp.bufferByteSize = byteSize;
    uploadOp.gpuDestinationTexture = texture;
    uploadOp.destinationTexelOffset = GfVec3i(0);
    uploadOp.mipLevel = 0;

    HgiBlitCmdsUniquePtr blitCmds = hgi->CreateBlitCmds();
    blitCmds->CopyTextureCpuToGpu(uploadOp);
    hgi->SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    bool success = true;
    if (_ReadTexture(hgi, texture, byteSize) != srcData) {
        TF_CODING_ERROR("3D texture round-trip failed");
        success = false;
    }

    hgi->DestroyTexture(&texture);
    return success;
}

class HgiUnitTestWindow : public GarchGLDebugWindow
{
public:
    HgiUnitTestWindow(const char* title, int width, int height)
        : GarchGLDebugWindow(title, width, height)
    {
    }

    void OnInitializeGL() override { GarchGLApiLoad(); }
};

int
main(int argc, char** argv)
{
    TfErrorMark mark;

    HgiUnitTestWindow window("testHgiCopy", 256, 256);
    window.Init();

    HgiInitializationTestDriver driver;
    Hgi* hgi = driver.GetHgi();
    if (!hgi) {
        TF_CODING_ERROR("Failed to create Hgi");
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Testing Hgi copy commands on: " << hgi->GetAPIName() << "\n";

    bool success = TestBufferCopies(hgi);
    success &= TestTextureCopies(hgi);
    success &= TestTextureMipUpload(hgi);
    success &= TestTexture3DCopies(hgi);

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    }

    std::cout << "FAILED" << std::endl;
    return EXIT_FAILURE;
}
