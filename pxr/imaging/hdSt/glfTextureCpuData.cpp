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

// Compute the number of mip levels given the dimensions of a texture using
// the same formula as OpenGL.
uint16_t
_ComputeNumMipLevels(const GfVec3i &dimensions)
{
    const int dim = std::max({dimensions[0], dimensions[1], dimensions[2]});

    for (uint16_t i = 1; i < 8 * sizeof(int) - 1; i++) {
        const int powerTwo = 1 << i;
        if (powerTwo > dim) {
            return i;
        }
    }
    
    // Can never be reached, but compiler doesn't know that.
    return 1;
}

bool
_IsValid(GlfBaseTextureDataRefPtr const &textureData)
{
    return
        textureData->ResizedWidth() > 0 &&
        textureData->ResizedHeight() > 0 &&
        textureData->ResizedDepth() > 0 &&
        textureData->HasRawBuffer();
}

template<typename T>
std::unique_ptr<const unsigned char[]>
_ConvertRGBToRGBA(
    const unsigned char * const data,
    const GfVec3i &dimensions,
    const T alpha)
{
    TRACE_FUNCTION();

    const T * const typedData = reinterpret_cast<const T*>(data);

    const size_t num = dimensions[0] * dimensions[1] * dimensions[2];

    std::unique_ptr<unsigned char[]> result =
        std::make_unique<unsigned char[]>(num * 4 * sizeof(T));

    T * const typedConvertedData = reinterpret_cast<T*>(result.get());

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
    const GfVec3i &dimensions)
{
    TRACE_FUNCTION();

    static_assert(std::numeric_limits<T>::is_integer, "Requires integral type");

    const T * const typedData = reinterpret_cast<const T*>(data);

    const size_t num = dimensions[0] * dimensions[1] * dimensions[2];

    std::unique_ptr<unsigned char[]> result =
        std::make_unique<unsigned char[]>(num * 4 * sizeof(T));

    T * const typedConvertedData = reinterpret_cast<T*>(result.get());

    // Perform all operations using floats.
    const float max = static_cast<float>(std::numeric_limits<T>::max());    

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
    const GfVec3i &dimensions)
{
    TRACE_FUNCTION();

    static_assert(GfIsFloatingPoint<T>::value, "Requires floating point type");

    const T * const typedData = reinterpret_cast<const T*>(data);

    const size_t num = dimensions[0] * dimensions[1] * dimensions[2];

    std::unique_ptr<unsigned char[]> result =
        std::make_unique<unsigned char[]>(num * 4 * sizeof(T));

    T * const typedConvertedData = reinterpret_cast<T*>(result.get());

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

// Some of these formats have been aliased to HgiFormatInvalid because
// they are not available on MTL. Guard against us trying to use
// formats that are no longer available.
template<HgiFormat f>
constexpr HgiFormat
_CheckValid()
{
    static_assert(f != HgiFormatInvalid, "Invalid HgiFormat");
    return f;
}

} // anonymous namespace

HdStGlfTextureCpuData::HdStGlfTextureCpuData(
    GlfBaseTextureDataRefPtr const &textureData,
    const std::string &debugName,
    const bool generateMips,
    const bool premultiplyAlpha,
    const GlfImage::ImageOriginLocation originLocation)
  : _textureData(textureData)
{
    TRACE_FUNCTION();

    _textureDesc.debugName = debugName;

    // Bail if we don't have texture data.
    if (!textureData) {
        return;
    }

    // Read texture file
    if (!textureData->Read(0, false, originLocation)) {
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
    _textureDesc.dimensions = GfVec3i(
        textureData->ResizedWidth(),
        textureData->ResizedHeight(),
        textureData->ResizedDepth());
    // Image data - might need RGB to RGBA conversion.
    _textureDesc.initialData = textureData->GetRawBuffer();

    if (generateMips) {
        _textureDesc.mipLevels = _ComputeNumMipLevels(_textureDesc.dimensions);
    }

    // Determine the format (e.g., float/byte, RED/RGBA).
    // Convert data if necessary, setting initialData to the buffer
    // with the new data and freeing _textureData
    _textureDesc.format = _DetermineFormatAndConvertIfNecessary(
        textureData->GLFormat(),
        textureData->GLType(),
        textureData->GLInternalFormat(),
        premultiplyAlpha);

    // Handle grayscale textures by expanding value to green and blue.
    if (HgiGetComponentCount(_textureDesc.format) == 1) {
        _textureDesc.componentMapping = {
            HgiComponentSwizzleR,
            HgiComponentSwizzleR,
            HgiComponentSwizzleR,
            HgiComponentSwizzleOne
        };
    }

    size_t blockWidth, blockHeight;
    const size_t bytesPerBlock = HgiDataSizeOfFormat(
        _textureDesc.format, &blockWidth, &blockHeight);

    // Size of initial data (note that textureData->ComputeBytesUsed()
    // includes the mip maps).
    _textureDesc.pixelsByteSize =
        ((textureData->ResizedWidth()  + blockWidth  - 1) / blockWidth ) *
        ((textureData->ResizedHeight() + blockHeight - 1) / blockHeight) *
        textureData->ResizedDepth() *
        bytesPerBlock;
}

HdStGlfTextureCpuData::~HdStGlfTextureCpuData() = default;

const
HgiTextureDesc &
HdStGlfTextureCpuData::GetTextureDesc() const
{
    return _textureDesc;
}

bool
HdStGlfTextureCpuData::IsValid() const
{
    return _textureDesc.initialData;
}

HgiFormat
HdStGlfTextureCpuData::_DetermineFormatAndConvertIfNecessary(
    const GLenum glFormat,
    const GLenum glType,
    const GLenum glInternalFormat,
    const bool premultiplyAlpha)
{
    // Format dispatch, mostly we can just use the CPU buffer from
    // the texture data provided.
    switch(glFormat) {
    case GL_RED:
        switch(glType) {
        case GL_UNSIGNED_BYTE:
            return _CheckValid<HgiFormatUNorm8>();
        case GL_HALF_FLOAT:
            return _CheckValid<HgiFormatFloat16>();
        case GL_FLOAT:
            return _CheckValid<HgiFormatFloat32>();
        default:
            TF_CODING_ERROR("Unsupported texture format GL_RED 0x%04x",
                            glType);
            return HgiFormatInvalid;
        }
    case GL_RG:
        switch(glType) {
        case GL_UNSIGNED_BYTE:
            return _CheckValid<HgiFormatUNorm8Vec2>();
        case GL_HALF_FLOAT:
            return _CheckValid<HgiFormatFloat16Vec2>();
        case GL_FLOAT:
            return _CheckValid<HgiFormatFloat32Vec2>();
        default:
            TF_CODING_ERROR("Unsupported texture format GL_RG 0x%04x",
                            glType);
            return HgiFormatInvalid;
        }
    case GL_RGB:
        switch(glType) {
        case GL_UNSIGNED_BYTE:
            // RGB (24bit) is not supported on MTL, so we need to convert it.
            _convertedRawData = 
                _ConvertRGBToRGBA<unsigned char>(
                    reinterpret_cast<const unsigned char *>(
                        _textureDesc.initialData),
                    _textureDesc.dimensions,
                    255);
            // Point to the buffer with the converted data.
            _textureDesc.initialData = _convertedRawData.get();
            // Drop the old buffer.
            _textureData = TfNullPtr;

            if (glInternalFormat == GL_SRGB8) {
                return _CheckValid<HgiFormatUNorm8Vec4srgb>();
            } else {
                return _CheckValid<HgiFormatUNorm8Vec4>();
            }
        case GL_HALF_FLOAT:
            return _CheckValid<HgiFormatFloat16Vec3>();
        case GL_FLOAT:
            return _CheckValid<HgiFormatFloat32Vec3>();
        default:
            TF_CODING_ERROR("Unsupported texture format GL_RGB 0x%04x",
                            glType);
            return HgiFormatInvalid;
        }
    case GL_RGBA:
        switch(glType) {
        case GL_UNSIGNED_BYTE: 
        {
            const bool isSRGB = (glInternalFormat == GL_SRGB8_ALPHA8);

            if (premultiplyAlpha) {
                if (isSRGB) {
                    _convertedRawData = _PremultiplyAlpha<unsigned char, 
                        /* isSRGB = */ true>(_textureDesc.initialData, 
                        _textureDesc.dimensions);
                } else {
                    _convertedRawData = _PremultiplyAlpha<unsigned char, 
                        /* isSRGB = */ false>(_textureDesc.initialData,
                        _textureDesc.dimensions);
                }

                // Point to the buffer with the converted data.
                _textureDesc.initialData = _convertedRawData.get();  
                // Drop the old buffer.
                _textureData = TfNullPtr;
            }

            if (isSRGB) {
                return _CheckValid<HgiFormatUNorm8Vec4srgb>();
            } else {
                return _CheckValid<HgiFormatUNorm8Vec4>();
            }
        }
        case GL_HALF_FLOAT:
            if (premultiplyAlpha) {
                _convertedRawData = _PremultiplyAlphaFloat<GfHalf>(
                    _textureDesc.initialData, _textureDesc.dimensions);

                // Point to the buffer with the converted data.
                _textureDesc.initialData = _convertedRawData.get();
                // Drop the old buffer.
                _textureData = TfNullPtr;
            }

            return _CheckValid<HgiFormatFloat16Vec4>();
        case GL_FLOAT:
            if (premultiplyAlpha) {
                _convertedRawData = _PremultiplyAlphaFloat<float>(
                    _textureDesc.initialData, _textureDesc.dimensions);

                // Point to the buffer with the converted data.
                _textureDesc.initialData = _convertedRawData.get();
                // Drop the old buffer.
                _textureData = TfNullPtr;
            }

            return _CheckValid<HgiFormatFloat32Vec4>();
        default:
            TF_CODING_ERROR("Unsupported texture format GL_RGBA 0x%04x",
                            glType);
            return HgiFormatInvalid;
        }
    case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
        switch(glType) {
        case GL_FLOAT:
            return _CheckValid<HgiFormatBC6UFloatVec3>();
        default:
            TF_CODING_ERROR(
                "Unsupported texture format "
                "GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x%04x",
                glType);
            return HgiFormatInvalid;
        }
    case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
        switch(glType) {
        case GL_FLOAT:
            return _CheckValid<HgiFormatBC6FloatVec3>();
        default:
            TF_CODING_ERROR(
                "Unsupported texture format "
                "GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x%04x",
                glType);
            return HgiFormatInvalid;
        }
    case GL_COMPRESSED_RGBA_BPTC_UNORM:
        switch(glType) {
        case GL_UNSIGNED_BYTE:
            return _CheckValid<HgiFormatBC7UNorm8Vec4>();
        default:
            TF_CODING_ERROR(
                "Unsupported texture format "
                "GL_COMPRESSED_RGBA_BPTC_UNORM 0x%04x",
                glType);
            return HgiFormatInvalid;
        }
    case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
        switch(glType) {
        case GL_UNSIGNED_BYTE:
            return _CheckValid<HgiFormatBC7UNorm8Vec4srgb>();
        default:
            TF_CODING_ERROR(
                "Unsupported texture format "
                "GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x%04x",
                glType);
            return HgiFormatInvalid;
        }
    default:
        TF_CODING_ERROR("Unsupported texture format 0x%04x 0x%04x",
                        glFormat, glType);
        return HgiFormatInvalid;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
