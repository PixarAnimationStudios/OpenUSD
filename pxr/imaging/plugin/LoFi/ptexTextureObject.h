//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_PTEX_TEXTURE_OBJECT_H
#define PXR_IMAGING_LOFI_PTEX_TEXTURE_OBJECT_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include "pxr/imaging/plugin/LoFi/textureObject.h"

#include "pxr/imaging/hgi/handle.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3i.h"

#ifdef PXR_PTEX_SUPPORT_ENABLED
#include "pxr/imaging/plugin/LoFi/ptexMipmapTextureLoader.h"
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
LOFI_API bool LoFiIsSupportedPtexTexture(std::string const & imageFilePath);

enum HgiFormat : int;
using HgiTextureHandle = HgiHandle<class HgiTexture>;

/// \class LoFiPtexTextureObject
///
/// A Ptex texture.
///
class LoFiPtexTextureObject final : public LoFiTextureObject
{
public:
    LOFI_API
    LoFiPtexTextureObject(
        const LoFiTextureIdentifier &textureId,
        LoFiTextureObjectRegistry *textureObjectRegistry);

    LOFI_API
    ~LoFiPtexTextureObject() override;

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
    HgiFormat _format;
    size_t _numChannels;
    size_t _numBytesPerPixel;
    GfVec3i _texelDimensions;
    GfVec2i _layoutDimensions;
    size_t _numFaces;

    std::unique_ptr<uint8_t[]> _texelData;
    std::unique_ptr<uint32_t[]> _layoutData;

    HgiTextureHandle _texelTexture;
    HgiTextureHandle _layoutTexture;

    void _DestroyTextures();
};

template<>
struct LoFiTypedTextureObjectHelper<HdTextureType::Ptex> {
    using type = LoFiPtexTextureObject;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
