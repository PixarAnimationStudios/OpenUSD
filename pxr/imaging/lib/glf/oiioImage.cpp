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

// use gf types to read and write metadata
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/arch/pragmas.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

ARCH_PRAGMA_PUSH
ARCH_PRAGMA_MACRO_REDEFINITION // due to Python copysign
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
ARCH_PRAGMA_POP

PXR_NAMESPACE_OPEN_SCOPE


OIIO_NAMESPACE_USING

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
    virtual bool _OpenForReading(std::string const & filename, int subimage);
    virtual bool _OpenForWriting(std::string const & filename);

private:
    std::string _filename;
    int _subimage;
    ImageBuf _imagebuf;
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
    : _subimage(0)
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
    return _imagebuf.spec().width;
}

/* virtual */
int
Glf_OIIOImage::GetHeight() const
{
    return _imagebuf.spec().height;
}

/* virtual */
GLenum
Glf_OIIOImage::GetFormat() const
{
    return _GLFormatFromImageData(_imagebuf.spec().nchannels);
}

/* virtual */
GLenum
Glf_OIIOImage::GetType() const
{
    return _GLTypeFromImageData(_imagebuf.spec().format);
}

/* virtual */
int
Glf_OIIOImage::GetBytesPerPixel() const
{
    return _imagebuf.spec().pixel_bytes();
}

/* virtual */
bool
Glf_OIIOImage::IsColorSpaceSRGB() const
{
    return ((_imagebuf.spec().nchannels == 3  ||
             _imagebuf.spec().nchannels == 4) &&
            _imagebuf.spec().format == TypeDesc::UINT8);
}

/* virtual */
bool
Glf_OIIOImage::GetMetadata(TfToken const & key, VtValue * value) const
{
    VtValue result = _FindAttribute(_imagebuf.spec(), key.GetString());
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
                VtValue smode = _FindAttribute(_imagebuf.spec(), "s mode");
                if (!smode.IsEmpty() && smode.IsHolding<std::string>()) {
                    *param = VtValue(_TranslateWrap(smode.Get<std::string>()));
                    return true;
                }
            } return false;
        case GL_TEXTURE_WRAP_T: {
                VtValue tmode = _FindAttribute(_imagebuf.spec(), "t mode");
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

/* virtual */
bool
Glf_OIIOImage::_OpenForReading(std::string const & filename, int subimage)
{
    _filename = filename;
    _subimage = subimage;
    _imagebuf.clear();
    return _imagebuf.init_spec(_filename, subimage, /*mipmap*/0)
           && (_imagebuf.nsubimages() > subimage);
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
    // read from file
    ImageBuf * image = &_imagebuf;

    // Convert double precision images to float
    if (image->spec().format == TypeDesc::DOUBLE) {
        if (!image->read(_subimage, /*miplevel*/0, /*force*/false,
                            TypeDesc::FLOAT)) {
            TF_CODING_ERROR("unable to read image (as float)");
            return false;
        }
    } else {
        if (!image->read(_subimage)) {
            TF_CODING_ERROR("unable to read image");
            return false;
        }
    }

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

//XXX:
//'OpenImageIO::v1_7::ImageBuf::get_pixels': Use get_pixels(ROI, ...) instead. [1.6] 
ARCH_PRAGMA_PUSH
ARCH_PRAGMA_DEPRECATED_POSIX_NAME

    // Read pixel data
    TypeDesc type = _GetOIIOBaseType(storage.type);
    if (!image->get_pixels(0, storage.width, 0, storage.height, 0, 1,
                              type, storage.data)) {
        TF_CODING_ERROR("unable to get_pixels");
        return false;
    }

ARCH_PRAGMA_POP

    if (image != &_imagebuf) {
        _imagebuf.swap(*image);
    }
    return true;
}

/* virtual */
bool
Glf_OIIOImage::_OpenForWriting(std::string const & filename)
{
    _filename = filename;
    _imagebuf.clear();
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

    if (image != &_imagebuf) {
        _imagebuf.swap(*image);
    }
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE

