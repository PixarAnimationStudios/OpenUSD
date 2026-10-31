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

#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hio/types.h"

#include "pxr/imaging/garch/glApi.h"
#include "pxr/imaging/garch/glDebugWindow.h"

#include "pxr/base/gf/vec3i.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

static constexpr HgiFormat _hgiFormat = HgiFormatUNorm8Vec4;
static constexpr HioFormat _hioFormat = HioFormatUNorm8Vec4;

static int
_MipLevelCount(int width, int height)
{
    int levels = 1;
    while (width > 1 || height > 1) {
        width = std::max(width >> 1, 1);
        height = std::max(height >> 1, 1);
        ++levels;
    }
    return levels;
}

static bool
_LoadImage(std::string const& path, int* width, int* height,
    std::vector<uint8_t>* pixels)
{
    HioImageSharedPtr const image = HioImage::OpenForReading(
        path, /*subimage=*/0, /*mip=*/0, HioImage::SourceColorSpace::Raw);
    if (!image) {
        TF_CODING_ERROR("Failed to open input image: %s", path.c_str());
        return false;
    }

    const HioFormat format = image->GetFormat();
    if (HioGetComponentCount(format) != 4 ||
        HioGetDataSizeOfFormat(format) != 4) {
        TF_CODING_ERROR("Input image must be 8-bit RGBA: %s", path.c_str());
        return false;
    }

    *width = image->GetWidth();
    *height = image->GetHeight();
    pixels->assign(HgiGetDataSize(_hgiFormat, GfVec3i(*width, *height, 1)), 0);

    HioImage::StorageSpec storage;
    storage.width = *width;
    storage.height = *height;
    storage.format = format;
    storage.flipped = false;
    storage.data = pixels->data();

    if (!image->Read(storage)) {
        TF_CODING_ERROR("Failed to read input image: %s", path.c_str());
        return false;
    }

    return true;
}

static bool
_WriteMipLevel(Hgi* hgi, HgiTextureHandle const& texture, int baseWidth,
    int baseHeight, int level, std::string const& path)
{
    const int width = std::max(baseWidth >> level, 1);
    const int height = std::max(baseHeight >> level, 1);
    const size_t byteSize =
        HgiGetDataSize(_hgiFormat, GfVec3i(width, height, 1));

    std::vector<uint8_t> pixels(byteSize, 0);

    HgiTextureGpuToCpuOp copyOp;
    copyOp.gpuSourceTexture = texture;
    copyOp.sourceTexelOffset = GfVec3i(0);
    copyOp.mipLevel = level;
    copyOp.cpuDestinationBuffer = pixels.data();
    copyOp.destinationByteOffset = 0;
    copyOp.destinationBufferByteSize = byteSize;

    HgiBlitCmdsUniquePtr blitCmds = hgi->CreateBlitCmds();
    blitCmds->CopyTextureGpuToCpu(copyOp);
    hgi->SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    HioImage::StorageSpec storage;
    storage.width = width;
    storage.height = height;
    storage.format = _hioFormat;
    storage.flipped = false;
    storage.data = pixels.data();

    HioImageSharedPtr const image = HioImage::OpenForWriting(path);
    if (!image || !image->Write(storage)) {
        TF_CODING_ERROR("Failed to write mip level image: %s", path.c_str());
        return false;
    }

    return true;
}

static bool
TestMipmap(Hgi* hgi, std::string const& inputPath, std::string const& prefix)
{
    int width = 0;
    int height = 0;
    std::vector<uint8_t> mip0;
    if (!_LoadImage(inputPath, &width, &height, &mip0)) {
        return false;
    }
    std::cout << "Input image " << inputPath << " (" << width << "x" << height
              << ")\n";

    const int mipLevels = _MipLevelCount(width, height);

    HgiTextureDesc texDesc;
    texDesc.debugName = "testHgiMipmap";
    texDesc.usage =
        HgiTextureUsageBitsShaderRead | HgiTextureUsageBitsColorTarget;
    texDesc.format = _hgiFormat;
    texDesc.type = HgiTextureType2D;
    texDesc.dimensions = GfVec3i(width, height, 1);
    texDesc.mipLevels = mipLevels;
    HgiTextureHandle texture = hgi->CreateTexture(texDesc);

    // Upload mip 0.
    {
        HgiTextureCpuToGpuOp uploadOp;
        uploadOp.cpuSourceBuffer = mip0.data();
        uploadOp.bufferByteSize = mip0.size();
        uploadOp.gpuDestinationTexture = texture;
        uploadOp.destinationTexelOffset = GfVec3i(0);
        uploadOp.mipLevel = 0;

        HgiBlitCmdsUniquePtr blitCmds = hgi->CreateBlitCmds();
        blitCmds->CopyTextureCpuToGpu(uploadOp);
        hgi->SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);
    }

    // Generate the mip chain.
    {
        HgiBlitCmdsUniquePtr blitCmds = hgi->CreateBlitCmds();
        blitCmds->GenerateMipMaps(texture);
        hgi->SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);
    }

    // Write every mip level out as a PNG for image-diff comparison.
    bool success = true;
    for (int level = 0; level < mipLevels; ++level) {
        const std::string path = prefix + std::to_string(level) + ".png";
        success &= _WriteMipLevel(hgi, texture, width, height, level, path);
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
    std::string inputPath = "mipmapInput.png";
    std::string prefix = "mipmapLevel";
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--input" && i + 1 < argc) {
            inputPath = argv[++i];
        } else if (arg == "--write" && i + 1 < argc) {
            prefix = argv[++i];
        }
    }

    TfErrorMark mark;

    HgiUnitTestWindow window("testHgiMipmap", 256, 256);
    window.Init();

    HgiInitializationTestDriver driver;
    Hgi* hgi = driver.GetHgi();
    if (!hgi) {
        TF_CODING_ERROR("Failed to create Hgi");
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Testing Hgi mipmap generation on: " << hgi->GetAPIName()
              << "\n";

    const bool success = TestMipmap(hgi, inputPath, prefix);

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    }

    std::cout << "FAILED" << std::endl;
    return EXIT_FAILURE;
}
