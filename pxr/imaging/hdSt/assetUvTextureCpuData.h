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
#ifndef PXR_IMAGING_HDST_ASSET_UV_TEXTURE_CPU_DATA_H
#define PXR_IMAGING_HDST_ASSET_UV_TEXTURE_CPU_DATA_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/textureCpuData.h"
#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hgi/texture.h"

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdStAssetUvTextureCpuData
///
/// Implements HdStTextureCpuData by reading a uv texture from
/// a file.
///
class HdStAssetUvTextureCpuData : public HdStTextureCpuData
{
public:
    HDST_API
    HdStAssetUvTextureCpuData(
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
        HioImage::SourceColorSpace sourceColorSpace);
    
    HDST_API
    ~HdStAssetUvTextureCpuData() override;
    
    HDST_API
    const HgiTextureDesc &GetTextureDesc() const override;

    HDST_API
    bool GetGenerateMipmaps() const override;

    HDST_API
    bool IsValid() const override;

    HDST_API
    const std::pair<HdWrap, HdWrap> &GetWrapInfo() const {
        return _wrapInfo;
    }

private:
    // A structure that keeps the mips loaded from disk in the format
    // that the gpu needs.
    struct Mip {
        Mip() 
            : size(0), offset(0), width(0), height(0)
        { }

        size_t size;
        size_t offset;
        int width;
        int height;
    };

    bool _Read(int degradeLevel, bool useMipmap,
               HioImage::ImageOriginLocation originLocation);

    // Given a HioImage it will return the number of mip levels that 
    // are actually valid to be loaded to the GPU. For instance, it will
    // drop textures with non valid OpenGL pyramids.
    std::vector<HioImageSharedPtr> _GetAllValidMipLevels(
        const HioImageSharedPtr image) const;

    // Reads an image using HioImage. If possible and requested, it will
    // load a down-sampled version (when mipmapped .tex file) of the image.
    // If targetMemory is > 0, it will iterate through the down-sampled version
    // until the estimated required GPU memory is smaller than targetMemory.
    // Otherwise, it will use the given degradeLevel.
    // When estimating the required GPU memory, it will take into account that
    // the GPU might generate MipMaps.
    std::vector<HioImageSharedPtr> _ReadDegradedImageInput(
        const HioImageSharedPtr &fullImage,
        bool useMipmap,
        size_t targetMemory,
        size_t degradeLevel);

    const std::string _filePath;
    unsigned int _cropTop, _cropBottom, _cropLeft, _cropRight;
    size_t _targetMemory;
    GfVec3i _dimensions;

    HioFormat _hioFormat;
    size_t _hioSize;

    std::pair<HdWrap, HdWrap> _wrapInfo;

    std::unique_ptr<unsigned char[]> _rawBuffer;
    std::vector<Mip> _rawBufferMips;

    HioImage::SourceColorSpace _sourceColorSpace;

    // The result, including a pointer to the potentially
    // converted texture data in _textureDesc.initialData.
    HgiTextureDesc _textureDesc;

    // If true, initialData only contains mip level 0 data
    // and the GPU is supposed to generate the other mip levels.
    bool _generateMipmaps;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
