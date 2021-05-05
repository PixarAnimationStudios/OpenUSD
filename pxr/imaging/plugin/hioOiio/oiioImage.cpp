//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hio/types.h"

#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"

// use gf types to read and write metadata
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/arch/pragmas.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

ARCH_PRAGMA_PUSH
ARCH_PRAGMA_MACRO_REDEFINITION // due to Python copysign
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/filesystem.h>
#include <OpenImageIO/typedesc.h>
ARCH_PRAGMA_POP

PXR_NAMESPACE_OPEN_SCOPE

OIIO_NAMESPACE_USING

// _ioProxySupportedExtensions is a list of hardcoded file extensions that 
// support ioProxy. Although OIIO has an api call for checking whether or 
// not a file type supports ioProxy, version 2.0.9 does not include this 
// for EXR's, even though EXR's support ioProxy. This issue was fixed in
// commit 7677d498b599295fa8277d050ef994efbd297b55. Thus, for now we check 
// whether or not a file extension is included in our hardcoded list of 
// extensions we know to support ioProxy. 
TF_MAKE_STATIC_DATA(std::vector<std::string>, _ioProxySupportedExtensions)
{
    _ioProxySupportedExtensions->push_back("exr");
}

class HioOIIO_Image : public HioImage
{
public:
    using Base = HioImage;

    HioOIIO_Image();

    ~HioOIIO_Image() override;

    // HioImage overrides
    std::string const & GetFilename() const override;
    int GetWidth() const override;
    int GetHeight() const override;
    HioFormat GetFormat() const override;
    int GetBytesPerPixel() const override;
    int GetNumMipLevels() const override;

    bool IsColorSpaceSRGB() const override;

    bool GetMetadata(TfToken const & key, 
                             VtValue * value) const override;
    bool GetSamplerMetadata(HioAddressDimension pname,
                            HioAddressMode * param) const override;

    bool Read(StorageSpec const & storage) override;
    bool ReadCropped(int const cropTop,
                     int const cropBottom,
                     int const cropLeft,
                     int const cropRight,
                     StorageSpec const & storage) override;

    bool Write(StorageSpec const & storage,
               VtDictionary const & metadata) override;

protected:
    bool _OpenForReading(std::string const & filename, int subimage,
                                 int mip, 
                                 HioImage::SourceColorSpace sourceColorSpace,
                                 bool suppressErrors) override;
    bool _OpenForWriting(std::string const & filename) override;

private:
    std::string _GetFilenameExtension() const;
#if OIIO_VERSION >= 20003
    cspan<unsigned char> _GenerateBufferCSpan(
        const std::shared_ptr<const char>& buffer,
        int bufferSize) const;
#endif
    bool _CanUseIOProxyForExtension(std::string extension, 
                                    const ImageSpec &config) const;
    std::string _filename;
    int _subimage;
    int _miplevel;
    ImageSpec _imagespec;
    HioImage::SourceColorSpace _sourceColorSpace;
};

TF_REGISTRY_FUNCTION(TfType)
{
    using Image = HioOIIO_Image;
    TfType t = TfType::Define<Image, TfType::Bases<Image::Base> >();
    t.SetFactory< HioImageFactory<Image> >();
}

/// Converts an OpenImageIO component type to its HioFormat equivalent.
static HioFormat
_GetHioFormatFromImageData(unsigned int nchannels, TypeDesc typedesc,
                           bool isSRGB)
{
    switch (nchannels) {
        case 1:
            switch (typedesc.basetype) {
                case TypeDesc::UINT8:
                    if (isSRGB) {
                        return HioFormatUNorm8srgb;
                    }
                    return HioFormatUNorm8;
                case TypeDesc::INT8:
                    return HioFormatSNorm8;
                case TypeDesc::UINT16:
                    return HioFormatUInt16;
                case TypeDesc::INT16:
                    return HioFormatInt16;
                case TypeDesc::UINT32:
                    return HioFormatUInt32;
                case TypeDesc::INT32:
                    return HioFormatInt32;
                case TypeDesc::HALF:
                    return HioFormatFloat16;
                case TypeDesc::FLOAT:
                    return HioFormatFloat32;                
            }
        case 2:
            switch (typedesc.basetype) {
                case TypeDesc::UINT8:
                    if (isSRGB) {
                        return HioFormatUNorm8Vec2srgb;
                    }
                    return HioFormatUNorm8Vec2;
                case TypeDesc::INT8:
                    return HioFormatSNorm8Vec2;
                case TypeDesc::UINT16:
                    return HioFormatUInt16Vec2;
                case TypeDesc::INT16:
                    return HioFormatInt16Vec2;
                case TypeDesc::UINT32:
                    return HioFormatUInt32Vec2;
                case TypeDesc::INT32:
                    return HioFormatInt32Vec2;
                case TypeDesc::HALF:
                    return HioFormatFloat16Vec2;
                case TypeDesc::FLOAT:
                    return HioFormatFloat32Vec2;             
            }
        case 3:
            switch (typedesc.basetype) {
                case TypeDesc::UINT8:
                    if (isSRGB) {
                        return HioFormatUNorm8Vec3srgb;
                    }
                    return HioFormatUNorm8Vec3;
                case TypeDesc::INT8:
                    return HioFormatSNorm8Vec3;
                case TypeDesc::UINT16:
                    return HioFormatUInt16Vec3;
                case TypeDesc::INT16:
                    return HioFormatInt16Vec3;
                case TypeDesc::UINT32:
                    return HioFormatUInt32Vec3;
                case TypeDesc::INT32:
                    return HioFormatInt32Vec3;
                case TypeDesc::HALF:
                    return HioFormatFloat16Vec3;
                case TypeDesc::FLOAT:
                    return HioFormatFloat32Vec3;             
            }
        case 4:
            switch (typedesc.basetype) {
                case TypeDesc::UINT8:
                    if (isSRGB) {
                        return HioFormatUNorm8Vec4srgb;
                    }
                    return HioFormatUNorm8Vec4;
                case TypeDesc::INT8:
                    return HioFormatSNorm8Vec4;
                case TypeDesc::UINT16:
                    return HioFormatUInt16Vec4;
                case TypeDesc::INT16:
                    return HioFormatInt16Vec4;
                case TypeDesc::UINT32:
                    return HioFormatUInt32Vec4;
                case TypeDesc::INT32:
                    return HioFormatInt32Vec4;
                case TypeDesc::HALF:
                    return HioFormatFloat16Vec4;
                case TypeDesc::FLOAT:
                    return HioFormatFloat32Vec4;             
            }
        default:
            TF_CODING_ERROR("Unsupported type");
            return HioFormatUNorm8Vec3;
    }
}
/// Converts an HioFormat into its OpenImageIO component type equivalent.
// XXX does not support conversions for:HioFormatBC6FloatVec3, 
// HioFormatBC6UFloatVec3, HioFormatBC7UNorm8Vec4, or HioFormatBC7UNorm8Vec4srgb
static TypeDesc
_GetOIIOBaseType(HioFormat hioFormat)
{
    switch (hioFormat) {
        case HioFormatUNorm8:
        case HioFormatUNorm8Vec2:
        case HioFormatUNorm8Vec3:
        case HioFormatUNorm8Vec4:
        case HioFormatUNorm8srgb:
        case HioFormatUNorm8Vec2srgb:
        case HioFormatUNorm8Vec3srgb:
        case HioFormatUNorm8Vec4srgb:
            return TypeDesc::UINT8;
        case HioFormatSNorm8:
        case HioFormatSNorm8Vec2:
        case HioFormatSNorm8Vec3:
        case HioFormatSNorm8Vec4:
            return TypeDesc::INT8;
        case HioFormatUInt16:
        case HioFormatUInt16Vec2:
        case HioFormatUInt16Vec3:
        case HioFormatUInt16Vec4:
            return TypeDesc::UINT16;
        case HioFormatInt16:
        case HioFormatInt16Vec2:
        case HioFormatInt16Vec3:
        case HioFormatInt16Vec4:
            return TypeDesc::INT16;
        case HioFormatUInt32:
        case HioFormatUInt32Vec2:
        case HioFormatUInt32Vec3:
        case HioFormatUInt32Vec4:
            return TypeDesc::UINT32;
        case HioFormatInt32:
        case HioFormatInt32Vec2:
        case HioFormatInt32Vec3:
        case HioFormatInt32Vec4:
            return TypeDesc::INT32;
        case HioFormatFloat16:
        case HioFormatFloat16Vec2:
        case HioFormatFloat16Vec3:
        case HioFormatFloat16Vec4:
            return TypeDesc::HALF;
        case HioFormatFloat32:
        case HioFormatFloat32Vec2:
        case HioFormatFloat32Vec3:
        case HioFormatFloat32Vec4:
            return TypeDesc::FLOAT;
        default:
            TF_CODING_ERROR("Unsupported type");
            return TypeDesc::FLOAT;
    }
}

// For compatability with Ice/Imr we transmogrify some matrix metadata
static std::string
_TranslateMetadataKey(std::string const & metadataKey, bool *convertMatrixTypes)
{
    if (metadataKey == "NP") {
        *convertMatrixTypes = true;
        return "worldtoscreen";
    } else
    if (metadataKey == "Nl") {
        *convertMatrixTypes = true;
        return "worldtocamera";
    } else {
        return metadataKey;
    }
}

static VtValue
_FindAttribute(ImageSpec const & spec, std::string const & metadataKey)
{
    bool convertMatrixTypes = false;
    std::string key = _TranslateMetadataKey(metadataKey, &convertMatrixTypes);

    ImageIOParameter const * param = spec.find_attribute(key);
    if (!param) {
        return VtValue();
    }

    TypeDesc const & type = param->type();
    switch (type.aggregate) {
    case TypeDesc::SCALAR:
        switch (type.basetype) {
        case TypeDesc::STRING:
            return VtValue(std::string((char*)param->data()));
        case TypeDesc::INT8:
            return VtValue(*((char*)param->data()));
        case TypeDesc::UINT8:
            return VtValue(*((unsigned char*)param->data()));
        case TypeDesc::INT32:
            return VtValue(*((int*)param->data()));
        case TypeDesc::UINT32:
            return VtValue(*((unsigned int*)param->data()));
        case TypeDesc::FLOAT:
            return VtValue(*((float*)param->data()));
        case TypeDesc::DOUBLE:
            return VtValue(*((double*)param->data()));
        }
        break;
    case TypeDesc::MATRIX44:
        switch (type.basetype) {
        case TypeDesc::FLOAT:
            // For compatibility with Ice/Imr read float matrix as double matrix
            if (convertMatrixTypes) {
                GfMatrix4d doubleMatrix(*((GfMatrix4f*)param->data()));
                return VtValue(doubleMatrix);
            } else {
                return VtValue(*((GfMatrix4f*)param->data()));
            }
        case TypeDesc::DOUBLE:
            return VtValue(*((GfMatrix4d*)param->data()));
        }
        break;
    }

    return VtValue();
}

static void
_SetAttribute(ImageSpec * spec,
              std::string const & metadataKey, VtValue const & value)
{
    bool convertMatrixTypes = false;
    std::string key = _TranslateMetadataKey(metadataKey, &convertMatrixTypes);

    if (value.IsHolding<std::string>()) {
        spec->attribute(key, TypeDesc(TypeDesc::STRING,
                                      TypeDesc::SCALAR),
                                      value.Get<std::string>().c_str());
    } else
    if (value.IsHolding<char>()) {
        spec->attribute(key, TypeDesc(TypeDesc::INT8,
                                      TypeDesc::SCALAR),
                                      &value.Get<char>());
    } else
    if (value.IsHolding<unsigned char>()) {
        spec->attribute(key, TypeDesc(TypeDesc::UINT8,
                                      TypeDesc::SCALAR),
                                      &value.Get<unsigned char>());
    } else
    if (value.IsHolding<int>()) {
        spec->attribute(key, TypeDesc(TypeDesc::INT32,
                                      TypeDesc::SCALAR),
                                      &value.Get<int>());
    } else
    if (value.IsHolding<unsigned int>()) {
        spec->attribute(key, TypeDesc(TypeDesc::UINT32,
                                      TypeDesc::SCALAR),
                                      &value.Get<unsigned int>());
    } else
    if (value.IsHolding<float>()) {
        spec->attribute(key, TypeDesc(TypeDesc::FLOAT,
                                      TypeDesc::SCALAR),
                                      &value.Get<float>());
    } else
    if (value.IsHolding<double>()) {
        spec->attribute(key, TypeDesc(TypeDesc::DOUBLE,
                                      TypeDesc::SCALAR),
                                      &value.Get<double>());
    } else
    if (value.IsHolding<GfMatrix4f>()) {
        spec->attribute(key, TypeDesc(TypeDesc::FLOAT,
                                      TypeDesc::MATRIX44),
                                      &value.Get<GfMatrix4f>());
    } else
    if (value.IsHolding<GfMatrix4d>()) {
        // For compatibility with Ice/Imr write double matrix as float matrix
        if (convertMatrixTypes) {
            GfMatrix4f floatMatrix(value.Get<GfMatrix4d>());
            spec->attribute(key, TypeDesc(TypeDesc::FLOAT,
                                          TypeDesc::MATRIX44),
                                          &floatMatrix);
        } else {
            spec->attribute(key, TypeDesc(TypeDesc::DOUBLE,
                                          TypeDesc::MATRIX44),
                                          &value.Get<GfMatrix4d>());
        }
    }
}

HioOIIO_Image::HioOIIO_Image()
    : _subimage(0), _miplevel(0)
{
}

/* virtual */
HioOIIO_Image::~HioOIIO_Image()
{
}

/* virtual */
std::string const &
HioOIIO_Image::GetFilename() const
{
    return _filename;
}

/* virtual */
int
HioOIIO_Image::GetWidth() const
{
    return _imagespec.width;
}

/* virtual */
int
HioOIIO_Image::GetHeight() const
{
    return _imagespec.height;
}

/* virtual */
HioFormat
HioOIIO_Image::GetFormat() const
{
    return _GetHioFormatFromImageData(_imagespec.nchannels, _imagespec.format,
                                      IsColorSpaceSRGB());
}

/* virtual */
int
HioOIIO_Image::GetBytesPerPixel() const
{
    return _imagespec.pixel_bytes();
}

/* virtual */
bool
HioOIIO_Image::IsColorSpaceSRGB() const
{
    if (_sourceColorSpace == HioImage::SRGB) {
        return true;
    } 
    if (_sourceColorSpace == HioImage::Raw) {
        return false;
    }

    return ((_imagespec.nchannels == 3 || _imagespec.nchannels == 4) &&
           _imagespec.format == TypeDesc::UINT8);

}

/* virtual */
bool
HioOIIO_Image::GetMetadata(TfToken const & key, VtValue * value) const
{
    VtValue result = _FindAttribute(_imagespec, key.GetString());
    if (!result.IsEmpty()) {
        *value = result;
        return true;
    }
    return false;
}

static HioAddressMode
_TranslateWrap(std::string const & wrapMode)
{
    if (wrapMode == "black")
        return HioAddressModeClampToBorderColor;
    if (wrapMode == "clamp")
        return HioAddressModeClampToEdge;
    if (wrapMode == "periodic")
        return HioAddressModeRepeat;
    if (wrapMode == "mirror")
        return HioAddressModeMirrorRepeat;

    return HioAddressModeClampToEdge;
}

/* virtual */
bool
HioOIIO_Image::GetSamplerMetadata(HioAddressDimension pname,
                                  HioAddressMode * param) const
{
    switch (pname) {
        case HioAddressDimensionU: {
                const VtValue smode = _FindAttribute(_imagespec, "s mode");
                if (!smode.IsEmpty() && smode.IsHolding<std::string>()) {
                    *param = _TranslateWrap(smode.Get<std::string>());
                    return true;
                }
            } return false;
        case HioAddressDimensionV: {
                const VtValue tmode = _FindAttribute(_imagespec, "t mode");
                if (!tmode.IsEmpty() && tmode.IsHolding<std::string>()) {
                    *param = _TranslateWrap(tmode.Get<std::string>());
                    return true;
                }
            } return false;
        default:
            return false;
    }
}

/* virtual */
int
HioOIIO_Image::GetNumMipLevels() const
{
    // XXX Add support for mip counting
    return 1;
}

std::string 
HioOIIO_Image::_GetFilenameExtension() const
{
    std::string fileExtension = ArGetResolver().GetExtension(_filename);
    return TfStringToLower(fileExtension);
}

#if OIIO_VERSION >= 20003
cspan<unsigned char>
HioOIIO_Image::_GenerateBufferCSpan(const std::shared_ptr<const char>& buffer,
                                    int bufferSize) const
{
    const char* bufferPtr = buffer.get(); 
    const unsigned char* bufferPtrUnsigned = (const unsigned char *) bufferPtr;
    cspan<unsigned char> bufferCSpan(bufferPtrUnsigned, bufferSize);
    return bufferCSpan;
}
#endif

bool
HioOIIO_Image::_CanUseIOProxyForExtension(std::string extension,
                                          const ImageSpec & config) const
{
    if (std::find(_ioProxySupportedExtensions->begin(), 
                  _ioProxySupportedExtensions->end(), 
                  extension)
            != _ioProxySupportedExtensions->end()) {
        return true;
    }
    std::string inputFilename("test.");
    inputFilename.append(extension);
    std::unique_ptr<ImageInput> imageInput(
        ImageInput::open(inputFilename, &config));

    if (!imageInput) {
        return false;
    }
    if (imageInput->supports("ioproxy")) {
        return true;
    }
    return false;
}

/* virtual */
bool
HioOIIO_Image::_OpenForReading(std::string const & filename, int subimage,
                               int mip, 
                               HioImage::SourceColorSpace sourceColorSpace,
                               bool suppressErrors)
{
    _filename = filename;
    _subimage = subimage;
    _miplevel = mip;    
    _sourceColorSpace = sourceColorSpace;
    _imagespec = ImageSpec();

#if OIIO_VERSION >= 20003
    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(
        ArResolvedPath(_filename));
    if (!asset) { 
        return false;
    }

    std::shared_ptr<const char> buffer = asset->GetBuffer();
    if (!buffer) {
        return false;
    }

    size_t bufferSize = asset->GetSize();

    Filesystem::IOMemReader memreader(_GenerateBufferCSpan(buffer, bufferSize));
    void *ptr = &memreader;
    ImageSpec config;
    config.attribute("oiio:ioproxy", TypeDesc::PTR, &ptr);

    std::string extension = _GetFilenameExtension();

    std::unique_ptr<ImageInput> imageInput;

    if (_CanUseIOProxyForExtension(extension, config)) {
        std::string inputFileName("in.");
        inputFileName.append(extension);
        imageInput = ImageInput::open(inputFileName, &config);
    }
    else {
        imageInput = ImageInput::open(_filename);
    }
#else
    std::unique_ptr<ImageInput> imageInput(ImageInput::open(_filename));
#endif

    if (!imageInput) {
        return false;
    }

    if (!imageInput->seek_subimage(subimage, mip, _imagespec)) {
        return false;
    }

    return true;
}

/* virtual */
bool
HioOIIO_Image::Read(StorageSpec const & storage)
{
    return ReadCropped(0, 0, 0, 0, storage);
}

/* virtual */
bool
HioOIIO_Image::ReadCropped(int const cropTop,
                           int const cropBottom,
                           int const cropLeft,
                           int const cropRight,
                           StorageSpec const & storage)
{

#if OIIO_VERSION >= 20003
    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(
        ArResolvedPath(_filename));
    if (!asset) { 
        return false;
    }

    std::shared_ptr<const char> buffer = asset->GetBuffer();
    if (!buffer) {
        return false;
    }

    size_t bufferSize = asset->GetSize();
    
    Filesystem::IOMemReader memreader(_GenerateBufferCSpan(buffer, bufferSize));
    void *ptr = &memreader;
    ImageSpec config;
    config.attribute("oiio:ioproxy", TypeDesc::PTR, &ptr);

    std::string extension = _GetFilenameExtension();

    std::unique_ptr<ImageInput> imageInput;

    if (_CanUseIOProxyForExtension(extension, config)) {
        std::string inputFileName("in.");
        inputFileName.append(extension);

        imageInput = ImageInput::open(inputFileName, &config);
    }
    else {
        imageInput = ImageInput::open(_filename);
    }

#else
    // read from file
    std::unique_ptr<ImageInput> imageInput(ImageInput::open(_filename));

#endif

    //// seek subimage
    ImageSpec spec = imageInput->spec();
    if (!imageInput->seek_subimage(_subimage, _miplevel, spec)){
        imageInput->close();
        TF_CODING_ERROR("Unable to seek subimage");
        return false;
    }
   
    int strideLength = imageInput->spec().width * 
                       imageInput->spec().pixel_bytes();
    int readStride = (storage.flipped)? 
                     (-strideLength) : (strideLength);
    int size = imageInput->spec().height * strideLength;

    std::unique_ptr<uint8_t[]>pixelData(new uint8_t[size]);
    unsigned char *pixels = pixelData.get();
    void *start = (storage.flipped)? 
                  (pixels + size - strideLength) : (pixels);

    // Read Image into pixels, flipping upon load so that
    // origin is at lower left corner
    // If needed, convert double precision images to float
    if (imageInput->spec().format == TypeDesc::DOUBLE) {
        imageInput->read_image(TypeDesc::FLOAT,
                               start,
                               AutoStride,
                               readStride,
                               AutoStride);
    } else{
        imageInput->read_image(imageInput->spec().format,
                         start,
                         AutoStride,
                         readStride,
                         AutoStride);
    }
    
    imageInput->close();
    
    // Construct ImageBuf that wraps around allocated pixels memory
    ImageBuf imagebuf =ImageBuf(imageInput->spec(), pixels);
    ImageBuf *image = &imagebuf;

    // Convert color images to linear (unless they are sRGB)
    // (Currently unimplemented, requires OpenColorIO support from OpenImageIO)

    // Crop 
    ImageBuf cropped;
    if (cropTop || cropBottom || cropLeft || cropRight) {
        ImageBufAlgo::cut(cropped, *image,
                ROI(cropLeft, image->spec().width - cropRight,
                    cropTop, image->spec().height - cropBottom));
        image = &cropped;
    }

    // Reformat
    ImageBuf scaled;
    if (image->spec().width != storage.width || 
        image->spec().height != storage.height) {
        ImageBufAlgo::resample(scaled, *image, /*interpolate=*/false,
                ROI(0, storage.width, 0, storage.height));
        image = &scaled;
    }

    // Read pixel data
    if (storage.format == HioFormatInvalid) {
        TF_CODING_ERROR("invalid storage format");
        return false;
    }
    TypeDesc type = _GetOIIOBaseType(storage.format);

#if OIIO_VERSION > 10603
    if (!image->get_pixels(ROI(0, storage.width, 0, storage.height, 0, 1),
                           type, storage.data)) {
#else
    if (!image->get_pixels(0, storage.width, 0, storage.height, 0, 1,
                           type, storage.data)) {
#endif
        TF_CODING_ERROR("unable to get_pixels");
        return false;
    }

    _imagespec = image->spec();

    return true;
}

/* virtual */
bool
HioOIIO_Image::_OpenForWriting(std::string const & filename)
{
    _filename = filename;
    _imagespec = ImageSpec();
    return true;
}

bool
HioOIIO_Image::Write(StorageSpec const & storage,
                     VtDictionary const & metadata)
{
    int nchannels = HioGetComponentCount(storage.format);
    TypeDesc format = _GetOIIOBaseType(storage.format);
    ImageSpec spec(storage.width, storage.height, nchannels, format);

    for (const std::pair<std::string, VtValue>& m : metadata) {
        _SetAttribute(&spec, m.first, m.second);
    }

    // Read from storage
    ImageBuf src(_filename, spec, storage.data);
    ImageBuf *image = &src;

    // Flip top-to-bottom
    ImageBuf flipped;
    if (storage.flipped) {
        ImageBufAlgo::flip(flipped, *image);
        image = &flipped;
    }

    // Write pixel data
    if (!image->write(_filename)) {
        TF_RUNTIME_ERROR("unable to write");
        image->clear();
        return false;
    }

    _imagespec = image->spec();

    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE

