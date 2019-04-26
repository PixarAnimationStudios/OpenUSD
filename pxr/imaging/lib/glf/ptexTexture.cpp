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
/// \file glf/ptexTexture.cpp

// 

#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/ptexTexture.h"

#include "pxr/base/tf/stringUtils.h"


PXR_NAMESPACE_OPEN_SCOPE

bool GlfIsSupportedPtexTexture(std::string const & imageFilePath)
{
#ifdef PXR_PTEX_SUPPORT_ENABLED
    return (TfStringEndsWith(imageFilePath, ".ptx") || 
            TfStringEndsWith(imageFilePath, ".ptex"));
#else
    return false;
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE

#ifdef PXR_PTEX_SUPPORT_ENABLED

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/ptexMipmapTextureLoader.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/trace/trace.h"

#include <Ptexture.h>
#include <PtexUtils.h>

#include <string>
#include <vector>
#include <list>
#include <algorithm>

using std::string;
using namespace boost;

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    typedef GlfPtexTexture Type;
    TfType t = TfType::Define<Type, TfType::Bases<GlfTexture> >();
    t.SetFactory< GlfTextureFactory<Type> >();
}

//------------------------------------------------------------------------------
GlfPtexTextureRefPtr
GlfPtexTexture::New(const TfToken &imageFilePath)
{
    return TfCreateRefPtr(new GlfPtexTexture(imageFilePath));
}

//------------------------------------------------------------------------------
GlfPtexTexture::GlfPtexTexture(const TfToken &imageFilePath) :
    _loaded(false),
    _layout(0), _texels(0),
    _width(0), _height(0), _depth(0),
    _imageFilePath(imageFilePath)
{ 
}

//------------------------------------------------------------------------------
GlfPtexTexture::~GlfPtexTexture()
{ 
    _FreePtexTextureObject();
}

//------------------------------------------------------------------------------
void
GlfPtexTexture::_OnMemoryRequestedDirty()
{
    _loaded = false;
}

//------------------------------------------------------------------------------
bool 
GlfPtexTexture::_ReadImage()
{
    TRACE_FUNCTION();
    
    _FreePtexTextureObject( );

    const std::string & filename = _imageFilePath;

    GLint maxNumPages = 0;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxNumPages);


    TRACE_SCOPE("GlfPtexTexture::_ReadImage() (read ptex)");

    // create a temporary ptex cache
    // (required to build guttering pixels efficiently)
    static const int PTEX_MAX_CACHE_SIZE = 128*1024*1024;
    PtexCache *cache = PtexCache::create(1, PTEX_MAX_CACHE_SIZE);
    if (!cache) {
        TF_WARN("Unable to create PtexCache");
        return false;
    }

    // load
    Ptex::String ptexError;
    PtexTexture *reader = cache->get(filename.c_str(), ptexError);
    //PtexTexture *reader = PtexTexture::open(filename.c_str(), ptexError, true);
    if (!reader) {
        TF_WARN("Unable to open ptex %s : %s",
                filename.c_str(), ptexError.c_str());
        cache->release();
        return false;
    }

    // Read the ptexture data and pack the texels

    TRACE_SCOPE("GlfPtexTexture::_ReadImage() (generate texture)");
    size_t targetMemory = GetMemoryRequested();


    // maxLevels = -1 : load all mip levels
    // maxLevels = 0  : load only the highest resolution
    int maxLevels = -1;
    GlfPtexMipmapTextureLoader loader(reader,
                                      maxNumPages,
                                      maxLevels,
                                      targetMemory);

    {   // create & bind the GL texture array
        GLenum format, type;
        switch (reader->dataType())
        {
            case Ptex::dt_uint16 : type = GL_UNSIGNED_SHORT; break;
            case Ptex::dt_float  : type = GL_FLOAT; break;
            case Ptex::dt_half   : type = GL_HALF_FLOAT_ARB; break;
            default              : type = GL_UNSIGNED_BYTE; break;
        }

        int numChannels = reader->numChannels();
        switch (numChannels)
        { 
            case 1 : format = GL_LUMINANCE; break;
            case 2 : format = GL_LUMINANCE_ALPHA; break;
            case 3 : format = GL_RGB; break;
            case 4 : format = GL_RGBA; break;
            default: format = GL_LUMINANCE; break;
        }
        // 'type' and 'format' are texel format in the source ptex data (input)
        // '_format' is an internal format (GPU)
    
        _format = GL_RGBA8;
        if (type == GL_FLOAT) {
            static GLenum floatFormats[] =
                { GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F };
            _format = floatFormats[numChannels-1];
        } else if (type == GL_UNSIGNED_SHORT) {
            static GLenum uint16Formats[] =
                { GL_R16, GL_RG16, GL_RGB16, GL_RGBA16 };
            _format = uint16Formats[numChannels-1];
        } else if (type == GL_HALF_FLOAT_ARB) {
            static GLenum halfFormats[] =
                { GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F };
            _format = halfFormats[numChannels-1];
        } else {
            static GLenum uint8Formats[] =
                { GL_R8, GL_RG8, GL_RGB8, GL_RGBA8 };
            _format = uint8Formats[numChannels-1];
        }

        int numFaces = loader.GetNumFaces();

        // layout texture buffer

        // ptex layout struct (6 * uint16)
        // struct Layout {
        //     uint16_t page;
        //     uint16_t nMipmap;
        //     uint16_t u;
        //     uint16_t v;
        //     uint16_t adjSizeDiffs; //(4:4:4:4)
        //     uint8_t  width log2;
        //     uint8_t  height log2;
        // };

        glGenTextures(1, & _layout);
        GLuint layoutBuffer;
        glGenBuffers(1, & layoutBuffer );
        glBindBuffer( GL_TEXTURE_BUFFER, layoutBuffer );
        glBufferData( GL_TEXTURE_BUFFER, numFaces * 6 * sizeof(GLshort),
                                         loader.GetLayoutBuffer(),
                                         GL_STATIC_DRAW);
        glBindTexture( GL_TEXTURE_BUFFER, _layout);
        glTexBuffer( GL_TEXTURE_BUFFER, GL_R16I, layoutBuffer);
        glDeleteBuffers(1,&layoutBuffer);


        // actual texels texture array
        glGenTextures(1,&_texels);
        glBindTexture(GL_TEXTURE_2D_ARRAY,_texels);
        glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

        _width = loader.GetPageWidth();
        _height = loader.GetPageHeight();
        _depth = loader.GetNumPages();

        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0,
                     _format,
                     loader.GetPageWidth(),
                     loader.GetPageHeight(),
                     loader.GetNumPages(),
                     0, format, type,
                     loader.GetTexelBuffer());

        GLF_POST_PENDING_GL_ERRORS();
    }

    reader->release();

    _SetMemoryUsed(loader.GetMemoryUsage());

    // also releases PtexCache
    cache->release();

    _loaded = true;

    return true;
}

//------------------------------------------------------------------------------
void
GlfPtexTexture::_FreePtexTextureObject()
{
    GlfSharedGLContextScopeHolder sharedGLContextScopeHolder;

    // delete layout lookup --------------------------------
    if (glIsTexture(_layout))
       glDeleteTextures(1,&_layout);
       
    // delete textures lookup ------------------------------
    if (glIsTexture(_texels))
       glDeleteTextures(1,&_texels);
}

//------------------------------------------------------------------------------

/* virtual */
GlfTexture::BindingVector
GlfPtexTexture::GetBindings(TfToken const & identifier, GLuint samplerName)
{
    if (!_loaded) {
        _ReadImage();
    }

    BindingVector result;
    result.reserve(2);

    result.push_back(Binding(
        TfToken(identifier.GetString() + "_Data"), GlfTextureTokens->texels,
        GL_TEXTURE_2D_ARRAY, _texels, samplerName));

    // packing buffer doesn't need external sampler
    result.push_back(Binding(
        TfToken(identifier.GetString() + "_Packing"), GlfTextureTokens->layout,
        GL_TEXTURE_BUFFER, _layout, /*samplerId=*/0));

    return result;
}

//------------------------------------------------------------------------------

VtDictionary
GlfPtexTexture::GetTextureInfo(bool forceLoad)
{
    if (!_loaded && forceLoad) {
        _ReadImage();
    }

    VtDictionary info;

    info["memoryUsed"] = GetMemoryUsed();
    info["width"] = (int)_width;
    info["height"] = (int)_height;
    info["depth"] = (int)_depth;
    info["format"] = (int)_format;
    info["imageFilePath"] = _imageFilePath;
    info["referenceCount"] = GetCurrentCount();

    return info;
}

bool
GlfPtexTexture::IsMinFilterSupported(GLenum filter)
{
    switch(filter) {
    case GL_NEAREST:
    case GL_LINEAR:
        return true;
    default:
        return false;
    }
}

bool
GlfPtexTexture::IsMagFilterSupported(GLenum filter)
{
    switch(filter) {
    case GL_NEAREST:
    case GL_LINEAR:
        return true;
    default:
        return false;
    }
}

GLuint
GlfPtexTexture::GetLayoutTextureName()
{
    if (!_loaded) {
        _ReadImage();
    }

    return _layout;
}

GLuint
GlfPtexTexture::GetTexelsTextureName()
{
    if (!_loaded) {
        _ReadImage();
    }

    return _texels;
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_PTEX_SUPPORT_ENABLED


