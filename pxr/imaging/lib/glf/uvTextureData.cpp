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
#include "pxr/imaging/glf/uvTextureData.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tracelite/trace.h"

GlfUVTextureDataRefPtr
GlfUVTextureData::New(
    std::string const &filePath,
    size_t targetMemory,
    unsigned int cropTop,
    unsigned int cropBottom,
    unsigned int cropLeft,
    unsigned int cropRight)
{
    GlfUVTextureData::Params params;
    params.targetMemory = targetMemory;
    params.cropTop      = cropTop;
    params.cropBottom   = cropBottom;
    params.cropLeft     = cropLeft;
    params.cropRight    = cropRight;
    return New(filePath, params);
}

GlfUVTextureDataRefPtr
GlfUVTextureData::New(std::string const &filePath, Params const &params)
{
    return TfCreateRefPtr(new GlfUVTextureData(filePath, params));
}

GlfUVTextureData::~GlfUVTextureData()
{
    if (_rawBuffer) {
        delete [] _rawBuffer;
	_rawBuffer = 0;
    }
}

GlfUVTextureData::GlfUVTextureData(std::string const &filePath,
                                     Params const &params)
  : _filePath(filePath),
    _params(params),
    _targetMemory(0),
    _nativeWidth(0), _nativeHeight(0),
    _resizedWidth(0), _resizedHeight(0),
    _bytesPerPixel(0),
    _glInternalFormat(GL_RGB),
    _glFormat(GL_RGB),
    _glType(GL_UNSIGNED_BYTE),
    _size(0),
    _rawBuffer(0)
{
    /* nothing */
}

// Compute required GPU memory
size_t
GlfUVTextureData_ComputeMemory(GlfImageSharedPtr const &img,
                                bool generateMipmap)
{
    // Mipmapping on GPU means we need an
    // extra 1/4 + 1/16 + 1/64 + 1/256 + ... of memory
    const double scale = generateMipmap ? 4.0 / 3 : 1.0;

    if (GlfIsCompressedFormat(img->GetFormat())) {
         return scale * GlfGetCompressedTextureSize(img->GetWidth(), 
                            img->GetHeight(), img->GetFormat(), img->GetType());
    }

    const size_t numPixels = img->GetWidth() * img->GetHeight();
    return scale * numPixels * img->GetBytesPerPixel();
}

GlfUVTextureData::_DegradedImageInput
GlfUVTextureData::_ReadDegradedImageInput(bool generateMipmap,
                                           size_t targetMemory,
                                           size_t degradeLevel)
{
    // Read the header of the image (no subimageIndex given, so at full
    // resolutin when evaluated).
    const GlfImageSharedPtr fullImage = GlfImage::OpenForReading(_filePath);
    const GlfImageSharedPtr nullImage;

    // Bail if image file could not be opened.
    if (not fullImage) {
        return _DegradedImageInput(1.0, 1.0, nullImage);
    }

    // Return full resolution if the targetMemory and the degradeLevel are not
    // set, i.e., equal to 0.
    if (not (targetMemory > 0 or degradeLevel > 0)) {
        return _DegradedImageInput(1.0, 1.0, fullImage);
    }

    // Compute the estimate required memory at full resolution.
    const size_t fullSize = GlfUVTextureData_ComputeMemory(fullImage,
                                                            generateMipmap);

    // If targetMemory is set and more than required for full resolution,
    // return full resolution.
    if (targetMemory > 0 and fullSize <= targetMemory) {
        return _DegradedImageInput(1.0, 1.0, fullImage);
    }
    
    // If no targetMemory set, use degradeLevel to determine mipLevel
    if (targetMemory == 0) {
        GlfImageSharedPtr image =
		GlfImage::OpenForReading(_filePath, degradeLevel);
        if (not image) {
            return _DegradedImageInput(1.0, 1.0, nullImage);
        }
        return _DegradedImageInput(
            double(image->GetWidth()) / fullImage->GetWidth(),
            double(image->GetHeight()) / fullImage->GetHeight(),
            image);
    }

    // We actually have an image requiring more memory than targetMemory.
    // Iterate through the levels of down-sampled images until either:
    // - The required memory is less or equal to targetMemory
    // - There are no more down-sampled images
    // - An iteration limit (32) has been reached

    // Remember the previous image and size to detect that there are no more
    // down-sampled images
    GlfImageSharedPtr prevImage = fullImage;
    size_t prevSize = fullSize;

    for (int i = 1; i < 32; i++) {

        // Open the image and is requested to use the i-th
        // down-sampled image (mipLevel).
        GlfImageSharedPtr image = GlfImage::OpenForReading(_filePath, i);

        // If mipLevel could not be opened, return fullImage. We are
        // not supposed to hit this. GlfImage will return the last
        // down-sampled image if the subimageIndex is beyond the range.
        if (not image) {
            return _DegradedImageInput(1.0, 1.0, fullImage);
        }

        // Compute the size at the down-sampled resolution.
        size_t size = GlfUVTextureData_ComputeMemory(image,
                                                     generateMipmap);
        if (size <= targetMemory) {
            // We found an image with small enough memory requirement,
            // return it.
            return _DegradedImageInput(
                double(image->GetWidth()) / fullImage->GetWidth(),
                double(image->GetHeight()) / fullImage->GetHeight(),
                image);
        }
        
        if (not (size < prevSize)) {
            // GlfImage stopped providing more further-downsampled
            // images, no point to continue, return image from last
            // iteration.
            return _DegradedImageInput(
                double(prevImage->GetWidth()) / fullImage->GetWidth(),
                double(prevImage->GetHeight()) / fullImage->GetHeight(),
                prevImage);
        }
        
        // Set values to see try to fetch the next downsampled images.
        prevImage = image;
        prevSize = size;
    }

    // Iteration limit reached, return image from last iteration.
    return _DegradedImageInput(
        double(prevImage->GetWidth()) / fullImage->GetWidth(),
        double(prevImage->GetHeight()) / fullImage->GetHeight(),
        prevImage);
}

bool
GlfUVTextureData::Read(int degradeLevel, bool generateMipmap)
{   
    TRACE_FUNCTION();

    if (not TfPathExists(_filePath)) {
        TF_CODING_ERROR("Unable to find Texture '%s'.", _filePath.c_str());
        return false;
    }

    // Read the image from a file, if possible and necessary, a down-sampled
    // version
    const _DegradedImageInput degradedImage = _ReadDegradedImageInput(
        generateMipmap, _params.targetMemory, degradeLevel);

    GlfImageSharedPtr image = degradedImage.image;

    if (not image) {
        TF_CODING_ERROR("Unable to load Texture '%s'.", _filePath.c_str());
        return false;
    }

    int imageWidth = image->GetWidth();
    int imageHeight = image->GetHeight();

    int cropTop    = _params.cropTop;
    int cropBottom = _params.cropBottom;
    int cropLeft   = _params.cropLeft;
    int cropRight  = _params.cropRight;

    // Check if the format of the image is compressed
    bool isCompressed = GlfIsCompressedFormat(image->GetFormat());

    if (cropTop or cropBottom or cropLeft or cropRight) {

        TRACE_SCOPE("GlfUVTextureData::Read(int, bool) (cropping)");

        if (isCompressed) {
            TF_CODING_ERROR("Compressed images can not be cropped '%s'.", 
                             _filePath.c_str());
            return false;
         }

        // The cropping parameters are with respect to the original image,
        // we need to scale them if we have a down-sampled image.
        // Usually, we crop the slates that are black and the boundary might
        // not hit a pixel boundary of the down-sampled image and thus black
        // bleeds into the pixels near the border of the texture. To avoid
        // this, we use ceil here to cut out the pixels with black bleeding.
        cropTop    = ceil(_params.cropTop * degradedImage.scaleY);
        cropBottom = ceil(_params.cropBottom * degradedImage.scaleY);
        cropLeft   = ceil(_params.cropLeft * degradedImage.scaleX);
        cropRight  = ceil(_params.cropRight * degradedImage.scaleX);

        imageWidth = std::max(0, imageWidth - (cropLeft + cropRight));
        imageHeight = std::max(0, imageHeight - (cropTop + cropBottom));
    }

    _targetMemory = _params.targetMemory;
    _nativeWidth = _resizedWidth = imageWidth;
    _nativeHeight = _resizedHeight = imageHeight;
    _glFormat = image->GetFormat();
    _glType = image->GetType();
    
    // When using compressed formats the bytesPerPixel is not 
    // used and the glFormat matches the glInternalFormat.
    if (isCompressed) {
        _bytesPerPixel = image->GetBytesPerPixel(); 
        _glInternalFormat = _glFormat;
    } else { 
        _bytesPerPixel = GlfGetNumElements(_glFormat) * 
                         GlfGetElementSize(_glType);
        _glInternalFormat = _GLInternalFormatFromImageData(
                                _glFormat, _glType, image->IsColorSpaceSRGB());
    }

    // Extract wrap modes from image metadata.
    _wrapInfo.hasWrapModeS =
        image->GetSamplerMetadata(GL_TEXTURE_WRAP_S, &_wrapInfo.wrapModeS);
    _wrapInfo.hasWrapModeT =
        image->GetSamplerMetadata(GL_TEXTURE_WRAP_T, &_wrapInfo.wrapModeT);

    const double scale = generateMipmap ? 4.0 / 3 : 1.0;

    _size = _nativeWidth * _nativeHeight * _bytesPerPixel * scale;

    int imageSize = 0;
    if (isCompressed) {
        TF_VERIFY((_nativeWidth == _resizedWidth) &&
                  (_nativeHeight == _resizedHeight));

        int compressedSize = GlfGetCompressedTextureSize(_nativeWidth, 
                                            _nativeHeight, _glFormat, _glType);
        _size =  compressedSize * scale;
        imageSize = compressedSize;
    } else {
        while (_targetMemory > 0 && (_size > _targetMemory)) {
            _resizedWidth >>= 1;
            _resizedHeight >>= 1;
            _size = _resizedWidth * _resizedHeight * _bytesPerPixel * scale;
        }

        if (_targetMemory == 0) {
            for (int i = 0; i < degradeLevel; i++) {
                _resizedWidth >>= 1;
                _resizedHeight >>= 1;
            }
        }
        imageSize = _resizedWidth * _resizedHeight * _bytesPerPixel;
    } 

    if (_rawBuffer) {
        delete [] _rawBuffer;
        _rawBuffer = NULL;
    }    

    _rawBuffer = new unsigned char[imageSize];
    if (_rawBuffer == NULL) {
        TF_RUNTIME_ERROR("Unable to allocate image buffer.");
        return false;
    }

    // Retrieve raw pixels.
    GlfImage::StorageSpec storage;
    storage.width = _resizedWidth;
    storage.height = _resizedHeight;
    storage.format = _glFormat;
    storage.type = _glType;
    storage.data = _rawBuffer;

    return image->ReadCropped(cropTop, cropBottom, cropLeft, cropRight, storage);
}

int GlfUVTextureData::ComputeBytesUsed() const
{
    if (_rawBuffer) {
        if (GlfIsCompressedFormat(_glFormat)) {
            return GlfGetCompressedTextureSize(_resizedWidth, _resizedHeight, 
                                               _glFormat, _glType);
        } else {
            return _resizedWidth * _resizedHeight * _bytesPerPixel;
        }
    } else {
        return 0;
    }
}

bool GlfUVTextureData::HasRawBuffer() const
{
    return (_rawBuffer != NULL);
}

unsigned char * GlfUVTextureData::GetRawBuffer() const
{
    return _rawBuffer;
}

