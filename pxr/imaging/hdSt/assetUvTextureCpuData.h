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

    /// The wrap info extracted from the image file.
    HDST_API
    const std::pair<HdWrap, HdWrap> &GetWrapInfo() const {
        return _wrapInfo;
    }

private:
    void _SetWrapInfo(HioImageSharedPtr const &image);

    // Pointer to the potentially converted data.
    std::unique_ptr<unsigned char[]> _rawBuffer;

    // The result, including a pointer to the potentially
    // converted texture data in _textureDesc.initialData.
    HgiTextureDesc _textureDesc;

    // If true, initialData only contains mip level 0 data
    // and the GPU is supposed to generate the other mip levels.
    bool _generateMipmaps;

    // Wrap modes
    std::pair<HdWrap, HdWrap> _wrapInfo;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
