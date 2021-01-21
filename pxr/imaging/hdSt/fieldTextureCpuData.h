//
// Copyright 2021 Pixar
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
#ifndef PXR_IMAGING_HD_ST_FIELD_TEXTURE_CPU_DATA_H
#define PXR_IMAGING_HD_ST_FIELD_TEXTURE_CPU_DATA_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hdSt/textureCpuData.h"
#include "pxr/imaging/hgi/texture.h"

#include "pxr/base/tf/declarePtrs.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HioFieldTextureDataSharedPtr = std::shared_ptr<class HioFieldTextureData>;

/// \class HdStTextureCpuData
///
/// An implmentation of HdStTextureCpuData that can be initialized
/// from HioFieldTextureData.
///
class HdSt_FieldTextureCpuData : public HdStTextureCpuData
{
public:
    /// It is assumed that Read(...) has already been called
    /// on textureData.

    HDST_API
    HdSt_FieldTextureCpuData(
        HioFieldTextureDataSharedPtr const &textureData,
        const std::string &debugName,
        bool premultiplyAlpha = true);

    HDST_API
    ~HdSt_FieldTextureCpuData() override;
    
    HDST_API
    const HgiTextureDesc &GetTextureDesc() const override;

    HDST_API
    bool GetGenerateMipmaps() const override;

    HDST_API
    bool IsValid() const override;

private:
    // The result, including a pointer to the potentially
    // converted texture data in _textureDesc.initialData.
    HgiTextureDesc _textureDesc;

    // If true, initialData only contains mip level 0 data
    // and the GPU is supposed to generate the other mip levels.
    bool _generateMipmaps;

    // To avoid a copy, hold on to original data if we
    // can use them.
    HioFieldTextureDataSharedPtr _textureData;

    // Buffer if we had to convert the data.
    std::unique_ptr<const unsigned char[]> _convertedData;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
