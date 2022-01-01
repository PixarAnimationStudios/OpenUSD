//
// Copyright 2020 benmalartre
//
// Unlicensed (copied from LoFi)
//
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/imaging/plugin/LoFi/textureResource.h"
#include "pxr/imaging/plugin/LoFi/glConversions.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/glf/baseTexture.h"
#include "pxr/imaging/glf/ptexTexture.h"
#include "pxr/imaging/glf/udimTexture.h"

PXR_NAMESPACE_OPEN_SCOPE

LoFiTextureResource::~LoFiTextureResource()
{
    // nothing
}


LoFiSimpleTextureResource::LoFiSimpleTextureResource(
                                    GlfTextureHandleRefPtr const &textureHandle,
                                    HdTextureType textureType,
                                    HdWrap wrapS,
                                    HdWrap wrapT,
                                    HdWrap wrapR,
                                    HdMinFilter minFilter,
                                    HdMagFilter magFilter,
                                    size_t memoryRequest)
 : LoFiTextureResource()
 , _textureHandle(textureHandle)
 , _texture()
 , _borderColor(0.0,0.0,0.0,0.0)
 , _maxAnisotropy(16.0)
 , _sampler(0)
 , _textureType(textureType)
 , _memoryRequest(memoryRequest)
 , _wrapS(wrapS)
 , _wrapT(wrapT)
 , _wrapR(wrapR)
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
}

LoFiSimpleTextureResource::~LoFiSimpleTextureResource() 
{ 
    if (_textureHandle) {
        _textureHandle->DeleteMemoryRequest(_memoryRequest);
    }

    if (_textureType != HdTextureType::Ptex) {
        if (!glDeleteSamplers) { // GL initialization guard for headless unit test
            return;
        }
        if (_sampler) {
            glDeleteSamplers(1, &_sampler);
        }
    }
}

HdTextureType LoFiSimpleTextureResource::GetTextureType() const
{
    return _textureType;
}

GLuint LoFiSimpleTextureResource::GetTexelsTextureId() 
{
    if (_texture) {
        return _texture->GetGlTextureName();
    }
    return 0;
}

GLuint LoFiSimpleTextureResource::GetTexelsSamplerId() 
{
    if (!TF_VERIFY(_textureType != HdTextureType::Ptex)) {
        return 0;
    }

    // Check for headless test
    if (glGenSamplers == nullptr) {
        return 0;
    }

    // Lazy sampler creation.
    if (_sampler == 0) {
        // If the LoFiSimpleTextureResource defines a wrap mode it will
        // use it, otherwise it gives an opportunity to the texture to define
        // its own wrap mode. The fallback value is always HdWrapRepeat
        GLenum fwrapS = LoFiGLConversions::GetWrap(_wrapS);
        GLenum fwrapT = LoFiGLConversions::GetWrap(_wrapT);
        GLenum fwrapR = LoFiGLConversions::GetWrap(_wrapR);
        GLenum fminFilter = LoFiGLConversions::GetMinFilter(_minFilter);
        GLenum fmagFilter = LoFiGLConversions::GetMagFilter(_magFilter);

        if (_texture) {
            VtDictionary txInfo = _texture->GetTextureInfo(true);

            if ((_wrapS == HdWrapUseMetadata || _wrapS == HdWrapLegacy) &&
                VtDictionaryIsHolding<GLuint>(txInfo, "wrapModeS")) {
                fwrapS = VtDictionaryGet<GLuint>(txInfo, "wrapModeS");
            }

            if ((_wrapT == HdWrapUseMetadata || _wrapT == HdWrapLegacy) &&
                VtDictionaryIsHolding<GLuint>(txInfo, "wrapModeT")) {
                fwrapT = VtDictionaryGet<GLuint>(txInfo, "wrapModeT");
            }

            if ((_wrapR == HdWrapUseMetadata || _wrapR == HdWrapLegacy) &&
                VtDictionaryIsHolding<GLuint>(txInfo, "wrapModeR")) {
                fwrapR = VtDictionaryGet<GLuint>(txInfo, "wrapModeR");
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
        if (_textureType == HdTextureType::Field) {
            glSamplerParameteri(_sampler, GL_TEXTURE_WRAP_R, fwrapR);
        }
        glSamplerParameteri(_sampler, GL_TEXTURE_MIN_FILTER, fminFilter);
        glSamplerParameteri(_sampler, GL_TEXTURE_MAG_FILTER, fmagFilter);
        glSamplerParameterf(_sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT,
            _maxAnisotropy);
        glSamplerParameterfv(_sampler, GL_TEXTURE_BORDER_COLOR,
            _borderColor.GetArray());
    }


    return _sampler;
}

GLuint64EXT LoFiSimpleTextureResource::GetTexelsTextureHandle() 
{ 
    GLuint textureId = GetTexelsTextureId();

    if (!TF_VERIFY(glGetTextureHandleARB) ||
        !TF_VERIFY(glGetTextureSamplerHandleARB)) {
        return 0;
    }

    if (textureId == 0) {
        return 0;
    }

    GLuint64EXT handle = 0;
    if (_textureType != HdTextureType::Uv) {
        handle = glGetTextureHandleARB(textureId);
    } else {
        GLuint samplerId = GetTexelsSamplerId();
        handle = glGetTextureSamplerHandleARB(textureId, samplerId);
    }

    if (handle == 0) {
        return 0;
    }

    bool bindlessTexture =
        GlfContextCaps::GetInstance().bindlessTextureEnabled;
    if (bindlessTexture) {
        if (!glIsTextureHandleResidentARB(handle)) {
            glMakeTextureHandleResidentARB(handle);
        }
    }

    return handle;
}

GLuint LoFiSimpleTextureResource::GetLayoutTextureId() 
{
    if (_textureType == HdTextureType::Udim) {
        GlfUdimTextureRefPtr udimTexture =
            TfDynamic_cast<GlfUdimTextureRefPtr>(_texture);
        if (udimTexture) {
            return udimTexture->GetGlLayoutName();
        }
    } else if (_textureType == HdTextureType::Ptex) {
#ifdef PXR_PTEX_SUPPORT_ENABLED
        GlfPtexTextureRefPtr ptexTexture =
            TfDynamic_cast<GlfPtexTextureRefPtr>(_texture);
        if (ptexTexture) {
            return ptexTexture->GetLayoutTextureName();
        }
#else
        TF_CODING_ERROR("Ptex support is disabled.  "
            "This code path should be unreachable");
#endif
    } else {
        TF_CODING_ERROR(
            "Using GetLayoutTextureId in a Uv texture is incorrect");
    }
    return 0;
}

GLuint64EXT LoFiSimpleTextureResource::GetLayoutTextureHandle() 
{
    if (!TF_VERIFY(_textureType != HdTextureType::Uv)) {
        return 0;
    }
    
    if (!TF_VERIFY(glGetTextureHandleARB)) {
        return 0;
    }

    GLuint textureId = GetLayoutTextureId();
    if (textureId == 0) {
        return 0;
    }

    GLuint64EXT handle = glGetTextureHandleARB(textureId);
    if (handle == 0) {
        return 0;
    }

    bool bindlessTexture =
        GlfContextCaps::GetInstance().bindlessTextureEnabled;
    if (bindlessTexture) {
        if (!glIsTextureHandleResidentARB(handle)) {
            glMakeTextureHandleResidentARB(handle);
        }
    }

    return handle;
}

size_t LoFiSimpleTextureResource::GetMemoryUsed()
{
    if (_texture) {
        return _texture->GetMemoryUsed();
    } else  {
        return 0;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

