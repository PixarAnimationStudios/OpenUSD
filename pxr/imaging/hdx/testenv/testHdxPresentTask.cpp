//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Unit test for HdxPresentTask.

#include "pxr/imaging/garch/glApi.h"
#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgi/tokens.h"
#include "pxr/imaging/hgi/types.h"
#include "pxr/imaging/hgiInterop/hgiInterop.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/vt/value.h"
#include "pxr/pxr.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

// ---- test texture layout ---------------------------------------------------
//
// 4x4 RGBA8 texture stored bottom-to-top (GL convention, row 0 = bottom).
// The four 2x2 quadrants each hold a distinct solid color:
//
//   (GL y-up)              rows 2-3  BLUE  | WHITE
//                          rows 0-1  RED   | GREEN
//                                  cols 0-1  cols 2-3
//
// UV (0,0) = bottom-left = RED.
// UV (1,1) = top-right   = WHITE.
//
namespace {

const int kW = 4;
const int kH = 4;

const std::array<uint8_t, 4> kRed   = {255,   0,   0, 255};
const std::array<uint8_t, 4> kGreen = {  0, 255,   0, 255};
const std::array<uint8_t, 4> kBlue  = {  0,   0, 255, 255};
const std::array<uint8_t, 4> kWhite = {255, 255, 255, 255};

std::vector<uint8_t>
BuildTestPixels()
{
    std::vector<uint8_t> data(static_cast<size_t>(kW) * kH * 4);
    for (int row = 0; row < kH; ++row) {
        for (int col = 0; col < kW; ++col) {
            const uint8_t *c;
            if (row < 2) { c = col < 2 ? kRed.data()   : kGreen.data(); }
            else         { c = col < 2 ? kBlue.data()  : kWhite.data(); }
            const int i = (row * kW + col) * 4;
            data[i+0] = c[0]; data[i+1] = c[1];
            data[i+2] = c[2]; data[i+3] = c[3];
        }
    }
    return data;
}

std::vector<uint8_t>
ReadPixels(GLuint fbo, int w, int h)
{
    std::vector<uint8_t> buf(static_cast<size_t>(w) * h * 4, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf.data());
    return buf;
}

// Returns true if every pixel in the buffer matches the expected color.
bool
AllPixelsMatch(const std::vector<uint8_t> &px, int w, int h,
               const uint8_t *expected)
{
    for (int i = 0; i < w * h; ++i) {
        const int o = i * 4;
        if (px[o] != expected[0] || px[o+1] != expected[1] || px[o+2] != expected[2]) {
            return false;
        }
    }
    return true;
}

} // namespace

int
main(int /*argc*/, char * /*argv*/[])
{
    GarchGLDebugWindow window("testHdxPresentTask", 256, 256);
    window.Init();

    // ---- create Hgi and source texture -------------------------------------

    HgiUniquePtr hgi = Hgi::CreatePlatformDefaultHgi();

    const std::vector<uint8_t> pixelData = BuildTestPixels();

    HgiTextureDesc texDesc;
    texDesc.debugName    = "testSrcRegionTexture";
    texDesc.dimensions   = GfVec3i(kW, kH, 1);
    texDesc.format       = HgiFormatUNorm8Vec4;
    texDesc.usage        = HgiTextureUsageBitsShaderRead;
    texDesc.initialData  = pixelData.data();
    texDesc.pixelsByteSize = pixelData.size() * sizeof(uint8_t);

    HgiTextureHandle srcTex = hgi->CreateTexture(texDesc);
    TF_VERIFY(srcTex);

    // Use GL_NEAREST so that each sample lands exactly in one texel; with a
    // 4x4 source and 4x4 dest the UV centers align perfectly with texel
    // centers (both at 0.125, 0.375, 0.625, 0.875), giving exact results.
    const auto texName = static_cast<GLuint>(srcTex->GetRawResource());
    glBindTexture(GL_TEXTURE_2D, texName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // ---- create destination FBO --------------------------------------------

    GLuint fbo = 0, fbTex = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &fbTex);
    glBindTexture(GL_TEXTURE_2D, fbTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kW, kH,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, fbTex, 0);
    TF_VERIFY(glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
              GL_FRAMEBUFFER_COMPLETE);

    HgiInterop interop;
    const GfVec4i dstRegion(0, 0, kW, kH);
    const VtValue dstFb(static_cast<uint32_t>(fbo));
    bool allPassed = true;

    // Helper: clear FBO, blit with given srcRegion, read back.
    auto blit = [&](const GfVec4f &srcRegion) -> std::vector<uint8_t> {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClearColor(0.F, 0.F, 0.F, 0.F);
        glClear(GL_COLOR_BUFFER_BIT);
        interop.TransferToApp(hgi.get(), srcTex, HgiTextureHandle{},
                              HgiTokens->OpenGL, dstFb, dstRegion, srcRegion);
        glFinish();
        return ReadPixels(fbo, kW, kH);
    };

    auto check = [&](const char *name, const GfVec4f &srcRegion,
                     const uint8_t *expected) {
        const auto px = blit(srcRegion);
        if (!AllPixelsMatch(px, kW, kH, expected)) {
            std::cerr << name << " FAILED: pixel(0,0)=("
                      << int(px[0]) << "," << int(px[1]) << "," << int(px[2])
                      << "), expected (" << int(expected[0]) << ","
                      << int(expected[1]) << "," << int(expected[2]) << ")\n";
            allPassed = false;
        } else {
            std::cout << "  " << name << ": OK\n";
        }
    };

    // ---- test cases --------------------------------------------------------

    // 1. Identity srcRegion: verify all 16 output pixels reproduce the source
    //    texture exactly (quadrant colors in the correct positions).
    {
        const auto px = blit(GfVec4f(0.F, 0.F, 1.F, 1.F));
        bool ok = true;
        for (int row = 0; row < kH && ok; ++row) {
            for (int col = 0; col < kW && ok; ++col) {
                const uint8_t *exp;
                if (row < 2) { exp = col < 2 ? kRed.data()   : kGreen.data(); }
                else         { exp = col < 2 ? kBlue.data()  : kWhite.data(); }
                const int i = (row * kW + col) * 4;
                if (px[i] != exp[0] || px[i+1] != exp[1] || px[i+2] != exp[2]) {
                    ok = false;
                }
            }
        }
        if (!ok) {
            TF_RUNTIME_ERROR("identity srcRegion FAILED");
            allPassed = false;
        } else {
            std::cout << "  identity srcRegion: OK\n";
        }
    }

    // 2-5. Crop to each quadrant: all output pixels should be that quadrant's
    //      color.
    check("bottom-left quadrant (RED)",
          GfVec4f(0.F,  0.F,  0.5F, 0.5F), kRed.data());
    check("bottom-right quadrant (GREEN)",
          GfVec4f(0.5F, 0.F,  0.5F, 0.5F), kGreen.data());
    check("top-left quadrant (BLUE)",
          GfVec4f(0.F,  0.5F, 0.5F, 0.5F), kBlue.data());
    check("top-right quadrant (WHITE)",
          GfVec4f(0.5F, 0.5F, 0.5F, 0.5F), kWhite.data());

    // ---- cleanup -----------------------------------------------------------

    hgi->DestroyTexture(&srcTex);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &fbTex);

    if (allPassed) {
        std::cout << "OK\n";
        return EXIT_SUCCESS;
    }
    std::cout << "FAILED\n";
    return EXIT_FAILURE;
}
