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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/textureResource.h"

#include "pxr/imaging/hd/conversions.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/glf/baseTexture.h"
#include "pxr/imaging/glf/ptexTexture.h"

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((fallbackPtexPath, "PtExNoNsEnSe"))
    ((fallbackUVPath, "UvNoNsEnSe"))
);

HdTextureResource::~HdTextureResource()
{
}

/* static */
HdTextureResource::ID
HdTextureResource::ComputeHash(TfToken const &sourceFile)
{
    HD_TRACE_FUNCTION();

    uint32_t hash = 0;
    std::string const &filename = sourceFile.GetString();
    hash = ArchHash(filename.c_str(), filename.size(), hash);

    return hash;
}

/* static */
HdTextureResource::ID
HdTextureResource::ComputeFallbackPtexHash()
{
    HD_TRACE_FUNCTION();

    uint32_t hash = 0;
    std::string const &filename = _tokens->fallbackPtexPath.GetString();
    hash = ArchHash(filename.c_str(), filename.size(), hash);

    return hash;
}

/* static */
HdTextureResource::ID
HdTextureResource::ComputeFallbackUVHash()
{
    HD_TRACE_FUNCTION();

    uint32_t hash = 0;
    std::string const &filename = _tokens->fallbackUVPath.GetString();
    hash = ArchHash(filename.c_str(), filename.size(), hash);

    return hash;
}

// HdSimpleTextureResource implementation

HdSimpleTextureResource::HdSimpleTextureResource(
    GlfTextureHandleRefPtr const &textureHandle, bool isPtex):
        HdSimpleTextureResource(textureHandle, isPtex, 
        /*wrapS*/ HdWrapRepeat, /*wrapT*/ HdWrapRepeat, 
        /*minFilter*/ HdMinFilterNearestMipmapLinear, 
        /*magFilter*/ HdMagFilterLinear)
{
}

HdSimpleTextureResource::HdSimpleTextureResource(
    GlfTextureHandleRefPtr const &textureHandle, bool isPtex, 
        HdWrap wrapS, HdWrap wrapT, 
        HdMinFilter minFilter, HdMagFilter magFilter)
            : _textureHandle(textureHandle)
            , _texture(textureHandle->GetTexture())
            , _sampler(0)
            , _isPtex(isPtex)
{
    if (not glGenSamplers) { // GL initialization guard for headless unit test
        return;
    }

    // When we are not using Ptex we will use samplers,
    // that includes both, bindless textures and no-bindless textures
    if (not _isPtex) {
        glGenSamplers(1, &_sampler);
        glSamplerParameteri(_sampler, GL_TEXTURE_WRAP_S, HdConversions::GetWrap(wrapS));
        glSamplerParameteri(_sampler, GL_TEXTURE_WRAP_T, HdConversions::GetWrap(wrapT));
        glSamplerParameteri(_sampler, GL_TEXTURE_MIN_FILTER, HdConversions::GetMinFilter(minFilter));
        glSamplerParameteri(_sampler, GL_TEXTURE_MAG_FILTER, HdConversions::GetMagFilter(magFilter));
    }

    bool bindlessTexture = 
        HdRenderContextCaps::GetInstance().bindlessTextureEnabled;
    if (bindlessTexture) {
        size_t handle = GetTexelsTextureHandle();
        if (handle) {
            if (not glIsTextureHandleResidentNV(handle)) {
                glMakeTextureHandleResidentNV(handle);
            }
        }

        if (_isPtex) {
            handle = GetLayoutTextureHandle();
            if (handle) {
                if (not glIsTextureHandleResidentNV(handle)) {
                    glMakeTextureHandleResidentNV(handle);
                }
            }
        }
    }
}

HdSimpleTextureResource::~HdSimpleTextureResource() 
{ 
    if (not _isPtex) {
        if (not glDeleteSamplers) { // GL initialization guard for headless unit test
            return;
        }
        glDeleteSamplers(1, &_sampler);
    }
}

bool HdSimpleTextureResource::IsPtex() const 
{ 
    return _isPtex; 
}

GLuint HdSimpleTextureResource::GetTexelsTextureId() 
{
    if (_isPtex) {
        return TfDynamic_cast<GlfPtexTextureRefPtr>(_texture)->GetTexelsTextureName();
    }

    return TfDynamic_cast<GlfBaseTextureRefPtr>(_texture)->GetGlTextureName();
}

GLuint HdSimpleTextureResource::GetTexelsSamplerId() 
{
    return _sampler;
}

GLuint64EXT HdSimpleTextureResource::GetTexelsTextureHandle() 
{ 
    GLuint textureId = GetTexelsTextureId();
    GLuint samplerId = GetTexelsSamplerId();

    if (not TF_VERIFY(glGetTextureHandleARB) or
        not TF_VERIFY(glGetTextureSamplerHandleARB)) {
        return 0;
    }

    if (_isPtex) {
        return textureId ? glGetTextureHandleARB(textureId) : 0;
    } 

    return textureId ? glGetTextureSamplerHandleARB(textureId, samplerId) : 0;
}

GLuint HdSimpleTextureResource::GetLayoutTextureId() 
{
    return TfDynamic_cast<GlfPtexTextureRefPtr>(_texture)->GetLayoutTextureName();
}

GLuint64EXT HdSimpleTextureResource::GetLayoutTextureHandle() 
{
    if (not TF_VERIFY(_isPtex)) {
        return 0;
    }
    
    if (not TF_VERIFY(glGetTextureHandleARB)) {
        return 0;
    }

    GLuint textureId = GetLayoutTextureId();

    return textureId ? glGetTextureHandleARB(textureId) : 0;
}
