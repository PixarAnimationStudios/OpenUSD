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
#include "pxr/imaging/hdSt/ptexTextureObject.h"

#include "pxr/imaging/hdSt/subtextureIdentifier.h"
#include "pxr/imaging/hdSt/textureIdentifier.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgi/types.h"

#include "pxr/imaging/hio/types.h"

#include "pxr/base/trace/trace.h"

#ifdef PXR_PTEX_SUPPORT_ENABLED
#include <Ptexture.h>
#include <PtexUtils.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
// Helpers

namespace {

using _Data = std::unique_ptr<uint8_t[]>;

template<typename T, uint32_t alpha>
_Data
_ConvertRGBToRGBA(
    const void * const data,
    const size_t numPixels)
{
    TRACE_FUNCTION();

    const T * const typedData = reinterpret_cast<const T*>(data);

    std::unique_ptr<uint8_t[]> result =
        std::make_unique<uint8_t[]>(numPixels * sizeof(T) * 4);

    T * const typedConvertedData = reinterpret_cast<T*>(result.get());

    for (size_t i = 0; i < numPixels; i++) {
        typedConvertedData[4 * i + 0] = typedData[3 * i + 0];
        typedConvertedData[4 * i + 1] = typedData[3 * i + 1];
        typedConvertedData[4 * i + 2] = typedData[3 * i + 2];
        typedConvertedData[4 * i + 3] = T(alpha);
    }

    return result;
}

using _ConversionFunction = _Data(*)(const void * data, size_t numTargetBytes);

// anonymous namespace
}

bool HdStIsSupportedPtexTexture(std::string const & imageFilePath)
{
#ifdef PXR_PTEX_SUPPORT_ENABLED
    return (TfStringEndsWith(imageFilePath, ".ptx") ||
            TfStringEndsWith(imageFilePath, ".ptex"));
#else
    return false;
#endif
}


///////////////////////////////////////////////////////////////////////////////
// Ptex texture

HdStPtexTextureObject::HdStPtexTextureObject(
    const HdStTextureIdentifier &textureId,
    HdSt_TextureObjectRegistry * const textureObjectRegistry)
  : HdStTextureObject(textureId, textureObjectRegistry)
  , _format(HgiFormatInvalid)
  , _numChannels(0)
  , _numBytesPerPixel(0)
  , _texelDimensions(0)
  , _layoutDimensions(0)
  , _numFaces(0)
{
}

HdStPtexTextureObject::~HdStPtexTextureObject()
{
    _DestroyTextures();
}

void
HdStPtexTextureObject::_DestroyTextures()
{
    if (Hgi * hgi = _GetHgi()) {
        if (_texelTexture) {
            hgi->DestroyTexture(&_texelTexture);
        }
        if (_layoutTexture) {
            hgi->DestroyTexture(&_layoutTexture);
        }
    }
}

void
HdStPtexTextureObject::_Load()
{
    TRACE_FUNCTION();

    _DestroyTextures();

    TRACE_SCOPE("HdStPtexTextureObject::_Load() (read ptex)");

    HioType hioFormat = HioTypeUnsignedByte;

    const unsigned char *loaderLayoutBuffer = nullptr;
    const unsigned char *loaderTexelBuffer = nullptr;

    size_t layoutNumInt32Vec3ToAlloc = 0;
    size_t layoutNumInt16ToCopy = 0;

#ifdef PXR_PTEX_SUPPORT_ENABLED
    const std::string & filename = GetTextureIdentifier().GetFilePath();

    const bool premultiplyAlpha =
        _GetPremultiplyAlpha(GetTextureIdentifier().GetSubtextureIdentifier());

    // create a temporary ptex cache
    // (required to build guttering pixels efficiently)
    static const int PTEX_MAX_CACHE_SIZE = 128 * 1024 * 1024;
    std::unique_ptr<PtexCache, std::function<void(PtexCache*)>> cache(
        PtexCache::create(1, PTEX_MAX_CACHE_SIZE, premultiplyAlpha),
            [](PtexCache* p) { p->release(); });
    if (!cache) {
        TF_WARN("Unable to create PtexCache");
        return;
    }

    // load
    Ptex::String ptexError;
    std::unique_ptr<PtexTexture, std::function<void(PtexTexture*)>> reader(
        cache->get(filename.c_str(), ptexError),
        [](PtexTexture* p) { p->release(); });
    if (!reader) {
        TF_WARN("Unable to open ptex %s : %s",
                filename.c_str(), ptexError.c_str());
        return;
    }

    // Read the ptexture data and pack the texels

    TRACE_SCOPE("HdStPtexTextureObject::_Load() (generate texture)");
    const size_t targetMemory = GetTargetMemory();

    // This is the minimum texture layers guaranteed by OpenGL 4.5 and Metal
    const size_t maxNumPages = 2048;

    // maxLevels = -1 : load all mip levels
    // maxLevels = 0  : load only the highest resolution
    constexpr int maxLevels = -1;
    HdStPtexMipmapTextureLoader loader(
            reader.get(), maxNumPages, maxLevels, targetMemory);

    const Ptex::DataType type = reader->dataType();
    if (type == Ptex::dt_float) {
        hioFormat = HioTypeFloat;
    } else if (type == Ptex::dt_half) {
        hioFormat = HioTypeHalfFloat;
    } else if (type == Ptex::dt_uint16) {
        hioFormat = HioTypeUnsignedShort;
    } else if (type == Ptex::dt_uint8) {
        hioFormat = HioTypeUnsignedByte;
    }

    // Texel data in memory buffer after load
    _numChannels = reader->numChannels();
    _numFaces = loader.GetNumFaces();
    _texelDimensions = GfVec3i(loader.GetPageWidth(),
                          loader.GetPageHeight(),
                          loader.GetNumPages());
    loaderLayoutBuffer = loader.GetLayoutBuffer();

    // Layout data in memory buffer after load
    const size_t maxTextureSize = 16384;
    const size_t layoutNumInt16 = _numFaces * 6; // 6 uint16_t per face
    const size_t layoutNumInt32Vec3 = layoutNumInt16 / 6; // 1 ivec3 per texel
    const size_t layoutNumInt32Vec3PerLayer = maxTextureSize;
    const size_t layoutNumLayers =
        (size_t) ceil((float)layoutNumInt32Vec3 / layoutNumInt32Vec3PerLayer);

    layoutNumInt32Vec3ToAlloc = layoutNumLayers * layoutNumInt32Vec3PerLayer;
    layoutNumInt16ToCopy = layoutNumInt16;
    _layoutDimensions = GfVec2i(layoutNumInt32Vec3PerLayer,
                                layoutNumLayers);
    loaderTexelBuffer = loader.GetTexelBuffer();
#endif

    if (!loaderLayoutBuffer || !loaderTexelBuffer) {
        return;
    }

    bool convertRGBtoRGBA = false;
    if (_numChannels == 3) {
        _numChannels = 4;
        convertRGBtoRGBA = true;
    }

    const size_t numPixels =
        _texelDimensions[0] * _texelDimensions[1] * _texelDimensions[2];
    _numBytesPerPixel = 0;
    _ConversionFunction conversionFunction = nullptr;

    if (hioFormat == HioTypeFloat) {
        static HgiFormat floatFormats[] =
            { HgiFormatFloat32, HgiFormatFloat32Vec2,
              HgiFormatFloat32Vec4, HgiFormatFloat32Vec4 };
        _format = floatFormats[_numChannels - 1];

        _numBytesPerPixel = _numChannels *  sizeof(float);

        if (convertRGBtoRGBA) {
            conversionFunction = _ConvertRGBToRGBA<float, 1>;
        }
    } else if (hioFormat == HioTypeUnsignedShort) {
        static HgiFormat uint16Formats[] =
            { HgiFormatInt32, HgiFormatInt32Vec2,
              HgiFormatInt32Vec4, HgiFormatInt32Vec4 };
        _format = uint16Formats[_numChannels - 1];

        _numBytesPerPixel = _numChannels * sizeof(uint16_t);

        if (convertRGBtoRGBA) {
            conversionFunction = _ConvertRGBToRGBA<uint16_t, 65535>;
        }
    } else if (hioFormat == HioTypeHalfFloat) {
        static HgiFormat halfFormats[] =
            { HgiFormatFloat16, HgiFormatFloat16Vec2,
              HgiFormatFloat16Vec4, HgiFormatFloat16Vec4 };
        _format = halfFormats[_numChannels - 1];

        _numBytesPerPixel = _numChannels * sizeof(GfHalf);

        if (convertRGBtoRGBA) {
            conversionFunction = _ConvertRGBToRGBA<GfHalf, 1>;
        }
    } else if (hioFormat == HioTypeUnsignedByte) {
        static HgiFormat uint8Formats[] =
            { HgiFormatUNorm8, HgiFormatUNorm8Vec2,
              HgiFormatUNorm8Vec4, HgiFormatUNorm8Vec4 };
        _format = uint8Formats[_numChannels - 1];

        _numBytesPerPixel = _numChannels * sizeof(uint8_t);

        if (convertRGBtoRGBA) {
            conversionFunction = _ConvertRGBToRGBA<uint8_t, 255>;
        }
    } else {
        _format = HgiFormatUNorm8;
        TF_CODING_ERROR("Unsupported format");
    }

    const size_t texelDataSize = numPixels * _numBytesPerPixel;
    if (conversionFunction) {
        _texelData =
            conversionFunction(loaderTexelBuffer, numPixels);
    } else {
        _texelData = std::make_unique<uint8_t[]>(texelDataSize);
        memcpy(_texelData.get(), loaderTexelBuffer, texelDataSize);
    }

    const size_t layoutDataSize = layoutNumInt16ToCopy * sizeof(uint16_t);
    _layoutData = std::make_unique<uint32_t[]>(3*layoutNumInt32Vec3ToAlloc);
    memcpy(_layoutData.get(), loaderLayoutBuffer, layoutDataSize);
}

void
HdStPtexTextureObject::_Commit()
{
    TRACE_FUNCTION();

    if (_format == HgiFormatInvalid) {
        return;
    }

    Hgi * const hgi = _GetHgi();
    if (!TF_VERIFY(hgi)) {
        return;
    }

    _DestroyTextures();

    // Texel GPU texture creation
    {
        HgiTextureDesc texDesc;
        texDesc.debugName = _GetDebugName(GetTextureIdentifier());
        texDesc.usage = HgiTextureUsageBitsShaderRead;
        texDesc.type = HgiTextureType2DArray;
        texDesc.dimensions = GfVec3i(_texelDimensions[0],
                                     _texelDimensions[1], 1);
        texDesc.layerCount = _texelDimensions[2];
        texDesc.format = _format;
        texDesc.mipLevels = 1;
        texDesc.initialData = _texelData.get();
        texDesc.pixelsByteSize = _numBytesPerPixel;
        _texelTexture = hgi->CreateTexture(texDesc);
    }

    // Layout GPU texture creation
    {
        // ptex layout struct (6 * uint16_t)
        // struct Layout {
        //     uint16_t page;
        //     uint16_t nMipmap;
        //     uint16_t u;
        //     uint16_t v;
        //     uint16_t adjSizeDiffs; //(4:4:4:4)
        //     uint8_t  width log2;
        //     uint8_t  height log2;
        // };

        HgiTextureDesc texDesc;
        texDesc.debugName = _GetDebugName(GetTextureIdentifier());
        texDesc.usage = HgiTextureUsageBitsShaderRead;
        texDesc.type = HgiTextureType1DArray;
        texDesc.dimensions = GfVec3i(_layoutDimensions[0], 1, 1);
        texDesc.layerCount = _layoutDimensions[1];
        texDesc.format = HgiFormatInt32Vec3;
        texDesc.mipLevels = 1;
        texDesc.initialData = _layoutData.get();
        texDesc.pixelsByteSize = sizeof(uint32_t);
        _layoutTexture = hgi->CreateTexture(texDesc);
    }

    // Free CPU data
    _texelData.reset();
    _layoutData.reset();
}

bool
HdStPtexTextureObject::IsValid() const
{
    // Checking whether ptex texture is valid not supported yet.
    return true;
}

HdTextureType
HdStPtexTextureObject::GetTextureType() const
{
    return HdTextureType::Ptex;
}

PXR_NAMESPACE_CLOSE_SCOPE
