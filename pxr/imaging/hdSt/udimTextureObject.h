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
#ifndef PXR_IMAGING_HD_ST_UDIM_TEXTURE_OBJECT_H
#define PXR_IMAGING_HD_ST_UDIM_TEXTURE_OBJECT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hdSt/textureObject.h"

#include "pxr/imaging/hgi/handle.h"

#include "pxr/base/gf/vec3i.h"

PXR_NAMESPACE_OPEN_SCOPE

enum HgiFormat : int;
using HgiTextureHandle = HgiHandle<class HgiTexture>;

/// Returns true if the file given by \p imageFilePath represents a udim file,
/// and false otherwise.
///
/// This function simply checks the existence of the <udim> tag in the
/// file name and does not otherwise guarantee that
/// the file is in any way valid for reading.
///
HDST_API bool HdStIsSupportedUdimTexture(std::string const& imageFilePath);

/// \class HdStUdimTextureObject
///
/// A UDIM texture.
///
class HdStUdimTextureObject final : public HdStTextureObject
{
public:
    HDST_API
    HdStUdimTextureObject(
        const HdStTextureIdentifier &textureId,
        HdSt_TextureObjectRegistry *textureObjectRegistry);

    HDST_API
    ~HdStUdimTextureObject() override;

    /// Get the gpu texture name for the texels
    ///
    /// Only valid after commit phase.
    ///
    HgiTextureHandle const& GetTexelTexture() const { return _texelTexture; }

    /// Get the gpu texture name for the layout
    ///
    /// Only valid after commit phase.
    ///
    HgiTextureHandle const& GetLayoutTexture() const { return _layoutTexture; }

    HDST_API
    bool IsValid() const override;

    HDST_API
    HdTextureType GetTextureType() const override;

protected:
    HDST_API
    void _Load() override;

    HDST_API
    void _Commit() override;

private:
    std::vector<uint8_t> _textureData;
    std::vector<float> _layoutData;

    GfVec3i _dimensions;
    size_t _tileCount;
    size_t _mipCount;
    HgiFormat _hgiFormat;

    HgiTextureHandle _texelTexture;
    HgiTextureHandle _layoutTexture;

    void _DestroyTextures();
};

template<>
struct HdSt_TypedTextureObjectHelper<HdTextureType::Udim> {
    using type = HdStUdimTextureObject;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
