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

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/image.h"
#include "pxr/imaging/glf/texture.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"

#include "pxr/imaging/garch/gl.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_AND_REF_PTRS(GlfBaseTexture);
TF_DECLARE_WEAK_AND_REF_PTRS(GlfBaseTextureData);

/// \class GlfBaseTexture
///
/// Represents a texture object in Glf
///
class GlfBaseTexture : public GlfTexture {
public:
    GLF_API
    virtual ~GlfBaseTexture();

    /// Returns the OpenGl texture name for the texture. 
    GLF_API
    GLuint GetGlTextureName();

    GLF_API
    int	GetWidth();

    GLF_API
    int GetHeight();

    GLF_API
    int GetFormat();

    // GlfTexture overrides
    GLF_API
    virtual BindingVector GetBindings(TfToken const & identifier,
                                      GLuint samplerName);
    GLF_API
    virtual VtDictionary GetTextureInfo(bool forceLoad);


protected:
    
    GLF_API
    GlfBaseTexture();

    GLF_API
    GlfBaseTexture(GlfImage::ImageOriginLocation originLocation);

    GLF_API
    virtual void _OnMemoryRequestedDirty() final;

    GLF_API
    virtual void _ReadTexture() = 0;

    GLF_API
    void _UpdateTexture(GlfBaseTextureDataConstPtr texData);
    GLF_API
    void _CreateTexture(GlfBaseTextureDataConstPtr texData,
                        bool const useMipmaps,
                        int const unpackCropTop = 0,
                        int const unpackCropBottom = 0,
                        int const unpackCropLeft = 0,
                        int const unpackCropRight = 0);

    GLF_API
    void _SetLoaded();

private:

    // GL texture object
    const GLuint _textureName;

    // required for stats/tracking
    bool    _loaded;
    int     _currentWidth, _currentHeight;
    int     _format;
    bool    _hasWrapModeS;
    bool    _hasWrapModeT;
    GLenum	_wrapModeS;
    GLenum	_wrapModeT;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // GLF_BASETEXTURE_H
