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
#ifndef PXR_IMAGING_HD_ST_TEXTURE_IDENTIFIER_H
#define PXR_IMAGING_HD_ST_TEXTURE_IDENTIFIER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/base/tf/token.h"

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

    HdStTextureIdentifier();

    /// C'tor for files that can contain only one texture.
    ///
    explicit HdStTextureIdentifier(const TfToken &filePath);

    /// C'tor for files that can contain more than one texture (e.g.,
    /// frames in a movie, grids in a VDB file).
    ///
    HdStTextureIdentifier(
        const TfToken &filePath,
        std::unique_ptr<const HdStSubtextureIdentifier> &&subtextureId);

    HdStTextureIdentifier(const HdStTextureIdentifier &textureId);

    HdStTextureIdentifier &operator=(HdStTextureIdentifier &&textureId);

    HdStTextureIdentifier &operator=(const HdStTextureIdentifier &textureId);

    ~HdStTextureIdentifier();

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
    const HdStSubtextureIdentifier * GetSubtextureIdentifier() const {
        return _subtextureId.get();
    }

    bool operator==(const HdStTextureIdentifier &other) const;
    bool operator!=(const HdStTextureIdentifier &other) const;

private:
    TfToken _filePath;
    std::unique_ptr<const HdStSubtextureIdentifier> _subtextureId;
};

size_t hash_value(const HdStTextureIdentifier &);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
