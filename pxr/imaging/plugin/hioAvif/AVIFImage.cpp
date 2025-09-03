//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/pragmas.h"

// Not all functions in the AVIF and aom libraries are used by Hio. 
// Therefore, the unused function warning is suppressed as the messages are
// not useful for development, as it is expected that many functions are
// defined but not referenced or exported.
ARCH_PRAGMA_UNUSED_FUNCTION

#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hio/types.h"

#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/writableAsset.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/export.h"
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/gf/colorSpace.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

#include "pxr/imaging/plugin/hioAvif/AVIF/src/avif/avif.h"

#if defined(PXR_STATIC)
#   define HIOAVIF_API
#   define HIOAVIF_API_TEMPLATE_CLASS(...)
#   define HIOAVIF_API_TEMPLATE_STRUCT(...)
#   define HIOAVIF_LOCAL
#else
#   define HIOAVIF_API ARCH_EXPORT
#   define HIOAVIF_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#   define HIOAVIF_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   define HIOAVIF_LOCAL ARCH_HIDDEN
#endif

PXR_NAMESPACE_OPEN_SCOPE

namespace {

    // XXX These image processing utility functions duplicate those
    // in the OpenEXR plugin. In the future, they may be deduplicated
    // into Hio utility functions.
    static float integrate_gaussian(float x, float sigma)
    {
        float p1 = erf((x - 0.5f) / sigma * sqrtf(0.5f));
        float p2 = erf((x + 0.5f) / sigma * sqrtf(0.5f));
        return (p2-p1) * 0.5f;
    }

    // Enum capturing the underlying data type on a channel.
    typedef enum
    {
        EXR_PIXEL_UINT  = 0,
        EXR_PIXEL_HALF  = 1,
        EXR_PIXEL_FLOAT = 2,
        EXR_PIXEL_LAST_TYPE
    } exr_pixel_type_t;

    // structure to hold image data that is read from an AVIF file
    typedef struct {
        uint8_t* data;
        size_t dataSize;
        exr_pixel_type_t pixelType;
        int channelCount; // 1 for luminance, 3 for RGB, 4 for RGBA
        int width, height;
        int dataWindowMinY, dataWindowMaxY;
    } nanoexr_ImageData_t;

    bool nanoexr_Gaussian_resample(const nanoexr_ImageData_t* src,
                                nanoexr_ImageData_t* dst)
    {
        if (src->pixelType != EXR_PIXEL_FLOAT && dst->pixelType != EXR_PIXEL_FLOAT)
            return false;
        if (src->channelCount != dst->channelCount)
            return false;
        
        const int srcWidth  = src->width;
        const int dstWidth  = dst->width;
        const int srcHeight = src->height;
        const int dstHeight = dst->height;
        if (srcWidth == dstWidth && srcHeight == dstHeight) {
            memcpy(dst->data, src->data, 
                src->channelCount * srcWidth * srcHeight * sizeof(float));
            return true;
        }
        
        float* srcData = (float*)src->data;
        float* dstData = (float*)dst->data;

        // two pass image resize using a Gaussian filter per:
        // https://bartwronski.com/2021/10/31/practical-gaussian-filter-binomial-filter-and-small-sigma-gaussians
        // chose sigma to suppress high frequencies that can't be represented 
        // in the downsampled image
        const float ratio_w = (float)dstWidth / (float)srcWidth;
        const float ratio_h = (float)dstHeight / (float)srcHeight;
        const float sigma_w = 1.f / 2.f * ratio_w;
        const float sigma_h = 1.f / 2.f * ratio_h;
        const float support = 0.995f;
        float radius = ceilf(sqrtf(-2.0f * sigma_w * sigma_w * logf(1.0f - support)));
        int filterSize_w = (int)radius;
        if (!filterSize_w)
            return false;
        
        float* filter_w = (float*) malloc(sizeof(float) * (filterSize_w + 1) * 2);
        float sum = 0.0f;
        for (int i = 0; i <= filterSize_w; i++) {
            int idx = i + filterSize_w;
            filter_w[idx] = integrate_gaussian((float) i, sigma_w);
            if (i > 0)
                sum += 2 * filter_w[idx];
            else
                sum = filter_w[idx];
        }
        for (int i = 0; i <= filterSize_w; ++i) {
            filter_w[i + filterSize_w] /= sum;
        }
        for (int i = 0; i < filterSize_w; ++i) {
            filter_w[filterSize_w - i - 1] = filter_w[i + filterSize_w + 1];
        }
        int fullFilterSize_w = filterSize_w * 2 + 1;

        // again for height
        radius = ceilf(sqrtf(-2.0f * sigma_h * sigma_h * logf(1.0f - support)));
        int filterSize_h = (int)radius;
        if (!filterSize_h)
            return false;
        
        float* filter_h = (float*) malloc(sizeof(float) * (1 + filterSize_h) * 2);
        sum = 0.0f;
        for (int i = 0; i <= filterSize_h; i++) {
            int idx = i + filterSize_h;
            filter_h[idx] = integrate_gaussian((float) i, sigma_h);
            if (i > 0)
                sum += 2 * filter_h[idx];
            else
                sum = filter_h[idx];
        }
        for (int i = 0; i <= filterSize_h; ++i) {
            filter_h[i + filterSize_h] /= sum;
        }
        for (int i = 0; i < filterSize_h; ++i) {
            filter_h[filterSize_h - i - 1] = filter_h[i + filterSize_h + 1];
        }
        int fullFilterSize_h = filterSize_h * 2 + 1;
        
        // first pass: resize horizontally
        int srcFloatsPerLine = src->channelCount * srcWidth;
        int dstFloatsPerLine = src->channelCount * dstWidth;
        float* firstPass = (float*)malloc(dstWidth * src->channelCount * srcHeight * sizeof(float));
        for (int y = 0; y < srcHeight; ++y) {
            for (int x = 0; x < dstWidth; ++x) {
                for (int c = 0; c < src->channelCount; ++c) {
                    float sum = 0.0f;
                    for (int i = 0; i < fullFilterSize_w; ++i) {
                        int srcX = (int)((x + 0.5f) / ratio_w - 0.5f) + i - filterSize_w;
                        if (srcX < 0 || srcX >= srcWidth)
                            continue;
                        int idx = y * srcFloatsPerLine + (srcX * src->channelCount) + c;
                        sum += srcData[idx] * filter_w[i];
                    }
                    firstPass[y * dstFloatsPerLine + (x * src->channelCount) + c] = sum;
                }
            }
        }

        // second pass: resize vertically
        float* secondPass = dstData;
        for (int y = 0; y < dstHeight; ++y) {
            for (int x = 0; x < dstWidth; ++x) {
                for (int c = 0; c < src->channelCount; ++c) {
                    float sum = 0.0f;
                    for (int i = 0; i < fullFilterSize_h; ++i) {
                        int srcY = (int)((y + 0.5f) / ratio_h - 0.5f) + i - filterSize_h;
                        if (srcY < 0 || srcY >= srcHeight)
                            continue;
                        int idx = src->channelCount * srcY * dstWidth + (x * src->channelCount) + c;
                        sum += firstPass[idx] * filter_h[i];
                    }
                    secondPass[dst->channelCount * y * dstWidth + (x * dst->channelCount) + c] = sum;
                }
            }
        }
        free(filter_h);
        free(filter_w);
        free(firstPass);
        return true;
    }

    
    template<typename T>
    class ImageProcessor
    {
    public:
        // Flip the image in place.
        static void FlipImage(T* buffer, int width, int height, int channelCount)
        {
            // use std::swap_ranges to flip the image in place
            for (int y = 0; y < height / 2; ++y) {
                std::swap_ranges(
                                 buffer + y * width * channelCount,
                                 buffer + (y + 1) * width * channelCount,
                                 buffer + (height - y - 1) * width * channelCount);
            }
        }
        
        // Crop the image in-place.
        static void CropImage(T* buffer, 
                              int width, int height, int channelCount,
                              int cropTop, int cropBottom,
                              int cropLeft, int cropRight)
        {
            int newWidth = width - cropLeft - cropRight;
            int newHeight = height - cropTop - cropBottom;
            
            if (newWidth <= 0 || newHeight <= 0
                || (newWidth == width && newHeight == height))
                return;
            
            for (int y = 0; y < newHeight; ++y) {
                for (int x = 0; x < newWidth; ++x) {
                    for (int c = 0; c < channelCount; ++c) {
                        buffer[(y * newWidth + x) * channelCount + c] =
                        buffer[((y + cropTop) * width + x + cropLeft)
                               * channelCount + c];
                    }
                }
            }
        }
        
        static void FloatToHalf(float* buffer, GfHalf* outBuffer,
                                int width, int height, int channelCount)
        {
            if (!buffer || !outBuffer)
                return;
            
            for (int i = 0; i < width * height * channelCount; ++i) {
                outBuffer[i] = buffer[i];
            }
        }

        // return true for a successful resample
        static bool ResizeImage(const float* src, float* dst,
                                int width, int height, int channelCount)
        {
            nanoexr_ImageData_t srcImg = {
                (uint8_t*) src,
                channelCount * sizeof(float) * width * height,
                EXR_PIXEL_FLOAT,
                channelCount, width, height, 0, height - 1
            };
            nanoexr_ImageData_t dstImg = srcImg;
            dstImg.data = (uint8_t*) dst;
            return nanoexr_Gaussian_resample(&srcImg, &dstImg);
        }

        static bool ResizeImage(const float* src, float* dst,
                                int srcWidth, int srcHeight, int dstWidth, int dstHeight, int channelCount) {
            nanoexr_ImageData_t srcImg = {
                (uint8_t*) src,
                channelCount * sizeof(float) * srcWidth * srcHeight,
                EXR_PIXEL_FLOAT,
                channelCount, srcWidth, srcHeight, 0, srcHeight - 1
            };
            nanoexr_ImageData_t dstImg = {
                (uint8_t*) dst,
                channelCount * sizeof(float) * dstWidth * dstHeight,
                EXR_PIXEL_FLOAT,
                channelCount, dstWidth, dstHeight, 0, dstHeight - 1
            };
            return nanoexr_Gaussian_resample(&srcImg, &dstImg);
        }
    };


    class AvifWrapper {
        avifImage* _yuvImage = nullptr;
        std::shared_ptr<float[]> _buffer;

    public:
        AvifWrapper() = default;
        ~AvifWrapper() {
            if (_yuvImage) {
                avifImageDestroy(_yuvImage);
            }
        }

        avifImage* Image() const { return _yuvImage; }

        avifResult Read(uint8_t* data, size_t dataSize) {
            if (!dataSize) {
                return AVIF_RESULT_NO_CONTENT;
            }
            _yuvImage = avifImageCreateEmpty();
            if (!_yuvImage) {
                return AVIF_RESULT_UNKNOWN_ERROR;
            }
            avifDecoder* decoder = avifDecoderCreate();
            if (!decoder) {
                avifImageDestroy(_yuvImage);
                return AVIF_RESULT_NO_CODEC_AVAILABLE;
            }
            avifResult result = avifDecoderReadMemory(decoder, _yuvImage,
                                                      data, dataSize);
            avifDecoderDestroy(decoder);
            return result;
        }

        bool SourceIsSRGB() const {
            // if the transfer function is SRGB-like, assume SRGB
            switch (_yuvImage->transferCharacteristics) {
                case AVIF_TRANSFER_CHARACTERISTICS_UNSPECIFIED:
                    // special case for BT709 with unspecified transfer function to match
                    // behavior observed in Apple's Finder and web browsers.
                    return _yuvImage->colorPrimaries == AVIF_COLOR_PRIMARIES_BT709 ||
                           _yuvImage->colorPrimaries == AVIF_COLOR_PRIMARIES_UNSPECIFIED;

                case AVIF_TRANSFER_CHARACTERISTICS_BT709:
                case AVIF_TRANSFER_CHARACTERISTICS_BT470M:
                case AVIF_TRANSFER_CHARACTERISTICS_SRGB:
                    return true;

                default:
                    return false;
            }
        }

        GfVec2f Dimensions() const {
            if (!_yuvImage) {
                return GfVec2f(0.0f, 0.0f);
            }
            return GfVec2f(_yuvImage->width, _yuvImage->height); 
        }

        // templated CovertTORGBA, note that only GfHalf and uint8_t are
        // by the avif library.
        template <typename T>
        bool ConvertToRGBA(T* buffer, size_t bufferSize) {
            static_assert(std::is_same<T, GfHalf>::value || std::is_same<T, uint8_t>::value,
                          "Only GfHalf and uint8_t are supported");
            if (!_yuvImage) {
                return false;
            }
            avifRGBImage rgb;
            memset(&rgb, 0, sizeof(rgb));
            avifRGBImageSetDefaults(&rgb, _yuvImage);
            rgb.width = _yuvImage->width;
            rgb.height = _yuvImage->height;
            rgb.depth = sizeof(T) * 8; // bits per channel
            rgb.format = AVIF_RGB_FORMAT_RGBA;
            rgb.chromaUpsampling = AVIF_CHROMA_UPSAMPLING_AUTOMATIC;
            rgb.chromaDownsampling = AVIF_CHROMA_DOWNSAMPLING_AUTOMATIC;
            rgb.avoidLibYUV = AVIF_FALSE;
            rgb.ignoreAlpha = AVIF_FALSE;
            rgb.alphaPremultiplied = AVIF_FALSE;
            rgb.isFloat = std::is_same<T, GfHalf>::value;
            rgb.maxThreads = 1;
            rgb.pixels = (uint8_t*) buffer;
            rgb.rowBytes = rgb.width * 4 * sizeof(T);
            avifResult result = avifImageYUVToRGB(_yuvImage, &rgb);
            return result == AVIF_RESULT_OK;
        }

        std::shared_ptr<float[]> RGBAFloatBuffer() {
            if (!_yuvImage) {
                return {};
            }
            // if the yuvImage is 8 bit, call ConvertToRGBA<uint8_t>, otherwise
            // call ConvertToRGBA<GfHalf>.
            size_t bufferSize = _yuvImage->width * _yuvImage->height * 4;
            if (_yuvImage->depth == 8) {
                std::unique_ptr<uint8_t[]> buffer(new uint8_t[bufferSize]);
                if (!ConvertToRGBA(buffer.get(), bufferSize)) {
                    return {};
                }
                // convert to float
                std::shared_ptr<float[]> floatBuffer(new float[bufferSize]);
                for (size_t i = 0; i < bufferSize; ++i) {
                    floatBuffer[i] = buffer[i] / 255.0f;
                }
                return floatBuffer;
            }
            std::unique_ptr<GfHalf[]> buffer(new GfHalf[bufferSize]);
            if (!ConvertToRGBA(buffer.get(), bufferSize)) {
                return {};
            }
            // convert to float
            std::shared_ptr<float[]> floatBuffer(new float[bufferSize]);
            for (size_t i = 0; i < bufferSize; ++i) {
                floatBuffer[i] = buffer[i];
            }
            return floatBuffer;
        }
    };
   
} // anon


// It's necessary to export the class so it's typeinfo is visible for registration
class HIOAVIF_API Hio_AVIFImage final : public HioImage
{
    std::shared_ptr<ArAsset> _asset;
    std::string              _filename;
    int _width = 0;
    int _height = 0;

    SourceColorSpace _sourceColorSpace;

    // mutable because GetMetadata is const, yet it doesn't make sense
    // to cache the dictionary unless metadata is requested.
    mutable VtDictionary     _metadata;

public:
    Hio_AVIFImage() = default;
    ~Hio_AVIFImage() = default;

    const std::shared_ptr<ArAsset> Asset() const { return _asset; }

    bool Read(StorageSpec const &storage) override {
        return ReadCropped(0, 0, 0, 0, storage); 
    }
    bool ReadCropped(int const cropTop,  int const cropBottom,
                     int const cropLeft, int const cropRight,
                     StorageSpec const &storage) override;
    bool Write(StorageSpec const &storage,
               VtDictionary const &metadata) override { return false; }

    // We're decoding AVIF to linear float16.
    bool IsColorSpaceSRGB() const override {
        return false;
    }

    // hardcoded to f16v4, as it's a common hardware requirement that f16
    // textures are stored in RGBA format.
    HioFormat GetFormat() const override { return HioFormatFloat16Vec4; }
    int  GetWidth() const override { return _width; }
    int  GetHeight() const override { return _height; }
    int  GetBytesPerPixel() const override { return 16; } // 4 * sizeof(float16)
    int  GetNumMipLevels() const override { return 0; } // AVIF can store mips, an improvement for the future.
    bool GetMetadata(TfToken const &key, VtValue *value) const override { return false; }
    bool GetSamplerMetadata(HioAddressDimension dim,
                            HioAddressMode *param) const override { return false; }
    std::string const& GetFilename() const override { return _filename; }

    const VtDictionary& GetMetadata() const { return _metadata; }

protected:
    bool _OpenForReading(std::string const &filename, int subimage, int mip,
                         SourceColorSpace sourceColorSpace,
                         bool suppressErrors) override;
    bool _OpenForWriting(std::string const &filename) override { return false; }
};

TF_REGISTRY_FUNCTION(TfType)
{
    using Image = Hio_AVIFImage;
    TfType t = TfType::Define<Image, TfType::Bases<Image::HioImage>>();
    t.SetFactory<HioImageFactory<Image>>();
}

bool Hio_AVIFImage::_OpenForReading(std::string const &filename,
                                    int subimage, int mip,
                                    SourceColorSpace sourceColorSpace,
                                    bool /*suppressErrors*/)
{
    _width = 0;
    _height = 0;
    _sourceColorSpace = sourceColorSpace;
    _filename = filename;
    _asset = ArGetResolver().OpenAsset(ArResolvedPath(filename));
    if (!_asset) {
        return false;
    }

    size_t dataSize = _asset->GetSize();
    if (!dataSize) {
        return false;
    }

    std::unique_ptr<uint8_t[]> data(new uint8_t[dataSize]);
    const size_t offset = 0;
    size_t readSize = _asset->Read(data.get(), dataSize, offset);
    if (readSize <  dataSize) {
        return false;
    }

    AvifWrapper avif;
    avifResult result = avif.Read(data.get(), dataSize);
    if (result != AVIF_RESULT_OK) {
        TF_RUNTIME_ERROR("Error parsing AVIF file: %s\n", avifResultToString(result));
        return false;
    }

    GfVec2f size = avif.Dimensions();
    _width = size[0];
    _height = size[1];

    return true;
}

bool Hio_AVIFImage::ReadCropped(
                int const cropTop,  int const cropBottom,
                int const cropLeft, int const cropRight, 
                StorageSpec const& storage)
{
    // Check if the AVIF file was opened for read prior to calling ReadCropped.
    if (!_asset) {
        return false;
    }

    if (cropTop < 0 || cropBottom < 0 || cropLeft < 0 || cropRight < 0) {
        return false;
    }

    // if cropping would elide the entire image, return.
    int newWidth = _width - cropLeft - cropRight;
    int newHeight = _height - cropTop - cropBottom;
    if (newWidth <= 0 || newHeight <= 0) {
        return false;
    }

    bool outputIsFloat = HioGetHioType(storage.format) == HioTypeFloat;
    bool outputIsHalf =  HioGetHioType(storage.format) == HioTypeHalfFloat;
    if (!(outputIsFloat || outputIsHalf)) {
        return false;
    }

    const int channelCount = HioGetComponentCount(storage.format);
    if (channelCount < 4) {
        return false;
    }

    size_t dataSize = _asset->GetSize();
    if (!dataSize) {
        return false;
    }

    std::unique_ptr<uint8_t[]> data(new uint8_t[dataSize]);
    const size_t offset = 0;
    size_t readSize = _asset->Read(data.get(), dataSize, offset);
    if (readSize <  dataSize) {
        return false;
    }

    AvifWrapper avif;
    avifResult result = avif.Read(data.get(), dataSize);
    if (result != AVIF_RESULT_OK) {
        TF_RUNTIME_ERROR("Error parsing AVIF file: %s\n", avifResultToString(result));
        return false;
    }

    std::shared_ptr<float[]> buffer = avif.RGBAFloatBuffer();
    
    // The image is now in linear float16 format.
    // Crop in place.
    if (newWidth != _width || newHeight != _height) {
        // crop in place.
        ImageProcessor<float>::CropImage(buffer.get(),
                                          _width, _height, channelCount,
                                          cropTop, cropBottom,
                                          cropLeft, cropRight);
    }

    // flip the cropped image in place.
    if (storage.flipped) {
        ImageProcessor<float>::FlipImage(buffer.get(),
                                          newWidth, newHeight, channelCount);
    }

    // apply or remove the sRGB transfer function as needed.


    // note that in the future, Hio will support more color spaces than Rec709,
    // but today, we need to conform AVIF files to Rec709.
    const bool readRawImageData = _sourceColorSpace == HioImage::Raw;
    const bool removeSRGB = !readRawImageData && avif.SourceIsSRGB();
    float srcPrimaries[8];
    avifColorPrimariesGetValues(avif.Image()->colorPrimaries, srcPrimaries);
    GfColorSpace src(TfToken("from AVIF"),
                                { srcPrimaries[0], srcPrimaries[1] }, // red
                                { srcPrimaries[2], srcPrimaries[3] }, // green
                                { srcPrimaries[4], srcPrimaries[5] }, // blue
                                { srcPrimaries[6], srcPrimaries[7] }, // white
                                removeSRGB ? 2.4f : 1.0f,
                                removeSRGB ? 0.055f : 0.0f);

    GfColorSpace dst(GfColorSpaceNames->LinearRec709);
    src.ConvertRGBASpan(dst, TfSpan<float>(buffer.get(), newWidth * newHeight * channelCount));
    
    if (newWidth == storage.width && newHeight == storage.height) {
        if (outputIsHalf) {
            // convert to half directly to storage.data
            ImageProcessor<GfHalf>::FloatToHalf(buffer.get(),
                                                (GfHalf*) storage.data,
                                                 newWidth, newHeight, channelCount);
            return true;
        }

        // copy directly to storage.data
        memcpy(storage.data, buffer.get(),
                newWidth * newHeight * channelCount * sizeof(float));
        return true;
    }

    // if the output is float, resize the image directly to the storage.data buffer.
    if (outputIsFloat) {
        return ImageProcessor<float>::ResizeImage(buffer.get(),
                                                    (float*) storage.data,
                                                    newWidth, newHeight, 
                                                    _width, _height, channelCount);
    }

    // The output is half, resize the image to a temporary buffer and then
    // convert to half to the storage.data buffer.
    std::unique_ptr<float[]> resizedBuffer(new float[newWidth * newHeight * channelCount]);
    if (!ImageProcessor<float>::ResizeImage(buffer.get(),
                                            resizedBuffer.get(),
                                            newWidth, newHeight, 
                                            _width, _height, channelCount)) {
        return false;
    }

    ImageProcessor<float>::FloatToHalf(resizedBuffer.get(),
                                        (GfHalf*) storage.data,
                                        storage.width, storage.height, channelCount);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
