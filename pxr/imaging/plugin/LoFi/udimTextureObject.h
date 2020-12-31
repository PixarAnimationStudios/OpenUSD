//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_UDIM_TEXTURE_OBJECT_H
#define PXR_IMAGING_LOFI_UDIM_TEXTURE_OBJECT_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"

#include "pxr/imaging/plugin/LoFi/textureObject.h"

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
LOFI_API bool LoFiIsSupportedUdimTexture(std::string const& imageFilePath);

/// \class LoFiUdimTextureObject
///
/// A UDIM texture.
///
class LoFiUdimTextureObject final : public LoFiTextureObject
{
public:
    LOFI_API
    LoFiUdimTextureObject(
        const LoFiTextureIdentifier &textureId,
        LoFiTextureObjectRegistry *textureObjectRegistry);

    LOFI_API
    ~LoFiUdimTextureObject() override;

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

    LOFI_API
    bool IsValid() const override;

    LOFI_API
    HdTextureType GetTextureType() const override;

protected:
    LOFI_API
    void _Load() override;

    LOFI_API
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
struct LoFiTypedTextureObjectHelper<HdTextureType::Udim> {
    using type = LoFiUdimTextureObject;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
