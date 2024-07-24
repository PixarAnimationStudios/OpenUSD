//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_PTEX_TEXTURE_OBJECT_H
#define PXR_IMAGING_HD_ST_PTEX_TEXTURE_OBJECT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/textureObject.h"

#include "pxr/imaging/hgi/handle.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3i.h"

#ifdef PXR_PTEX_SUPPORT_ENABLED
#include "pxr/imaging/hdSt/ptexMipmapTextureLoader.h"
#endif

PXR_NAMESPACE_OPEN_SCOPE

/// Returns true if the file given by \p imageFilePath represents a ptex file,
/// and false otherwise.
///
/// This function simply checks the extension of the file name and does not
/// otherwise guarantee that the file is in any way valid for reading.
///
/// If ptex support is disabled, this function will always return false.
///
HDST_API bool HdStIsSupportedPtexTexture(std::string const & imageFilePath);

enum HgiFormat : int;
using HgiTextureHandle = HgiHandle<class HgiTexture>;

/// \class HdStPtexTextureObject
///
/// A Ptex texture.
///
class HdStPtexTextureObject final : public HdStTextureObject
{
public:
    HDST_API
    HdStPtexTextureObject(
        const HdStTextureIdentifier &textureId,
        HdSt_TextureObjectRegistry *textureObjectRegistry);

    HDST_API
    ~HdStPtexTextureObject() override;

    /// Get the GPU texture handle for the texels
    ///
    /// Only valid after commit phase.
    ///
    HgiTextureHandle GetTexelTexture() const { return _texelTexture; }

    /// Get the GPU texture handle for the layout
    ///
    /// Only valid after commit phase.
    ///
    HgiTextureHandle GetLayoutTexture() const { return _layoutTexture; }

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
    HgiFormat _format;
    GfVec3i _texelDimensions;
    int _texelLayers;
    size_t _texelDataSize;
    GfVec2i _layoutDimensions;
    size_t _layoutDataSize;

    std::unique_ptr<uint8_t[]> _texelData;
    std::unique_ptr<uint8_t[]> _layoutData;

    HgiTextureHandle _texelTexture;
    HgiTextureHandle _layoutTexture;

    void _DestroyTextures();
};

template<>
struct HdSt_TypedTextureObjectHelper<HdStTextureType::Ptex> {
    using type = HdStPtexTextureObject;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
