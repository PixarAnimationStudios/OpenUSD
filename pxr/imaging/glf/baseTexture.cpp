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
    _loaded(false),
    _currentWidth(0),
    _currentHeight(0),
    // 1 since a 2d-texture can be thought of as x*y*1 3d-texture
    _currentDepth(1),
    _format(GL_RGBA),
    _hasWrapModeS(false),
    _hasWrapModeT(false),
    _hasWrapModeR(false),
    _wrapModeS(GL_REPEAT),
    _wrapModeT(GL_REPEAT),
    _wrapModeR(GL_REPEAT)
{
    /* nothing */
}

GlfBaseTexture::GlfBaseTexture(GlfImage::ImageOriginLocation originLocation)
  : GlfTexture(originLocation),
    _textureName(_GenName()),
    _loaded(false),
    _currentWidth(0),
    _currentHeight(0),
    // 1 since a 2d-texture can be thought of as x*y*1 3d-texture
    _currentDepth(1),
    _format(GL_RGBA),
    _hasWrapModeS(false),
    _hasWrapModeT(false),
    _hasWrapModeR(false),
    _wrapModeS(GL_REPEAT),
    _wrapModeT(GL_REPEAT),
    _wrapModeR(GL_REPEAT)
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

void
GlfBaseTexture::_ReadTextureIfNotLoaded()
{
    if (!_loaded) {
        _ReadTexture();
    }
}

GLuint
GlfBaseTexture::GetGlTextureName()
{
    _ReadTextureIfNotLoaded();

    return _textureName;
}

int
GlfBaseTexture::GetWidth()
{
    _ReadTextureIfNotLoaded();

    return _currentWidth;
}

int
GlfBaseTexture::GetHeight()
{
    _ReadTextureIfNotLoaded();

    return _currentHeight;
}

int
GlfBaseTexture::GetDepth()
{
    _ReadTextureIfNotLoaded();

    return _currentDepth;
}

int
GlfBaseTexture::GetFormat()
{
    _ReadTextureIfNotLoaded();

    return _format;
}

static
GLenum
_NumDimensionsToGlTextureTarget(const int d)
{
    switch(d) {
    case 1:
        return GL_TEXTURE_1D;
    case 2:
        return GL_TEXTURE_2D;
    case 3:
        return GL_TEXTURE_3D;
    default:
        TF_CODING_ERROR("Bad dimension for texture: %d", d);
        return GL_TEXTURE_2D;
    }
}

/* virtual */
GlfTexture::BindingVector
GlfBaseTexture::GetBindings(TfToken const & identifier,
                             GLuint samplerName)
{
    _ReadTextureIfNotLoaded();

    return BindingVector(1,
                Binding(
                    identifier, GlfTextureTokens->texels,
                    _NumDimensionsToGlTextureTarget(GetNumDimensions()),
                    _textureName, samplerName));
}

VtDictionary
GlfBaseTexture::GetTextureInfo(bool forceLoad)
{
    VtDictionary info;

    if (forceLoad) {
        _ReadTextureIfNotLoaded();
    }

    if (_loaded) {
        info["memoryUsed"] = GetMemoryUsed();
        info["width"] = _currentWidth;
        info["height"] = _currentHeight;
        info["depth"] = _currentDepth;
        info["format"] = _format;

        if (_hasWrapModeS) {
            info["wrapModeS"] = _wrapModeS;
        }

        if (_hasWrapModeT) {
            info["wrapModeT"] = _wrapModeT;
        }

        if (_hasWrapModeR) {
            info["wrapModeR"] = _wrapModeR;
        }
    } else {
        info["memoryUsed"] = (size_t)0;
        info["width"] = 0;
        info["height"] = 0;
        info["depth"] = 1;
        info["format"] = _format;
    }
    info["referenceCount"] = GetCurrentCount();

    return info;
}

void
GlfBaseTexture::_OnMemoryRequestedDirty()
{
    _loaded = false;
}

void 
GlfBaseTexture::_UpdateTexture(GlfBaseTextureDataConstPtr texData)
{
    // Copy or clear fields required for tracking/reporting.
    if (texData && texData->HasRawBuffer()) {
        _currentWidth  = texData->ResizedWidth();
        _currentHeight = texData->ResizedHeight();
        _currentDepth  = texData->ResizedDepth();
        _format        = texData->GLFormat();
        _hasWrapModeS  = texData->GetWrapInfo().hasWrapModeS;
        _hasWrapModeT  = texData->GetWrapInfo().hasWrapModeT;
        _hasWrapModeR  = texData->GetWrapInfo().hasWrapModeR;
        _wrapModeS     = texData->GetWrapInfo().wrapModeS;
        _wrapModeT     = texData->GetWrapInfo().wrapModeT;
        _wrapModeR     = texData->GetWrapInfo().wrapModeR;

        _SetMemoryUsed(texData->ComputeBytesUsed());

    } else {
        _currentWidth  = _currentHeight = 0;
        _currentDepth  = 1;
        _format        =  GL_RGBA;
        _hasWrapModeS  = _hasWrapModeT  = _hasWrapModeR = false;
        _wrapModeS     = _wrapModeT     = _wrapModeR    = GL_REPEAT;

        _SetMemoryUsed(0);
    }
}

static
void _GlTexImageND(const int numDimensions,
                   const GLenum target,
                   const GLint level,
                   const GLint internalformat,
                   const GLsizei width,
                   const GLsizei height,
                   const GLsizei depth,
                   const GLint border,
                   const GLenum format,
                   const GLenum type,
                   const GLvoid* data)
{
    switch(numDimensions) {
    case 1:
        glTexImage1D(target, level, internalformat,
                     width,
                     border, format, type, data);
        break;
    case 2:
        glTexImage2D(target, level, internalformat,
                     width, height,
                     border, format, type, data);
        break;
    case 3:
        glTexImage3D(target, level, internalformat,
                     width, height, depth,
                     border, format, type, data);
        break;
    default:
        TF_CODING_ERROR("Bad dimension for OpenGL texture %d", numDimensions);
    }
}

static
void _GlCompressedTexImageND(const int numDimensions,
                             const GLenum target,
                             const GLint level,
                             const GLint internalformat,
                             const GLsizei width,
                             const GLsizei height,
                             const GLsizei depth,
                             const GLint border,
                             const GLsizei imageSize,
                             const GLvoid* data)
{
    switch(numDimensions) {
    case 1:
        glCompressedTexImage1D(target, level, internalformat,
                               width,
                               border, imageSize, data);
        break;
    case 2:
        glCompressedTexImage2D(target, level, internalformat,
                               width, height,
                               border, imageSize, data);
        break;
    case 3:
        glCompressedTexImage3D(target, level, internalformat,
                               width, height, depth,
                               border, imageSize, data);
        break;
    default:
        TF_CODING_ERROR("Bad dimension for OpenGL texture %d", numDimensions);
    }
}

void 
GlfBaseTexture::_CreateTexture(GlfBaseTextureDataConstPtr texData,
                               bool const useMipmaps,
                               int const unpackCropTop,
                               int const unpackCropBottom,
                               int const unpackCropLeft,
                               int const unpackCropRight,
                               int const unpackCropFront,
                               int const unpackCropBack)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (texData && texData->HasRawBuffer()) {
        const int numDimensions = GetNumDimensions();

        if (texData->NumDimensions() != numDimensions) {
            TF_CODING_ERROR("Dimension mismatch %d != %d between "
                            "GlfBaseTextureData and GlfBaseTexture",
                            texData->NumDimensions(), numDimensions);
            return;
        }

        // GL_TEXTURE_1D, GL_TEXTURE_2D, or GL_TEXTURE_3d
        const GLenum textureTarget = _NumDimensionsToGlTextureTarget(numDimensions);

        glBindTexture(textureTarget, _textureName);

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
                glTexParameteri(textureTarget, GL_TEXTURE_MAX_LEVEL, numMipLevels-1);
            } else {
                glTexParameteri(textureTarget, GL_GENERATE_MIPMAP, GL_TRUE);
            }
        } else {
            glTexParameteri(textureTarget, GL_GENERATE_MIPMAP, GL_FALSE);
        }

        if (texData->IsCompressed()) {
            // Compressed textures don't have as many options, so 
            // we just need to send the mips to the driver.
            for (int i = 0 ; i < numMipLevels; i++) {
                _GlCompressedTexImageND(
                    numDimensions,
                    textureTarget, i,
                    texData->GLInternalFormat(),
                    texData->ResizedWidth(i),
                    texData->ResizedHeight(i),
                    texData->ResizedDepth(i),
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
                    textureTarget,
                    GL_TEXTURE_SWIZZLE_RGBA, 
                    swizzleMask);
            }

            // If we are not sending full mipchains to the gpu then we can 
            // do some extra work in the driver to prepare our textures.
            if (numMipLevels == 1) {
                const int width = texData->ResizedWidth();
                const int height = texData->ResizedHeight();
                const int depth = texData->ResizedDepth();

                int croppedWidth  = width;
                int croppedHeight = height;
                int croppedDepth  = depth;

                if (unpackCropLeft < 0 || unpackCropLeft > croppedWidth) {
                    return;
                }

                croppedWidth -= unpackCropLeft;

                if (unpackCropRight < 0 || unpackCropRight > croppedWidth) {
                    return;
                }

                croppedWidth -= unpackCropRight;

                if (unpackCropTop < 0 || unpackCropTop > croppedHeight) {
                    return;
                }

                croppedHeight -= unpackCropTop;

                if (unpackCropBottom < 0 || unpackCropBottom > croppedHeight) {
                    return;
                }

                croppedHeight -= unpackCropBottom;

                if (unpackCropFront < 0 || unpackCropFront > croppedDepth) {
                    return;
                }

                croppedDepth -= unpackCropFront;

                if (unpackCropBack < 0 || unpackCropBack > croppedDepth) {
                    return;
                }

                croppedDepth -= unpackCropBack;
                
                glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

                {
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    glPixelStorei(GL_UNPACK_SKIP_PIXELS, unpackCropLeft);
                }

                if (numDimensions >= 2) {
                    glPixelStorei(GL_UNPACK_SKIP_ROWS, unpackCropTop);
                    glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
                }

                if (numDimensions >= 3) {
                    glPixelStorei(GL_UNPACK_SKIP_IMAGES, unpackCropFront);
                    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, height);
                }

                // Send the mip to the driver now
                _GlTexImageND(numDimensions,
                              textureTarget, 0,
                              texData->GLInternalFormat(),
                              croppedWidth,
                              croppedHeight,
                              croppedDepth,
                              0,
                              texData->GLFormat(),
                              texData->GLType(),
                              texData->GetRawBuffer(0));
                
                // Reset the OpenGL state if we have modify it previously
                glPopClientAttrib();
            } else {
                // Send the mips to the driver now
                for (int i = 0 ; i < numMipLevels; i++) {
                    _GlTexImageND(numDimensions,
                                  textureTarget, i,
                                  texData->GLInternalFormat(),
                                  texData->ResizedWidth(i),
                                  texData->ResizedHeight(i),
                                  texData->ResizedDepth(i),
                                  0,
                                  texData->GLFormat(),
                                  texData->GLType(),
                                  texData->GetRawBuffer(i));
                }
            }
        }

        glBindTexture(textureTarget, 0);
        _SetMemoryUsed(texData->ComputeBytesUsed());
    }
}

GLF_API
void GlfBaseTexture::_SetLoaded()
{
    _loaded = true;
}


PXR_NAMESPACE_CLOSE_SCOPE

