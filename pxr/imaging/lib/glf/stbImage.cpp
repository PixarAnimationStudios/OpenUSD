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

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

PXR_NAMESPACE_OPEN_SCOPE


class Glf_StbImage : public GlfImage {
public:
    typedef GlfImage Base;

    Glf_StbImage();

    virtual ~Glf_StbImage();

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
                                 bool suppressErrors);
    virtual bool _OpenForWriting(std::string const & filename);

private:
    std::string _GetFilenameExtension();
    bool _IsValidCrop(int cropTop, int cropBottom, int cropLeft, int cropRight);
    void _GetInfoFromStorageSpec(GlfImage::StorageSpec const & storage);
    bool _CropAndResize(void const *sourceData, int const cropTop,
                   int const cropBottom,
                   int const cropLeft,
                   int const cropRight,
                   bool resizeNeeded,
                   StorageSpec const & storage);
        
    std::string _filename;
    int _subimage;
    int _width;
    int _height;
    
    //GL_UNSIGNED_BYTE, GL_FLOAT
    GLenum _outputType; 
    
    int _nchannels;
};

TF_REGISTRY_FUNCTION(TfType)
{
    typedef Glf_StbImage Image;
    TfType t = TfType::Define<Image, TfType::Bases<Image::Base> >();
    t.SetFactory< GlfImageFactory<Image> >();
}

static GLenum
_GLFormatFromImageData(unsigned int nchannels)
{
    return (nchannels == 1) ? GL_RED : ((nchannels == 4) ? GL_RGBA : GL_RGB);
}

/// Returns the bpc (bits per channel) based on the GLType stored in storage
static int
_GetBytesPerChannelFromType(GLenum const & type)
{
    switch(type) {
    case GL_UNSIGNED_BYTE:
        return 1;
    case GL_FLOAT:
        return 4;
    default:
        TF_CODING_ERROR("Unsupported type");
        return 4;
    }
}

static int 
_GetNumChannelsFromGLFormat(GLenum const & format)
{
    switch(format) {
    case GL_RED :
        return 1;
    case GL_RGB :
        return 3;
    case GL_RGBA :
        return 4;
    default:
        TF_CODING_ERROR("Unsupported format");
        return 4;
    }
}

bool 
Glf_StbImage::_IsValidCrop(int cropTop, int cropBottom, int cropLeft, int cropRight)
{
    int cropImageWidth = _width - (cropLeft + cropRight);
    int cropImageHeight = _height - (cropTop + cropBottom);
    return (cropTop >= 0 &&
            cropBottom >= 0 &&
            cropLeft >= 0 &&
            cropRight >= 0 &&
            cropImageWidth > 0 &&
            cropImageHeight > 0);
}

std::string 
Glf_StbImage::_GetFilenameExtension()
{
    std::string fileExtension = ArGetResolver().GetExtension(_filename);
    //convert to lowercase
    transform(fileExtension.begin(), 
              fileExtension.end(), 
              fileExtension.begin(), ::tolower);
    return fileExtension;
}

void
Glf_StbImage::_GetInfoFromStorageSpec(GlfImage::StorageSpec const & storage)
{
    _width = storage.width;
    _height = storage.height;
    _outputType = storage.type;
    _nchannels = _GetNumChannelsFromGLFormat(storage.format);
}

Glf_StbImage::Glf_StbImage()
    : _subimage(0)
{
}

/// Copies the region of the source image defined by cropTop, cropBottom,
/// cropLeft, and cropRight into storage.data.  If needed, we resize
/// the incoming data to fit the dimensions defined in storage.  _width
/// and _height are updated to match those in storage
bool 
Glf_StbImage::_CropAndResize(void const *sourceData, int const cropTop,
        int const cropBottom,
        int const cropLeft,
        int const cropRight,
        bool resizeNeeded,
        StorageSpec const & storage)
{
    if (!TF_VERIFY(_IsValidCrop(cropTop, cropBottom, cropLeft, cropRight),
        "Invalid crop parameters")) {
        return false;
    }
    int bpp = GetBytesPerPixel();
    
    int cropWidth = _width - cropRight - cropLeft;
    int cropHeight = _height - cropTop - cropBottom;
    int croppedStrideLength = cropWidth * bpp;

    int strideLength = _width * bpp; 

    
    // set destination
    // if resizing is needed, then copy into temporary memory
    // otherwise, copy straight into storage.data
    void*croppedData;
    std::unique_ptr<uint8_t[]>tempData;

    if (resizeNeeded) {
        int croppedImageSize = croppedStrideLength * cropHeight; 
        tempData.reset(new uint8_t[croppedImageSize]);
        croppedData = tempData.get();
    } else {
        croppedData = storage.data;
    }
    
    for (int i = 0; i < cropHeight; i ++)
    {
        unsigned char *src = (unsigned char*) sourceData + 
                             ((cropTop + i) * strideLength) + 
                             (cropLeft * bpp);
        unsigned char *dest = (unsigned char *) croppedData + 
                              (i * croppedStrideLength);

        //memcpy 1 row of data
        memcpy(dest, src, croppedStrideLength);
    }

    if (resizeNeeded) {
        //resize and copy data into storage
        if (IsColorSpaceSRGB()) {
            int alphaIndex = (_nchannels == 3)?
                                 STBIR_ALPHA_CHANNEL_NONE : 3;
            stbir_resize_uint8_srgb((unsigned char *) croppedData, 
                                    cropWidth, cropHeight, 
                                    croppedStrideLength,
                                    (unsigned char *)storage.data, 
                                    storage.width,
                                    storage.height,
                                    storage.width * bpp,
                                    _nchannels, alphaIndex, 0);
        } else {
            if (_outputType == GL_FLOAT) {
                    stbir_resize_float((float *) croppedData, 
                                       cropWidth, cropHeight, 
                                       croppedStrideLength,
                                       (float *)storage.data, 
                                       storage.width,
                                       storage.height,
                                       storage.width * bpp,
                                       _nchannels);
            } else {
                stbir_resize_uint8((unsigned char *) croppedData, 
                                   cropWidth, cropHeight,
                                   croppedStrideLength,
                                   (unsigned char *)storage.data,
                                   storage.width,
                                   storage.height,
                                   storage.width * bpp,
                                   _nchannels);
            }
        }
    }
    _width = storage.width;
    _height = storage.height;
    return true;
}

/* virtual */
Glf_StbImage::~Glf_StbImage()
{
}

/* virtual */
std::string const &
Glf_StbImage::GetFilename() const
{
    return _filename;
}


/* virtual */
int
Glf_StbImage::GetWidth() const
{
    return _width;
}

/* virtual */
int
Glf_StbImage::GetHeight() const
{
    return _height;
}

/* virtual */
GLenum
Glf_StbImage::GetFormat() const
{
    return _GLFormatFromImageData(_nchannels);
}

/* virtual */
GLenum
Glf_StbImage::GetType() const
{
    return _outputType;
}

/* virtual */
int
Glf_StbImage::GetBytesPerPixel() const
{
    return _GetBytesPerChannelFromType(_outputType) * _nchannels;
}

/* virtual */
bool
Glf_StbImage::IsColorSpaceSRGB() const
{
    return ((_nchannels == 3  || _nchannels == 4) && 
            GetType() == GL_UNSIGNED_BYTE);
}


//XXX Still need to investigate Metadata handling
/* virtual */
bool
Glf_StbImage::GetMetadata(TfToken const & key, VtValue * value) const
{
    return false;
}

/* virtual */
bool
Glf_StbImage::GetSamplerMetadata(GLenum pname, VtValue * param) const
{
    return false;
}

/* virtual */
int
Glf_StbImage::GetNumMipLevels() const
{
    return 1;
}

/* virtual */
bool
Glf_StbImage::_OpenForReading(std::string const & filename, int subimage,
                               bool suppressErrors)
{
    _filename = filename;
    _subimage = subimage;
   
    std::string fileExtension = _GetFilenameExtension();
    if (fileExtension == "hdr") {
        _outputType = GL_FLOAT;
    } else {
        _outputType = GL_UNSIGNED_BYTE;
    }

    //read the header file to obtain width, height, and bpp info
    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(_filename);
    if (!asset) { 
        return false;
    }

    std::shared_ptr<const char> buffer = asset->GetBuffer();
    if (!buffer) {
        return false;
    }

    return stbi_info_from_memory(
        reinterpret_cast<stbi_uc const*>(buffer.get()), asset->GetSize(), 
        &_width, &_height, &_nchannels);
}

/* virtual */
bool
Glf_StbImage::Read(StorageSpec const & storage)
{
    return ReadCropped(0, 0, 0, 0, storage);
}

/* virtual */
/// Reads the image named _filename into storage.  If needed, the image is
/// cropped and/or resized.  The _width and _height are updated to match 
/// storage.width and storage.height
bool
Glf_StbImage::ReadCropped(int const cropTop,
                           int const cropBottom,
                           int const cropLeft,
                           int const cropRight,
                           StorageSpec const & storage)
{
    // read from file
    // NOTE: stbi_load will always return image data as a contiguous block 
    //       of memory for every image format (i.e. image data is packed 
    //       contiguously)

    //Read based on the storage type (8-bit, 16-bit, or float)
    void* imageData = NULL;
  
    //If image needs to be flipped, configure stb to flip image after load
    if (storage.flipped) {
        stbi_set_flip_vertically_on_load(true);
    } else {
        stbi_set_flip_vertically_on_load(false);
    }

    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(_filename);
    if (!asset) 
    {
        TF_CODING_ERROR("Cannot open image %s for reading", _filename.c_str());
        return false;
    }

    std::shared_ptr<const char> buffer = asset->GetBuffer();
    if (buffer) {
        size_t bufferSize = asset->GetSize();
        if (_outputType == GL_FLOAT) {
            imageData = stbi_loadf_from_memory(
                reinterpret_cast<stbi_uc const *>(buffer.get()), bufferSize,
                &_width, &_height, &_nchannels, 0);
        }
        else {
            imageData = stbi_load_from_memory(
                reinterpret_cast<stbi_uc const *>(buffer.get()), bufferSize,
                &_width, &_height, &_nchannels, 0);
        }
    }

    //// Read pixel data
    if (!imageData)
    {
        TF_CODING_ERROR("unable to get_pixels");
        return false;
    }
   
    //// Crop Needed
    if (cropTop || cropBottom || cropLeft || cropRight)
    {
        //check if resizing is still necessary after cropping
        bool resizeNeeded = (_width - cropRight - cropLeft != storage.width ) || 
                            (_height - cropTop - cropBottom != storage.height);
        
        // copy (and potentially resize)
        // cropped region of imageData into storage.data
        if (!_CropAndResize(imageData, cropTop, cropBottom, cropLeft, cropRight, 
              resizeNeeded, storage))
        {
            TF_CODING_ERROR("Unable to crop and resize");
            stbi_image_free(imageData);
            return false;
        }

    }
    else
    {
        int bpp = GetBytesPerPixel();
        int inputStrideInBytes = _width * bpp; 
        bool resizeNeeded = _width != storage.width || _height != storage.height;
        
        if (resizeNeeded) {
            if (IsColorSpaceSRGB()) {
                int alphaIndex = (_nchannels == 3)?
                                 STBIR_ALPHA_CHANNEL_NONE : 3;
                stbir_resize_uint8_srgb((unsigned char*)imageData, 
                                        _width, 
                                        _height, 
                                        inputStrideInBytes,
                                        (unsigned char *)storage.data, 
                                        storage.width,
                                        storage.height,
                                        storage.width * bpp,
                                        _nchannels, alphaIndex, 0);
            } else {
                if (_outputType == GL_FLOAT) {
                    stbir_resize_float((float *)imageData, 
                                       _width, 
                                       _height,
                                       inputStrideInBytes,
                                       (float *)storage.data,
                                       storage.width,
                                       storage.height,
                                       storage.width * bpp,
                                       _nchannels);
                } else {
                    stbir_resize_uint8((unsigned char *)imageData, 
                                       _width, 
                                       _height,
                                       inputStrideInBytes,
                                       (unsigned char *)storage.data,
                                       storage.width,
                                       storage.height,
                                       storage.width * bpp,
                                       _nchannels);
                }
            }

            _width = storage.width;
            _height = storage.height;
        }
        else
        {
            int imageSize = bpp * _width * _height;
            //no resizing needed, just copy image data to storage
            memcpy(storage.data, imageData, imageSize);
        }
    }

    if (!storage.data)
    {
        TF_CODING_ERROR("Failed to copy data to storage.data");
    }

    stbi_image_free(imageData);
    return true;
}

/* virtual */
bool
Glf_StbImage::_OpenForWriting(std::string const & filename)
{
    _filename = filename;
    // think about whether or not we need to clear the data ptr
    return true;
}

namespace
{

uint8_t
_Quantize(float value)
{
    static const int min = 0;
    static const int max = std::numeric_limits<uint8_t>::max();

    int result = min + std::floor((max - min) * value + 0.499999f);
    return std::min(max, std::max(min, result));
}


} // end anonymous namespace

/// Writes image data stored in storage.data to a file (specified by _filename
/// during _OpenForWriting).  Valid file types are jpg, png, bmp, tga, and hdr.
/// Expects data to be of type GL_FLOAT when writing
/// hdr files, otherwise, expects data to be of type GL_UNSIGNED_BYTE.
/// An error occurs if the type does not match the expected type for the given
/// file type.
/* virtual */
bool
Glf_StbImage::Write(StorageSpec const & storageIn,
                     VtDictionary const & metadata)
{
    const std::string fileExtension = _GetFilenameExtension();

    StorageSpec quantizedSpec;
    std::unique_ptr<uint8_t[]> quantizedData;
    if (storageIn.type == GL_FLOAT && fileExtension != "hdr") 
    {
        // stb requires unsigned byte data to write non .hdr file formats.
        // We'll quantize the data ourselves here.
        size_t numElements = 
            storageIn.width * storageIn.height *
            _GetNumChannelsFromGLFormat(storageIn.format);

        quantizedData.reset(new uint8_t[numElements]);

        const float* inData = static_cast<float*>(storageIn.data);
        for (size_t i = 0; i < numElements; ++i) {
            quantizedData[i] = _Quantize(inData[i]);
        }

        quantizedSpec = storageIn; // shallow copy
        quantizedSpec.data = quantizedData.get();
        quantizedSpec.type = GL_UNSIGNED_BYTE;
    }
    else if (storageIn.type != GL_UNSIGNED_BYTE && fileExtension != "hdr") {
        TF_CODING_ERROR("stb expects unsigned byte data to write filetype %s",
                        fileExtension.c_str());
        return false;
    }
    else if (storageIn.type != GL_FLOAT && fileExtension == "hdr") 
    {
        TF_CODING_ERROR("stb expects linear float data to write filetype hdr");
        return false;
    }

    StorageSpec const& storage = quantizedData ? quantizedSpec : storageIn;

    //set info to match storage
    _GetInfoFromStorageSpec(storage); 

    //Again, how to store metadata?
    //for (const std::pair<std::string, VtValue>& m : metadata) {
    //    _SetAttribute(&spec, m.first, m.second);
    //}

    //configure to flip vertically
    if (storage.flipped) stbi_flip_vertically_on_write(true);
    else  stbi_flip_vertically_on_write(false);

    // Read from storage.data and write pixel data to file
    int success = 0;
   
    static const std::vector<std::string> jpgExtensions = {"jpg", "jpeg", 
                                                           "jpe", "jfif", 
                                                           "jfi", "jif"};
    static const std::vector<std::string> tgaExtensions = {"tga", "icb", 
                                                           "vda", "vst"}; 
    if (std::find(jpgExtensions.begin(), jpgExtensions.end(), fileExtension) !=
        jpgExtensions.end())
    {
        success = 
            stbi_write_jpg(_filename.c_str(), _width, _height, 
                           _nchannels, storage.data, 100);
    } 
    else if (fileExtension.compare("png") == 0)
    {
        //Assuming pixel data is packed consecutively in memory
        //Thus stride length = image width * bytes per pixel
        success = stbi_write_png(_filename.c_str(), _width, _height, 
                                 _nchannels, storage.data, 
                                 _width * GetBytesPerPixel());
    } 
    else if (fileExtension.compare("bmp") == 0 ||
               fileExtension.compare("dib") == 0)  
    {
        success = stbi_write_bmp(_filename.c_str(), _width, _height, 
                                 _nchannels, storage.data);
    } 
    else if (std::find(tgaExtensions.begin(), tgaExtensions.end(), 
             fileExtension) != tgaExtensions.end())
    {
        success = stbi_write_tga(_filename.c_str(), _width, _height, 
                                 _nchannels, storage.data);
    } 
    else if (fileExtension.compare("hdr") == 0) {
        success = stbi_write_hdr(_filename.c_str(), _width, _height, 
                                 _nchannels, (float *)storage.data);
    }
    
    if (!success) {
        TF_RUNTIME_ERROR("Unable to write");
        return false;
    }

    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE

