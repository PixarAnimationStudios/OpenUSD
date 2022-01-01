//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/plugin/LoFi/assetUvTextureCpuData.h"
#include "pxr/imaging/plugin/LoFi/textureUtils.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

// For texture debug string
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

static
GfVec3i
_GetDimensions(HioImageSharedPtr const &image)
{
    return GfVec3i(image->GetWidth(), image->GetHeight(), 1);
}

static
size_t
_ComputeSize(const GfVec3i &dimensions)
{
    return dimensions[0] * dimensions[1] * dimensions[2];
}

LoFiAssetUvTextureCpuData::LoFiAssetUvTextureCpuData(
    std::string const &filePath,
    const size_t targetMemory,
    const bool premultiplyAlpha,
    const HioImage::ImageOriginLocation originLocation,
    const HioImage::SourceColorSpace sourceColorSpace)
  : _generateMipmaps(false)
  , _wrapInfo{ HdWrapNoOpinion, HdWrapNoOpinion }
{
    TRACE_FUNCTION();

    // Open all mips for the image.
    const std::vector<HioImageSharedPtr> mips =
        LoFiTextureUtils::GetAllMipImages(filePath, sourceColorSpace);
    if (mips.empty()) {
        return;
    }

    // Extract wrap info and the CPU data format from first mip image.
    _SetWrapInfo(mips.front());
    const HioFormat hioFormat = mips.front()->GetFormat();

    _textureDesc.type = HgiTextureType2D;

    // Determine the corresponding GPU format (e.g., float/byte,
    // RED/RGBA) and give function to convert data if necessary.
    LoFiTextureUtils::ConversionFunction conversionFunction = nullptr;
    _textureDesc.format = LoFiTextureUtils::GetHgiFormat(
        hioFormat,
        premultiplyAlpha,
        /* avoidThreeComponentFormats = */ false,
        &conversionFunction);

    if (_textureDesc.format == HgiFormatInvalid) {
        TF_WARN("Unsupported texture format for UV texture");
        return;
    }

    // The index of the mip image in mips that we start with.
    size_t firstMip;

    // Use target memory to determine the first mip to use the
    // dimensions of the GPU texture (which can be even smaller than
    // that of the mip image).
    //
    _textureDesc.dimensions =
        LoFiTextureUtils::ComputeDimensionsFromTargetMemory(
            mips,
            _textureDesc.format,
            /* tileCount = */ 1,
            targetMemory,
            &firstMip);

    // Compute the GPU mip sizes.
    const std::vector<HgiMipInfo> mipInfos =
        HgiGetMipInfos(
            _textureDesc.format,
            _textureDesc.dimensions,
            /* layerCount = */ 1);

    // We always use the data of the first mip. Determine, how many
    // of the following mips from the image we can use.
    // The requirement is that the authored mip image has the correct
    // dimension to be suitable as mip for the GPU.
    size_t numUsableMips = 1;
    while (firstMip + numUsableMips < mips.size() &&
           _GetDimensions(mips[firstMip + numUsableMips]) ==
                                        mipInfos[numUsableMips].dimensions) {
        numUsableMips++;
    }

    if (numUsableMips > 1) {
        // We have authored mips we can use, so use them.
        _textureDesc.mipLevels = numUsableMips;
    } else {
        // No authored mips, generate the mipmaps from the image.
        _generateMipmaps = true;
        _textureDesc.mipLevels = mipInfos.size();
    }

    // Compute how much memory we need to allocate to upload the
    // desirable mips.
    const HgiMipInfo &lastMipInfo = mipInfos[numUsableMips - 1];
    const size_t memSize = lastMipInfo.byteOffset + lastMipInfo.byteSize;
    {
        TRACE_FUNCTION_SCOPE("allocating memory");
        _rawBuffer =
            std::make_unique<unsigned char[]>(memSize);
    }

    {
        // Read the actual mips from each image and store them in a big buffer of
        // contiguous memory.
        TRACE_FUNCTION_SCOPE("filling in image data");

        for (size_t i = 0; i < numUsableMips; ++i) {
            HioImage::StorageSpec storage;
            storage.width = mipInfos[i].dimensions[0];
            storage.height = mipInfos[i].dimensions[1];
            storage.format = hioFormat;
            storage.flipped = (originLocation == HioImage::OriginLowerLeft);
            storage.data = _rawBuffer.get() + mipInfos[i].byteOffset;
            if (!mips[firstMip + i]->Read(storage)) {
                TF_WARN("Unable to read Texture '%s'.", filePath.c_str());
                return;
            }

            if (conversionFunction) {
                conversionFunction(storage.data,
                                   _ComputeSize(mipInfos[i].dimensions),
                                   storage.data);
            }
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

    // We successfully made it to the end of the function. Indicate that
    // the texture descriptor is valid by setting the data and its size.
    _textureDesc.initialData = _rawBuffer.get();
    _textureDesc.pixelsByteSize = memSize;
}

LoFiAssetUvTextureCpuData::~LoFiAssetUvTextureCpuData() = default;

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

void
LoFiAssetUvTextureCpuData::_SetWrapInfo(
    HioImageSharedPtr const &image)
{
    _wrapInfo.first  = _GetWrapMode(image, HioAddressDimensionU);
    _wrapInfo.second = _GetWrapMode(image, HioAddressDimensionV);
}

const HgiTextureDesc &
LoFiAssetUvTextureCpuData::GetTextureDesc() const
{
    return _textureDesc;
}

bool
LoFiAssetUvTextureCpuData::GetGenerateMipmaps() const
{
    return _generateMipmaps;
}

bool
LoFiAssetUvTextureCpuData::IsValid() const
{
    return _textureDesc.initialData;
}

PXR_NAMESPACE_CLOSE_SCOPE

