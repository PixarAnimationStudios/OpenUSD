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

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE


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

GlfBaseTexture::GlfBaseTexture(GlfImage::ImageOriginLocation originLocation)
  : GlfTexture(originLocation),
    _textureName(_GenName()),
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
    if (texData && texData->HasRawBuffer()) {
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
                bool const useMipmaps,
                int const unpackCropTop,
                int const unpackCropBottom,
                int const unpackCropLeft,
                int const unpackCropRight)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (texData && texData->HasRawBuffer()) {
        glBindTexture(GL_TEXTURE_2D, _textureName);

        // Check if mip maps have been requested, if so, it will either
        // enable automatic generation or use the ones loaded in cpu memory
        int numMipLevels = 1;

        if (useMipmaps) {
            numMipLevels = texData->GetNumMipLevels();
            
            // When we are using uncompressed textures and late cropping
            // we won't use cpu loaded mips.
            if (!texData->IsCompressed() &&
                (unpackCropRight || unpackCropLeft ||
                unpackCropTop || unpackCropBottom)) {
                    numMipLevels = 1;
            }
            if (numMipLevels > 1) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numMipLevels-1);
            } else {
                glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
            }
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
        }

        if (texData->IsCompressed()) {
            // Compressed textures don't have as many options, so 
            // we just need to send the mips to the driver.
            for (int i = 0 ; i < numMipLevels; i++) {
                glCompressedTexImage2D( GL_TEXTURE_2D, i,
                                texData->GLInternalFormat(),
                                texData->ResizedWidth(i),
                                texData->ResizedHeight(i),
                                0,
                                texData->ComputeBytesUsedByMip(i),
                                texData->GetRawBuffer(i));
            }
        } else {
            // Uncompressed textures can have cropping and other special 
            // behaviours.
            if (GlfGetNumElements(texData->GLFormat()) == 1) {
                GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
                glTexParameteriv(
                    GL_TEXTURE_2D,
                    GL_TEXTURE_SWIZZLE_RGBA, 
                    swizzleMask);
            }

            // If we are not sending full mipchains to the gpu then we can 
            // do some extra work in the driver to prepare our textures.
            if (numMipLevels == 1) {
                int texDataWidth = texData->ResizedWidth();
                int texDataHeight = texData->ResizedHeight();
                int unpackRowLength = texDataWidth;
                int unpackSkipPixels = 0;
                int unpackSkipRows = 0;

                if (unpackCropTop < 0 || unpackCropTop > texDataHeight) {
                    return;
                } else if (unpackCropTop > 0) {
                    unpackSkipRows = unpackCropTop;
                    texDataHeight -= unpackCropTop;
                }
                if (unpackCropBottom < 0 || unpackCropBottom > texDataHeight) {
                    return;
                } else if (unpackCropBottom) {
                    texDataHeight -= unpackCropBottom;
                }
                if (unpackCropLeft < 0 || unpackCropLeft > texDataWidth) {
                    return;
                } else {
                    unpackSkipPixels = unpackCropLeft;
                    texDataWidth -= unpackCropLeft;
                }
                if (unpackCropRight < 0 || unpackCropRight > texDataWidth) {
                    return;
                } else if (unpackCropRight > 0) {
                    texDataWidth -= unpackCropRight;
                }

                glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, unpackRowLength);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glPixelStorei(GL_UNPACK_SKIP_PIXELS, unpackSkipPixels);
                glPixelStorei(GL_UNPACK_SKIP_ROWS, unpackSkipRows);

                // Send the mip to the driver now
                glTexImage2D( GL_TEXTURE_2D, 0,
                                texData->GLInternalFormat(),
                                texDataWidth,
                                texDataHeight,
                                0,
                                texData->GLFormat(),
                                texData->GLType(),
                                texData->GetRawBuffer(0));

                // Reset the OpenGL state if we have modify it previously
                glPopClientAttrib();
            } else {
                // Send the mips to the driver now
                for (int i = 0 ; i < numMipLevels; i++) {
                    glTexImage2D( GL_TEXTURE_2D, i,
                                    texData->GLInternalFormat(),
                                    texData->ResizedWidth(i),
                                    texData->ResizedHeight(i),
                                    0,
                                    texData->GLFormat(),
                                    texData->GLType(),
                                    texData->GetRawBuffer(i));
                }
            }
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        _SetMemoryUsed(texData->ComputeBytesUsed());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

