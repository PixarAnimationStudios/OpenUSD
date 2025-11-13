//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_TEXTURE_IDENTIFIER_H
#define PXR_IMAGING_HD_ST_TEXTURE_IDENTIFIER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdStSubtextureIdentifier;

///
/// \class HdStTextureIdentifier
///
/// Class to identify a texture file or a texture within the texture file
/// (e.g., a frame in a movie).
///
/// The class has value semantics and uses HdStSubtextureIdentifier in a
/// polymorphic way.
///
class HdStTextureIdentifier final
{
public:
    using ID = size_t;

    HDST_API
    HdStTextureIdentifier();

    /// C'tor for files that can contain only one texture.
    ///
    HDST_API
    explicit HdStTextureIdentifier(
        const TfToken &filePath,
        const VtValue &fallback = VtValue(),
        bool defaultToFallback = false);

    /// C'tor for files that can contain more than one texture (e.g.,
    /// frames in a movie, grids in a VDB file).
    ///
    HDST_API
    HdStTextureIdentifier(
        const TfToken &filePath,
        std::unique_ptr<const HdStSubtextureIdentifier> &&subtextureId,
        const VtValue &fallback = VtValue(),
        bool defaultToFallback = false);

    HDST_API
    HdStTextureIdentifier(const HdStTextureIdentifier &textureId);

    HDST_API
    HdStTextureIdentifier &operator=(HdStTextureIdentifier &&textureId);

    HDST_API
    HdStTextureIdentifier &operator=(const HdStTextureIdentifier &textureId);

    HDST_API
    ~HdStTextureIdentifier();

    /// Get file path of texture file.
    ///
    const TfToken &GetFilePath() const {
        return _filePath;
    }

    /// Get fallback value that's used if loading from _filePath fails
    ///
    const VtValue &GetFallback() const {
        return _fallback;
    }

    /// Get if reading from _filePath should be skipped
    ///
    bool ShouldDefaultToFallback() const {
        return _defaultToFallback;
    }
    
    /// Get additional information identifying a texture in a file that
    /// can contain more than one texture (e.g., a frame in a movie or
    /// a grid in a VDB file).
    ///
    /// nullptr for files (e.g., png) that can contain only one texture.
    ///
    const HdStSubtextureIdentifier * GetSubtextureIdentifier() const {
        return _subtextureId.get();
    }

    HDST_API
    bool operator==(const HdStTextureIdentifier &other) const;
    HDST_API
    bool operator!=(const HdStTextureIdentifier &other) const;

private:
    TfToken _filePath;
    VtValue _fallback;
    bool _defaultToFallback = false;
    std::unique_ptr<const HdStSubtextureIdentifier> _subtextureId;
};

HDST_API
size_t hash_value(const HdStTextureIdentifier &);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
