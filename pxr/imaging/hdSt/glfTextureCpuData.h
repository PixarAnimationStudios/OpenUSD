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
#ifndef PXR_IMAGING_HD_ST_GLF_TEXTURE_CPU_DATA_H
#define PXR_IMAGING_HD_ST_GLF_TEXTURE_CPU_DATA_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hdSt/textureCpuData.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/glf/image.h"
#include "pxr/base/tf/declarePtrs.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(GlfBaseTextureData);

/// \class HdStTextureCpuData
///
/// An implmentation of HdStTextureCpuData that can be initialized
/// from GlfBaseTextureData.
///
class HdStGlfTextureCpuData : public HdStTextureCpuData
{
public:
    HDST_API
    HdStGlfTextureCpuData(
        GlfBaseTextureDataRefPtr const &textureData,
        const std::string &debugName,
        bool generateMips = false,
        bool premultiplyAlpha = true,
        GlfImage::ImageOriginLocation originLoc = GlfImage::OriginUpperLeft);
    
    HDST_API
    ~HdStGlfTextureCpuData();
    
    HDST_API
    const HgiTextureDesc &GetTextureDesc() const override;

    HDST_API
    bool IsValid() const override;

private:
    // Determine format for texture descriptor.
    //
    // If necessary, converts the RGB to RGBA data or pre-multiplies by alpha,
    // updating _textureDesc.initialData to point to the newly allocated data
    // (and dropping _textureData).
    //
    HgiFormat _DetermineFormatAndConvertIfNecessary(
        const GLenum glFormat,
        const GLenum glType,
        const GLenum glInternalFormat,
        const bool premultiplyAlpha);

    // The result, including a pointer to the potentially
    // converted texture data in _textureDesc.initialData.
    HgiTextureDesc _textureDesc;

    // To avoid a copy, hold on to original data if we
    // can use them.
    GlfBaseTextureDataRefPtr _textureData;

    // Buffer if we had to convert the data.
    std::unique_ptr<const unsigned char[]> _convertedRawData;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
