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

HdStAssetUvTextureCpuData::HdStAssetUvTextureCpuData(
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
        HdStTextureUtils::GetAllMipImages(filePath, sourceColorSpace);
    if (mips.empty()) {
        return;
    }

    // Extract wrap info and the CPU data format from first mip image.
    _SetWrapInfo(mips.front());
    const HioFormat hioFormat = mips.front()->GetFormat();

    _textureDesc.type = HgiTextureType2D;

    // Determine the corresponding GPU format (e.g., float/byte,
    // RED/RGBA) and give function to convert data if necessary.
    _textureDesc.format = HdStTextureUtils::GetHgiFormat(
        hioFormat,
        premultiplyAlpha);

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
        HdStTextureUtils::ComputeDimensionsFromTargetMemory(
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
    const size_t memSize =
        lastMipInfo.byteOffset + lastMipInfo.byteSizePerLayer;
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
            if (!HdStTextureUtils::ReadAndConvertImage(
                    mips[firstMip + i],
                    /* flipped = */ originLocation == HioImage::OriginLowerLeft,
                    premultiplyAlpha,
                    mipInfos[i],
                    /* layer = */ 0,
                    _rawBuffer.get())) {
                TF_WARN("Unable to read Texture '%s'.", filePath.c_str());
                return;
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

HdStAssetUvTextureCpuData::~HdStAssetUvTextureCpuData() = default;

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
HdStAssetUvTextureCpuData::_SetWrapInfo(
    HioImageSharedPtr const &image)
{
    _wrapInfo.first  = _GetWrapMode(image, HioAddressDimensionU);
    _wrapInfo.second = _GetWrapMode(image, HioAddressDimensionV);
}

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

PXR_NAMESPACE_CLOSE_SCOPE

