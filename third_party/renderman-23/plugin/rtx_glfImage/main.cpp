//
// Copyright 2019 Pixar
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

#include "RtxPlugin.h"
#include "RixInterfaces.h"
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/image.h"
#include "pxr/imaging/glf/utils.h"
#include "pxr/base/gf/gamma.h"
#include <mutex>

class RixContext;

// We want to use some pxr namespace types here, while staying in
// the top-level namespace so that prman can find the expected symbols.
PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Per TextureCtx user data.
struct RtxGlfImagePluginUserData {
    GlfImageSharedPtr image;

    std::mutex mipLevelsMutex;
    std::vector<GlfImage::StorageSpec> mipLevels;
};

/// A Renderman Rtx texture plugin that uses GlfImage to read files,
/// allowing support for additional file types beyond .tex.
class RtxGlfImagePlugin : public RtxPlugin {
public:
    RtxGlfImagePlugin(RixContext* rixCtx, const char* pluginName);
    ~RtxGlfImagePlugin() override;

    // Convenience to get the user data as a RtxGlfImagePluginUserData*.
    RtxGlfImagePluginUserData* data(TextureCtx& tCtx) {
        return static_cast<RtxGlfImagePluginUserData*>(tCtx.userData);
    }

    // RtxPlugin overrides
    int Open(TextureCtx& tCtx) override;
    int Fill(TextureCtx& tCtx, FillRequest& fillReq) override;
    int Close(TextureCtx& userData) override;

private:
    RixMessages* m_msgHandler;
};

RtxGlfImagePlugin::RtxGlfImagePlugin(RixContext *rixCtx, const char *pluginName) :
    m_msgHandler((RixMessages *)rixCtx->GetRixInterface(k_RixMessages))
{
}

RtxGlfImagePlugin::~RtxGlfImagePlugin()
{
    // Do nothing
}

static bool
_ConvertWrapMode(GLenum glWrapMode, RixMessages *msgs,
                 const std::string &filename,
                 RtxPlugin::TextureCtx::WrapMode *rmanWrapMode)
{
    switch(glWrapMode) {
    case GL_REPEAT:
        *rmanWrapMode = RtxPlugin::TextureCtx::k_Periodic;
        return true;
    case GL_MIRRORED_REPEAT:
        msgs->ErrorAlways(
            "RtxGlfImagePlugin: "
            "Texture %s has unsupported GL_MIRROR_REPEAT; using "
            "k_Periodic instead.",
            filename.c_str());
        *rmanWrapMode = RtxPlugin::TextureCtx::k_Periodic;
        return true;
    case GL_CLAMP_TO_EDGE:
        *rmanWrapMode = RtxPlugin::TextureCtx::k_Clamp;
        return true;
    case GL_CLAMP_TO_BORDER:
        msgs->ErrorAlways(
            "RtxGlfImagePlugin: "
            "Texture %s has unsupported GL_CLAMP_TO_BORDER; using "
            "k_Black instead.",
            filename.c_str());
        *rmanWrapMode = RtxPlugin::TextureCtx::k_Black;
        return true;
    default:
        return false;
    }
}

template <class T>
static void
_ConvertSRGBtoLinear(T *dest, unsigned nPixels, 
                     unsigned nChannels, unsigned channelOffset)
{
    const bool hasAlphaChannel = (channelOffset + nChannels == 4);

    T *s = dest;
    for (unsigned i=0; i<(nPixels*nChannels); i++) {

        // The alpha channel is generally linear already -- skip pixel
        const bool isAlphaChannel = 
            hasAlphaChannel && ((i + 1) % nChannels == 0);
        if (!isAlphaChannel) {
            *s = GfConvertDisplayToLinear(*s);
        }

        s++;
    }
}

int
RtxGlfImagePlugin::Open(TextureCtx& tCtx)
{
    tCtx.userData = nullptr;

    // Parse args.
    std::string filename;
    for (unsigned int i = 0; i < tCtx.argc; i += 2) {
        if (strcmp(tCtx.argv[i], "filename") == 0) {
            filename = tCtx.argv[i + 1];
        }
    }

    // Open GlfImage.
    GlfImageSharedPtr image = GlfImage::OpenForReading(filename);
    if (!image) {
        m_msgHandler->ErrorAlways(
            "RtxGlfImagePlugin %p: "
            "failed to open '%s'\n", this, filename.c_str());
        return 1;
    }

    // Set up Renderman texture context.
    tCtx.isLocked = false;
    tCtx.retryOnError = true;
    tCtx.numLayers = 1;
    tCtx.pyramidType = TextureCtx::k_MIP;
    tCtx.minRes.X = 1;
    tCtx.minRes.Y = 1;
    tCtx.maxRes.X = image->GetWidth();
    tCtx.maxRes.Y = image->GetHeight();
    // Component data type.
    switch (image->GetType()) {
    case GL_FLOAT:
        tCtx.dataType = TextureCtx::k_Float;
        tCtx.numChannels = image->GetBytesPerPixel() / sizeof(float);
        break;
    case GL_UNSIGNED_BYTE:
        tCtx.dataType = TextureCtx::k_Byte;
        tCtx.numChannels = image->GetBytesPerPixel();
        break;
    default:
        m_msgHandler->ErrorAlways(
            "RtxGlfImagePlugin %p: "
            "unsupported data type for %s\n", this, filename.c_str());
        return 1;
    }
    // Wrapping mode.
    tCtx.sWrap = TextureCtx::k_Black;
    tCtx.tWrap = TextureCtx::k_Black;
    GLenum wrapModeS, wrapModeT;
    if (image->GetSamplerMetadata(GL_TEXTURE_WRAP_S, &wrapModeS)) {
        _ConvertWrapMode(wrapModeS, m_msgHandler, filename, &tCtx.sWrap);
    }
    if (image->GetSamplerMetadata(GL_TEXTURE_WRAP_T, &wrapModeT)) {
        _ConvertWrapMode(wrapModeT, m_msgHandler, filename, &tCtx.tWrap);
    }

    // Allocate storage for this context.  Renderman will
    // request as tiles, which we will service from this buffer.
    RtxGlfImagePluginUserData* data = new RtxGlfImagePluginUserData();
    tCtx.userData = data;
    data->image = image;

    return 0;
}

int
RtxGlfImagePlugin::Fill(TextureCtx& tCtx, FillRequest& fillReq)
{
    RtxGlfImagePluginUserData* data = this->data(tCtx);
    assert(nullptr != data);

    // Find (or create) appropriate MIP level.
    GlfImage::StorageSpec level;
    level.flipped = true;
    {
        // Lock mutex while scanning or modifying mipLevels.
        std::lock_guard<std::mutex> lock(data->mipLevelsMutex);
        for (GlfImage::StorageSpec &cachedLevel: data->mipLevels) {
            if (cachedLevel.width == fillReq.imgRes.X &&
                cachedLevel.height == fillReq.imgRes.Y) {
                level = cachedLevel;
                break;
            }
        }
        if (!level.data) {
            // Allocate a new MIP level.
            level.width = fillReq.imgRes.X;
            level.height = fillReq.imgRes.Y;
            level.depth = data->image->GetBytesPerPixel();
            level.format = data->image->GetFormat();

            if (tCtx.dataType == TextureCtx::k_Byte) {
                level.type = GL_UNSIGNED_BYTE;
            } else if (tCtx.dataType == TextureCtx::k_Float) {
                level.type = GL_FLOAT;
            } else {
                m_msgHandler->ErrorAlways(
                    "RtxGlfImagePlugin %p: unsupported data type\n", this);
                return 1;
            }

            const int numBytes = level.width * level.height * level.depth;
            level.data = new char[numBytes];
            data->image->Read(level);
            data->mipLevels.push_back(level);
        }
    }

    const bool isSRGB = data->image->IsColorSpaceSRGB();
    const GLenum type = data->image->GetType();

    const int numImageChannels = GlfGetNumElements(level.format);
    const int bytesPerChannel = GlfGetElementSize(type);

    // Copy out tile data, one row at a time.
    const int bytesPerImagePixel = level.depth;
    const int bytesPerImageRow = bytesPerImagePixel * level.width;
    const int bytesPerTilePixel = bytesPerChannel * fillReq.numChannels;
    const int bytesPerTileRow = bytesPerTilePixel * fillReq.tile.size.X;
    const RtInt startX = fillReq.tile.offset.X * fillReq.tile.size.X;
    const RtInt startY = fillReq.tile.offset.Y * fillReq.tile.size.Y;
    const RtInt endY = startY + fillReq.tile.size.Y;
    char *src = (char*) level.data
        + (startY * level.width + startX) * bytesPerImagePixel
        + (fillReq.channelOffset * bytesPerChannel);
    char *dest = (char*) fillReq.tileData;

    // If fill request wants all channels in the image, just memcpy each row.
    // Otherwise we need to iterate over each pixel and copy just the 
    // requested channels.
    if (fillReq.channelOffset == 0 && 
        fillReq.numChannels == numImageChannels) {

        for (int y = startY; y < endY; y++) {
            memcpy(dest, src, bytesPerTileRow);
            src += bytesPerImageRow;
            dest += bytesPerTileRow;
        }
    }
    else {
        for (int y = startY; y < endY; y++) {
            for (char *d = dest, *dEnd = dest + bytesPerTileRow, *s = src; 
                 d != dEnd; d += bytesPerTilePixel, s += bytesPerImagePixel) {
                memcpy(d, s, bytesPerTilePixel);
            }
            src += bytesPerImageRow;
            dest += bytesPerTileRow;
        }
    }

    // Make sure texture data is linear
    if (isSRGB) {
        if (type == GL_FLOAT) {
            _ConvertSRGBtoLinear(
                (float*)fillReq.tileData, 
                fillReq.tile.size.X * fillReq.tile.size.Y,
                fillReq.numChannels, fillReq.channelOffset);
        } else if (type == GL_UNSIGNED_BYTE) {
            _ConvertSRGBtoLinear(
                (unsigned char*)fillReq.tileData, 
                fillReq.tile.size.X * fillReq.tile.size.Y,
                fillReq.numChannels, fillReq.channelOffset);
        }
    }

    return 0;
}

int
RtxGlfImagePlugin::Close(TextureCtx& tCtx)
{
    RtxGlfImagePluginUserData* data = this->data(tCtx);
    if (nullptr != data) {
        for (GlfImage::StorageSpec &cachedLevel: data->mipLevels) {
            delete [] (char*) cachedLevel.data;
        }
        delete data;
    }
    return 0;
}

}

RTXPLUGINCREATE
{
    return new RtxGlfImagePlugin(rixCtx, pluginName);
}
