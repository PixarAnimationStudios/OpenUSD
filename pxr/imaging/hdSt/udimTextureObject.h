//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    HdStTextureType GetTextureType() const override;

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
struct HdSt_TypedTextureObjectHelper<HdStTextureType::Udim> {
    using type = HdStUdimTextureObject;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
