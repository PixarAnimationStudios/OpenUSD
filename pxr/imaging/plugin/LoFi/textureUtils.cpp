//
// Copyright 2020 benmalartre
//
// Unlicensed
//

#include "pxr/imaging/plugin/LoFi/textureUtils.h"

#include "pxr/base/gf/half.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

template<typename T>
constexpr T
_OpaqueAlpha() {
    return std::numeric_limits<T>::is_integer ?
        std::numeric_limits<T>::max() : T(1);
}

template<typename T>
void
_ConvertRGBToRGBA(
    const void * const src,
    const size_t numTexels,
    void * const dst)
{
    TRACE_FUNCTION();

    const T * const typedSrc = reinterpret_cast<const T*>(src);
    T * const typedDst = reinterpret_cast<T*>(dst);

    size_t i = numTexels;
    // Going backward so that we can convert in place.
    do {
        i--;
        typedDst[4 * i + 3] = _OpaqueAlpha<T>();
        typedDst[4 * i + 2] = typedSrc[3 * i + 2];
        typedDst[4 * i + 1] = typedSrc[3 * i + 1];
        typedDst[4 * i + 0] = typedSrc[3 * i + 0];
    } while(i);
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
void
_PremultiplyAlpha(
    const void * const src,
    const size_t numTexels,
    void * const dst)
{
    TRACE_FUNCTION();

    static_assert(std::numeric_limits<T>::is_integer, "Requires integral type");

    const T * const typedSrc = reinterpret_cast<const T*>(src);
    T * const typedDst = reinterpret_cast<T*>(dst);

    // Perform all operations using floats.
    constexpr float max = static_cast<float>(std::numeric_limits<T>::max());

    for (size_t i = 0; i < numTexels; i++) {
        const float alpha = static_cast<float>(typedSrc[4 * i + 3]) / max;

        for (size_t j = 0; j < 3; j++) {
            float p = static_cast<float>(typedSrc[4 * i + j]);

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
            typedDst[4 * i + j] = p + 0.5f;  
        }
        // Only necessary when not converting in place.
        typedDst[4 * i + 3] = typedSrc[4 * i + 3];
    }
}

// Pre-multiply alpha function to be used for floating point types
template<typename T>
void
_PremultiplyAlphaFloat(
    const void * const src,
    const size_t numTexels,
    void * const dst)
{
    TRACE_FUNCTION();

    static_assert(GfIsFloatingPoint<T>::value, "Requires floating point type");

    const T * const typedSrc = reinterpret_cast<const T*>(src);
    T * const typedDst = reinterpret_cast<T*>(dst);

    for (size_t i = 0; i < numTexels; i++) {
        const T alpha = typedSrc[4 * i + 3];

        // Pre-multiply RGB values with alpha.
        for (size_t j = 0; j < 3; j++) {
            typedDst[4 * i + j] = typedSrc[4 * i + j] * alpha;
        }
        typedDst[4 * i + 3] = typedSrc[4 * i + 3];
    }
}

} // anonymous namespace

HgiFormat
LoFiTextureUtils::GetHgiFormat(
    const HioFormat hioFormat,
    const bool premultiplyAlpha,
    const bool avoidThreeComponentFormats,
    ConversionFunction * const conversionFunction)
{
    // Format dispatch, mostly we can just use the CPU buffer from
    // the texture data provided.
    switch(hioFormat) {

        // UNorm 8.
        case HioFormatUNorm8:
            return HgiFormatUNorm8;
        case HioFormatUNorm8Vec2:
            return HgiFormatUNorm8Vec2;
        case HioFormatUNorm8Vec3:
            // RGB (24bit) is not supported on MTL, so we need to
            // always convert it.
            *conversionFunction =
                _ConvertRGBToRGBA<unsigned char>;
            return HgiFormatUNorm8Vec4;
        case HioFormatUNorm8Vec4: 
            if (premultiplyAlpha) {
                *conversionFunction =
                    _PremultiplyAlpha<unsigned char, /* isSRGB = */ false>;
            }
            return HgiFormatUNorm8Vec4;
            
        // SNorm8
        case HioFormatSNorm8:
            return HgiFormatSNorm8;
        case HioFormatSNorm8Vec2:
            return HgiFormatSNorm8Vec2;
        case HioFormatSNorm8Vec3:
            *conversionFunction =
                _ConvertRGBToRGBA<signed char>;
            return HgiFormatSNorm8Vec4;
        case HioFormatSNorm8Vec4:
            if (premultiplyAlpha) {
                // Pre-multiplying only makes sense for RGBA colors and
                // the signed integers do not make sense for RGBA.
                //
                // However, for consistency, we do premultiply here so
                // that one can tell from the material network topology
                // only whether premultiplication is happening.
                //
                *conversionFunction =
                    _PremultiplyAlpha<signed char, /* isSRGB = */ false>;
            }
            return HgiFormatSNorm8Vec4;

        // Float16
        case HioFormatFloat16:
            return HgiFormatFloat16;
        case HioFormatFloat16Vec2:
            return HgiFormatFloat16Vec2;
        case HioFormatFloat16Vec3:
            if (avoidThreeComponentFormats) {
                *conversionFunction =
                    _ConvertRGBToRGBA<GfHalf>;
                return HgiFormatFloat16Vec4;
            }
            return HgiFormatFloat16Vec3;
        case HioFormatFloat16Vec4:
            if (premultiplyAlpha) {
                *conversionFunction =
                    _PremultiplyAlphaFloat<GfHalf>;
            }
            return HgiFormatFloat16Vec4;

        // Float32
        case HioFormatFloat32:
            return HgiFormatFloat32;
        case HioFormatFloat32Vec2:
            return HgiFormatFloat32Vec2;
        case HioFormatFloat32Vec3:
            if (avoidThreeComponentFormats) {
                *conversionFunction =
                    _ConvertRGBToRGBA<float>;
                return HgiFormatFloat32Vec4;
            }
            return HgiFormatFloat32Vec3;
        case HioFormatFloat32Vec4:
            if (premultiplyAlpha) {
                *conversionFunction =
                    _PremultiplyAlphaFloat<float>;
            }
            return HgiFormatFloat32Vec4;

        // Double64
        case HioFormatDouble64:
        case HioFormatDouble64Vec2:
        case HioFormatDouble64Vec3:
        case HioFormatDouble64Vec4:
            TF_WARN("Double texture formats not supported by Storm");
            return HgiFormatInvalid;
            
        // UInt16
        case HioFormatUInt16:
            return HgiFormatUInt16;
        case HioFormatUInt16Vec2:
            return HgiFormatUInt16Vec2;
        case HioFormatUInt16Vec3:
            if (avoidThreeComponentFormats) {
                *conversionFunction =
                    _ConvertRGBToRGBA<uint16_t>;
                return HgiFormatUInt16Vec4;
            }
            return HgiFormatUInt16Vec3;
        case HioFormatUInt16Vec4:
            if (premultiplyAlpha) {
                // Pre-multiplying only makes sense for RGBA colors and
                // the signed integers do not make sense for RGBA.
                //
                // However, for consistency, we do premultiply here so
                // that one can tell from the material network topology
                // only whether premultiplication is happening.
                //
                *conversionFunction =
                    _PremultiplyAlpha<uint16_t, /* isSRGB = */ false>;
            }
            return HgiFormatUInt16Vec4;
            
        // Int16
        case HioFormatInt16:
        case HioFormatInt16Vec2:
        case HioFormatInt16Vec3:
        case HioFormatInt16Vec4:
            TF_WARN("Signed 16-bit integer texture formats "
                    "not supported by Storm");
            return HgiFormatInvalid;

        // UInt32
        case HioFormatUInt32:
        case HioFormatUInt32Vec2:
        case HioFormatUInt32Vec3:
        case HioFormatUInt32Vec4:
            TF_WARN("Unsigned 32-bit integer texture formats "
                    "not supported by Storm");
            return HgiFormatInvalid;

        // Int32
        case HioFormatInt32:
            return HgiFormatInt32;
        case HioFormatInt32Vec2:
            return HgiFormatInt32Vec2;
        case HioFormatInt32Vec3:
            if (avoidThreeComponentFormats) {
                *conversionFunction =
                    _ConvertRGBToRGBA<int32_t>;
                return HgiFormatInt32Vec4;
            }
            return HgiFormatInt32Vec3;
        case HioFormatInt32Vec4:
            // Pre-multiplying only makes sense for RGBA colors and
            // the signed integers do not make sense for RGBA.
            //
            // However, for consistency, we do premultiply here so
            // that one can tell from the material network topology
            // only whether premultiplication is happening.
            //
            if (premultiplyAlpha) {
                *conversionFunction =
                    _PremultiplyAlpha<int32_t, /* isSRGB = */ false>;
            }
            return HgiFormatInt32Vec4;

        // UNorm8 SRGB
        case HioFormatUNorm8srgb:
        case HioFormatUNorm8Vec2srgb:
            TF_WARN("One and two channel srgb texture formats "
                    "not supported by Storm");
            return HgiFormatInvalid;
        case HioFormatUNorm8Vec3srgb:
            // RGB (24bit) is not supported on MTL, so we need to convert it.
            *conversionFunction =
                _ConvertRGBToRGBA<unsigned char>;
            return HgiFormatUNorm8Vec4srgb;
        case HioFormatUNorm8Vec4srgb:
            if (premultiplyAlpha) {
                *conversionFunction =
                    _PremultiplyAlpha<unsigned char, /* isSRGB = */ true>;
            }
            return HgiFormatUNorm8Vec4srgb;

        // BPTC compressed
        case HioFormatBC6FloatVec3:
            return HgiFormatBC6FloatVec3;
        case HioFormatBC6UFloatVec3:
            return HgiFormatBC6UFloatVec3;
        case HioFormatBC7UNorm8Vec4:
            return HgiFormatBC7UNorm8Vec4;
        case HioFormatBC7UNorm8Vec4srgb:
            // Pre-multiplying alpha would require decompressing and
            // recompressing, so not doing it here.
            return HgiFormatBC7UNorm8Vec4srgb;

        // S3TC/DXT compressed
        case HioFormatBC1UNorm8Vec4:
            return HgiFormatBC1UNorm8Vec4;
        case HioFormatBC3UNorm8Vec4:
            // Pre-multiplying alpha would require decompressing and
            // recompressing, so not doing it here.
            return HgiFormatBC3UNorm8Vec4;

        case HioFormatInvalid:
            return HgiFormatInvalid;
        case HioFormatCount:
            TF_CODING_ERROR("HioFormatCount passed to function");
            return HgiFormatInvalid;
    }

    TF_CODING_ERROR("Invalid HioFormat enum value");
    return HgiFormatInvalid;
}

std::vector<HioImageSharedPtr>
LoFiTextureUtils::GetAllMipImages(
    const std::string &filePath,
    const HioImage::SourceColorSpace sourceColorSpace)
{
    TRACE_FUNCTION();

    constexpr int maxMipReads = 32;
    std::vector<HioImageSharedPtr> result;

    unsigned int prevWidth = std::numeric_limits<unsigned int>::max();
    unsigned int prevHeight = std::numeric_limits<unsigned int>::max();

    // Ignoring image->GetNumMipLevels() since it can be unreliable.
    for (int mip = 0; mip < maxMipReads; ++mip) {
        HioImageSharedPtr const image =
            HioImage::OpenForReading(
                filePath, /* subimage = */ 0, mip, sourceColorSpace);

        if (!image) {
            break;
        }

        const unsigned int currHeight = image->GetWidth();
        const unsigned int currWidth = image->GetHeight();
        if (!(currWidth < prevWidth || currHeight < prevHeight)) {
            break;
        }

        result.push_back(std::move(image));

        prevWidth = currWidth;
        prevHeight = currHeight;
    }

    return result;
};

static
GfVec3i
_GetDimensions(HioImageSharedPtr const &image)
{
    return GfVec3i(image->GetWidth(), image->GetHeight(), 1);
}

GfVec3i
LoFiTextureUtils::ComputeDimensionsFromTargetMemory(
    const std::vector<HioImageSharedPtr> &mips,
    const HgiFormat targetFormat,
    const size_t tileCount,
    const size_t targetMemory,
    size_t * const mipIndex)
{
    TRACE_FUNCTION();

    // Return full resolution of image if no target memory given.
    if (targetMemory == 0) {
        if (mipIndex) {
            *mipIndex = 0;
        }
        return _GetDimensions(mips.front());
    }

    // Iterate through mips until one is found that fits into the target
    // memory.
    for (size_t i = 0; i < mips.size(); i++) {
        HioImageSharedPtr const &image = mips[i];
        const GfVec3i dim = _GetDimensions(image);
        // The factor of 4/3 = 1 + 1/4 + 1/16 + ... accounts for all the
        // lower mipmaps.
        const size_t totalMem = 
            HgiGetDataSize(targetFormat, dim) * tileCount * 4 / 3;
        if (totalMem <= targetMemory) {
            if (mipIndex) {
                *mipIndex = i;
            }
            return dim;
        }
    }

    if (mipIndex) {
        *mipIndex = mips.size() - 1;
    }

    // If none of the mips fit, take the last one and compute
    // mip chain from it.
    const GfVec3i dim = _GetDimensions(mips.back());
    const std::vector<HgiMipInfo> mipInfos =
        HgiGetMipInfos(targetFormat, dim, tileCount);

    // Iterate through mip chain until one is found that fits into the
    // target memory.
    for (const HgiMipInfo &mipInfo : mipInfos) {
        // The factor of 4/3 = 1 + 1/4 + 1/16 + ... accounts for all the
        // lower mipmaps.
        if (mipInfo.byteSize * tileCount * 4 / 3 <= targetMemory) {
            return mipInfo.dimensions;
        }
    }

    // Last resort, should be just (1,1,1).
    return mipInfos.back().dimensions;
}

PXR_NAMESPACE_CLOSE_SCOPE
