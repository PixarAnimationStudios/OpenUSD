//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_TEXTURE_IDENTIFIER_H
#define PXR_IMAGING_LOFI_TEXTURE_IDENTIFIER_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"

#include "pxr/base/tf/token.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class LoFiSubtextureIdentifier;

///
/// \class LoFiTextureIdentifier
///
/// Class to identify a texture file or a texture within the texture file
/// (e.g., a frame in a movie).
///
/// The class has value semantics and uses LoFiSubtextureIdentifier in a
/// polymorphic way.
///
class LoFiTextureIdentifier final
{
public:
    using ID = size_t;

    LoFiTextureIdentifier();

    /// C'tor for files that can contain only one texture.
    ///
    explicit LoFiTextureIdentifier(const TfToken &filePath);

    /// C'tor for files that can contain more than one texture (e.g.,
    /// frames in a movie, grids in a VDB file).
    ///
    LoFiTextureIdentifier(
        const TfToken &filePath,
        std::unique_ptr<const LoFiSubtextureIdentifier> &&subtextureId);

    LoFiTextureIdentifier(const LoFiTextureIdentifier &textureId);

    LoFiTextureIdentifier &operator=(LoFiTextureIdentifier &&textureId);

    LoFiTextureIdentifier &operator=(const LoFiTextureIdentifier &textureId);

    ~LoFiTextureIdentifier();

    /// Get file path of texture file.
    ///
    const TfToken &GetFilePath() const {
        return _filePath;
    }
    
    /// Get additional information identifying a texture in a file that
    /// can contain more than one texture (e.g., a frame in a movie or
    /// a grid in a VDB file).
    ///
    /// nullptr for files (e.g., png) that can contain only one texture.
    ///
    const LoFiSubtextureIdentifier * GetSubtextureIdentifier() const {
        return _subtextureId.get();
    }

    bool operator==(const LoFiTextureIdentifier &other) const;
    bool operator!=(const LoFiTextureIdentifier &other) const;

private:
    TfToken _filePath;
    std::unique_ptr<const LoFiSubtextureIdentifier> _subtextureId;
};

size_t hash_value(const LoFiTextureIdentifier &);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
