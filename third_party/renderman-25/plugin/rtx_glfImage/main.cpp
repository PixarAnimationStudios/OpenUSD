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
#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hio/types.h"
#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/trace/trace.h"

#include <mutex>

class RixContext;

// We want to use some pxr namespace types here, while staying in
// the top-level namespace so that prman can find the expected symbols.
PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Per TextureCtx user data.
struct RtxHioImagePluginUserData {
    HioImageSharedPtr image;
    bool flipped;

    std::mutex mipLevelsMutex;
    std::vector<HioImage::StorageSpec> mipLevels;
};

/// A Renderman Rtx texture plugin that uses HioImage to read files,
/// allowing support for additional file types beyond .tex.
class RtxHioImagePlugin : public RtxPlugin {
public:
    RtxHioImagePlugin(RixContext* rixCtx, const char* pluginName);
    ~RtxHioImagePlugin() override;

    // Convenience to get the user data as a RtxHioImagePluginUserData*.
    RtxHioImagePluginUserData* data(TextureCtx& tCtx) {
        return static_cast<RtxHioImagePluginUserData*>(tCtx.userData);
    }

    // RtxPlugin overrides
    int Open(TextureCtx& tCtx) override;
    int Fill(TextureCtx& tCtx, FillRequest& fillReq) override;
    int Close(TextureCtx& userData) override;

private:
    RixMessages* m_msgHandler;
};

RtxHioImagePlugin::RtxHioImagePlugin(RixContext *rixCtx, const char *pluginName) :
    m_msgHandler(rixCtx ? (RixMessages *)rixCtx->GetRixInterface(k_RixMessages) : nullptr)
{
}

RtxHioImagePlugin::~RtxHioImagePlugin()
{
    // Do nothing
}

static bool
_ConvertWrapMode(HioAddressMode hioWrapMode, RixMessages *msgs,
                 const std::string &filename,
                 RtxPlugin::TextureCtx::WrapMode *rmanWrapMode)
{
    switch(hioWrapMode) {
    case HioAddressModeRepeat:
        *rmanWrapMode = RtxPlugin::TextureCtx::k_Periodic;
        return true;
    case HioAddressModeMirrorRepeat:
        if(msgs) {
            msgs->ErrorAlways(
                "RtxHioImagePlugin: "
                "Texture %s has unsupported HioAddressModeMirrorRepeat; using "
                "k_Periodic instead.",
                filename.c_str());
        }
        *rmanWrapMode = RtxPlugin::TextureCtx::k_Periodic;
        return true;
    case HioAddressModeClampToEdge:
        *rmanWrapMode = RtxPlugin::TextureCtx::k_Clamp;
        return true;
    case HioAddressModeClampToBorderColor:
        if(msgs) {
            msgs->ErrorAlways(
                "RtxHioImagePlugin: "
                "Texture %s has unsupported HioAddressModeClampToBorderColor; using "
                "k_Black instead.",
                filename.c_str());
        }
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

template<class T>
static void
_ConvertToFloatAndFill(
    const HioImage::StorageSpec& level, 
    RtxPlugin::FillRequest& fillReq)
{
    TRACE_FUNCTION();
    
    const int numImageChannels = HioGetComponentCount(level.format);

    const int startX = fillReq.tile.offset.X * fillReq.tile.size.X;
    const int startY = fillReq.tile.offset.Y * fillReq.tile.size.Y;
    const int endY = startY + fillReq.tile.size.Y;

    const T *src = reinterpret_cast<const T*>(level.data) 
        + (startY * level.width + startX) * numImageChannels
        + (fillReq.channelOffset);
    float *dest = (float*) fillReq.tileData;

    // Get all channels in image.
    if (fillReq.channelOffset == 0 && fillReq.numChannels == numImageChannels) {
        for (int y = startY; y < endY; y++) {
            const T *s = src;
            for (int x = 0; x < numImageChannels * fillReq.tile.size.X; 
                 x++, s++, dest++) {
                *dest = static_cast<float>(*s);
            }
            src += numImageChannels * level.width;
        }
    } else {
        for (int y = startY; y < endY; y++) {
            const T *s = src;
            for (int x = 0; x < fillReq.tile.size.X; x++) {
                for (int i = 0; i < fillReq.numChannels; i++, s++, dest++) {
                    *dest = static_cast<float>(*s);
                }
                s += numImageChannels - fillReq.numChannels;
            }
            src += numImageChannels * level.width;
        }
    }
}

int
RtxHioImagePlugin::Open(TextureCtx& tCtx)
{
    tCtx.userData = nullptr;

    // Parse args.
    std::string filename;
    std::string wrapS, wrapT;
    std::string flipped;
    HioImage::SourceColorSpace sourceColorSpace = HioImage::Auto;
    for (unsigned int i = 0; i < tCtx.argc; i += 2) {
        if (strcmp(tCtx.argv[i], "filename") == 0) {
            filename = tCtx.argv[i + 1];
        } else if (strcmp(tCtx.argv[i], "wrapS") == 0) {
            wrapS = tCtx.argv[i + 1];
        } else if (strcmp(tCtx.argv[i], "wrapT") == 0) {
            wrapT = tCtx.argv[i + 1];
        } else if (strcmp(tCtx.argv[i], "flipped") == 0) {
            flipped = tCtx.argv[i + 1];
        } else if (strcmp(tCtx.argv[i], "sourceColorSpace") == 0) {
            if (strcmp(tCtx.argv[i + 1], "sRGB") == 0) {
                sourceColorSpace = HioImage::SRGB;
            } else if (strcmp(tCtx.argv[i + 1], "raw") == 0) {
                sourceColorSpace = HioImage::Raw;
            }
        }
    }

    // Open HioImage.
    HioImageSharedPtr image = HioImage::OpenForReading(filename, 0, 0, 
        sourceColorSpace);
    if (!image) {
        if(m_msgHandler) {
            m_msgHandler->ErrorAlways(
                "RtxHioImagePlugin %p: "
                "failed to open '%s'\n", this, filename.c_str());
        }
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
    tCtx.numChannels = HioGetComponentCount(image->GetFormat());
    // Component data type.
    HioType channelType = HioGetHioType(image->GetFormat());
    switch (channelType) {
    case HioTypeSignedByte:
    case HioTypeUnsignedShort:
    case HioTypeSignedShort:
    case HioTypeUnsignedInt:
    case HioTypeInt:
    case HioTypeHalfFloat:
    case HioTypeFloat:
    case HioTypeDouble:
        // If not float, will convert data to float during Fill
        tCtx.dataType = TextureCtx::k_Float;
        break;
    case HioTypeUnsignedByte:
        tCtx.dataType = TextureCtx::k_Byte;
        break;
    default:
        if(m_msgHandler) {
            m_msgHandler->ErrorAlways(
                "RtxHioImagePlugin %p: "
                "unsupported data type for %s\n", this, filename.c_str());
        }
        return 1;
    }
    // Wrapping mode.
    // The wrap mode can be specified in the plugin arguments.
    // If "useMetadata" is given, or nothing is specified, then
    // fall back to check metadata in the texture asset.
    tCtx.sWrap = TextureCtx::k_Black;
    tCtx.tWrap = TextureCtx::k_Black;
    HioAddressMode wrapModeS, wrapModeT;
    if (wrapS.empty() || wrapS == "useMetadata") {
        if (image->GetSamplerMetadata(HioAddressDimensionU, &wrapModeS)) {
            _ConvertWrapMode(wrapModeS, m_msgHandler, filename, &tCtx.sWrap);
        }
    } else if (wrapS == "black") {
        tCtx.sWrap = RtxPlugin::TextureCtx::k_Black;
    } else if (wrapS == "clamp") {
        tCtx.sWrap = RtxPlugin::TextureCtx::k_Clamp;
    } else if (wrapS == "repeat") {
        tCtx.sWrap = RtxPlugin::TextureCtx::k_Periodic;
    }
    if (wrapT.empty() || wrapT == "useMetadata") {
        if (image->GetSamplerMetadata(HioAddressDimensionV, &wrapModeT)) {
            _ConvertWrapMode(wrapModeT, m_msgHandler, filename, &tCtx.tWrap);
        }
    } else if (wrapT == "black") {
        tCtx.tWrap = RtxPlugin::TextureCtx::k_Black;
    } else if (wrapT == "clamp") {
        tCtx.tWrap = RtxPlugin::TextureCtx::k_Clamp;
    } else if (wrapT == "repeat") {
        tCtx.tWrap = RtxPlugin::TextureCtx::k_Periodic;
    }

    // Allocate storage for this context.  Renderman will
    // request as tiles, which we will service from this buffer.
    RtxHioImagePluginUserData* data = new RtxHioImagePluginUserData();
    tCtx.userData = data;
    data->image = image;

    // Flip vertically. The default is true.
    data->flipped = (flipped != "false");

    return 0;
}

int
RtxHioImagePlugin::Fill(TextureCtx& tCtx, FillRequest& fillReq)
{
    RtxHioImagePluginUserData* data = this->data(tCtx);
    assert(nullptr != data);

    // Find (or create) appropriate MIP level.
    HioImage::StorageSpec level;
    level.flipped = data->flipped;
    {
        // Lock mutex while scanning or modifying mipLevels.
        std::lock_guard<std::mutex> lock(data->mipLevelsMutex);
        for (HioImage::StorageSpec &cachedLevel: data->mipLevels) {
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

            if (tCtx.dataType != TextureCtx::k_Byte && 
                tCtx.dataType != TextureCtx::k_Float) {
                if(m_msgHandler) {
                    m_msgHandler->ErrorAlways(
                        "RtxHioImagePlugin %p: unsupported data type\n", this);
                }
                return 1;
            }

            const int numBytes = level.width * level.height * level.depth;
            level.data = new char[numBytes];
            data->image->Read(level);
            data->mipLevels.push_back(level);
        }
    }

    const HioType channelType = HioGetHioType(data->image->GetFormat());

    if (channelType == HioTypeSignedByte) {
        _ConvertToFloatAndFill<signed char>(level, fillReq);
    } else if (channelType == HioTypeUnsignedShort) {
        _ConvertToFloatAndFill<uint16_t>(level, fillReq);
    } else if (channelType == HioTypeSignedShort) {
        _ConvertToFloatAndFill<int16_t>(level, fillReq);
    } else if (channelType == HioTypeUnsignedInt) {
        _ConvertToFloatAndFill<uint32_t>(level, fillReq);
    } else if (channelType == HioTypeInt) {
        _ConvertToFloatAndFill<int32_t>(level, fillReq);
    } else if (channelType == HioTypeDouble) {
        _ConvertToFloatAndFill<double>(level, fillReq);
    } else if (channelType == HioTypeHalfFloat) {
        _ConvertToFloatAndFill<GfHalf>(level, fillReq);
    } else {
        const int numImageChannels = HioGetComponentCount(level.format);
        const int bytesPerChannel = HioGetDataSizeOfType(channelType);

        // Copy out tile data, one row at a time.
        const int bytesPerImagePixel = level.depth;
        const int bytesPerImageRow = bytesPerImagePixel * level.width;
        const int bytesPerTilePixel = bytesPerChannel * fillReq.numChannels;
        const int bytesPerTileRow = bytesPerTilePixel * fillReq.tile.size.X;
        const int startX = fillReq.tile.offset.X * fillReq.tile.size.X;
        const int startY = fillReq.tile.offset.Y * fillReq.tile.size.Y;
        const int endY = startY + fillReq.tile.size.Y;
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
    }

    // Make sure texture data is linear
    if (data->image->IsColorSpaceSRGB()) {
        if (tCtx.dataType == TextureCtx::k_Float) {
            _ConvertSRGBtoLinear(
                (float*)fillReq.tileData, 
                fillReq.tile.size.X * fillReq.tile.size.Y,
                fillReq.numChannels, fillReq.channelOffset);
        } else if (tCtx.dataType == TextureCtx::k_Byte) {
            _ConvertSRGBtoLinear(
                (unsigned char*)fillReq.tileData, 
                fillReq.tile.size.X * fillReq.tile.size.Y,
                fillReq.numChannels, fillReq.channelOffset);
        }
    }

    return 0;
}

int
RtxHioImagePlugin::Close(TextureCtx& tCtx)
{
    RtxHioImagePluginUserData* data = this->data(tCtx);
    if (nullptr != data) {
        for (HioImage::StorageSpec &cachedLevel: data->mipLevels) {
            delete [] (char*) cachedLevel.data;
        }
        delete data;
    }
    return 0;
}

}

RTXPLUGINCREATE
{
    return new RtxHioImagePlugin(rixCtx, pluginName);
}
