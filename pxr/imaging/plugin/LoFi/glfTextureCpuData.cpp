//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/plugin/LoFi/glfTextureCpuData.h"

#include "pxr/imaging/plugin/LoFi/textureUtils.h"

#include "pxr/imaging/glf/baseTextureData.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

HgiTextureType
_GetTextureType(const int numDimensions)
{
    switch(numDimensions) {
    case 2:
        return HgiTextureType2D;
    case 3:
        return HgiTextureType3D;
    default:
        TF_CODING_ERROR("Unsupported number of dimensions");
        return HgiTextureType2D;
    }
}

bool
_IsValid(GlfBaseTextureDataConstRefPtr const &textureData)
{
    return
        textureData->ResizedWidth() > 0 &&
        textureData->ResizedHeight() > 0 &&
        textureData->ResizedDepth() > 0 &&
        textureData->HasRawBuffer();
}

} // anonymous namespace

LoFiGlfTextureCpuData::LoFiGlfTextureCpuData(
    GlfBaseTextureDataConstRefPtr const &textureData,
    const std::string &debugName,
    const bool useOrGenerateMipmaps,
    const bool premultiplyAlpha)
  : _generateMipmaps(false)
{
    TRACE_FUNCTION();

    _textureDesc.debugName = debugName;

    // Bail if we don't have texture data.
    if (!textureData) {
        return;
    }

    // Sanity checks
    if (!_IsValid(textureData)) {
        return;
    }

    // If there is no file at the given path, we should have bailed
    // by now and left _textureDesc.initalData null indicating to
    // our clients that the texture is invalid.

    // Is this 2D or 3D texture?
    _textureDesc.type = _GetTextureType(textureData->NumDimensions());

    // Determine the format (e.g., float/byte, RED/RGBA) and give
    // function to convert data if necessary.
    // Possible conversions are:
    // - Unsigned byte RGB to RGBA (since the former is not support
    //   by modern graphics APIs)
    // - Pre-multiply alpha.

    const HioFormat hioFormat = textureData->GetFormat();

    LoFiTextureUtils::ConversionFunction conversionFunction = nullptr;
    _textureDesc.format = LoFiTextureUtils::GetHgiFormat(
        hioFormat,
        premultiplyAlpha,
        /* avoidThreeComponentFormats = */ false,
        &conversionFunction);

    // Handle grayscale textures by expanding value to green and blue.
    if (HgiGetComponentCount(_textureDesc.format) == 1) {
        _textureDesc.componentMapping = {
            HgiComponentSwizzleR,
            HgiComponentSwizzleR,
            HgiComponentSwizzleR,
            HgiComponentSwizzleOne
        };
    }

    _textureDesc.dimensions = GfVec3i(
        textureData->ResizedWidth(),
        textureData->ResizedHeight(),
        textureData->ResizedDepth());

    const std::vector<HgiMipInfo> mipInfos = HgiGetMipInfos(
        _textureDesc.format,
        _textureDesc.dimensions,
        _textureDesc.layerCount);

    // How many mipmaps to use from the file.
    unsigned int numGivenMipmaps = 1;

    if (useOrGenerateMipmaps) {
        numGivenMipmaps = textureData->GetNumMipLevels();
        if (numGivenMipmaps > 1) {
            // Use mipmaps from file.
            if (numGivenMipmaps > mipInfos.size()) {
                TF_CODING_ERROR("Too many mip maps in texture data.");
                numGivenMipmaps = mipInfos.size();
            }
            _textureDesc.mipLevels = numGivenMipmaps;
        } else {
            // No mipmaps in file, generate mipmaps on GPU.
            _generateMipmaps = true;
            _textureDesc.mipLevels = mipInfos.size();
        }
    }
    const HgiMipInfo &mipInfo = mipInfos[numGivenMipmaps - 1];

    // Size of initial data.
    _textureDesc.pixelsByteSize = mipInfo.byteOffset + mipInfo.byteSize;

    if (conversionFunction) {
        const size_t numPixels = 
            _textureDesc.pixelsByteSize /
            HgiGetDataSizeOfFormat(_textureDesc.format);

        // Convert the texture data
        std::unique_ptr<unsigned char[]> convertedData =
            std::make_unique<unsigned char[]>(_textureDesc.pixelsByteSize);
        conversionFunction(
            textureData->GetRawBuffer(), numPixels,
            convertedData.get());
        _convertedData = std::move(convertedData);

        // Point to converted data
        _textureDesc.initialData = _convertedData.get();
    } else {
        // Ensure that texture data are not deleted
        _textureData = textureData;
        // Point to raw buffer inside texture data
        _textureDesc.initialData = textureData->GetRawBuffer();
    }
}

LoFiGlfTextureCpuData::~LoFiGlfTextureCpuData() = default;

const
HgiTextureDesc &
LoFiGlfTextureCpuData::GetTextureDesc() const
{
    return _textureDesc;
}

bool
LoFiGlfTextureCpuData::GetGenerateMipmaps() const
{
    return _generateMipmaps;
}

bool
LoFiGlfTextureCpuData::IsValid() const
{
    return _textureDesc.initialData;
}

PXR_NAMESPACE_CLOSE_SCOPE
