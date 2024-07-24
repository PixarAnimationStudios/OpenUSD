//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/garch/glApi.h"
#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/graphicsCmds.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgiGL/hgi.h"
#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/device.h"
#include "pxr/imaging/hio/image.h"
#include "pxr/base/tf/errorMark.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

static const int _imgSize = 512;
static const HgiFormat _imgFormat = HgiFormatUNorm8Vec4;
static const HioFormat _imgHioFormat = HioFormatUNorm8Vec4;

static void
_SaveToPNG(
    int width,
    int height,
    const char* pixels,
    const std::string& filePath)
{
    HioImage::StorageSpec storage;
    storage.width = width;
    storage.height = height;
    storage.format = _imgHioFormat;
    storage.flipped = false;
    storage.data = (void*)pixels;

    HioImageSharedPtr image = HioImage::OpenForWriting(filePath);
    TF_VERIFY(image && image->Write(storage));
}

static void
_SaveGpuTextureToFile(
    HgiGL& hgiGL,
    HgiTextureHandle const& texHandle,
    int width,
    int height,
    HgiFormat format,
    std::string const& filePath)
{
    // Copy the pixels from gpu into a cpu buffer so we can save it to disk.
    const size_t bufferByteSize =
        width * height * HgiGetDataSizeOfFormat(format);
    std::vector<char> buffer(bufferByteSize);

    HgiTextureGpuToCpuOp copyOp;
    copyOp.gpuSourceTexture = texHandle;
    copyOp.sourceTexelOffset = GfVec3i(0);
    copyOp.mipLevel = 0;
    copyOp.cpuDestinationBuffer = buffer.data();
    copyOp.destinationByteOffset = 0;
    copyOp.destinationBufferByteSize = bufferByteSize;

    HgiBlitCmdsUniquePtr blitCmds = hgiGL.CreateBlitCmds();
    blitCmds->CopyTextureGpuToCpu(copyOp);
    hgiGL.SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    _SaveToPNG(width, height, buffer.data(), filePath);
}

static void
_SaveGpuBufferToFile(
    HgiGL& hgiGL,
    HgiBufferHandle const& bufHandle,
    int width,
    int height,
    HgiFormat format,
    std::string const& filePath)
{
    // Copy the pixels from gpu into a cpu buffer so we can save it to disk.
    const size_t bufferByteSize =
        width * height * HgiGetDataSizeOfFormat(format);
    std::vector<char> buffer(bufferByteSize);

    HgiBufferGpuToCpuOp copyOp;
    copyOp.gpuSourceBuffer = bufHandle;
    copyOp.sourceByteOffset = 0;
    copyOp.byteSize = bufferByteSize;
    copyOp.cpuDestinationBuffer = buffer.data();
    copyOp.destinationByteOffset = 0;    

    HgiBlitCmdsUniquePtr blitCmds = hgiGL.CreateBlitCmds();
    blitCmds->CopyBufferGpuToCpu(copyOp);
    hgiGL.SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    _SaveToPNG(width, height, buffer.data(), filePath);
}

static HgiTextureHandle
_CreateTexture(
    HgiGL& hgiGL,
    int width, 
    int height,
    HgiFormat format,
    void *data)
{
    const size_t textureByteSize =
        width * height * HgiGetDataSizeOfFormat(format);

    HgiTextureDesc texDesc;
    texDesc.debugName = "Debug texture";
    texDesc.dimensions = GfVec3i(width, height, 1);
    texDesc.format = format;
    texDesc.initialData = data;
    texDesc.layerCount = 1;
    texDesc.mipLevels = 1;
    texDesc.pixelsByteSize = textureByteSize;
    texDesc.sampleCount = HgiSampleCount1;
    texDesc.usage = HgiTextureUsageBitsShaderRead;

    return hgiGL.CreateTexture(texDesc);
}

static HgiBufferHandle
_CreateBuffer(
    HgiGL& hgiGL,
    size_t byteSize,
    void *data)
{
    HgiBufferDesc bufDesc;
    bufDesc.usage = HgiBufferUsageUniform;
    bufDesc.byteSize = byteSize;
    bufDesc.initialData = data;   

    return hgiGL.CreateBuffer(bufDesc);
}

static HgiGraphicsCmdsDesc
_CreateGraphicsCmdsColor0Color1Depth(
    HgiGL& hgiGL,
    GfVec3i const& size,
    HgiFormat colorFormat)
{
    // Create two color attachments
    HgiTextureDesc texDesc;
    texDesc.dimensions = size;
    texDesc.type = HgiTextureType2D;
    texDesc.format = colorFormat;
    texDesc.sampleCount = HgiSampleCount1;
    texDesc.usage = HgiTextureUsageBitsColorTarget;
    HgiTextureHandle colorTex0 = hgiGL.CreateTexture(texDesc);
    HgiTextureHandle colorTex1 = hgiGL.CreateTexture(texDesc);

    // Create a depth attachment
    texDesc.usage = HgiTextureUsageBitsDepthTarget;
    texDesc.format = HgiFormatFloat32;
    HgiTextureHandle depthTex = hgiGL.CreateTexture(texDesc);

    // Setup color and depth attachments
    HgiAttachmentDesc colorAttachment0;
    colorAttachment0.loadOp = HgiAttachmentLoadOpClear;
    colorAttachment0.storeOp = HgiAttachmentStoreOpStore;
    colorAttachment0.format = colorFormat;
    colorAttachment0.usage = HgiTextureUsageBitsColorTarget;

    HgiAttachmentDesc colorAttachment1;
    colorAttachment1.loadOp = HgiAttachmentLoadOpClear;
    colorAttachment1.storeOp = HgiAttachmentStoreOpStore;
    colorAttachment1.format = colorFormat;
    colorAttachment1.usage = HgiTextureUsageBitsColorTarget;

    HgiAttachmentDesc depthAttachment;
    depthAttachment.format = HgiFormatFloat32;
    depthAttachment.usage = HgiTextureUsageBitsDepthTarget;

    // Configure graphics cmds
    HgiGraphicsCmdsDesc desc;
    desc.colorAttachmentDescs.push_back(colorAttachment0);
    desc.colorAttachmentDescs.push_back(colorAttachment1);
    desc.depthAttachmentDesc = depthAttachment;
    desc.colorTextures.push_back(colorTex0);
    desc.colorTextures.push_back(colorTex1);
    desc.depthTexture = depthTex;

    return desc;
}

static void
_LogCaseHeader(std::ofstream &out, std::string const &msg)
{
    out << msg << std::endl;
    out << std::string(msg.length(), '-') << std::endl;
}

static void
_LogCaseFooter(std::ofstream &out)
{
    out << std::endl << std::endl;
}

static bool
TestContextArenaAndFramebufferCache()
{
    HgiGL hgiGL;
    const std::string path = "graphicsCmdsDescCache.txt";
    std::ofstream out(path);

    // The framebuffer descriptor cache caches framebuffers with their 
    // attachments based on the graphics cmds descriptor.
    // This is a perf optimization because creating framebuffer objects or
    // setting attachments on an existing framebuffer can be expensive when
    // done frequently.
    //
    // The cache is tied to the active context arena.
    // The expected behavior of the cache is that is keeps a small list of
    // cached framebuffers by garbage collecting invalid framebuffers
    // frequently. This is done either in HgiGL::EndFrame (when paired with
    // StartFrame) or after graphics command submission.
    //
    HgiGLDevice const& device = *hgiGL.GetPrimaryDevice();

    // 1. By creating 42 "active" framebuffers of different sizes, we will
    // end up with 42 entries at the end of the below loop.
    // Note that each entry isn't immediately garbage collected because the
    // underlying texture attachments aren't destroyed.
    const size_t count = 42;
    const size_t numTextureAttachments = 3;
    HgiTextureHandleVector textures;
    textures.reserve(count * numTextureAttachments);
    {
        for (size_t i = 0; i<count; i++) {
            GfVec3i size = GfVec3i(i+1, i+1, 1);
            HgiGraphicsCmdsDesc desc = 
                _CreateGraphicsCmdsColor0Color1Depth(hgiGL, size, _imgFormat);
            HgiGraphicsCmdsUniquePtr cmds= hgiGL.CreateGraphicsCmds(desc);

            // Track the texture handles so we can delete some later.
            std::copy(desc.colorTextures.begin(), desc.colorTextures.end(),
                      std::back_inserter(textures));
            textures.push_back(desc.depthTexture);

            hgiGL.SubmitCmds(cmds.get());
        }
        _LogCaseHeader(out, "Case 1: Add 42 framebuffer entries");
        out << device;
        _LogCaseFooter(out);
    }

    // 2. The second feature of the cache is that it should only maintain valid
    // entries after garbage collection is triggered. Any framebuffers with
    // texture attachment(s) that have been deleted must be GC'd.
    {
        TF_VERIFY(textures.size() == count * 3); // 2 color, 1 depth
        hgiGL.StartFrame();
        for (size_t i = 0; i < count; i+=2) {
            // Delete (some) attachments for every other framebuffer.
            const size_t numTexturesToDelete = size_t(rand() % 2) + 1;
            for (size_t j = 0; j < numTexturesToDelete; j++) {
                hgiGL.DestroyTexture(&textures[i*3 + j]);
            }
        }
        hgiGL.EndFrame();

        _LogCaseHeader(out, "Case 2: Delete every other framebuffer's textures");
        out << device;
        _LogCaseFooter(out);
    }

    // 3. Setting a custom context arena 
    // This should cause any framebuffer allocations to happen in the arena that
    // was set (and not the defaul arena).
    HgiGLContextArenaHandle arena = hgiGL.CreateContextArena();
    {
        hgiGL.SetContextArena(arena);
        
        const size_t count = 10;
        HgiTextureHandleVector arenaTextures;
        arenaTextures.reserve(count * numTextureAttachments);
        for (size_t i = 0; i < count; i++) {
            GfVec3i size = GfVec3i(i+1, i+1, 1);
            HgiGraphicsCmdsDesc desc = 
                _CreateGraphicsCmdsColor0Color1Depth(hgiGL, size, _imgFormat);
            HgiGraphicsCmdsUniquePtr cmds= hgiGL.CreateGraphicsCmds(desc);

            std::copy(desc.colorTextures.begin(), desc.colorTextures.end(),
                      std::back_inserter(arenaTextures));
            arenaTextures.push_back(desc.depthTexture);

            hgiGL.SubmitCmds(cmds.get());
        }
        
        _LogCaseHeader(out,
            "Case 3: Set custom context arena and add 10 framebuffer entries");
        out << device;
        _LogCaseFooter(out);
    }

    // 4. Switch to default arena
    // We should see the same output as in case 2.
    {
        hgiGL.SetContextArena(HgiGLContextArenaHandle());
        _LogCaseHeader(out, "Case 4: Switch back to default arena");
        out << device;
        _LogCaseFooter(out);
    }

    // 5. Destroy custom arena
    // Framebuffers created in step 3 will be deleted during the
    // context arena d'tor. We won't be able to log the arena once it is
    // destroyed, and use an error mark to ensure framebuffer objects are
    // successfully deleted. Note that the texture resources haven't been
    // destroyed at this point.
    // Note that the env var HGIGL_CONTEXT_ARENA_REPORT_ERRORS is enabled.
    {
        TfErrorMark mark;
        hgiGL.DestroyContextArena(&arena);
        _LogCaseHeader(out, "Case 5: Destroy custom context arena");
        if (mark.IsClean()) {
            out << "SUCCESS" << std::endl;
        } else {
            out << "FAILURE" << std::endl;
        }
        _LogCaseFooter(out);
    }

    // 6. Similar to 5; On destruction of the HgiGL instance once it goes out of
    // scope, the default arena's framebuffers will be deleted.
    // Again, this happens regardless of whether the texture attachments
    // were destroyed.
    // If this wasn't successful, the error mark in main() would flag it.

    out.close();

    return true;
}

static bool
TestGraphicsCmdsClear()
{
    HgiGL hgiGL;

    const size_t width = _imgSize;
    const size_t height = _imgSize;
    const HgiFormat format = _imgFormat;

    // Create a default cmds description and set the clearValue for the
    // first attachment to something other than black.
    // Setting 'loadOp' tp 'Clear' is important for this test since we expect
    // the attachment to be cleared when the graphics cmds is created.
    HgiGraphicsCmdsDesc desc =_CreateGraphicsCmdsColor0Color1Depth(
        hgiGL, GfVec3i(width, height, 1), format);
    desc.colorAttachmentDescs[0].loadOp = HgiAttachmentLoadOpClear;
    desc.colorAttachmentDescs[0].storeOp = HgiAttachmentStoreOpStore;
    desc.colorAttachmentDescs[0].clearValue = GfVec4f(1, 0, 0.5, 1);

    // We expect attachment0 to be cleared when the cmds is created via
    // the loadOp property in desc.
    HgiGraphicsCmdsUniquePtr gfxCmds = hgiGL.CreateGraphicsCmds(desc);
    hgiGL.SubmitCmds(gfxCmds.get());

    // Save attachment0 to disk
    _SaveGpuTextureToFile(
        hgiGL, 
        desc.colorTextures[0], 
        width, 
        height, 
        format, 
        "graphicsCmdsClear.png");

    // Cleanup
    for (HgiTextureHandle& tex : desc.colorTextures) {
        hgiGL.DestroyTexture(&tex);
    }
    if (desc.depthTexture) {
        hgiGL.DestroyTexture(&desc.depthTexture);
    }

    return true;
}

bool
TestCreateSrgbaTexture()
{
    HgiGL hgiGL;

    const size_t width = 128;
    const size_t height = 128;
    const HgiFormat format = HgiFormatUNorm8Vec4srgb;

    const size_t dataByteSize = width * height * HgiGetDataSizeOfFormat(format);

    // Create the texture
    std::vector<uint8_t> textureData(dataByteSize, 64);
    HgiTextureHandle tex = _CreateTexture(
        hgiGL, width, height, format, textureData.data());
    
    // Write texture to file
    static std::string const filePath = "srgba.png";
    _SaveGpuTextureToFile(hgiGL, tex, width, height, format, filePath); 
    
    hgiGL.DestroyTexture(&tex);

    return true;
}

bool
TestHgiGetMipInitialData()
{
    // Test helper function that is used during mipmap data upload.
    // (This does not actually upload any data)
    const HgiFormat format = HgiFormatUNorm8Vec4;
    const GfVec3i size0(37,53,1);
    const size_t layerCount = 1;

    const size_t texelByteSize = HgiGetDataSizeOfFormat(format);
    const size_t firstMipSize = size0[0] * size0[1] * size0[2] * texelByteSize;

    // We expect the second mip to be 1/4 of the bytesize of the first etc.
    GfVec3i size1 = size0 / 2;
    size1[2] = 1;

    const size_t secondMipSize = size1[0] * size1[1] * size1[2] * texelByteSize;
    
    GfVec3i size2 = size1 / 2;
    size2[2] = 1;

    const size_t thirdMipSize = size2[0] * size2[1] * size2[2] * texelByteSize;

    // Create some fake mipmap data for all three mips
    const size_t totalSize = firstMipSize + secondMipSize + thirdMipSize;

    const std::vector<HgiMipInfo> mipInfos = HgiGetMipInfos(
        format, size0, layerCount, totalSize);

    if (mipInfos.size() != 3) {
        TF_CODING_ERROR("TestHgiGetMipInfos returned wrong number of infos");
        return false;
    }

    // We expect the returned ptr to be at the start of the third mip's data
    // in the 'mipData' vector. And the returned dimensions and bytesize to
    // match the third mip.
    const size_t startOfThirdMip = firstMipSize + secondMipSize;

    if (mipInfos[2].dimensions != size2 ||
        mipInfos[2].byteSizePerLayer != thirdMipSize ||
        mipInfos[2].byteOffset != startOfThirdMip) {
        TF_CODING_ERROR("TestHgiGetMipInitialData incorrect return values");
        return false;
    }

    return true;
}

bool
TestHgiTextureToBufferCopy()
{
    HgiGL hgiGL;

    const int width = 128;
    const int height = 128;
    const HgiFormat format = HgiFormatUNorm8Vec4srgb;
    
    const size_t dataByteSize = width * height * HgiGetDataSizeOfFormat(format);

    // Create the texture
    std::vector<uint8_t> textureData(dataByteSize, 16);
    HgiTextureHandle tex = _CreateTexture(
        hgiGL, width, height, format, textureData.data());

    // Create the buffer
    HgiBufferHandle buf = _CreateBuffer(hgiGL, dataByteSize, nullptr);
    
    // Copy texture to buffer
    HgiTextureToBufferOp copyOp;
    copyOp.gpuSourceTexture = tex;
    copyOp.gpuDestinationBuffer = buf;
    copyOp.byteSize = dataByteSize;
    HgiBlitCmdsUniquePtr blitCmds = hgiGL.CreateBlitCmds();
    blitCmds->CopyTextureToBuffer(copyOp);
    hgiGL.SubmitCmds(blitCmds.get());

    static std::string const filePath = "copyTextureToBuffer.png";
    _SaveGpuBufferToFile(hgiGL, buf, width, height, format, filePath);

    hgiGL.DestroyBuffer(&buf);
    hgiGL.DestroyTexture(&tex);

    return true;
}

bool
TestHgiBufferToTextureCopy()
{
    HgiGL hgiGL;

    const int width = 128;
    const int height = 128;
    const HgiFormat format = HgiFormatUNorm8Vec4srgb;
    
    const size_t dataByteSize = width * height * HgiGetDataSizeOfFormat(format);

    // Create the buffer
    std::vector<uint8_t> bufferData(dataByteSize, 32);
    HgiBufferHandle buf = _CreateBuffer(
        hgiGL, dataByteSize, bufferData.data());

    // Create the texture
    HgiTextureHandle tex =
        _CreateTexture(hgiGL, width, height, format, nullptr);
    
    // Copy buffer to texture
    HgiBufferToTextureOp copyOp;
    copyOp.gpuSourceBuffer = buf;
    copyOp.gpuDestinationTexture = tex;
    copyOp.byteSize = dataByteSize;
    HgiBlitCmdsUniquePtr blitCmds = hgiGL.CreateBlitCmds();
    blitCmds->CopyBufferToTexture(copyOp);
    hgiGL.SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    static std::string const filePath = "copyBufferToTexture.png";
    _SaveGpuTextureToFile(hgiGL, tex, width, height, format, filePath);

    hgiGL.DestroyTexture(&tex);
    hgiGL.DestroyBuffer(&buf);

    return true;
}

class HgiGLUnitTestWindow : public GarchGLDebugWindow {
public:
    HgiGLUnitTestWindow(const char *title, int width, int height)
    : GarchGLDebugWindow(title, width, height)
    {}

    ~HgiGLUnitTestWindow() {}

    void OnInitializeGL() override {
        GarchGLApiLoad();
    }

    void OnUninitializeGL() override {}
};

static bool
TestHgiGL()
{
    // Setup OpenGL context
    HgiGLUnitTestWindow unitTestWindow("hgiGL", _imgSize, _imgSize);
    unitTestWindow.Init();

    // Run tests

    bool ret = true;
    std::cout <<  "*** Running test: HgiGL" << std::endl << std::endl;

    // Test descriptor cache
    ret &= TestContextArenaAndFramebufferCache();
    if (!ret) {
        TF_CODING_ERROR("TestContextArenaAndFramebufferCache failed");
        return false;
    }

    // Test clearing attachment0 in graphics cmds
    ret &= TestGraphicsCmdsClear();
    if (!ret) {
        TF_CODING_ERROR("TestGraphicsCmdsClear failed");
        return false;
    }

    // Test saving a SRGBA texture
    ret &= TestCreateSrgbaTexture();
    if (!ret) {
        TF_CODING_ERROR("TestCreateTexture failed");
        return false;
    }

    // Test getting texel data for mips
    ret &= TestHgiGetMipInitialData();
    if (!ret) {
        TF_CODING_ERROR("TestHgiGetMipInitialData failed");
        return false;
    }

    // Test copying a GPU texture to a GPU buffer via HgiTextureToBufferOp
    ret &= TestHgiTextureToBufferCopy();
    if (!ret) {
        TF_CODING_ERROR("TestHgiTextureToBufferCopy failed");
        return false;
    }

    // Test copying a GPU buffer to a GPU texture via HgiBufferToTextureOp
    ret &= TestHgiBufferToTextureCopy();
    if (!ret) {
        TF_CODING_ERROR("TestHgiBufferToTextureCopy failed");
        return false;
    }

    return ret;
}

int
main(int argc, char **argv)
{
    TfErrorMark mark;
    bool passed = TestHgiGL();

    if (passed && mark.IsClean()) {
        std::cout << "HgiGL: Passed" << std::endl;
        exit(0);
    } else {
        for (auto it = mark.begin(); it != mark.end(); it++) {
            std::cout << it->GetCommentary() << std::endl;
        }
        std::cout << "HgiGL: Failed" << std::endl;
        exit(1);
    }

}
