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
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/gf/colorSpace.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

#include "pxr/imaging/plugin/hioAvif/AVIF/src/avif/avif.h"


#include "pxr/base/arch/export.h"

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

// It's necessary to export the class so it's typeinfo is visible for registration
class HIOAVIF_API Hio_AVIFImage final : public HioImage
{
    std::shared_ptr<ArAsset> _asset;
    std::string              _filename;
    int _width = 0;
    int _height = 0;

    avifImage *_avifImage = nullptr;
    bool _imposeSRGBTransfer = false;

    // mutable because GetMetadata is const, yet it doesn't make sense
    // to cache the dictionary unless metadata is requested.
    mutable VtDictionary     _metadata;

public:
    Hio_AVIFImage() = default;
    ~Hio_AVIFImage() override {
        Cleanup();
    }

    void Cleanup() {
        if (_avifImage) {
            avifImageDestroy(_avifImage);
            _avifImage = nullptr;
        }
    }

    const std::shared_ptr<ArAsset> Asset() const { return _asset; }

    bool Read(StorageSpec const &storage) override {
        return ReadCropped(0, 0, 0, 0, storage); 
    }
    bool ReadCropped(int const cropTop,  int const cropBottom,
                     int const cropLeft, int const cropRight,
                     StorageSpec const &storage) override;
    bool Write(StorageSpec const &storage,
               VtDictionary const &metadata) override;

    // We're decoding AVIF to linear float16.
    bool IsColorSpaceSRGB() const override {
        if (_imposeSRGBTransfer)
            return true;

        if (_avifImage) {
            // special case for BT709 with unspecified transfer function to match
            // behavior observed in Apple's Finder and web browsers.
            if (_avifImage->colorPrimaries == AVIF_COLOR_PRIMARIES_BT709 &&
                _avifImage->transferCharacteristics == AVIF_TRANSFER_CHARACTERISTICS_UNSPECIFIED)
                return true;
            
            // if the transfer function is SRGB-like, assume SRGB
            switch (_avifImage->transferCharacteristics) {
                case AVIF_TRANSFER_CHARACTERISTICS_BT709:
                case AVIF_TRANSFER_CHARACTERISTICS_BT470M:
                case AVIF_TRANSFER_CHARACTERISTICS_SRGB:
                    return true;
                default:
                    return false;
            }
        }
        return false;
    }

    // hardcoded to f16v4, although in the future, f16v3 could be a future option
    // if our supported hardware enables support for that as a texture format.
    HioFormat GetFormat() const override { return HioFormatFloat16Vec4; }
    int  GetWidth() const override { return _height; }
    int  GetHeight() const override { return _width; }
    int  GetBytesPerPixel() const override { return 16; } // 4 * sizeof(float16)
    int  GetNumMipLevels() const override { return 0; }
    bool GetMetadata(TfToken const &key, VtValue *value) const override { return false; }
    bool GetSamplerMetadata(HioAddressDimension dim,
                            HioAddressMode *param) const override { return false; }
    std::string const& GetFilename() const override { return _filename; }

    const VtDictionary& GetMetadata() const { return _metadata; }

protected:
    bool _OpenForReading(std::string const &filename, int subimage, int mip,
                         SourceColorSpace sourceColorSpace,
                         bool suppressErrors) override;
    bool _OpenForWriting(std::string const &filename) override;
};

TF_REGISTRY_FUNCTION(TfType)
{
    using Image = Hio_AVIFImage;
    TfType t = TfType::Define<Image, TfType::Bases<Image::HioImage>>();
    t.SetFactory<HioImageFactory<Image>>();
}

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
        // Flip the image in-place.
        static void FlipImage(T* buffer, int width, int height, int channelCount)
        {
            // use std::swap_ranges to flip the image in-place
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
        
        static void HalfToFloat(GfHalf* buffer, float* outBuffer,
                                int width, int height, int channelCount)
        {
            if (!buffer || !outBuffer)
                return;
            
            for (int i = 0; i < width * height * channelCount; ++i) {
                outBuffer[i] = buffer[i];
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
    };
   
} // anon

bool Hio_AVIFImage::ReadCropped(
                int const cropTop,  int const cropBottom,
                int const cropLeft, int const cropRight, 
                StorageSpec const& storage)
{
    if (!_avifImage)
        return false;
    if (cropTop < 0 || cropBottom < 0 || cropLeft < 0 || cropRight < 0)
        return false;

    // Convert to RGB
    avifRGBImage rgb;
    const int bytesPerChannel = 2;
    const int channelCount = HioGetComponentCount(storage.format);
    if (channelCount < 3)
        return false;

    bool outputIsFloat = HioGetHioType(storage.format) == HioTypeFloat;
    bool outputIsHalf =  HioGetHioType(storage.format) == HioTypeHalfFloat;
    if (!(outputIsFloat || outputIsHalf))
        return false;

    bool flip = storage.flipped;

    memset(&rgb, 0, sizeof(rgb));
    avifRGBImageSetDefaults(&rgb, _avifImage);
    rgb.width = _avifImage->width;  // avifRGBImage and avifImage must agree
    rgb.height = _avifImage->height;
    rgb.depth = 8 * bytesPerChannel;
    rgb.format = channelCount == 3? AVIF_RGB_FORMAT_RGB : AVIF_RGB_FORMAT_RGBA;
    rgb.chromaUpsampling = AVIF_CHROMA_UPSAMPLING_AUTOMATIC;
    rgb.chromaDownsampling = AVIF_CHROMA_DOWNSAMPLING_AUTOMATIC;
    rgb.ignoreAlpha = false;
    rgb.alphaPremultiplied = false;
    rgb.isFloat = true;
    rgb.pixels = (uint8_t*)
            calloc(channelCount * bytesPerChannel * rgb.width * rgb.height, 1);
    rgb.rowBytes = rgb.width * channelCount * bytesPerChannel;
    if (rgb.depth != 16 && rgb.isFloat) {
        TF_CODING_ERROR("Only half floats are supported for AVIF decoding");
    }

    avifResult result = avifImageYUVToRGB(_avifImage, &rgb);
    if (result != AVIF_RESULT_OK) {
        TF_RUNTIME_ERROR("Error parsing AVIF file: %s\n", avifResultToString(result));
        free(rgb.pixels);
        Cleanup();
        return false;
    }

    // these input pointers will be used to track the data source
    // for the final copy to storage.data.
    GfHalf* inputHalf = (GfHalf*) rgb.pixels;
    float* inputFloat = nullptr;

    nanoexr_ImageData_t img;
    memset(&img, 0, sizeof(img));

    img.data = (uint8_t*) rgb.pixels;
    img.pixelType = EXR_PIXEL_HALF;
    img.channelCount = channelCount;
    img.width = rgb.width;
    img.height = rgb.height;
    img.dataWindowMinY = 0;
    img.dataWindowMaxY = rgb.height - 1;

    // crop in place
    ImageProcessor<GfHalf>::CropImage((GfHalf*) img.data,
                                      img.width, img.height,
                                      img.channelCount,
                                      cropTop, cropBottom,
                                      cropLeft, cropRight);

    // adjust dimensions as needed
    img.width = img.width - cropLeft - cropRight;
    img.width = img.height - cropTop - cropBottom;
    img.dataWindowMinY = img.height - 1;

    if (flip) {
        ImageProcessor<GfHalf>::FlipImage((GfHalf*) img.data,
                                          img.width,
                                          img.height,
                                          img.channelCount);
    }

    // resize
    bool resizing = (img.width != storage.width) ||
                    (img.width != storage.height);

    std::vector<float> outF32;
    if (resizing) {
        // resize in float
        std::vector<float> f32(img.width * img.height * channelCount);
        ImageProcessor<uint16_t>::HalfToFloat(
                                      inputHalf,
                                      f32.data(),
                                      img.width, img.height,
                                      channelCount);

        nanoexr_ImageData_t src = { 0 };
        src.data = reinterpret_cast<uint8_t*>(f32.data());
        src.channelCount = channelCount;
        src.dataSize = img.width * img.height * channelCount * sizeof(float);
        src.pixelType = EXR_PIXEL_FLOAT;
        src.width = img.width;
        src.height = img.height;

        outF32.resize(img.width * img.height * channelCount);
        inputFloat = outF32.data();

        nanoexr_ImageData_t dst = { 0 };
        dst.channelCount = channelCount;
        dst.dataSize = storage.width * storage.height * channelCount * sizeof(float);
        dst.pixelType = EXR_PIXEL_FLOAT;
        dst.width = storage.width;
        dst.height = storage.height;
        dst.data = (uint8_t*) inputFloat;

        nanoexr_Gaussian_resample(&src, &dst);

        inputHalf = nullptr;
    }

    if (_imposeSRGBTransfer) {
        // if the image was resized, the input data is now in inputFloat
        // otherwise, it is in inputHalf
        if (inputFloat == nullptr) {
            // resize outF32 to the correct size
            outF32.resize(img.width * img.height * channelCount);
            ImageProcessor<uint16_t>::HalfToFloat(
                                      inputHalf,
                                      outF32.data(),
                                      img.width, img.height,
                                      channelCount);
            inputFloat = outF32.data();
            inputHalf = nullptr;
        }
        GfColorSpace dst(GfColorSpaceNames->SRGBRec709);
        GfColorSpace src(GfColorSpaceNames->LinearRec709);
        if (channelCount == 3)
            dst.ConvertRGBSpan(src, TfSpan<float>(inputFloat, img.width * img.height * channelCount));
        else
            dst.ConvertRGBASpan(src, TfSpan<float>(inputFloat, img.width * img.height * channelCount));
    }

    // Finally, copy the data to storage.data
    if (outputIsHalf) {
        if (inputHalf != nullptr) {
            // copy rgb.pixels to storage.data
            memcpy(storage.data, inputHalf,
                   rgb.width * rgb.height * channelCount * sizeof(GfHalf));
        }
        else {
            // convert float to half
            ImageProcessor<float>::FloatToHalf(
                                    inputFloat, (GfHalf*) storage.data,
                                    rgb.width, rgb.height, channelCount);
        }
    }
    else {
        // output float case
        if (inputHalf != nullptr) {
            // convert half to float
            ImageProcessor<GfHalf>::HalfToFloat(
                                    inputHalf, (float*) storage.data,
                                    rgb.width, rgb.height, channelCount);
        }
        else {
            // copy rgb.pixels to storage.data
            memcpy(storage.data, inputFloat,
                   rgb.width * rgb.height * channelCount * sizeof(float));
        }
    }

    free(rgb.pixels);
    return true;
}

bool Hio_AVIFImage::_OpenForReading(std::string const &filename,
                                    int subimage, int mip,
                                    SourceColorSpace sourceColorSpace,
                                    bool /*suppressErrors*/)
{
    Cleanup();
    _width = 0;
    _height = 0;
    _filename = filename;
    _asset = ArGetResolver().OpenAsset(ArResolvedPath(filename));
    if (!_asset) {
        return false;
    }

    size_t sz = _asset->GetSize();
    uint8_t* data = (uint8_t*) malloc(sz);
    size_t offset = 0;
    size_t readSize = _asset->Read(data, sz, offset);
    if (!readSize)
        return false;

    // Initialize libavif
    _avifImage = avifImageCreateEmpty();
    avifDecoder *decoder = avifDecoderCreate();
    avifResult result = avifDecoderReadMemory(decoder, _avifImage, data, readSize);
    avifDecoderDestroy(decoder);
    if (result != AVIF_RESULT_OK) {
        TF_RUNTIME_ERROR("Error parsing AVIF file: %s\n", avifResultToString(result));
        Cleanup();
        return false;
    }
    _width = _avifImage->width;
    _height = _avifImage->height;

    // if the transfer function is unspecified, and the colorspace is Rec709
    // set the transfer function to sRGB. This is to match behavior observed
    // in Apple's Finder, and web browsers.
    _imposeSRGBTransfer =
        (sourceColorSpace == HioImage::SourceColorSpace::Auto ||
         sourceColorSpace == HioImage::SourceColorSpace::SRGB) &&
        (_avifImage->transferCharacteristics == AVIF_TRANSFER_CHARACTERISTICS_UNSPECIFIED) &&
        (_avifImage->colorPrimaries == AVIF_COLOR_PRIMARIES_UNSPECIFIED);
    return true;
}

// Writing is not supported.
bool Hio_AVIFImage::Write(StorageSpec const&, VtDictionary const &metadata)
{
    return false;
}

bool Hio_AVIFImage::_OpenForWriting(std::string const &filename)
{
    return false;
}


PXR_NAMESPACE_CLOSE_SCOPE
