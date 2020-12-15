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
#include "pxr/imaging/glf/uvTextureData.h"

#include "pxr/base/gf/vec3i.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE


GlfUVTextureDataRefPtr
GlfUVTextureData::New(
    std::string const &filePath,
    size_t targetMemory,
    unsigned int cropTop,
    unsigned int cropBottom,
    unsigned int cropLeft,
    unsigned int cropRight,
    HioImage::SourceColorSpace sourceColorSpace)
{
    GlfUVTextureData::Params params;
    params.targetMemory = targetMemory;
    params.cropTop      = cropTop;
    params.cropBottom   = cropBottom;
    params.cropLeft     = cropLeft;
    params.cropRight    = cropRight;
    return New(filePath, params, sourceColorSpace);
}

GlfUVTextureDataRefPtr
GlfUVTextureData::New(std::string const &filePath, Params const &params,
                      HioImage::SourceColorSpace sourceColorSpace)
{
    return TfCreateRefPtr(new GlfUVTextureData(filePath, params, 
                                               sourceColorSpace));
}

GlfUVTextureData::~GlfUVTextureData() = default;

GlfUVTextureData::GlfUVTextureData(std::string const &filePath,
                                   Params const &params, 
                                   HioImage::SourceColorSpace sourceColorSpace)
  : _filePath(filePath),
    _params(params),
    _targetMemory(0),
    _nativeWidth(0), _nativeHeight(0),
    _resizedWidth(0), _resizedHeight(0),
    _bytesPerPixel(0),
    _format(HioFormatUNorm8Vec3),
    _size(0),
    _sourceColorSpace(sourceColorSpace)
{
    /* nothing */
}

int
GlfUVTextureData::NumDimensions() const
{
    return 2;
}

// Compute required GPU memory
static
size_t
GlfUVTextureData_ComputeMemory(HioImageSharedPtr const &img,
                               const bool generateMipmap)
{
    // Mipmapping on GPU means we need an
    // extra 1/4 + 1/16 + 1/64 + 1/256 + ... of memory
    const double scale = generateMipmap ? 4.0 / 3 : 1.0;

    return scale * HioGetDataSize(
        img->GetFormat(), 
        GfVec3i(img->GetWidth(), img->GetHeight(), 1));
}

std::vector<HioImageSharedPtr>
GlfUVTextureData::_GetAllValidMipLevels(const HioImageSharedPtr fullImage) const
{
    std::vector<HioImageSharedPtr> result{ fullImage };

    // Ignoring image->GetNumMipLevels() since it is unreliable.
    const int numMips = 32;
  
    // Some of our texture loaders will always return an image (even if that
    // mip is not available) so the easiest way to figure out the number of 
    // mip levels is by loading mips and looking at the sizes.
    //
    for (int mipCounter = 1; mipCounter < numMips; mipCounter++) {
        HioImageSharedPtr const image = HioImage::OpenForReading(
            _filePath, /* subimage = */ 0, mipCounter, _sourceColorSpace, 
            /*suppressErrors=*/ true);
        if (!image) {
            break;
        }

        const int previousWidth  = result.back()->GetWidth();
        const int previousHeight = result.back()->GetHeight();
        const int currentWidth  = image->GetWidth();
        const int currentHeight = image->GetHeight();

        // If previous mip and current mip are equal we have found the end of
        // the chain.
        if (previousWidth == currentWidth &&
            previousHeight == currentHeight) {
            break;
        }

        // We need to make sure that the previous mip and the current mip
        // are consecutives powers of two.
        if (! (currentWidth  == std::max(1, previousWidth  >> 1) &&
               currentHeight == std::max(1, previousHeight >> 1))) {

            // Discard all authored mips - even the ones that are valid.
            return { fullImage };
        }
        
        result.push_back(image);
    }

    return std::move(result);
}

std::vector<HioImageSharedPtr>
GlfUVTextureData::_ReadDegradedImageInput(const HioImageSharedPtr &fullImage,
                                          bool generateMipmap,
                                          size_t targetMemory,
                                          size_t degradeLevel)
{
    TRACE_FUNCTION();

    const std::vector<HioImageSharedPtr> mips =
        generateMipmap
            ? _GetAllValidMipLevels(fullImage)
            : std::vector<HioImageSharedPtr>{ fullImage };

    // Load full chain if needed
    const int numMipLevels = mips.size();

    // If no targetMemory set, use degradeLevel to determine mipLevel
    if (targetMemory == 0) {
        if (degradeLevel == 0) {
            return mips;
        }
        const size_t level = std::min(
            static_cast<size_t>(numMipLevels - 1), degradeLevel);
        
        return { mips[level] };
    }

    // Iterate through the levels of down-sampled images until either:
    // - The required memory is less or equal to targetMemory
    // - There are no more down-sampled images
    for (int i = 0; i < numMipLevels; i++) {
        const size_t size =
            GlfUVTextureData_ComputeMemory(mips[i], generateMipmap);

        // Compute the size at the down-sampled resolution.
        if (size <= targetMemory) {
             // We found an image with small enough memory requirement,
             // return it.
            return std::vector<HioImageSharedPtr>(
                 mips.begin() + i, mips.end());
        }
    }

    // Iteration limit reached, return image from last iteration.
    return { mips.back() };
}

static bool
_IsValidCrop(HioImageSharedPtr image,
             int cropTop, int cropBottom, int cropLeft, int cropRight)
{
    const int cropImageWidth = image->GetWidth() - (cropLeft + cropRight);
    const int cropImageHeight = image->GetHeight() - (cropTop + cropBottom);
    return (cropTop >= 0 && 
            cropBottom >= 0 &&
            cropLeft >= 0 &&
            cropRight >= 0 &&
            cropImageWidth > 0 &&
            cropImageHeight > 0); 
}

bool
GlfUVTextureData::Read(int degradeLevel, bool generateMipmap,
                       HioImage::ImageOriginLocation originLocation)
{   
    TRACE_FUNCTION();

    // Read the header of the image (no subimageIndex given, so at full
    // resolutin when evaluated)
    HioImageSharedPtr const fullImage = HioImage::OpenForReading(
        _filePath, /*subimage = */ 0, /* mip = */ 0, _sourceColorSpace);
    if (!fullImage) {
        TF_WARN("Unable to load Texture '%s'.", _filePath.c_str());
        return false;
    }

    // Read the image from a file, if possible and necessary, a down-sampled
    // version
    const std::vector<HioImageSharedPtr> degradedImages =
        _ReadDegradedImageInput(
            fullImage, generateMipmap, _params.targetMemory, degradeLevel);
    
    // Load the first mip to extract important data
    HioImageSharedPtr const &image = degradedImages.front();
    _format = image->GetFormat();

    _targetMemory = _params.targetMemory;
    _wrapInfo.wrapModeS.first =
        image->GetSamplerMetadata(
            HioAddressDimensionU, &_wrapInfo.wrapModeS.second);
    _wrapInfo.wrapModeT.first =
        image->GetSamplerMetadata(
            HioAddressDimensionV, &_wrapInfo.wrapModeT.second);
    _size = 0;
    _nativeWidth = _resizedWidth = image->GetWidth();
    _nativeHeight = _resizedHeight = image->GetHeight();
    
    bool needsResizeOnLoad = false;
    int cropTop = 0, cropBottom = 0, cropLeft = 0, cropRight = 0;
    
    if (HioIsCompressed(_format)) {
        // When using compressed formats the bytesPerPixel is not 
        // used and the glFormat matches the glInternalFormat.
        // XXX internalFormat is used to get the HioFormat back until 
        // textureData is updated to include hioFormat 
        _bytesPerPixel = image->GetBytesPerPixel();
    } else {
        _bytesPerPixel = HioGetDataSizeOfFormat(_format);

        const bool needsCropping =
            _params.cropTop || _params.cropBottom ||
            _params.cropLeft || _params.cropRight;
        
        if (needsCropping) {
            TRACE_FUNCTION_SCOPE("cropping");

            // The cropping parameters are with respect to the original image,
            // we need to scale them if we have a down-sampled image.
            // Usually, we crop the slates that are black and the boundary might
            // not hit a pixel boundary of the down-sampled image and thus black
            // bleeds into the pixels near the border of the texture. To avoid
            // this, we use ceil here to cut out the pixels with black bleeding.
            cropLeft   = ceil(double(_params.cropLeft * image->GetWidth()) /
                              fullImage->GetWidth());
            cropRight  = ceil(double(_params.cropRight * image->GetWidth()) /
                              fullImage->GetWidth());
            cropTop    = ceil(double(_params.cropTop * image->GetHeight()) /
                              fullImage->GetHeight());
            cropBottom = ceil(double(_params.cropBottom * image->GetHeight()) /
                              fullImage->GetHeight());
            
            //Check that cropping parameters are valid
            if (!_IsValidCrop(image, cropTop, cropBottom, cropLeft, cropRight)) {
                TF_CODING_ERROR("Failed to load Texture - Invalid crop");
                return false;
            }

            _resizedWidth = std::max(0, _resizedWidth - (cropLeft + cropRight));
            _resizedHeight = std::max(0, _resizedHeight - (cropTop + cropBottom));

            needsResizeOnLoad = true;
        }

        if (_targetMemory == 0) {
            // This is weird: ReadDegradedImageInput already applied
            // degradeLevel when picking a mip level from the image
            // file, so why do we do it here again.
            for (int i = 0; i < degradeLevel; i++) {
                _resizedWidth  = std::max(_resizedWidth  >> 1, 1);
                _resizedHeight = std::max(_resizedHeight >> 1, 1);
                needsResizeOnLoad = true;
            }
        } else {
            const double scale = generateMipmap ? 4.0 / 3 : 1.0;
            
            while ((_resizedWidth > 1 || _resizedHeight > 1) &&
                    static_cast<size_t>(
                        _resizedWidth * _resizedHeight *
                        _bytesPerPixel * scale) > _targetMemory) {
                _resizedWidth  = std::max(_resizedWidth  >> 1, 1);
                _resizedHeight = std::max(_resizedHeight >> 1, 1);
                needsResizeOnLoad = true;
            }
        }
    }

    // Check if the image is providing a mip chain and check if it is valid
    // Also, if the user wants cropping/resize then the mip chain 
    // will be discarded.
    const bool usePregeneratedMips = !needsResizeOnLoad && generateMipmap;
    const int numMipLevels = usePregeneratedMips ? degradedImages.size() : 1;
    
    _rawBufferMips.resize(numMipLevels);
    
    // Read the metadata for the degraded mips in the structure that keeps
    // track of all the mips
    for(int i = 0 ; i < numMipLevels; i++) {
        HioImageSharedPtr const image = degradedImages[i];
        
        // Create the new mipmap
        Mip & mip  = _rawBufferMips[i];
        mip.width  = needsResizeOnLoad ? _resizedWidth : image->GetWidth();
        mip.height = needsResizeOnLoad ? _resizedHeight : image->GetHeight();
        
        mip.size   = HioGetDataSize(_format, GfVec3i(mip.width, mip.height,1));
        mip.offset = _size;
        _size += mip.size;
    }

    {
        TRACE_FUNCTION_SCOPE("memory allocation");
        
        _rawBuffer.reset(new (std::nothrow) unsigned char[_size]);
        if (!_rawBuffer) {
            TF_RUNTIME_ERROR("Unable to allocate memory for the mip levels.");
            return false;
        }
    }

    // Read the actual mips from each image and store them in a big buffer of
    // contiguous memory.
    TRACE_FUNCTION_SCOPE("filling in image data");

    std::atomic<bool> returnVal(true);

    WorkParallelForN(
        numMipLevels, 
        [this, &degradedImages, cropTop, cropBottom, cropLeft, cropRight,
         originLocation, &returnVal] (size_t begin, size_t end) {
            
         for (size_t i = begin; i < end; ++i) {
             const Mip & mip  = _rawBufferMips[i];

             HioImage::StorageSpec storage;
             storage.width = mip.width;
             storage.height = mip.height;
             storage.format = _format;
             storage.flipped = (originLocation == HioImage::OriginLowerLeft);
             storage.data = _rawBuffer.get() + mip.offset;
             
             if (!degradedImages[i]->ReadCropped(
                     cropTop, cropBottom, cropLeft, cropRight, storage)) {
                 TF_WARN("Unable to read Texture '%s'.", _filePath.c_str());
                 returnVal.store(false);
                 break;
             }
         }
        });
    
    return returnVal.load();
}

size_t 
GlfUVTextureData::ComputeBytesUsedByMip(int mipLevel) const
{
    // Returns specific mip level sizes
    if (static_cast<size_t>(mipLevel) >= _rawBufferMips.size()) return 0;
    return _rawBufferMips[mipLevel].size;
}

size_t
GlfUVTextureData::ComputeBytesUsed() const
{
    return _size;
}

bool 
GlfUVTextureData::HasRawBuffer(int mipLevel) const
{
    if (static_cast<size_t>(mipLevel) >= _rawBufferMips.size()) return false;
    return (_rawBufferMips[mipLevel].size > 0);
}

unsigned char * 
GlfUVTextureData::GetRawBuffer(int mipLevel) const
{
    if (static_cast<size_t>(mipLevel) >= _rawBufferMips.size() || !_rawBuffer) {
        return 0;
    }
    return _rawBuffer.get() + _rawBufferMips[mipLevel].offset;
}

int 
GlfUVTextureData::ResizedWidth(int mipLevel) const 
{
    if (static_cast<size_t>(mipLevel) >= _rawBufferMips.size()) return 0;
    return _rawBufferMips[mipLevel].width;
}

int 
GlfUVTextureData::ResizedHeight(int mipLevel) const 
{
    if (static_cast<size_t>(mipLevel) >= _rawBufferMips.size()) return 0;
    return _rawBufferMips[mipLevel].height;
}

int
GlfUVTextureData::ResizedDepth(int mipLevel) const
{
    // We can think of a 2d-texture as x*y*1 3d-texture.
    return 1;
}

int 
GlfUVTextureData::GetNumMipLevels() const 
{
    return (int)_rawBufferMips.size();   
}

PXR_NAMESPACE_CLOSE_SCOPE

