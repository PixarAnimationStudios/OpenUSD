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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdSt/glfTextureCpuData.h"

#include "pxr/imaging/glf/baseTextureData.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

HgiTextureType
_GetTextureType(const int numDimensions)
{
    switch(numDimensions) {
    case 2:
        return HgiTextureType2D;
    case 3:
        return HgiTextureType3D;
    default:
        TF_CODING_ERROR("Unsupported number of dimensions");
        return HgiTextureType2D;
    }
}

bool
_IsValid(GlfBaseTextureDataConstRefPtr const &textureData)
{
    return
        textureData->ResizedWidth() > 0 &&
        textureData->ResizedHeight() > 0 &&
        textureData->ResizedDepth() > 0 &&
        textureData->HasRawBuffer();
}

using _Data = std::unique_ptr<const unsigned char[]>;

template<typename T, T alpha>
_Data
_ConvertRGBToRGBA(
    const void * const data,
    const size_t numTargetBytes)
{
    TRACE_FUNCTION();

    const T * const typedData = reinterpret_cast<const T*>(data);

    std::unique_ptr<unsigned char[]> result =
        std::make_unique<unsigned char[]>(numTargetBytes);

    T * const typedConvertedData = reinterpret_cast<T*>(result.get());

    const size_t num = numTargetBytes / (4 * sizeof(T));

    for (size_t i = 0; i < num; i++) {
        typedConvertedData[4 * i + 0] = typedData[3 * i + 0];
        typedConvertedData[4 * i + 1] = typedData[3 * i + 1];
        typedConvertedData[4 * i + 2] = typedData[3 * i + 2];
        typedConvertedData[4 * i + 3] = alpha;
    }

    return std::move(result);
}

enum _ColorSpaceTransform
{
     _SRGBToLinear,
     _LinearToSRGB
};

// Convert a [0, 1] value between color spaces
template<_ColorSpaceTransform colorSpaceTransform>
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
std::unique_ptr<const unsigned char[]>
_PremultiplyAlpha(
    const void * const data,
    const size_t numTargetBytes)
{
    TRACE_FUNCTION();

    static_assert(std::numeric_limits<T>::is_integer, "Requires integral type");

    const T * const typedData = reinterpret_cast<const T*>(data);

    std::unique_ptr<unsigned char[]> result =
        std::make_unique<unsigned char[]>(numTargetBytes);

    T * const typedConvertedData = reinterpret_cast<T*>(result.get());

    // Perform all operations using floats.
    constexpr float max = static_cast<float>(std::numeric_limits<T>::max());

    const size_t num = numTargetBytes / (4 * sizeof(T));

    for (size_t i = 0; i < num; i++) {
        const float alpha = static_cast<float>(typedData[4 * i + 3]) / max;

        for (size_t j = 0; j < 3; j++) {
            float p = static_cast<float>(typedData[4 * i + j]);

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
            typedConvertedData[4 * i + j] = p + 0.5f;  
        }
        typedConvertedData[4 * i + 3] = typedData[4 * i + 3];
    }

    return std::move(result);
}

// Pre-multiply alpha function to be used for floating point types
template<typename T>
std::unique_ptr<const unsigned char[]>
_PremultiplyAlphaFloat(
    const void * const data,
    const size_t numTargetBytes)
{
    TRACE_FUNCTION();

    static_assert(GfIsFloatingPoint<T>::value, "Requires floating point type");

    const T * const typedData = reinterpret_cast<const T*>(data);

    std::unique_ptr<unsigned char[]> result =
        std::make_unique<unsigned char[]>(numTargetBytes);

    T * const typedConvertedData = reinterpret_cast<T*>(result.get());

    const size_t num = numTargetBytes / (4 * sizeof(T));

    for (size_t i = 0; i < num; i++) {
        const float alpha = typedData[4 * i + 3];

        // Pre-multiply RGB values with alpha.
        for (size_t j = 0; j < 3; j++) {
            typedConvertedData[4 * i + j] = typedData[4 * i + j] * alpha;
        }
        typedConvertedData[4 * i + 3] = typedData[4 * i + 3];
    }

    return std::move(result);
}

using _ConversionFunction = _Data(*)(const void * data, size_t numTargetBytes);

void
_GetHgiFormatAndConversionFunction(
    const GLenum glFormat,
    const GLenum glType,
    const GLenum glInternalFormat,
    const bool premultiplyAlpha,
    HgiFormat * const hgiFormat,
    _ConversionFunction * const conversionFunction)
{
    // Format dispatch, mostly we can just use the CPU buffer from
    // the texture data provided.
    switch(glFormat) {
    case GL_RED:
        switch(glType) {
        case GL_UNSIGNED_BYTE:
            *hgiFormat = HgiFormatUNorm8;
            break;
        case GL_HALF_FLOAT:
            *hgiFormat = HgiFormatFloat16;
            break;
        case GL_FLOAT:
            *hgiFormat = HgiFormatFloat32;
            break;
        default:
            TF_CODING_ERROR("Unsupported texture format GL_RED 0x%04x",
                            glType);
            break;
        }
        break;
    case GL_RG:
        switch(glType) {
        case GL_UNSIGNED_BYTE:
            *hgiFormat = HgiFormatUNorm8Vec2;
            break;
        case GL_HALF_FLOAT:
            *hgiFormat = HgiFormatFloat16Vec2;
            break;
        case GL_FLOAT:
            *hgiFormat = HgiFormatFloat32Vec2;
            break;
        default:
            TF_CODING_ERROR("Unsupported texture format GL_RG 0x%04x",
                            glType);
            break;
        }
        break;
    case GL_RGB:
        switch(glType) {
        case GL_UNSIGNED_BYTE:
            // RGB (24bit) is not supported on MTL, so we need to convert it.
            *conversionFunction = _ConvertRGBToRGBA<unsigned char, 255>;
            if (glInternalFormat == GL_SRGB8) {
                *hgiFormat = HgiFormatUNorm8Vec4srgb;
            } else {
                *hgiFormat = HgiFormatUNorm8Vec4;
            }
            break;
        case GL_HALF_FLOAT:
            *hgiFormat = HgiFormatFloat16Vec3;
            break;
        case GL_FLOAT:
            *hgiFormat = HgiFormatFloat32Vec3;
            break;
        default:
            TF_CODING_ERROR("Unsupported texture format GL_RGB 0x%04x",
                            glType);
            break;
        }
        break;
    case GL_RGBA:
        switch(glType) {
        case GL_UNSIGNED_BYTE: 
        {
            const bool isSRGB = (glInternalFormat == GL_SRGB8_ALPHA8);
            if (isSRGB) {
                *hgiFormat = HgiFormatUNorm8Vec4srgb;
            } else {
                *hgiFormat = HgiFormatUNorm8Vec4;
            }
            
            if (premultiplyAlpha) {
                if (isSRGB) {
                    *conversionFunction =
                        _PremultiplyAlpha<unsigned char, /* isSRGB = */ true>;
                } else {
                    *conversionFunction =
                        _PremultiplyAlpha<unsigned char, /* isSRGB = */ false>;
                }
            }
            break;
        }
        case GL_HALF_FLOAT:
            if (premultiplyAlpha) {
                *conversionFunction = _PremultiplyAlphaFloat<GfHalf>;
            }

            *hgiFormat = HgiFormatFloat16Vec4;
            break;
        case GL_FLOAT:
            if (premultiplyAlpha) {
                *conversionFunction = _PremultiplyAlphaFloat<float>;
            }

            *hgiFormat = HgiFormatFloat32Vec4;
            break;
        default:
            TF_CODING_ERROR("Unsupported texture format GL_RGBA 0x%04x",
                            glType);
            break;
        }
        break;
    case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
        switch(glType) {
        case GL_FLOAT:
            *hgiFormat = HgiFormatBC6UFloatVec3;
            break;
        default:
            TF_CODING_ERROR(
                "Unsupported texture format "
                "GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x%04x",
                glType);
        }
        break;
    case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
        switch(glType) {
        case GL_FLOAT:
            *hgiFormat = HgiFormatBC6FloatVec3;
            break;
        default:
            TF_CODING_ERROR(
                "Unsupported texture format "
                "GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x%04x",
                glType);
            break;
        }
        break;
    case GL_COMPRESSED_RGBA_BPTC_UNORM:
        switch(glType) {
        case GL_UNSIGNED_BYTE:
            *hgiFormat = HgiFormatBC7UNorm8Vec4;
            break;
        default:
            TF_CODING_ERROR(
                "Unsupported texture format "
                "GL_COMPRESSED_RGBA_BPTC_UNORM 0x%04x",
                glType);
            break;
        }
        break;
    case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
        switch(glType) {
        case GL_UNSIGNED_BYTE:
            *hgiFormat = HgiFormatBC7UNorm8Vec4srgb;
        default:
            TF_CODING_ERROR(
                "Unsupported texture format "
                "GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x%04x",
                glType);
            break;
        }
        break;
    default:
        TF_CODING_ERROR("Unsupported texture format 0x%04x 0x%04x",
                        glFormat, glType);
        break;
    }
}

} // anonymous namespace

HdStGlfTextureCpuData::HdStGlfTextureCpuData(
    GlfBaseTextureDataConstRefPtr const &textureData,
    const std::string &debugName,
    const bool useOrGenerateMipmaps,
    const bool premultiplyAlpha)
  : _generateMipmaps(false)
{
    TRACE_FUNCTION();

    _textureDesc.debugName = debugName;

    // Bail if we don't have texture data.
    if (!textureData) {
        return;
    }

    // Sanity checks
    if (!_IsValid(textureData)) {
        return;
    }

    // If there is no file at the given path, we should have bailed
    // by now and left _textureDesc.initalData null indicating to
    // our clients that the texture is invalid.

    // Is this 2D or 3D texture?
    _textureDesc.type = _GetTextureType(textureData->NumDimensions());

    // Determine the format (e.g., float/byte, RED/RGBA) and give
    // function to convert data if necessary.
    // Possible conversions are:
    // - Unsigned byte RGB to RGBA (since the former is not support
    //   by modern graphics APIs)
    // - Pre-multiply alpha.
    _ConversionFunction conversionFunction = nullptr;
    _GetHgiFormatAndConversionFunction(
        textureData->GLFormat(),
        textureData->GLType(),
        textureData->GLInternalFormat(),
        premultiplyAlpha,
        &_textureDesc.format,
        &conversionFunction);

    // Handle grayscale textures by expanding value to green and blue.
    if (HgiGetComponentCount(_textureDesc.format) == 1) {
        _textureDesc.componentMapping = {
            HgiComponentSwizzleR,
            HgiComponentSwizzleR,
            HgiComponentSwizzleR,
            HgiComponentSwizzleOne
        };
    }

    _textureDesc.dimensions = GfVec3i(
        textureData->ResizedWidth(),
        textureData->ResizedHeight(),
        textureData->ResizedDepth());

    const std::vector<HgiMipInfo> mipInfos = HgiGetMipInfos(
        _textureDesc.format,
        _textureDesc.dimensions);

    // How many mipmaps to use from the file.
    unsigned int numGivenMipmaps = 1;

    if (useOrGenerateMipmaps) {
        numGivenMipmaps = textureData->GetNumMipLevels();
        if (numGivenMipmaps > 1) {
            // Use mipmaps from file.
            if (numGivenMipmaps > mipInfos.size()) {
                TF_CODING_ERROR("Too many mip maps in texture data.");
                numGivenMipmaps = mipInfos.size();
            }
            _textureDesc.mipLevels = numGivenMipmaps;
        } else {
            // No mipmaps in file, generate mipmaps on GPU.
            _generateMipmaps = true;
            _textureDesc.mipLevels = mipInfos.size();
        }
    }
    const HgiMipInfo &mipInfo = mipInfos[numGivenMipmaps - 1];

    // Size of initial data.
    _textureDesc.pixelsByteSize = mipInfo.byteOffset + mipInfo.byteSize;

    if (conversionFunction) {
        // Convert the texture data
        _convertedData = conversionFunction(
            textureData->GetRawBuffer(), _textureDesc.pixelsByteSize);
        // Point to converted data
        _textureDesc.initialData = _convertedData.get();
    } else {
        // Ensure that texture data are not deleted
        _textureData = textureData;
        // Point to raw buffer inside texture data
        _textureDesc.initialData = textureData->GetRawBuffer();
    }
}

HdStGlfTextureCpuData::~HdStGlfTextureCpuData() = default;

const
HgiTextureDesc &
HdStGlfTextureCpuData::GetTextureDesc() const
{
    return _textureDesc;
}

bool
HdStGlfTextureCpuData::GetGenerateMipmaps() const
{
    return _generateMipmaps;
}

bool
HdStGlfTextureCpuData::IsValid() const
{
    return _textureDesc.initialData;
}

PXR_NAMESPACE_CLOSE_SCOPE
