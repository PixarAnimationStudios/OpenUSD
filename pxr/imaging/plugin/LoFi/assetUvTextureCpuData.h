//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_ASSET_UV_TEXTURE_CPU_DATA_H
#define PXR_IMAGING_LOFI_ASSET_UV_TEXTURE_CPU_DATA_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include "pxr/imaging/plugin/LoFi/textureCpuData.h"
#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hgi/texture.h"

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class LoFiAssetUvTextureCpuData
///
/// Implements LoFiTextureCpuData by reading a uv texture from
/// a file.
///
class LoFiAssetUvTextureCpuData : public LoFiTextureCpuData
{
public:
    LOFI_API
    LoFiAssetUvTextureCpuData(
        std::string const &filePath,
        size_t targetMemory,
        bool premultiplyAlpha,
        HioImage::ImageOriginLocation originLocation,
        HioImage::SourceColorSpace sourceColorSpace);
    
    LOFI_API
    ~LoFiAssetUvTextureCpuData() override;
    
    LOFI_API
    const HgiTextureDesc &GetTextureDesc() const override;

    LOFI_API
    bool GetGenerateMipmaps() const override;

    LOFI_API
    bool IsValid() const override;

    /// The wrap info extracted from the image file.
    LOFI_API
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
