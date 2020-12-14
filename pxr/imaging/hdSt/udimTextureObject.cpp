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
#include "pxr/imaging/hdSt/udimTextureObject.h"

#include "pxr/imaging/hdSt/subtextureIdentifier.h"
#include "pxr/imaging/hdSt/textureIdentifier.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgi/types.h"

#include "pxr/imaging/hio/image.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"

#include "pxr/usd/ar/resolver.h"

PXR_NAMESPACE_OPEN_SCOPE

bool HdStIsSupportedUdimTexture(std::string const& imageFilePath)
{
    return TfStringContains(imageFilePath, "<UDIM>");
}

///////////////////////////////////////////////////////////////////////////////
// Helpers

namespace {

struct _TextureSize {
    _TextureSize(uint32_t w, uint32_t h) : width(w), height(h) { }
    uint32_t width;
    uint32_t height;
};

struct _MipDesc {
    _MipDesc(const _TextureSize& s, HioImageSharedPtr&& i) :
        size(s), image(std::move(i)) { }
    _TextureSize size;
    HioImageSharedPtr image;
};

using _MipDescVector = std::vector<_MipDesc>;

static
_MipDescVector
_GetMipLevels(const TfToken& filePath,
              HioImage::SourceColorSpace sourceColorSpace)
{
    constexpr int maxMipReads = 32;
    _MipDescVector ret {};
    ret.reserve(maxMipReads);
    unsigned int prevWidth = std::numeric_limits<unsigned int>::max();
    unsigned int prevHeight = std::numeric_limits<unsigned int>::max();
    for (unsigned int mip = 0; mip < maxMipReads; ++mip) {
        HioImageSharedPtr image = HioImage::OpenForReading(filePath, 0, mip,
                                                           sourceColorSpace);
        if (image == nullptr) {
            break;
        }
        const unsigned int currHeight = std::max(1, image->GetWidth());
        const unsigned int currWidth = std::max(1, image->GetHeight());
        if (currWidth < prevWidth &&
            currHeight < prevHeight) {
            prevWidth = currWidth;
            prevHeight = currHeight;
            ret.push_back({{currWidth, currHeight}, std::move(image)});
        }
    }
    return ret;
};

static
HgiFormat
_GetPixelFormatForImage(const HioFormat hioFormat)
{
    constexpr HgiFormat hgiFormats[][4] = {
        // HioTypeUnsignedByte
        { HgiFormatUNorm8, HgiFormatUNorm8Vec2,
          HgiFormatUNorm8Vec4, HgiFormatUNorm8Vec4 },
        // HioTypeUnsignedByteSRGB
        { HgiFormatInvalid, HgiFormatInvalid,
          HgiFormatUNorm8Vec4srgb, HgiFormatUNorm8Vec4srgb },
        // HioTypeSignedByte
        { HgiFormatSNorm8, HgiFormatSNorm8Vec2,
          HgiFormatSNorm8Vec4, HgiFormatSNorm8Vec4 },
        // HioTypeUnsignedShort
        { HgiFormatUInt16, HgiFormatUInt16Vec2,
          HgiFormatUInt16Vec4, HgiFormatUInt16Vec4 },
        // HioTypeSignedShort
        { HgiFormatInvalid, HgiFormatInvalid,
          HgiFormatInvalid, HgiFormatInvalid },
        // HioTypeUnsignedInt
        { HgiFormatInvalid, HgiFormatInvalid,
          HgiFormatInvalid, HgiFormatInvalid },
        // HioTypeInt
        { HgiFormatInt32, HgiFormatInt32Vec2,
          HgiFormatInt32Vec4, HgiFormatInt32Vec4 },
        // HioTypeHalfFloat
        { HgiFormatFloat16, HgiFormatFloat16Vec2,
          HgiFormatFloat16Vec4, HgiFormatFloat16Vec4 },
        // HioTypeFloat
        { HgiFormatFloat32, HgiFormatFloat32Vec2,
          HgiFormatFloat32Vec4, HgiFormatFloat32Vec4 },
        // HioTypeDouble
        { HgiFormatInvalid, HgiFormatInvalid,
          HgiFormatInvalid, HgiFormatInvalid },
    };

    static_assert(TfArraySize(hgiFormats) == HioTypeCount,
                  "hgiFormats to HioType enum mismatch");

    const uint32_t nChannels = HioGetComponentCount(hioFormat);
    const HioType hioType = HioGetHioType(hioFormat);
    return hgiFormats[hioType][nChannels - 1];
}

enum _ColorSpaceTransform
{
     _SRGBToLinear,
     _LinearToSRGB
};

// Convert a [0, 1] value between color spaces
template<_ColorSpaceTransform colorSpaceTransform>
static
float _ConvertColorSpace(const float in)
{
    float out = in;
    if (colorSpaceTransform == _SRGBToLinear) {
        if (in <= 0.04045) {
            out = in / 12.92;
        } else {
            out = pow((in + 0.055) / 1.055, 2.4);
        }
    } else if (colorSpaceTransform == _LinearToSRGB) {
        if (in <= 0.0031308) {
            out = 12.92 * in;
        } else {
            out = 1.055 * pow(in, 1.0 / 2.4) - 0.055;
        }
    }

    return GfClamp(out, 0.f, 1.f);
}

// Pre-multiply alpha function to be used for integral types
template<typename T, bool isSRGB>
static
void
_PremultiplyAlpha(
    T * const data,
    const GfVec3i &dimensions)
{
    TRACE_FUNCTION();

    static_assert(std::numeric_limits<T>::is_integer, "Requires integral type");

    const size_t num = dimensions[0] * dimensions[1] * dimensions[2];

    // Perform all operations using floats.
    const float max = static_cast<float>(std::numeric_limits<T>::max());

    for (size_t i = 0; i < num; i++) {
        const float alpha = static_cast<float>(data[4 * i + 3]) / max;

        for (size_t j = 0; j < 3; j++) {
            float p = static_cast<float>(data[4 * i + j]);

            if (isSRGB) {
                // Convert value from sRGB to linear.
                p = max * _ConvertColorSpace<_SRGBToLinear>(p / max);
            }

            // Pre-multiply RGB values with alpha in linear space.
            p *= alpha;

            if (isSRGB) {
                // Convert value from linear to sRGB.
                p = max * _ConvertColorSpace<_LinearToSRGB>(p / max);
            }

            // Add 0.5 when converting float to integral type.
            data[4 * i + j] = p + 0.5f;
        }
    }
}

// Pre-multiply alpha function to be used for floating point types
template<typename T>
static
void _PremultiplyAlphaFloat(
    T * const data,
    const GfVec3i &dimensions)
{
    TRACE_FUNCTION();

    static_assert(GfIsFloatingPoint<T>::value, "Requires floating point type");

    const size_t num = dimensions[0] * dimensions[1] * dimensions[2];

    for (size_t i = 0; i < num; i++) {
        const float alpha = data[4 * i + 3];

        // Pre-multiply RGB values with alpha.
        for (size_t j = 0; j < 3; j++) {
            data[4 * i + j] = data[4 * i + j] * alpha;
        }
    }
}

template<typename T, uint32_t alpha>
void
_ConvertRGBToRGBA(
    void * const data,
    const size_t numPixels)
{
    TRACE_FUNCTION();

    T * const typedData = reinterpret_cast<T*>(data);

    size_t i = numPixels;
    while(i--) {
        typedData[4 * i + 0] = typedData[3 * i + 0];
        typedData[4 * i + 1] = typedData[3 * i + 1];
        typedData[4 * i + 2] = typedData[3 * i + 2];
        typedData[4 * i + 3] = T(alpha);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Udim texture

static const char UDIM_PATTERN[] = "<UDIM>";
static const int UDIM_START_TILE = 1001;
static const int UDIM_END_TILE = 1100;

// Split a udim file path such as /someDir/myFile.<UDIM>.exr into a
// prefix (/someDir/myFile.) and suffix (.exr).
static
std::pair<std::string, std::string>
_SplitUdimPattern(const std::string &path)
{
    static const std::string pattern(UDIM_PATTERN);

    const std::string::size_type pos = path.find(pattern);

    if (pos != std::string::npos) {
        return { path.substr(0, pos), path.substr(pos + pattern.size()) };
    }

    return { std::string(), std::string() };
}

// Find all udim tiles for a given udim file path /someDir/myFile.<UDIM>.exr as
// pairs, e.g., (0, /someDir/myFile.1001.exr), ...
//
// The scene delegate is assumed to already have resolved the asset path with
// the <UDIM> pattern to a "file path" with the <UDIM> pattern as above.
// This function will replace <UDIM> by different integers and check whether
// the "file" exists using an ArGetResolver.
//
// Note that the ArGetResolver is still needed, for, e.g., usdz file
// where the path we get from the scene delegate is
// /someDir/myFile.usdz[myImage.<UDIM>.EXR] and we need to use the
// ArGetResolver to check whether, e.g., myImage.1001.EXR exists in
// the zip file /someDir/myFile.usdz by calling
// resolver.Resolve(/someDir/myFile.usdz[myImage.1001.EXR]).
// However, we don't need to bind, e.g., the usd stage's resolver context
// because that part of the resolution will be done by the scene delegate
// for us already.
//
static
std::vector<std::tuple<int, TfToken>>
_FindUdimTiles(const std::string &filePath)
{
    std::vector<std::tuple<int, TfToken>> result;

    // Get prefix and suffix from udim pattern.
    const std::pair<std::string, std::string>
        splitPath = _SplitUdimPattern(filePath);
    if (splitPath.first.empty() && splitPath.second.empty()) {
        TF_WARN("Expected udim pattern but got '%s'.",
                filePath.c_str());
        return result;
    }

    ArResolver& resolver = ArGetResolver();

    for (int i = UDIM_START_TILE; i < UDIM_END_TILE; i++) {
        // Add integer between prefix and suffix and see whether
        // the tile exists by consulting the resolver.
        const std::string resolvedPath =
            resolver.Resolve(
                splitPath.first + std::to_string(i) + splitPath.second);
        if (!resolvedPath.empty()) {
            // Record pair in result.
            result.emplace_back(i - UDIM_START_TILE, resolvedPath);
        }
    }

    return result;
}
} // anonymous namespace

HdStUdimTextureObject::HdStUdimTextureObject(
    const HdStTextureIdentifier &textureId,
    HdSt_TextureObjectRegistry * const textureObjectRegistry)
  : HdStTextureObject(textureId, textureObjectRegistry)
  , _dimensions(0)
  , _mipCount(0)
  , _numBytesPerPixel(0)
  , _hgiFormat(HgiFormatInvalid)
{
}

HdStUdimTextureObject::~HdStUdimTextureObject()
{
    _DestroyTextures();
}

void
HdStUdimTextureObject::_DestroyTextures()
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
HdStUdimTextureObject::_Load()
{
    const std::vector<std::tuple<int, TfToken>> tiles =
        _FindUdimTiles(GetTextureIdentifier().GetFilePath());
    const HioImage::SourceColorSpace sourceColorSpace =
        _GetSourceColorSpace(GetTextureIdentifier().GetSubtextureIdentifier());
    const _MipDescVector firstImageMips =
        _GetMipLevels(std::get<1>(tiles[0]), sourceColorSpace);
    const bool premultipliedAlpha =
        _GetPremultiplyAlpha(GetTextureIdentifier().GetSubtextureIdentifier());

    if (firstImageMips.empty()) {
        return;
    }

    const HioFormat hioFormat = firstImageMips[0].image->GetFormat();
    int numChannels = HioGetComponentCount(hioFormat);
    const size_t sizePerElem = HioGetDataSizeOfType(hioFormat);

    _hgiFormat = _GetPixelFormatForImage(hioFormat);
    if (firstImageMips[0].image->IsColorSpaceSRGB()) {
        _hgiFormat = HgiFormatUNorm8Vec4srgb;
    }

    const bool convertRGBtoRGBA = numChannels == 3;
    if (convertRGBtoRGBA) {
        numChannels = 4;
    }

    const unsigned int maxTileCount =
        std::get<0>(tiles.back()) + 1;
    _dimensions[2] = static_cast<int>(tiles.size());
    const unsigned int numBytesPerPixel = sizePerElem * numChannels;
    const unsigned int numBytesPerPixelLayer =
        numBytesPerPixel * _dimensions[2];

    size_t targetPixelCount =
        static_cast<size_t>(GetTargetMemory());
    const bool loadAllTiles = targetPixelCount == 0;
    targetPixelCount /= _dimensions[2] * numBytesPerPixel;

    std::vector<_TextureSize> mips {};
    mips.reserve(firstImageMips.size());
    if (firstImageMips.size() == 1) {
        unsigned int width = firstImageMips[0].size.width;
        unsigned int height = firstImageMips[0].size.height;
        while (true) {
            mips.emplace_back(width, height);
            if (width == 1 && height == 1) {
                break;
            }
            width = std::max(1u, width / 2u);
            height = std::max(1u, height / 2u);
        }
        if (!loadAllTiles) {
            std::reverse(mips.begin(), mips.end());
        }
    } else {
        if (loadAllTiles) {
            for (_MipDesc const& mip: firstImageMips) {
                mips.push_back(mip.size);
            }
        } else {
            for (auto it = firstImageMips.crbegin();
                 it != firstImageMips.crend(); ++it) {
                mips.push_back(it->size);
            }
        }
    }

    unsigned int mipCount = mips.size();
    if (!loadAllTiles) {
        mipCount = 0;
        for (auto const& mip: mips) {
            const unsigned int currentPixelCount = mip.width * mip.height;
            if (targetPixelCount <= currentPixelCount) {
                break;
            }
            ++mipCount;
            targetPixelCount -= currentPixelCount;
        }

        if (mipCount == 0) {
            mips.clear();
            mips.emplace_back(1, 1);
            mipCount = 1;
        } else {
            mips.resize(mipCount, {0, 0});
            std::reverse(mips.begin(), mips.end());
        }
    }

    std::vector<std::vector<uint8_t>> mipData;
    mipData.resize(mipCount);

    _dimensions[0] = mips[0].width;
    _dimensions[1] = mips[0].height;

    // Texture array queries will use a float as the array specifier.
    _layoutData.resize(maxTileCount, 0);

    size_t totalTextureMemory = 0;
    for (unsigned int mip = 0; mip < mipCount; ++mip) {
        _TextureSize const& mipSize = mips[mip];
        const unsigned int currentMipMemory =
        mipSize.width * mipSize.height * numBytesPerPixelLayer;
        mipData[mip].resize(currentMipMemory, 0);
        totalTextureMemory += currentMipMemory;
    }

    WorkParallelForN(tiles.size(), [&](size_t begin, size_t end) {
        for (size_t tileId = begin; tileId < end; ++tileId) {
            std::tuple<int, TfToken> const& tile = tiles[tileId];
            _layoutData[std::get<0>(tile)] = tileId + 1;
            const _MipDescVector images = _GetMipLevels(
                std::get<1>(tile), sourceColorSpace);
            if (images.empty()) { continue; }
            for (unsigned int mip = 0; mip < mipCount; ++mip) {
                _TextureSize const& mipSize = mips[mip];
                const unsigned int numBytesPerLayer =
                    mipSize.width * mipSize.height * numBytesPerPixel;
                HioImage::StorageSpec spec;
                spec.width = mipSize.width;
                spec.height = mipSize.height;
                spec.format = hioFormat;
                spec.flipped = true;
                spec.data = mipData[mip].data() + (tileId * numBytesPerLayer);
                const auto it = std::find_if(
                    images.rbegin(), images.rend(),
                    [&mipSize](_MipDesc const& i) {
                        return mipSize.width <= i.size.width &&
                               mipSize.height <= i.size.height;});
                const _MipDesc &mipDesc = 
                    it == images.rend() ? images.front() : *it;
                mipDesc.image->Read(spec);

                // XXX: Unfortunately, pre-multiplication is occurring after
                // mip generation. However, it is still worth it to pre-multiply
                // textures before texture filtering.
                if (premultipliedAlpha && (numChannels == 4)) {
                    switch (_hgiFormat) {
                    case HgiFormatUNorm8Vec4srgb:
                        _PremultiplyAlpha<unsigned char, /*isSRGB=*/ true>(
                            reinterpret_cast<unsigned char *>(spec.data),
                            GfVec3i(mipSize.width, mipSize.height, 1));
                        break;
                    case HgiFormatUNorm8Vec4:
                        _PremultiplyAlpha<unsigned char, /*isSRGB=*/ false>(
                            reinterpret_cast<unsigned char *>(spec.data),
                            GfVec3i(mipSize.width, mipSize.height, 1));
                        break;
                    case HgiFormatFloat16Vec4:
                        _PremultiplyAlphaFloat<GfHalf>(
                            reinterpret_cast<GfHalf *>(spec.data),
                            GfVec3i(mipSize.width, mipSize.height, 1));
                        break;
                    case HgiFormatFloat32Vec4:
                        _PremultiplyAlphaFloat<float>(
                            reinterpret_cast<float *>(spec.data),
                            GfVec3i(mipSize.width, mipSize.height, 1));
                        break;
                    default:
                        TF_CODING_ERROR("Unsupported format");
                        break;
                    }
                } else if (convertRGBtoRGBA) {
                    switch (_hgiFormat) {
                    case HgiFormatUNorm8Vec4srgb:
                    case HgiFormatUNorm8Vec4:
                        _ConvertRGBToRGBA<unsigned char, 255>(
                            reinterpret_cast<unsigned char *>(spec.data),
                            mipSize.width * mipSize.height);
                        break;
                    case HgiFormatFloat16Vec4:
                        _ConvertRGBToRGBA<GfHalf, 1>(
                            reinterpret_cast<GfHalf *>(spec.data),
                            mipSize.width * mipSize.height);
                        break;
                    case HgiFormatFloat32Vec4:
                        _ConvertRGBToRGBA<float, 1>(
                            reinterpret_cast<float *>(spec.data),
                            mipSize.width * mipSize.height);
                        break;
                    default:
                        TF_CODING_ERROR("Unsupported format");
                        break;
                    }
                }
            }
        }
    }, 1);

    // Concatenate mipData into single memory block, ready for upload to GPU
    _textureData.resize(totalTextureMemory);
    size_t writeOffset = 0;
    for (unsigned int mip = 0; mip < mipCount; ++mip) {
        const size_t bytesSize = mipData[mip].size();
        memcpy(_textureData.data() + writeOffset,
               mipData[mip].data(), bytesSize);
        writeOffset += bytesSize;
    }

    _mipCount = mipData.size();
    _numBytesPerPixel = numBytesPerPixel;
}

void
HdStUdimTextureObject::_Commit()
{
    TRACE_FUNCTION();

    if (_hgiFormat == HgiFormatInvalid) {
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
        texDesc.type = HgiTextureType2DArray;
        texDesc.dimensions = GfVec3i(_dimensions[0], _dimensions[1], 1);
        texDesc.layerCount = _dimensions[2];
        texDesc.format = _hgiFormat;
        texDesc.mipLevels = _mipCount;
        texDesc.initialData = _textureData.data();
        texDesc.pixelsByteSize = _textureData.size();
        _texelTexture = hgi->CreateTexture(texDesc);
    }

    // Layout GPU texture creation
    {
        HgiTextureDesc texDesc;
        texDesc.debugName = _GetDebugName(GetTextureIdentifier());
        texDesc.type = HgiTextureType1D;
        texDesc.dimensions = GfVec3i(_layoutData.size(), 1, 1);
        texDesc.format = HgiFormatFloat32;
        texDesc.initialData = _layoutData.data();
        texDesc.pixelsByteSize = _layoutData.size() * sizeof(float);
        _layoutTexture = hgi->CreateTexture(texDesc);
    }

    // Free CPU memory after transfer to GPU
    _textureData.clear();
    _layoutData.clear();
}

bool
HdStUdimTextureObject::IsValid() const
{
    // Checking whether ptex texture is valid not supported yet.
    return true;
}

HdTextureType
HdStUdimTextureObject::GetTextureType() const
{
    return HdTextureType::Udim;
}

PXR_NAMESPACE_CLOSE_SCOPE
