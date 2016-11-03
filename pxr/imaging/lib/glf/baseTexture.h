//
// Copyright 2016 Pixar
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
#ifndef GLF_BASETEXTURE_H
#define GLF_BASETEXTURE_H

/// \file glf/uvTexture.h

#include "pxr/imaging/glf/texture.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"

#include "pxr/imaging/garch/gl.h"

#include <string>

TF_DECLARE_WEAK_AND_REF_PTRS(GlfBaseTexture);
TF_DECLARE_WEAK_AND_REF_PTRS(GlfBaseTextureData);

/// \class GlfBaseTexture
///
/// Represents a texture object in Glf
///
class GlfBaseTexture : public GlfTexture {
public:
    virtual ~GlfBaseTexture();

    /// Returns the OpenGl texture name for the texture. 
    GLuint GetGlTextureName() const {
        return _textureName;
    }

    int	GetWidth() const {
        return _currentWidth;
    }

    int GetHeight() const {
        return _currentHeight;
    }

    int GetFormat() const {
        return _format;
    }

    // GlfTexture overrides
    virtual BindingVector GetBindings(TfToken const & identifier,
                                      GLuint samplerName) const;
    virtual VtDictionary GetTextureInfo() const;

protected:
    
    GlfBaseTexture();

    void _UpdateTexture(GlfBaseTextureDataConstPtr texData);
    void _CreateTexture(GlfBaseTextureDataConstPtr texData,
                        bool const useMipmaps,
                        int const unpackCropTop = 0,
                        int const unpackCropBottom = 0,
                        int const unpackCropLeft = 0,
                        int const unpackCropRight = 0);

private:

    // GL texture object
    const GLuint _textureName;

    // required for stats/tracking
    int     _currentWidth, _currentHeight;
    int     _format;
    bool    _hasWrapModeS;
    bool    _hasWrapModeT;
    GLenum	_wrapModeS;
    GLenum	_wrapModeT;
};

#endif // GLF_BASETEXTURE_H
