//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/arch/pragmas.h"

// Not all functions in the OpenEXR library are used by Hio, and the OpenEXR
// symbols themselves are declared static for inclusion within Hio.
// Therefore, the unused function warning is suppressed as the messages are
// not useful for development, as it is expected that many functions are
// defined but not referenced or exported.
ARCH_PRAGMA_UNUSED_FUNCTION

#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hio/types.h"

#include "OpenEXR/openexr-c.h"
#include "OpenEXR/OpenEXRCore/internal_coding.h"

#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/writableAsset.h"

#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/range2d.h"
#include "pxr/base/gf/vec2f.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class Hio_OpenEXRImage final : public HioImage
{
    std::shared_ptr<ArAsset> _asset;
    std::string              _filename;
    nanoexr_Reader_t         _exrReader = {};
    SourceColorSpace         _sourceColorSpace = SourceColorSpace::Raw;
    int                      _subimage = 0;
    int                      _mip = 0;
    
    // mutable because GetMetadata is const, yet it doesn't make sense
    // to cache the dictionary unless metadata is requested.
    mutable VtDictionary     _metadata;

    // The callback dictionary is a pointer to the dictionary that was
    // passed to the Write method.  It is valid only during the
    // Write call.
    VtDictionary const*      _callbackDict = nullptr;

public:
    Hio_OpenEXRImage() = default;
    ~Hio_OpenEXRImage() override {
        nanoexr_free_storage(&_exrReader);
    }
    
    const std::shared_ptr<ArAsset> Asset() const { return _asset; }

    using Base = HioImage;
    bool Read(StorageSpec const &storage) override {
        return ReadCropped(0, 0, 0, 0, storage); 
    }
    bool ReadCropped(int const cropTop,  int const cropBottom,
                     int const cropLeft, int const cropRight,
                     StorageSpec const &storage) override;
    bool Write(StorageSpec const &storage,
               VtDictionary const &metadata) override;

    // IsColorSpaceSRGB asks if the color values are SRGB encoded against the
    // SRGB curve, although what Hydra really wants to know is whether the 
    // pixels are gamma pixels. OpenEXR images are always linear, so always 
    // return false.
    bool IsColorSpaceSRGB() const override { return false; }
    HioFormat GetFormat() const override;
    int  GetWidth() const override { return _exrReader.width; }
    int  GetHeight() const override { return _exrReader.height; }
    int  GetBytesPerPixel() const override;
    int  GetNumMipLevels() const override;
    bool GetMetadata(TfToken const &key, VtValue *value) const override;
    bool GetSamplerMetadata(HioAddressDimension dim,
                            HioAddressMode *param) const override;
    std::string const &GetFilename() const override { return _filename; }

    const VtDictionary &GetMetadata() const { return _metadata; }

protected:
    bool _OpenForReading(std::string const &filename, int subimage, int mip,
                         SourceColorSpace sourceColorSpace,
                         bool suppressErrors) override;
    bool _OpenForWriting(std::string const &filename) override;
    static void _AttributeReadCallback(void* self_, exr_context_t exr);
    static void _AttributeWriteCallback(void* self_, exr_context_t exr);
};

TF_REGISTRY_FUNCTION(TfType)
{
    using Image = Hio_OpenEXRImage;
    TfType t = TfType::Define<Image, TfType::Bases<Image::Base>>();
    t.SetFactory<HioImageFactory<Image>>();
}

HioFormat Hio_OpenEXRImage::GetFormat() const
{
    switch (_exrReader.pixelType)
    {
    case EXR_PIXEL_UINT:
        switch (_exrReader.channelCount)
        {
        case 1:  return HioFormatInt32;
        case 2:  return HioFormatInt32Vec2;
        case 3:  return HioFormatInt32Vec3;
        case 4:  return HioFormatInt32Vec4;
        default: return HioFormatInvalid;
        }

    case EXR_PIXEL_HALF:
        switch (_exrReader.channelCount)
        {
        case 1:  return HioFormatFloat16;
        case 2:  return HioFormatFloat16Vec2;
        case 3:  return HioFormatFloat16Vec3;
        case 4:  return HioFormatFloat16Vec4;
        default: return HioFormatInvalid;
        }

    case EXR_PIXEL_FLOAT:
        switch (_exrReader.channelCount)
        {
        case 1:  return HioFormatFloat32;
        case 2:  return HioFormatFloat32Vec2;
        case 3:  return HioFormatFloat32Vec3;
        case 4:  return HioFormatFloat32Vec4;
        default: return HioFormatInvalid;
        }

    default:
        return HioFormatInvalid;
    }
}

int Hio_OpenEXRImage::GetBytesPerPixel() const
{
    return _exrReader.channelCount *
                static_cast<int>(HioGetDataSizeOfType(GetFormat()));
}

int Hio_OpenEXRImage::GetNumMipLevels() const
{
    return _exrReader.numMipLevels;
}

namespace {
    // For consistency with other Hio plugins, reading is done through ArAsset,
    // but writing is not.
    int64_t exr_AssetRead_Func(
                               exr_const_context_t         ctxt,
                               void*                       userdata,
                               void*                       buffer,
                               uint64_t                    sz,
                               uint64_t                    offset,
                               exr_stream_error_func_ptr_t error_cb)
    {
        Hio_OpenEXRImage* self = (Hio_OpenEXRImage*) userdata;
        if (!self || !self->Asset() || !buffer || !sz) {
            if (error_cb) {
                error_cb(ctxt, EXR_ERR_INVALID_ARGUMENT,
                         "%s", "Invalid arguments to read callback");
	    }
            return -1;
        }
        return self->Asset()->Read(buffer, sz, offset);
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
                || (newWidth == width && newHeight == height)) {
                return;
            }
            
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
            if (!buffer || !outBuffer) {
                return;
            }
            
            for (int i = 0; i < width * height * channelCount; ++i) {
                outBuffer[i] = buffer[i];
            }
        }
        
        static void FloatToHalf(float* buffer, GfHalf* outBuffer,
                                int width, int height, int channelCount)
        {
            if (!buffer || !outBuffer) {
                return;
            }
            
            for (int i = 0; i < width * height * channelCount; ++i) {
                outBuffer[i] = buffer[i];
            }
        }
    };
   
} // anon

bool Hio_OpenEXRImage::ReadCropped(
                int const cropTop,  int const cropBottom,
                int const cropLeft, int const cropRight, 
                StorageSpec const& storage)
{
	// not opened for read prior to calling ReadCropped.
    if (!_asset) {
        return false;
    }

    if (cropTop < 0 || cropBottom < 0 || cropLeft < 0 || cropRight < 0) {
        return false;
    }

    // cache values for the read/crop/resize pipeline

    int fileWidth =  _exrReader.width;
    int fileHeight = _exrReader.height;
    int fileChannelCount = _exrReader.channelCount;
    exr_pixel_type_t filePixelType = _exrReader.pixelType;

    int outWidth =  storage.width;
    int outHeight = storage.height;
    int outChannelCount = HioGetComponentCount(storage.format);

    bool inputIsHalf =   filePixelType == EXR_PIXEL_HALF;
    bool inputIsFloat =  filePixelType == EXR_PIXEL_FLOAT;
    bool inputIsUInt =    filePixelType == EXR_PIXEL_UINT;
    bool outputIsFloat = HioGetHioType(storage.format) == HioTypeFloat;
    bool outputIsHalf =  HioGetHioType(storage.format) == HioTypeHalfFloat;
    bool outputIsUInt =  HioGetHioType(storage.format) == HioTypeUnsignedInt;

    // no conversion to anything except these formats
    if (!(outputIsHalf || outputIsFloat || outputIsUInt)) {
        return false;
    }

    // no conversion to uint from non uint
    if (outputIsUInt && !inputIsUInt) {
        return false;
    }

    // no coversion of non float to float
    if (outputIsFloat && !(inputIsFloat || inputIsHalf)) {
        return false;
    }

    int outputBytesPerPixel = 
	    (int) HioGetDataSizeOfType(storage.format) * outChannelCount;

    int readWidth = fileWidth - cropLeft - cropRight;
    int readHeight = fileHeight - cropTop - cropBottom;
    if (readHeight <= 0 || readWidth <= 0) {
        memset(storage.data, 0, outWidth * outHeight * outputBytesPerPixel);
        return true;
    }
    bool resizing = (readWidth != outWidth) || (readHeight != outHeight);
    if (outputIsUInt && resizing) {
        // resizing is not supported for uint types.
        return false;
    }
    
    bool flip = storage.flipped;

    if (outputIsUInt) {
        // no conversion to float; read the data, and crop it if necessary.
        nanoexr_ImageData_t img;
        const int partIndex = 0;
        exr_result_t rv = nanoexr_read_exr(_exrReader.filename,
                                           exr_AssetRead_Func, this,
                                           &img, nullptr, 
                                           outChannelCount,
                                           partIndex, _mip);
        if (rv != EXR_ERR_SUCCESS) {
            return false;
        }
        ImageProcessor<uint32_t>::CropImage(reinterpret_cast<uint32_t*>(img.data),
                                            fileWidth, fileHeight,
                                            img.channelCount,
                                            cropTop, cropBottom,
                                            cropLeft, cropRight);
        if (flip) {
            ImageProcessor<uint32_t>::FlipImage(reinterpret_cast<uint32_t*>(img.data),
                                                fileWidth - cropLeft - cropRight,
                                                fileHeight - cropTop - cropBottom,
                                                img.channelCount);
        }

        // copy the data to the output buffer.
        memcpy(storage.data, img.data, outWidth * outHeight * outputBytesPerPixel);
        nanoexr_release_image_data(&img);
        return true;
    }

    // ensure there's enough memory for the greater of input and output channel
    // count, for in place conversions.
    int maxChannelCount = std::max(fileChannelCount, outChannelCount);
    std::vector<GfHalf> halfInputBuffer;
    if (inputIsHalf) {
        halfInputBuffer.resize(fileWidth * fileHeight * maxChannelCount);
    }
    std::vector<float> floatInputBuffer;
    if (inputIsFloat || (inputIsHalf && (resizing || outputIsFloat))) {
        floatInputBuffer.resize(fileWidth * fileHeight * maxChannelCount);
    }

    {
        nanoexr_ImageData_t img;
        const int partIndex = 0;
        exr_result_t rv = nanoexr_read_exr(_exrReader.filename,
                                           exr_AssetRead_Func, this,
                                           &img, nullptr, 
                                           outChannelCount, partIndex, _mip);
        if (rv != EXR_ERR_SUCCESS) {
            return false;
        }

        if (img.pixelType == EXR_PIXEL_HALF) {
            memcpy(&halfInputBuffer[0], img.data, img.dataSize);
        }
        else {
            memcpy(&floatInputBuffer[0], img.data, img.dataSize);
        }
        
        nanoexr_release_image_data(&img);

        // flip and crop the image in place
        if (inputIsHalf) {
            ImageProcessor<GfHalf>::CropImage(&halfInputBuffer[0],
                                              fileWidth, fileHeight,
                                              img.channelCount,
                                              cropTop, cropBottom,
                                              cropLeft, cropRight);
            if (flip) {
                ImageProcessor<GfHalf>::FlipImage(&halfInputBuffer[0],
                                                  fileWidth - cropLeft - cropRight,
                                                  fileHeight - cropTop - cropBottom,
                                                  img.channelCount);
            }
        }
        else {
            ImageProcessor<float>::CropImage(&floatInputBuffer[0],
                                             fileWidth, fileHeight,
                                             img.channelCount,
                                             cropTop, cropBottom,
                                             cropLeft, cropRight);
            if (flip) {
                ImageProcessor<float>::FlipImage(&floatInputBuffer[0],
                                                 fileWidth - cropLeft - cropRight,
                                                 fileHeight - cropTop - cropBottom,
                                                 img.channelCount);
            }
        }
    }

    if (!resizing) {
        uint32_t outSize = outWidth * outHeight * outputBytesPerPixel;
        uint32_t outCount = outWidth * outHeight * outChannelCount;
        if (inputIsHalf && outputIsHalf) {
            memcpy(reinterpret_cast<void*>(storage.data),
               halfInputBuffer.data(), outSize);
        }
        else if (inputIsFloat && outputIsFloat) {
            memcpy(reinterpret_cast<void*>(storage.data),
               floatInputBuffer.data(), outSize);
        }
        else if (outputIsFloat) {
            GfHalf* src = halfInputBuffer.data();
            float* dst = reinterpret_cast<float*>(storage.data);
            for (size_t i = 0; i < outCount; ++i)
                dst[i] = src[i];
        }
        else {
            float* src = floatInputBuffer.data();
            GfHalf* dst = reinterpret_cast<GfHalf*>(storage.data);
            for (size_t i = 0; i < outCount; ++i)
                dst[i] = src[i];
        }
        return true;
    }

    // resize the image, so promote to float if necessary
    if (inputIsHalf) {
        ImageProcessor<uint16_t>::HalfToFloat(&halfInputBuffer[0],
                                              &floatInputBuffer[0],
                                              fileWidth, fileHeight,
                                              fileChannelCount);
        inputIsFloat = true;
        inputIsHalf = false;
    }

    std::vector<float> resizeOutputBuffer;

    nanoexr_ImageData_t src = { 0 };
    src.data = reinterpret_cast<uint8_t*>(&floatInputBuffer[0]);
    src.channelCount = fileChannelCount;
    src.dataSize = readWidth * readHeight * GetBytesPerPixel();
    src.pixelType = EXR_PIXEL_FLOAT;
    src.width = readWidth;
    src.height = readHeight;

    nanoexr_ImageData_t dst = { 0 };
    dst.channelCount = outChannelCount;
    dst.dataSize = outWidth * outHeight * outChannelCount * sizeof(float);
    dst.pixelType = EXR_PIXEL_FLOAT;
    dst.width = outWidth;
    dst.height = outHeight;

    if (outputIsFloat) {
        dst.data = reinterpret_cast<uint8_t*>(storage.data);
    }
    else {
        resizeOutputBuffer.resize(outWidth * outHeight * outChannelCount);
        dst.data = reinterpret_cast<uint8_t*>(&resizeOutputBuffer[0]);
    }
    nanoexr_Gaussian_resample(&src, &dst);
    if (outputIsFloat) {
        memcpy(reinterpret_cast<void*>(storage.data), dst.data, dst.dataSize);
        return true;
    }

    ImageProcessor<float>::FloatToHalf(&resizeOutputBuffer[0],
                                       reinterpret_cast<GfHalf*>(dst.data),
                                       outWidth, outHeight, outChannelCount);
    memcpy(reinterpret_cast<void*>(storage.data), &resizeOutputBuffer[0],
           outWidth * outHeight * GetBytesPerPixel());
    return true;
}

namespace {
    // Note that the alternative names and casing are for historical
    // compatibility. The OpenEXR standard attribute names are worldToNDC and
    // and worldToCamera.
    bool isWorldToNDC(const std::string& name)
    {
        return name == "NP" || name == "worldtoscreen"
                || name == "worldToScreen" || name == "worldToNDC";
    }

    bool isWorldToCamera(const std::string& name)
    {
        return name == "Nl" || name == "worldtocamera"
                || name == "worldToCamera";
    }
}

bool Hio_OpenEXRImage::GetMetadata(TfToken const &key, VtValue *value) const
{
    if (!value) {
        TF_CODING_ERROR("Invalid value pointer");
        return false;
    }

    auto convertM4dIfNecessary = [](const VtValue& v) -> VtValue {
        if (v.CanCastToTypeid(typeid(GfMatrix4d))) {
            return v.CastToTypeid(v, typeid(GfMatrix4d));
        }
        return v;
    };
    
    bool isW2N = isWorldToNDC(key);
    bool isW2C = isWorldToCamera(key);
    if (isW2N || isW2C) {
        auto candidate = _metadata.find(key);
        if (candidate != _metadata.end()) {
            *value = convertM4dIfNecessary(candidate->second);
            return true;
        }
    }
    
    // try translating common alternatives to a standard attribute
    if (isW2N) {
        auto candidate = _metadata.find("worldToNDC");
        if (candidate != _metadata.end()) {
            *value = convertM4dIfNecessary(candidate->second);
            return true;
        }
    }
    if (isW2C) {
        auto candidate = _metadata.find("worldToCamera");
        if (candidate != _metadata.end()) {
            *value = convertM4dIfNecessary(candidate->second);
            return true;
        }
    }

    // any other key is returned as it's found
    auto candidate = _metadata.find(key);
    if (candidate != _metadata.end()) {
        *value = candidate->second;
        return true;
    }

    return false;
}

bool Hio_OpenEXRImage::GetSamplerMetadata(HioAddressDimension dim,
                                          HioAddressMode *param) const
{
    if (!param)
        return false;
    
    switch (_exrReader.wrapMode) {
        case nanoexr_WrapModeClampToEdge:
            *param = HioAddressModeClampToEdge; break;
        case nanoexr_WrapModeMirrorClampToEdge:
            *param = HioAddressModeClampToEdge; break;
        case nanoexr_WrapModeRepeat:
            *param = HioAddressModeRepeat; break;
        case nanoexr_WrapModeMirrorRepeat:
            *param = HioAddressModeMirrorRepeat; break;
        case nanoexr_WrapModeClampToBorderColor:
            *param = HioAddressModeClampToBorderColor;
    }
    return true;
}

//static
void Hio_OpenEXRImage::_AttributeReadCallback(void* self_, exr_context_t exr) {
    Hio_OpenEXRImage* self = reinterpret_cast<Hio_OpenEXRImage*>(self_);
    if (!self->_metadata.empty()) {
        return;
    }
    
    const int partIndex = self->_subimage;
    int attrCount = nanoexr_get_attribute_count(exr, partIndex);
    for (int i = 0; i < attrCount; ++i) {
        const exr_attribute_t* attr;
        nanoexr_get_attribute_by_index(exr, partIndex, i, &attr);
        if (!attr) {
            continue;
        }

        // this switch is an exhaustive alphabetical treatment of all the
        // possible attribute types.
        switch(attr->type) {
            case EXR_ATTR_UNKNOWN:
                continue;
            case EXR_ATTR_BOX2I: {
                // no GfVec2i, convert to float
                GfVec2f box_min, box_max;
                box_min.Set((float) attr->box2i->min.x, (float) attr->box2i->min.y);
                box_max.Set((float) attr->box2i->max.x, (float) attr->box2i->max.x);
                self->_metadata[attr->name] = VtValue(GfRange2f(box_min, box_max));
                break;
            }
            case EXR_ATTR_BOX2F: {
                GfVec2f box_min, box_max;
                box_min.Set(attr->box2f->min.x, attr->box2f->min.y);
                box_max.Set(attr->box2f->max.x, attr->box2f->max.y);
                self->_metadata[attr->name] = VtValue(GfRange2f(box_min, box_max));
                break;
            }
            case EXR_ATTR_CHLIST:
            case EXR_ATTR_CHROMATICITIES:
            case EXR_ATTR_COMPRESSION:
                // these are explicitly handled elsewhere, they aren't
                // metadata attributes for Hio's purposes.
                continue;
            case EXR_ATTR_DOUBLE:
                self->_metadata[attr->name] = VtValue(attr->d);
                break;
            case EXR_ATTR_ENVMAP:
                // Hio doesn't specifically treat cube and lot-lang maps.
                // If it did, this case would be handled elsewhere.
                break;
            case EXR_ATTR_FLOAT:
                self->_metadata[attr->name] = VtValue(attr->f);
                break;
            case EXR_ATTR_FLOAT_VECTOR: {
                std::vector<float> v;
                v.resize(attr->floatvector->length);
                memcpy(v.data(), attr->floatvector->arr, v.size() * sizeof(float));
                self->_metadata[attr->name] = VtValue(v);
            }
            case EXR_ATTR_INT:
                self->_metadata[TfToken(attr->name)] = VtValue(attr->i);
                break;
            case EXR_ATTR_KEYCODE:
            case EXR_ATTR_LINEORDER:
                // these are explicitly handled elsewhere, they aren't
                // metadata attributes for Hio's purposes.
                continue;
            case EXR_ATTR_M33F: {
                GfMatrix3f m;
                memcpy(m.GetArray(), attr->m33f, 9 * sizeof(float));
                self->_metadata[attr->name] = VtValue(m);
                break;
            }
            case EXR_ATTR_M33D: {
                GfMatrix3d m;
                memcpy(m.GetArray(), attr->m33d, 9 * sizeof(double));
                self->_metadata[attr->name] = VtValue(m);
                break;
            }
            case EXR_ATTR_M44F: {
                GfMatrix4f m;
                memcpy(m.GetArray(), attr->m44f, 16 * sizeof(float));
                self->_metadata[attr->name] = VtValue(m);
                break;
            }
            case EXR_ATTR_M44D: {
                GfMatrix4d m;
                memcpy(m.GetArray(), attr->m44d, 16 * sizeof(double));
                self->_metadata[attr->name] = VtValue(m);
                break;
            }
            case EXR_ATTR_PREVIEW:
                // EXR images may have a poster image, but Hio doesn't
                continue;
            case EXR_ATTR_RATIONAL: {
                // Gf doesn't have rational numbers, so degrade to a float.
                float f = (float) attr->rational->num / (float) attr->rational->denom;
                self->_metadata[attr->name] = VtValue(f);
                break;
            }
            case EXR_ATTR_STRING:
                self->_metadata[attr->name] = VtValue(attr->string);
                break;
            case EXR_ATTR_STRING_VECTOR: {
                std::vector<std::string> v;
                v.resize(attr->stringvector->n_strings);
                for (size_t i = 0; i < v.size(); ++i) {
                    v[i] = attr->stringvector->strings[i].str;
                }
                self->_metadata[attr->name] = VtValue(v);
                break;
            }
            case EXR_ATTR_TILEDESC:
                // this is explicitly handled elsewhere, it isn't
                // metadata attributes for Hio's purposes.
                continue;
            case EXR_ATTR_TIMECODE:
                // Is there a VtValue that can represent this?
                continue;
            case EXR_ATTR_V2I: {
                // there's no GfVec2i, convert to double
                GfVec2d v;
                v.Set((double) attr->v2i->x, (double) attr->v2i->y);
                self->_metadata[attr->name] = VtValue(v);
                break;
            }
            case EXR_ATTR_V2F: {
                GfVec2f v;
                v.Set(attr->v2f->x, attr->v2f->y);
                self->_metadata[attr->name] = VtValue(v);
                break;
            }
            case EXR_ATTR_V2D: {
                GfVec2d v;
                v.Set(attr->v2d->x, attr->v2d->y);
                self->_metadata[attr->name] = VtValue(v);
                break;
            }
            case EXR_ATTR_V3I: {
                // there's no GfVec3i, convert to double
                GfVec3d v;
                v.Set((double) attr->v3i->x,
                      (double) attr->v3i->y,
                      (double) attr->v3i->z);
                self->_metadata[attr->name] = VtValue(v);
                break;
            }
            case EXR_ATTR_V3F: {
                GfVec3f v;
                v.Set(attr->v3f->x, attr->v3f->y, attr->v3f->z);
                self->_metadata[attr->name] = VtValue(v);
                break;
            }
            case EXR_ATTR_V3D: {
                GfVec3d v;
                v.Set(attr->v3d->x, attr->v3d->y, attr->v3d->z);
                self->_metadata[attr->name] = VtValue(v);
                break;
            }
            case EXR_ATTR_LAST_KNOWN_TYPE:
            case EXR_ATTR_OPAQUE:
                // Not caching opaque data
                continue;
        }
    }
    if (self->_metadata.empty()) {
        self->_metadata["placeholder"] = VtValue(true);
    }
}

bool Hio_OpenEXRImage::_OpenForReading(std::string const &filename,
                                       int subimage, int mip,
                                       SourceColorSpace sourceColorSpace,
                                       bool /*suppressErrors*/)
{
    _asset = ArGetResolver().OpenAsset(ArResolvedPath(filename));
    if (!_asset) {
        return false;
    }

    _filename = filename;
    _subimage = subimage;
    _mip = mip;
    _sourceColorSpace = sourceColorSpace;

    nanoexr_set_defaults(_filename.c_str(), &_exrReader);

    int rv = nanoexr_read_header(&_exrReader, exr_AssetRead_Func,
                                 _AttributeReadCallback, this,
                                 _subimage);
    if (rv != 0) {
        TF_DIAGNOSTIC_WARNING("Cannot open image \"%s\" for reading, %s",
                        filename.c_str(), nanoexr_get_error_code_as_string(rv));
        return false;
    }

    if (_exrReader.numMipLevels <= mip) {
        TF_DIAGNOSTIC_WARNING("In image \"%s\" mip level %d does not exist",
                        filename.c_str(), mip);
        return false;
    }
    
    _exrReader.width >>= mip;
    _exrReader.height >>= mip;
    
    return true;
}

void Hio_OpenEXRImage::_AttributeWriteCallback(void* self_, exr_context_t exr) {
    Hio_OpenEXRImage* self = reinterpret_cast<Hio_OpenEXRImage*>(self_);
    for (const std::pair<std::string, VtValue> m : *self->_callbackDict) {
        const std::string& key = m.first;
        const VtValue& value = m.second;
        // note: OpenEXR can represent most values that can be found in a
        // VtValue, however, for the moment, this code is matching the behavior
        // of the OpenImageIO plugin.
        if (value.IsHolding<std::string>()) {
            nanoexr_attr_set_string(exr, self->_subimage, key.c_str(),
                                value.Get<std::string>().c_str());
        }
        else if (value.IsHolding<char>()) {
            nanoexr_attr_set_int(exr, self->_subimage, key.c_str(),
                             value.Get<char>());
        }
        else if (value.IsHolding<unsigned char>()) {
            nanoexr_attr_set_int(exr, self->_subimage, key.c_str(),
                             value.Get<unsigned char>());
        }
        else if (value.IsHolding<int>()) {
            nanoexr_attr_set_int(exr, self->_subimage, key.c_str(),
                             value.Get<int>());
        }
        else if (value.IsHolding<unsigned int>()) {
            nanoexr_attr_set_int(exr, self->_subimage, key.c_str(),
                             value.Get<unsigned int>());
        }
        else if (value.IsHolding<float>()) {
            nanoexr_attr_set_float(exr, self->_subimage, key.c_str(),
                               value.Get<float>());
        }
        else if (value.IsHolding<double>()) {
            nanoexr_attr_set_double(exr, self->_subimage, key.c_str(),
                               value.Get<double>());
        }
        else if (value.IsHolding<GfMatrix4f>()) {
            nanoexr_attr_set_m44f(exr, self->_subimage, key.c_str(),
                              value.Get<GfMatrix4f>().GetArray());
        }
        else if (value.IsHolding<GfMatrix4d>()) {
            // historic compatibility, downgread m44d matrices for these two
            // attributes to float.
            if (isWorldToNDC(key) || isWorldToCamera(key)) {
                // for Ice/Imr, convert to m44f.
                GfMatrix4f mf(value.Get<GfMatrix4d>());
                nanoexr_attr_set_m44f(exr, self->_subimage, key.c_str(),
                                  mf.GetArray());
            }
            else {
                nanoexr_attr_set_m44d(exr, self->_subimage, key.c_str(),
                                  value.Get<GfMatrix4d>().GetArray());
            }
        }
    }
}

bool Hio_OpenEXRImage::Write(StorageSpec const &storage,
                             VtDictionary const &metadata)
{
    _callbackDict = &metadata;
    exr_result_t rv;
    const HioType type = HioGetHioType(storage.format);
    int32_t pxsize = type == HioTypeFloat ? sizeof(float) : sizeof(GfHalf);
    int32_t ch = HioGetComponentCount(storage.format);
    int32_t pixelStride = pxsize * ch;
    int32_t lineStride = storage.width * pxsize * ch;
    if (type == HioTypeUnsignedByte) {
        // glf will attempt to write 8 bit unsigned frame buffer data to exr
        // files, so promote the pixels to float16.
        int32_t ch = HioGetComponentCount(storage.format);
        std::vector<GfHalf> pixels(storage.width * storage.height * ch);
        const uint8_t* src = reinterpret_cast<const uint8_t*>(storage.data);
        GfHalf* dst = pixels.data();
        for (int i = 0; i < storage.width * storage.height * ch; ++i) {
            *dst++ = GfHalf(*src++) / 255.0f;
        }
        int pixMul = 0;
        uint8_t* red = nullptr;
        uint8_t* green = nullptr;
        uint8_t* blue = nullptr;
        uint8_t* alpha = nullptr;
        if (ch > 0) {
            red = (uint8_t*) pixels.data() + (pxsize * pixMul);
            ++pixMul;
        }
        if (ch > 1) {
            green = (uint8_t*) pixels.data() + (pxsize * pixMul);
            ++pixMul;
        }
        if (ch > 2) {
            blue = (uint8_t*) pixels.data() + (pxsize * pixMul);
            ++pixMul;
        }
        if (ch > 3) {
            alpha = (uint8_t*) pixels.data() + (pxsize * pixMul);
            ++pixMul;
        }
        rv = nanoexr_write_exr(
                _filename.c_str(),
                _AttributeWriteCallback, this,
                storage.width, storage.height, storage.flipped,
                EXR_PIXEL_HALF,
                (uint8_t*) red,   pixelStride, lineStride,
                (uint8_t*) green, pixelStride, lineStride,
                (uint8_t*) blue,  pixelStride, lineStride,
                (uint8_t*) alpha, pixelStride, lineStride);
        _callbackDict = nullptr;
        return rv == EXR_ERR_SUCCESS;
    }
    else if (type != HioTypeFloat && type != HioTypeHalfFloat) {
        TF_CODING_ERROR("Unsupported pixel type %d", type);
        _callbackDict = nullptr;
        return false;
    }

    uint8_t* pixels = reinterpret_cast<uint8_t*>(storage.data);
    int pixMul = 0;
    uint8_t* red = nullptr;
    uint8_t* green = nullptr;
    uint8_t* blue = nullptr;
    uint8_t* alpha = nullptr;
    if (ch > 0) {
        red = pixels + (pxsize * pixMul);
        ++pixMul;
    }
    if (ch > 1) {
        green = pixels + (pxsize * pixMul);
        ++pixMul;
    }
    if (ch > 2) {
        blue = pixels + (pxsize * pixMul);
        ++pixMul;
    }
    if (ch > 3) {
        alpha = pixels + (pxsize * pixMul);
        ++pixMul;
    }

    if (type == HioTypeFloat) {
        rv = nanoexr_write_exr(
                _filename.c_str(),
                _AttributeWriteCallback, this,
                storage.width, storage.height, storage.flipped,
                EXR_PIXEL_FLOAT,
                red,   pixelStride, lineStride,
                green, pixelStride, lineStride,
                blue,  pixelStride, lineStride,
                alpha, pixelStride, lineStride);
    }
    else {
        rv = nanoexr_write_exr(
                _filename.c_str(),
                _AttributeWriteCallback, this,
                storage.width, storage.height, storage.flipped,
                EXR_PIXEL_HALF,
                red,   pixelStride, lineStride,
                green, pixelStride, lineStride,
                blue,  pixelStride, lineStride,
                alpha, pixelStride, lineStride);
    }

    _callbackDict = nullptr;
    return rv == EXR_ERR_SUCCESS;
}

bool Hio_OpenEXRImage::_OpenForWriting(std::string const &filename)
{
    _filename = filename;
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
