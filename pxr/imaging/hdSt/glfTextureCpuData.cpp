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
#include "pxr/imaging/hdSt/glfTextureCpuData.h"

#include "pxr/imaging/hdSt/textureUtils.h"

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

HdStGlfTextureCpuData::HdStGlfTextureCpuData(
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

    HdStTextureUtils::ConversionFunction conversionFunction = nullptr;
    _textureDesc.format = HdStTextureUtils::GetHgiFormat(
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

HdStGlfTextureCpuData::~HdStGlfTextureCpuData() = default;

const
HgiTextureDesc &
HdStGlfTextureCpuData::GetTextureDesc() const
{
    return _textureDesc;
}

bool
HdStGlfTextureCpuData::GetGenerateMipmaps() const
{
    return _generateMipmaps;
}

bool
HdStGlfTextureCpuData::IsValid() const
{
    return _textureDesc.initialData;
}

PXR_NAMESPACE_CLOSE_SCOPE
