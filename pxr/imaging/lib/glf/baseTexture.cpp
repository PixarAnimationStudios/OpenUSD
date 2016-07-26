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
/// \file baseTexture.cpp
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/baseTexture.h"
#include "pxr/imaging/glf/baseTextureData.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/utils.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tracelite/trace.h"

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<GlfBaseTexture, TfType::Bases<GlfTexture> >();
}

static GLuint
_GenName()
{
    GLuint rval(0);

    glGenTextures(1, &rval);

    return rval;
}

GlfBaseTexture::GlfBaseTexture()
  : _textureName(_GenName()),
    _currentWidth(0),
    _currentHeight(0),
    _format(GL_RGBA),
    _hasWrapModeS(false),
    _hasWrapModeT(false),
    _wrapModeS(GL_REPEAT),
    _wrapModeT(GL_REPEAT)
{
    /* nothing */
}

GlfBaseTexture::~GlfBaseTexture()
{
    GlfSharedGLContextScopeHolder sharedGLContextScopeHolder;

    if (glIsTexture(_textureName)) {
        glDeleteTextures(1, &_textureName);
    }
}

/* virtual */
GlfTexture::BindingVector
GlfBaseTexture::GetBindings(TfToken const & identifier,
                             GLuint samplerName) const
{
    return BindingVector(1,
                Binding(identifier, GlfTextureTokens->texels,
                        GL_TEXTURE_2D, _textureName, samplerName));
}

VtDictionary
GlfBaseTexture::GetTextureInfo() const
{
    VtDictionary info;
    info["memoryUsed"] = GetMemoryUsed();
    info["width"] = _currentWidth;
    info["height"] = _currentHeight;
    info["depth"] = 1;
    info["format"] = _format;
    info["referenceCount"] = GetRefCount().Get();

    if (_hasWrapModeS)
        info["wrapModeS"] = _wrapModeS;

    if (_hasWrapModeT)
        info["wrapModeT"] = _wrapModeT;

    return info;
}

void 
GlfBaseTexture::_UpdateTexture(GlfBaseTextureDataConstPtr texData)
{
    // Copy or clear fields required for tracking/reporting.
    if (texData and texData->HasRawBuffer()) {
        _currentWidth  = texData->ResizedWidth();
        _currentHeight = texData->ResizedHeight();
        _format        = texData->GLFormat();
        _hasWrapModeS  = texData->GetWrapInfo().hasWrapModeS;
        _hasWrapModeT  = texData->GetWrapInfo().hasWrapModeT;
        _wrapModeS     = texData->GetWrapInfo().wrapModeS;
        _wrapModeT     = texData->GetWrapInfo().wrapModeT;        

        _SetMemoryUsed(texData->ComputeBytesUsed());

    } else {
        _currentWidth  = _currentHeight = 0;
        _format        =  GL_RGBA;
        _hasWrapModeS  = _hasWrapModeT  = false;
        _wrapModeS     = _wrapModeT     = GL_REPEAT;

        _SetMemoryUsed(0);
    }
}

void 
GlfBaseTexture::_CreateTexture(GlfBaseTextureDataConstPtr texData,
                                bool const generateMipmap,
				int const unpackCropTop,
				int const unpackCropBottom,
				int const unpackCropLeft,
				int const unpackCropRight)
{
    TRACE_FUNCTION();
    
    if (texData and texData->HasRawBuffer()) {
        glBindTexture(
                GL_TEXTURE_2D,
               _textureName);

        glTexParameteri(
                GL_TEXTURE_2D,
                GL_GENERATE_MIPMAP,
                generateMipmap ? GL_TRUE
                               : GL_FALSE);

        if (not texData->IsCompressed()) {
            if (GlfGetNumElements(texData->GLFormat()) == 1) {
                GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
                glTexParameteriv(
                    GL_TEXTURE_2D,
                    GL_TEXTURE_SWIZZLE_RGBA, 
                    swizzleMask);
            }

            int texDataWidth = texData->ResizedWidth();
            int texDataHeight = texData->ResizedHeight();

            int unpackRowLength = texDataWidth;
            int unpackSkipPixels = 0;
            int unpackSkipRows = 0;

            if (unpackCropTop < 0 or unpackCropTop > texDataHeight) {
                return;
            } else if (unpackCropTop > 0) {
                unpackSkipRows = unpackCropTop;
                texDataHeight -= unpackCropTop;
            }
            if (unpackCropBottom < 0 or unpackCropBottom > texDataHeight) {
                return;
            } else if (unpackCropBottom) {
                texDataHeight -= unpackCropBottom;
            }
            if (unpackCropLeft < 0 or unpackCropLeft > texDataWidth) {
                return;
            } else {
                unpackSkipPixels = unpackCropLeft;
                texDataWidth -= unpackCropLeft;
            }
            if (unpackCropRight < 0 or unpackCropRight > texDataWidth) {
                return;
            } else if (unpackCropRight > 0) {
                texDataWidth -= unpackCropRight;
            }
        
            glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, unpackRowLength);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_SKIP_PIXELS, unpackSkipPixels);
            glPixelStorei(GL_UNPACK_SKIP_ROWS, unpackSkipRows);

            glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    texData->GLInternalFormat(),
                    texDataWidth,
                    texDataHeight,
                    0,
                    texData->GLFormat(),
                    texData->GLType(),
                    texData->GetRawBuffer());

            glPopClientAttrib();
        } else {
            // There should be no cropping when using compressed textures
            TF_VERIFY(unpackCropTop == 0 && unpackCropBottom == 0 &&
                      unpackCropLeft == 0 && unpackCropRight == 0);
            
            glCompressedTexImage2D(
                GL_TEXTURE_2D,
                0,
                texData->GLInternalFormat(),
                texData->ResizedWidth(),
                texData->ResizedHeight(),
                0,
                texData->ComputeBytesUsed(),
                texData->GetRawBuffer());
        }

        glBindTexture(
                GL_TEXTURE_2D,
                0);

        _SetMemoryUsed(texData->ComputeBytesUsed());
    }
}
