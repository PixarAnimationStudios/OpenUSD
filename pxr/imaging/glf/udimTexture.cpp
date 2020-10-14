//
// Copyright 2018 Pixar
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
/// \file glf/udimTexture.cpp

#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/udimTexture.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec3i.h"

#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"

#include "pxr/imaging/glf/image.h"
#include "pxr/imaging/glf/utils.h"

#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

struct _TextureSize {
    _TextureSize(unsigned int w, unsigned int h) : width(w), height(h) { }
    unsigned int width;
    unsigned int height;
};

struct _MipDesc {
    _MipDesc(const _TextureSize& s, GlfImageSharedPtr&& i) :
        size(s), image(std::move(i)) { }
    _TextureSize size;
    GlfImageSharedPtr image;
};

using _MipDescArray = std::vector<_MipDesc>;

_MipDescArray _GetMipLevels(const TfToken& filePath, 
                            GlfImage::SourceColorSpace sourceColorSpace)
{
    constexpr int maxMipReads = 32;
    _MipDescArray ret {};
    ret.reserve(maxMipReads);
    unsigned int prevWidth = std::numeric_limits<int>::max();
    unsigned int prevHeight = std::numeric_limits<int>::max();
    for (unsigned int mip = 0; mip < maxMipReads; ++mip) {
        GlfImageSharedPtr image = GlfImage::OpenForReading(filePath, 0, mip, 
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

}

bool GlfIsSupportedUdimTexture(std::string const& imageFilePath)
{
    return TfStringContains(imageFilePath, "<UDIM>");
}

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<GlfUdimTexture, TfType::Bases<GlfTexture>>();
}

GlfUdimTexture::GlfUdimTexture(
    TfToken const& imageFilePath,
    GlfImage::ImageOriginLocation originLocation,
    std::vector<std::tuple<int, TfToken>>&& tiles,
    bool const premultiplyAlpha,
    GlfImage::SourceColorSpace sourceColorSpace)
    : GlfTexture(originLocation), _tiles(std::move(tiles)), 
      _premultiplyAlpha(premultiplyAlpha), _sourceColorSpace(sourceColorSpace)
{
}

GlfUdimTexture::~GlfUdimTexture()
{
    _FreeTextureObject();
}

GlfUdimTextureRefPtr
GlfUdimTexture::New(
    TfToken const& imageFilePath,
    GlfImage::ImageOriginLocation originLocation,
    std::vector<std::tuple<int, TfToken>>&& tiles,
    bool const premultiplyAlpha,
    GlfImage::SourceColorSpace sourceColorSpace)
{
    return TfCreateRefPtr(new GlfUdimTexture(
        imageFilePath, originLocation, std::move(tiles), premultiplyAlpha, 
        sourceColorSpace));
}

GlfTexture::BindingVector
GlfUdimTexture::GetBindings(
    TfToken const& identifier,
    GLuint samplerId)
{
    _ReadImage();
    BindingVector ret;
    ret.push_back(Binding(
        TfToken(identifier.GetString() + "_Images"), GlfTextureTokens->texels,
            GL_TEXTURE_2D_ARRAY, _imageArray, samplerId));
    ret.push_back(Binding(
        TfToken(identifier.GetString() + "_Layout"), GlfTextureTokens->layout,
            GL_TEXTURE_1D, _layout, 0));

    return ret;
}

GLuint
GlfUdimTexture::GetGlTextureName()
{
    _ReadImage();
    return _imageArray;
}

VtDictionary
GlfUdimTexture::GetTextureInfo(bool forceLoad)
{
    VtDictionary ret;

    if (forceLoad) {
        _ReadImage();
    }

    if (_loaded) {
        ret["memoryUsed"] = GetMemoryUsed();
        ret["width"] = _width;
        ret["height"] = _height;
        ret["depth"] = _depth;
        ret["format"] = _format;
        if (!_tiles.empty()) {
            ret["imageFilePath"] = std::get<1>(_tiles.front());
        }
    } else {
        ret["memoryUsed"] = size_t{0};
        ret["width"] = 0;
        ret["height"] = 0;
        ret["depth"] = 1;
        ret["format"] = _format;
    }
    ret["referenceCount"] = GetCurrentCount();
    return ret;
}

void
GlfUdimTexture::_FreeTextureObject()
{
    GlfSharedGLContextScopeHolder sharedGLContextScopeHolder;

    if (glIsTexture(_imageArray)) {
        glDeleteTextures(1, &_imageArray);
        _imageArray = 0;
    }

    if (glIsTexture(_layout)) {
        glDeleteTextures(1, &_layout);
        _layout = 0;
    }
}

// XXX: This code is duplicated in hdSt/textureObject.cpp, but will hopefully
// be removed from this file when Storm begins using Hgi for UDIM textures
namespace {
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
}

void
GlfUdimTexture::_ReadImage()
{
    TRACE_FUNCTION();

    if (_loaded) {
        return;
    }
    _loaded = true;
    _FreeTextureObject();

    if (_tiles.empty()) {
        return;
    }

    const _MipDescArray firstImageMips = _GetMipLevels(std::get<1>(_tiles[0]), 
                                                       _sourceColorSpace);

    if (firstImageMips.empty()) {
        return;
    }

    HioFormat hioFormat = firstImageMips[0].image->GetHioFormat();
    _format = GlfGetGLFormat(hioFormat);
    const GLenum type = GlfGetGLType(hioFormat);

    unsigned int numChannels = GlfGetNumElements(hioFormat);

    GLenum internalFormat = GL_RGBA8;
    unsigned int sizePerElem = 1;
    if (type == GL_FLOAT) {
        constexpr GLenum internalFormats[] =
            { GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F };
        internalFormat = internalFormats[numChannels - 1];
        sizePerElem = 4;
    } else if (type == GL_UNSIGNED_SHORT) {
        constexpr GLenum internalFormats[] =
            { GL_R16, GL_RG16, GL_RGB16, GL_RGBA16 };
        internalFormat = internalFormats[numChannels - 1];
        sizePerElem = 2;
    } else if (type == GL_HALF_FLOAT_ARB) {
        constexpr GLenum internalFormats[] =
            { GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F };
        internalFormat = internalFormats[numChannels - 1];
        sizePerElem = 2;
    } else if (type == GL_UNSIGNED_BYTE) {
        constexpr GLenum internalFormats[] =
            { GL_R8, GL_RG8, GL_RGB8, GL_RGBA8 };
        constexpr GLenum internalFormatsSRGB[] =
            { GL_R8, GL_RG8, GL_SRGB8, GL_SRGB8_ALPHA8 };    
        if (firstImageMips[0].image->IsColorSpaceSRGB()) {
            internalFormat = internalFormatsSRGB[numChannels - 1];
        } else {
            internalFormat = internalFormats[numChannels - 1];
        }
        sizePerElem = 1;
    }

    const unsigned int maxTileCount =
        std::get<0>(_tiles.back()) + 1;
    _depth = static_cast<int>(_tiles.size());
    const unsigned int numBytesPerPixel = sizePerElem * numChannels;
    const unsigned int numBytesPerPixelLayer = numBytesPerPixel * _depth;

    unsigned int targetPixelCount =
        static_cast<unsigned int>(GetMemoryRequested());
    const bool loadAllTiles = targetPixelCount == 0;
    targetPixelCount /= _depth * numBytesPerPixel;

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
                mips.emplace_back(mip.size);
            }
        } else {
            for (auto it = firstImageMips.crbegin();
                 it != firstImageMips.crend(); ++it) {
                mips.emplace_back(it->size);
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

    _width = mips[0].width;
    _height = mips[0].height;

    // Texture array queries will use a float as the array specifier.
    std::vector<float> layoutData;
    layoutData.resize(maxTileCount, 0);

    glGenTextures(1, &_imageArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _imageArray);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY,
        mipCount, internalFormat,
        _width, _height, _depth);

    size_t totalTextureMemory = 0;
    for (unsigned int mip = 0; mip < mipCount; ++mip) {
        _TextureSize const& mipSize = mips[mip];
        const unsigned int currentMipMemory =
            mipSize.width * mipSize.height * numBytesPerPixelLayer;
        mipData[mip].resize(currentMipMemory, 0);
        totalTextureMemory += currentMipMemory;
    }

    WorkParallelForN(_tiles.size(), [&](size_t begin, size_t end) {
        for (size_t tileId = begin; tileId < end; ++tileId) {
            std::tuple<int, TfToken> const& tile = _tiles[tileId];
            layoutData[std::get<0>(tile)] = tileId + 1;
            _MipDescArray images = _GetMipLevels(std::get<1>(tile), 
                                                 _sourceColorSpace);
            if (images.empty()) { continue; }
            for (unsigned int mip = 0; mip < mipCount; ++mip) {
                _TextureSize const& mipSize = mips[mip];
                const unsigned int numBytesPerLayer =
                    mipSize.width * mipSize.height * numBytesPerPixel;
                GlfImage::StorageSpec spec;
                spec.width = mipSize.width;
                spec.height = mipSize.height;
                spec.hioFormat = hioFormat;
                spec.flipped = true;
                spec.data = mipData[mip].data() + (tileId * numBytesPerLayer);
                const auto it = std::find_if(images.rbegin(), images.rend(),
                    [&mipSize](_MipDesc const& i)
                    { return mipSize.width <= i.size.width &&
                             mipSize.height <= i.size.height;});
                (it == images.rend() ? images.front() : *it).image->Read(spec);

                // XXX: Unfortunately, pre-multiplication is occurring after
                // mip generation. However, it is still worth it to pre-multiply
                // textures before texture filtering.
                if (_premultiplyAlpha && (numChannels == 4)) {
                    const bool isSRGB = (internalFormat == GL_SRGB8_ALPHA8);

                    switch (type) {
                    case GL_UNSIGNED_BYTE:
                        if (isSRGB) {
                            _PremultiplyAlpha<unsigned char, /*isSRGB=*/ true>(
                                reinterpret_cast<unsigned char *>(spec.data), 
                                GfVec3i(mipSize.width, mipSize.height, 1));
                        } else {
                            _PremultiplyAlpha<unsigned char, /*isSRGB=*/ false>(
                                reinterpret_cast<unsigned char *>(spec.data), 
                                GfVec3i(mipSize.width, mipSize.height, 1));   
                        }
                        break;
                    case GL_UNSIGNED_SHORT:
                        if (isSRGB) {
                            _PremultiplyAlpha<unsigned short, /*isSRGB=*/ true>(
                                reinterpret_cast<unsigned short *>(spec.data), 
                                GfVec3i(mipSize.width, mipSize.height, 1));
                        } else {
                            _PremultiplyAlpha<unsigned short, /*isSRGB=*/false>(
                                reinterpret_cast<unsigned short *>(spec.data), 
                                GfVec3i(mipSize.width, mipSize.height, 1));
                        }
                        break;
                    case GL_HALF_FLOAT_ARB:
                        _PremultiplyAlphaFloat<GfHalf>(
                            reinterpret_cast<GfHalf *>(spec.data), 
                            GfVec3i(mipSize.width, mipSize.height, 1));
                        break;  
                    case GL_FLOAT:
                        _PremultiplyAlphaFloat<float>(
                            reinterpret_cast<float *>(spec.data), 
                            GfVec3i(mipSize.width, mipSize.height, 1));
                        break;       
                    }
                }
            }
        }
    }, 1);

    for (unsigned int mip = 0; mip < mipCount; ++mip) {
        _TextureSize const& mipSize = mips[mip];
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, mip, 0, 0, 0,
                        mipSize.width, mipSize.height, _depth, _format, type,
                        mipData[mip].data());
    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    glGenTextures(1, &_layout);
    glBindTexture(GL_TEXTURE_1D, _layout);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, layoutData.size(), 0,
        GL_RED, GL_FLOAT, layoutData.data());
    glBindTexture(GL_TEXTURE_1D, 0);

    GLF_POST_PENDING_GL_ERRORS();

    _SetMemoryUsed(totalTextureMemory + _tiles.size() * sizeof(float));
}

void
GlfUdimTexture::_OnMemoryRequestedDirty()
{
    _loaded = false;
}

PXR_NAMESPACE_CLOSE_SCOPE
