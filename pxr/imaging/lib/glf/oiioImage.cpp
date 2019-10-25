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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/image.h"
#include "pxr/imaging/glf/utils.h"

#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolver.h"

// use gf types to read and write metadata
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/arch/pragmas.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/staticData.h"

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

class Glf_OIIOImage : public GlfImage {
public:
    typedef GlfImage Base;

    Glf_OIIOImage();

    virtual ~Glf_OIIOImage();

    // GlfImage overrides
    virtual std::string const & GetFilename() const;
    virtual int GetWidth() const;
    virtual int GetHeight() const;
    virtual GLenum GetFormat() const;
    virtual GLenum GetType() const;
    virtual int GetBytesPerPixel() const;
    virtual int GetNumMipLevels() const;

    virtual bool IsColorSpaceSRGB() const;

    virtual bool GetMetadata(TfToken const & key, VtValue * value) const;
    virtual bool GetSamplerMetadata(GLenum pname, VtValue * param) const;

    virtual bool Read(StorageSpec const & storage);
    virtual bool ReadCropped(int const cropTop,
                             int const cropBottom,
                             int const cropLeft,
                             int const cropRight,
                             StorageSpec const & storage);

    virtual bool Write(StorageSpec const & storage,
                       VtDictionary const & metadata);

protected:
    virtual bool _OpenForReading(std::string const & filename, int subimage,
                                 int mip, bool suppressErrors);
    virtual bool _OpenForWriting(std::string const & filename);

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
};

TF_REGISTRY_FUNCTION(TfType)
{
    typedef Glf_OIIOImage Image;
    TfType t = TfType::Define<Image, TfType::Bases<Image::Base> >();
    t.SetFactory< GlfImageFactory<Image> >();
}

static GLenum
_GLFormatFromImageData(unsigned int nchannels)
{
    return (nchannels == 1) ? GL_RED : ((nchannels == 4) ? GL_RGBA : GL_RGB);
}

/// Converts an OpenImageIO component type to its GL equivalent.
static GLenum
_GLTypeFromImageData(TypeDesc typedesc)
{
    switch (typedesc.basetype) {
    case TypeDesc::UINT:
        return GL_UNSIGNED_INT;
    case TypeDesc::HALF:
        return GL_HALF_FLOAT;
    case TypeDesc::FLOAT:
    case TypeDesc::DOUBLE:
        return GL_FLOAT;
    case TypeDesc::UINT8:
    default:
        return GL_UNSIGNED_BYTE;
    }    
}

/// Converts a GL type into its OpenImageIO component type equivalent.
static TypeDesc
_GetOIIOBaseType(GLenum type)
{
    switch (type) {
    case GL_UNSIGNED_BYTE:
    case GL_BYTE:
        return TypeDesc::UINT8;
    case GL_UNSIGNED_INT:
    case GL_INT:
        return TypeDesc::UINT;
    case GL_HALF_FLOAT:
        return TypeDesc::HALF;
    case GL_FLOAT:
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

Glf_OIIOImage::Glf_OIIOImage()
    : _subimage(0), _miplevel(0)
{
}

/* virtual */
Glf_OIIOImage::~Glf_OIIOImage()
{
}

/* virtual */
std::string const &
Glf_OIIOImage::GetFilename() const
{
    return _filename;
}

/* virtual */
int
Glf_OIIOImage::GetWidth() const
{
    return _imagespec.width;
}

/* virtual */
int
Glf_OIIOImage::GetHeight() const
{
    return _imagespec.height;
}

/* virtual */
GLenum
Glf_OIIOImage::GetFormat() const
{
    return _GLFormatFromImageData(_imagespec.nchannels);
}

/* virtual */
GLenum
Glf_OIIOImage::GetType() const
{
    return _GLTypeFromImageData(_imagespec.format);
}

/* virtual */
int
Glf_OIIOImage::GetBytesPerPixel() const
{
    return _imagespec.pixel_bytes();
}

/* virtual */
bool
Glf_OIIOImage::IsColorSpaceSRGB() const
{
    return ((_imagespec.nchannels == 3  ||
             _imagespec.nchannels == 4) &&
            _imagespec.format == TypeDesc::UINT8);
}

/* virtual */
bool
Glf_OIIOImage::GetMetadata(TfToken const & key, VtValue * value) const
{
    VtValue result = _FindAttribute(_imagespec, key.GetString());
    if (!result.IsEmpty()) {
        *value = result;
        return true;
    }
    return false;
}

static GLenum
_TranslateWrap(std::string const & wrapMode)
{
    if (wrapMode == "black")
        return GL_CLAMP_TO_BORDER;
    if (wrapMode == "clamp")
        return GL_CLAMP_TO_EDGE;
    if (wrapMode == "periodic")
        return GL_REPEAT;
    if (wrapMode == "mirror")
        return GL_MIRRORED_REPEAT;

    return GL_CLAMP_TO_EDGE;
}

/* virtual */
bool
Glf_OIIOImage::GetSamplerMetadata(GLenum pname, VtValue * param) const
{
    switch (pname) {
        case GL_TEXTURE_WRAP_S: {
                VtValue smode = _FindAttribute(_imagespec, "s mode");
                if (!smode.IsEmpty() && smode.IsHolding<std::string>()) {
                    *param = VtValue(_TranslateWrap(smode.Get<std::string>()));
                    return true;
                }
            } return false;
        case GL_TEXTURE_WRAP_T: {
                VtValue tmode = _FindAttribute(_imagespec, "t mode");
                if (!tmode.IsEmpty() && tmode.IsHolding<std::string>()) {
                    *param = VtValue(_TranslateWrap(tmode.Get<std::string>()));
                    return true;
                }
            } return false;
        default:
            return false;
    }
}

/* virtual */
int
Glf_OIIOImage::GetNumMipLevels() const
{
    // XXX Add support for mip counting
    return 1;
}

std::string 
Glf_OIIOImage::_GetFilenameExtension() const
{
    std::string fileExtension = ArGetResolver().GetExtension(_filename);
    return TfStringToLower(fileExtension);
}

#if OIIO_VERSION >= 20003
cspan<unsigned char>
Glf_OIIOImage::_GenerateBufferCSpan(const std::shared_ptr<const char>& buffer, 
                                    int bufferSize) const
{
    const char* bufferPtr = buffer.get(); 
    const unsigned char* bufferPtrUnsigned = (const unsigned char *) bufferPtr;
    cspan<unsigned char> bufferCSpan(bufferPtrUnsigned, bufferSize);
    return bufferCSpan;
}
#endif

bool
Glf_OIIOImage::_CanUseIOProxyForExtension(std::string extension, 
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
Glf_OIIOImage::_OpenForReading(std::string const & filename, int subimage,
                               int mip, bool suppressErrors)
{
    _filename = filename;
    _subimage = subimage;
    _miplevel = mip;    
    _imagespec = ImageSpec();

#if OIIO_VERSION >= 20003
    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(_filename);
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
Glf_OIIOImage::Read(StorageSpec const & storage)
{
    return ReadCropped(0, 0, 0, 0, storage);
}

/* virtual */
bool
Glf_OIIOImage::ReadCropped(int const cropTop,
                           int const cropBottom,
                           int const cropLeft,
                           int const cropRight,
                           StorageSpec const & storage)
{

#if OIIO_VERSION >= 20003
    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(_filename);
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
   
    int strideLength = imageInput->spec().width * GetBytesPerPixel();
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
    TypeDesc type = _GetOIIOBaseType(storage.type);

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
Glf_OIIOImage::_OpenForWriting(std::string const & filename)
{
    _filename = filename;
    _imagespec = ImageSpec();
    return true;
}

bool
Glf_OIIOImage::Write(StorageSpec const & storage,
                     VtDictionary const & metadata)
{
    int nchannels = GlfGetNumElements(storage.format);
    TypeDesc format = _GetOIIOBaseType(storage.type);
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

