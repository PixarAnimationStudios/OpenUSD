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
#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/imaging/hdSt/textureResource.h"
#include "pxr/imaging/hdSt/glConversions.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/glf/baseTexture.h"
#include "pxr/imaging/glf/ptexTexture.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStTextureResource::~HdStTextureResource()
{
    // nothing
}


HdStSimpleTextureResource::HdStSimpleTextureResource(
                                    GlfTextureHandleRefPtr const &textureHandle,
                                    bool isPtex,
                                    HdWrap wrapS,
                                    HdWrap wrapT,
                                    HdMinFilter minFilter,
                                    HdMagFilter magFilter,
                                    size_t memoryRequest)
 : HdStTextureResource()
 , _textureHandle(textureHandle)
 , _texture()
 , _borderColor(0.0,0.0,0.0,0.0)
 , _maxAnisotropy(16.0)
 , _sampler(0)
 , _isPtex(isPtex)
 , _memoryRequest(memoryRequest)
 , _wrapS(wrapS)
 , _wrapT(wrapT)
 , _minFilter(minFilter)
 , _magFilter(magFilter)
{
    // In cases of upstream errors, texture handle can be null.
    if (_textureHandle) {
        _texture = _textureHandle->GetTexture();

        // Unconditionally add the memory request, before the early function
        // exit so that the destructor doesn't need to figure out if the request
        // was added or not.
        _textureHandle->AddMemoryRequest(_memoryRequest);
    }

    bool bindlessTexture = 
        GlfContextCaps::GetInstance().bindlessTextureEnabled;
    if (bindlessTexture) {
        size_t handle = GetTexelsTextureHandle();
        if (handle) {
            if (!glIsTextureHandleResidentNV(handle)) {
                glMakeTextureHandleResidentNV(handle);
            }
        }

        if (_isPtex) {
            handle = GetLayoutTextureHandle();
            if (handle) {
                if (!glIsTextureHandleResidentNV(handle)) {
                    glMakeTextureHandleResidentNV(handle);
                }
            }
        }
    }
}

HdStSimpleTextureResource::~HdStSimpleTextureResource() 
{ 
    if (_textureHandle) {
        _textureHandle->DeleteMemoryRequest(_memoryRequest);
    }

    if (!_isPtex) {
        if (!glDeleteSamplers) { // GL initialization guard for headless unit test
            return;
        }
        glDeleteSamplers(1, &_sampler);
    }
}

bool HdStSimpleTextureResource::IsPtex() const 
{ 
    return _isPtex; 
}

GLuint HdStSimpleTextureResource::GetTexelsTextureId() 
{
    if (_isPtex) {
#ifdef PXR_PTEX_SUPPORT_ENABLED
        GlfPtexTextureRefPtr ptexTexture =
                                 TfDynamic_cast<GlfPtexTextureRefPtr>(_texture);

        if (ptexTexture) {
            return ptexTexture->GetTexelsTextureName();
        }
        return 0;
#else
        TF_CODING_ERROR("Ptex support is disabled.  "
            "This code path should be unreachable");
        return 0;
#endif
    }

    GlfBaseTextureRefPtr baseTexture =
                             TfDynamic_cast<GlfBaseTextureRefPtr>(_texture);

    if (baseTexture) {
        return baseTexture->GetGlTextureName();
    }
    return 0;
}

GLuint HdStSimpleTextureResource::GetTexelsSamplerId() 
{
    if (!TF_VERIFY(!_isPtex)) {
        return 0;
    }

    // Lazy sampler creation.
    if (_sampler == 0) {
        // If the HdStSimpleTextureResource defines a wrap mode it will
        // use it, otherwise it gives an opportunity to the texture to define
        // its own wrap mode. The fallback value is always HdWrapRepeat
        GLenum fwrapS = HdStGLConversions::GetWrap(_wrapS);
        GLenum fwrapT = HdStGLConversions::GetWrap(_wrapT);
        GLenum fminFilter = HdStGLConversions::GetMinFilter(_minFilter);
        GLenum fmagFilter = HdStGLConversions::GetMagFilter(_magFilter);

        if (_texture) {
            VtDictionary txInfo = _texture->GetTextureInfo(true);

            if (_wrapS == HdWrapUseMetaDict &&
                VtDictionaryIsHolding<GLuint>(txInfo, "wrapModeS")) {
                fwrapS = VtDictionaryGet<GLuint>(txInfo, "wrapModeS");
            }

            if (_wrapT == HdWrapUseMetaDict &&
                VtDictionaryIsHolding<GLuint>(txInfo, "wrapModeT")) {
                fwrapT = VtDictionaryGet<GLuint>(txInfo, "wrapModeT");
            }

            if (!_texture->IsMinFilterSupported(fminFilter)) {
                fminFilter = GL_NEAREST;
            }

            if (!_texture->IsMagFilterSupported(fmagFilter)) {
                fmagFilter = GL_NEAREST;
            }
        }

        glGenSamplers(1, &_sampler);
        glSamplerParameteri(_sampler, GL_TEXTURE_WRAP_S, fwrapS);
        glSamplerParameteri(_sampler, GL_TEXTURE_WRAP_T, fwrapT);
        glSamplerParameteri(_sampler, GL_TEXTURE_MIN_FILTER, fminFilter);
        glSamplerParameteri(_sampler, GL_TEXTURE_MAG_FILTER, fmagFilter);
        glSamplerParameterf(_sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT,
            _maxAnisotropy);
        glSamplerParameterfv(_sampler, GL_TEXTURE_BORDER_COLOR,
            _borderColor.GetArray());
    }


    return _sampler;
}

GLuint64EXT HdStSimpleTextureResource::GetTexelsTextureHandle() 
{ 
    GLuint textureId = GetTexelsTextureId();

    if (!TF_VERIFY(glGetTextureHandleARB) ||
        !TF_VERIFY(glGetTextureSamplerHandleARB)) {
        return 0;
    }

    if (_isPtex) {
        return textureId ? glGetTextureHandleARB(textureId) : 0;
    } 

    GLuint samplerId = GetTexelsSamplerId();
    return textureId ? glGetTextureSamplerHandleARB(textureId, samplerId) : 0;
}

GLuint HdStSimpleTextureResource::GetLayoutTextureId() 
{
#ifdef PXR_PTEX_SUPPORT_ENABLED
    GlfPtexTextureRefPtr ptexTexture =
                                 TfDynamic_cast<GlfPtexTextureRefPtr>(_texture);

    if (ptexTexture) {
        return ptexTexture->GetLayoutTextureName();
    }
    return 0;
#else
    TF_CODING_ERROR("Ptex support is disabled.  "
        "This code path should be unreachable");
    return 0;
#endif
}

GLuint64EXT HdStSimpleTextureResource::GetLayoutTextureHandle() 
{
    if (!TF_VERIFY(_isPtex)) {
        return 0;
    }
    
    if (!TF_VERIFY(glGetTextureHandleARB)) {
        return 0;
    }

    GLuint textureId = GetLayoutTextureId();

    return textureId ? glGetTextureHandleARB(textureId) : 0;
}

size_t HdStSimpleTextureResource::GetMemoryUsed()
{
    if (_texture) {
        return _texture->GetMemoryUsed();
    } else  {
        return 0;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

