//
// Copyright 2020 Pixar
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
#include "pxr/imaging/hdSt/assetUvTextureCpuData.h"
#include "pxr/imaging/hdSt/textureUtils.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

static
const char *
_ToString(const HioImage::SourceColorSpace s)
{
    switch(s) {
    case HioImage::Raw:
        return "Raw";
    case HioImage::SRGB:
        return "SRGB";
    case HioImage::Auto:
        return "Auto";
    }
    return "Invalid";
}

HdStAssetUvTextureCpuData::HdStAssetUvTextureCpuData(
    std::string const &filePath,
    size_t targetMemory,
    unsigned int cropTop,
    unsigned int cropBottom,
    unsigned int cropLeft,
    unsigned int cropRight,
    int degradeLevel, 
    bool generateOrUseMipmap,
    bool premultiplyAlpha,
    HioImage::ImageOriginLocation originLocation,
    HioImage::SourceColorSpace sourceColorSpace)
  : _filePath(filePath)
  , _cropTop(cropTop)
  , _cropBottom(cropBottom)
  , _cropLeft(cropLeft)
  , _cropRight(cropRight)
  , _targetMemory(targetMemory)
  , _dimensions(0,0,1)
  , _hioFormat(HioFormatInvalid)
  , _hioSize(0)
  , _wrapInfo{ HdWrapUseMetadata, HdWrapUseMetadata }
  , _sourceColorSpace(sourceColorSpace)
  , _generateMipmaps(false)
{
    // Bail if we couldn't read any texture data.
    if (!_Read(degradeLevel, generateOrUseMipmap, originLocation)) {
        return;
    }

    _textureDesc.type = HgiTextureType2D;
    _textureDesc.dimensions = _dimensions;

    // Determine the format (e.g., float/byte, RED/RGBA) and give
    // function to convert data if necessary.
    // Possible conversions are:
    // - Unsigned byte RGB to RGBA (since the former is not support
    //   by modern graphics APIs)
    // - Pre-multiply alpha.
    //
    HdStTextureUtils::ConversionFunction conversionFunction = nullptr;
    _textureDesc.format = HdStTextureUtils::GetHgiFormat(
        _hioFormat,
        premultiplyAlpha,
        /* avoidThreeComponentFormats = */ false,
        &conversionFunction);
    if (_textureDesc.format == HgiFormatInvalid) {
        return;
    }

    if (conversionFunction) {
        const size_t numPixels =
            _hioSize / HioGetDataSizeOfFormat(_hioFormat);
        const size_t hgiSize =
            numPixels * HgiGetDataSizeOfFormat(_textureDesc.format);
        std::unique_ptr<unsigned char[]> convertedData =
            std::make_unique<unsigned char[]>(hgiSize);

        conversionFunction(_rawBuffer.get(), numPixels, convertedData.get());

        _rawBuffer = std::move(convertedData);
        _textureDesc.pixelsByteSize = hgiSize;
    } else {
        _textureDesc.pixelsByteSize = _hioSize;
    }
     
    _textureDesc.initialData = _rawBuffer.get();

    if (generateOrUseMipmap) {
        if (_rawBufferMips.size() > 1) {
            // Use mipmaps from file.
            _textureDesc.mipLevels = _rawBufferMips.size();
        } else {
            // No mipmaps in file, generate mipmaps on GPU.
            _generateMipmaps = true;
            _textureDesc.mipLevels = HgiGetMipInfos(
                _textureDesc.format,
                _textureDesc.dimensions,
                _textureDesc.layerCount).size();
        }
    }
    
    // Handle grayscale textures by expanding value to green and blue.
    if (HgiGetComponentCount(_textureDesc.format) == 1) {
        _textureDesc.componentMapping = {
            HgiComponentSwizzleR,
            HgiComponentSwizzleR,
            HgiComponentSwizzleR,
            HgiComponentSwizzleOne
        };
    }

    _textureDesc.debugName =
        filePath
        + " - flipVertically="
        + std::to_string(int(originLocation == HioImage::OriginUpperLeft))
        + " - premultiplyAlpha="
        + std::to_string(int(premultiplyAlpha))
        + " - sourceColorSpace="
        + _ToString(sourceColorSpace);
}

HdStAssetUvTextureCpuData::~HdStAssetUvTextureCpuData() = default;

const HgiTextureDesc &
HdStAssetUvTextureCpuData::GetTextureDesc() const
{
    return _textureDesc;
}

bool
HdStAssetUvTextureCpuData::GetGenerateMipmaps() const
{
    return _generateMipmaps;
}

bool
HdStAssetUvTextureCpuData::IsValid() const
{
    return _textureDesc.initialData;
}

// Compute required GPU memory
static
size_t
_ComputeMemory(HioImageSharedPtr const &img,
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
HdStAssetUvTextureCpuData::_GetAllValidMipLevels(
    const HioImageSharedPtr fullImage) const
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
HdStAssetUvTextureCpuData::_ReadDegradedImageInput(
    const HioImageSharedPtr &fullImage,
    bool useMipmap,
    size_t targetMemory,
    size_t degradeLevel)
{
    TRACE_FUNCTION();

    const std::vector<HioImageSharedPtr> mips =
        useMipmap
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
        const size_t size = _ComputeMemory(mips[i], useMipmap);

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

static
HdWrap
_GetWrapMode(HioImageSharedPtr const &image, const HioAddressDimension d)
{
    HioAddressMode mode;
    if (image->GetSamplerMetadata(d, &mode)) {
        switch(mode) {
        case HioAddressModeClampToEdge:
            return HdWrapClamp;
        case HioAddressModeMirrorClampToEdge:
            TF_WARN("Hydra does not support mirror clamp to edge wrap mode");
            return HdWrapRepeat;
        case HioAddressModeRepeat:
            return HdWrapRepeat;
        case HioAddressModeMirrorRepeat:
            return HdWrapMirror;
        case HioAddressModeClampToBorderColor:
             return HdWrapBlack;
        }
    }
    return HdWrapNoOpinion;
}

bool
HdStAssetUvTextureCpuData::_Read(int degradeLevel, bool useMipmap,
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
            fullImage, useMipmap, _targetMemory, degradeLevel);
    
    // Load the first mip to extract important data
    HioImageSharedPtr const &image = degradedImages.front();
    _hioFormat = image->GetFormat();

    _wrapInfo.first  = _GetWrapMode(image, HioAddressDimensionU);
    _wrapInfo.second = _GetWrapMode(image, HioAddressDimensionV);
    
    _hioSize = 0;
    _dimensions[0] = image->GetWidth();
    _dimensions[1] = image->GetHeight();
    _dimensions[2] = 1;
    
    bool needsResizeOnLoad = false;
    int cropTop = 0, cropBottom = 0, cropLeft = 0, cropRight = 0;
    
    if (!HioIsCompressed(_hioFormat)) {
        const bool needsCropping =
            _cropTop || _cropBottom ||
            _cropLeft || _cropRight;
        
        if (needsCropping) {
            TRACE_FUNCTION_SCOPE("cropping");

            // The cropping parameters are with respect to the original image,
            // we need to scale them if we have a down-sampled image.
            // Usually, we crop the slates that are black and the boundary might
            // not hit a pixel boundary of the down-sampled image and thus black
            // bleeds into the pixels near the border of the texture. To avoid
            // this, we use ceil here to cut out the pixels with black bleeding.
            cropLeft   = ceil(double(_cropLeft * image->GetWidth()) /
                              fullImage->GetWidth());
            cropRight  = ceil(double(_cropRight * image->GetWidth()) /
                              fullImage->GetWidth());
            cropTop    = ceil(double(_cropTop * image->GetHeight()) /
                              fullImage->GetHeight());
            cropBottom = ceil(double(_cropBottom * image->GetHeight()) /
                              fullImage->GetHeight());
            
            //Check that cropping parameters are valid
            if (!_IsValidCrop(image, cropTop, cropBottom, cropLeft, cropRight)) {
                TF_CODING_ERROR("Failed to load Texture - Invalid crop");
                return false;
            }

            _dimensions[0] =
                std::max(0, _dimensions[0] - (cropLeft + cropRight));
            _dimensions[1] =
                std::max(0, _dimensions[1] - (cropTop + cropBottom));

            needsResizeOnLoad = true;
        }

        if (_targetMemory == 0) {
            // This is weird: ReadDegradedImageInput already applied
            // degradeLevel when picking a mip level from the image
            // file, so why do we do it here again.
            for (int i = 0; i < degradeLevel; i++) {
                _dimensions[0] = std::max(_dimensions[0]  >> 1, 1);
                _dimensions[1] = std::max(_dimensions[1]  >> 1, 1);
                needsResizeOnLoad = true;
            }
        } else {
            const double scale = useMipmap ? 4.0 / 3 : 1.0;
            
            while ((_dimensions[0] > 1 ||
                    _dimensions[1] > 1) &&
                   size_t(
                       HioGetDataSize(_hioFormat, _dimensions) *
                       scale) > _targetMemory) {
                _dimensions[0] = std::max(_dimensions[0]  >> 1, 1);
                _dimensions[1] = std::max(_dimensions[1]  >> 1, 1);
                needsResizeOnLoad = true;
            }
        }
    }

    // Check if the image is providing a mip chain and check if it is valid
    // Also, if the user wants cropping/resize then the mip chain 
    // will be discarded.
    const bool usePregeneratedMips = !needsResizeOnLoad && useMipmap;
    const int numMipLevels = usePregeneratedMips ? degradedImages.size() : 1;
    
    _rawBufferMips.resize(numMipLevels);
    
    // Read the metadata for the degraded mips in the structure that keeps
    // track of all the mips
    for(int i = 0 ; i < numMipLevels; i++) {
        HioImageSharedPtr const image = degradedImages[i];
        
        // Create the new mipmap
        Mip & mip  = _rawBufferMips[i];
        mip.width  =
            needsResizeOnLoad ? _dimensions[0] : image->GetWidth();
        mip.height =
            needsResizeOnLoad ? _dimensions[1] : image->GetHeight();
        
        mip.size   = HioGetDataSize(_hioFormat, GfVec3i(mip.width, mip.height,1));
        mip.offset = _hioSize;
        _hioSize += mip.size;
    }

    {
        TRACE_FUNCTION_SCOPE("memory allocation");
        
        _rawBuffer.reset(
            new (std::nothrow) unsigned char[_hioSize]);
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
             storage.format = _hioFormat;
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

PXR_NAMESPACE_CLOSE_SCOPE

